#include <stdio.h>
#include <unistd.h>

#include <sys/wait.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct s_cmd
{
	char **argv;
	int argc;
} t_cmd;

void free_fds(int **fds, int n_cmds)
{
	int i;

	i = 0;
	while (i < n_cmds - 1)
	{
		free(fds[i]);
		i++;
	}
	free(fds);
}

int	ft_strlen(char *str)
{
	// calculer la taille d'un str
	int	size;
	
	size = 0;
	while (str[size])
		size++;
	return (size);
}

char *ft_strjoin(char *s1, char *s2)
{
	// pour joindre deux strs
	char *ptr;
	char *dst;
	// is s1 ou s2 est NULL, le change en vide
	if (!s1)
		s1 = "";
	if (!s2)
		s2 = "";
	// allocation de memoire
	ptr = malloc((ft_strlen(s1) + ft_strlen(s2) + 1) * sizeof(char));
	if (!ptr)
		return (NULL);
	dst = ptr;
	while (*s1)
	{
		*dst = *s1;
		dst++;
		s1++;
	}
	while (*s2)
	{
		*dst = *s2;
		dst++;
		s2++;
	}
	*dst = '\0';
	return (ptr);
}

t_cmd *build_single_cmd(char **argv, int start, int end)
{
	t_cmd *cmd;
	int k;

	k = 0;
	cmd = malloc(sizeof(t_cmd));
	if (!cmd)
		return (NULL);
	cmd->argc = end - start + 1;
	cmd->argv = malloc((cmd->argc + 1) * sizeof(char *));
	while (k < cmd->argc)
	{
		(cmd->argv)[k] = argv[start + k];
		k++;
	}
	(cmd->argv)[k] = NULL;
	return (cmd);
}

int run_cmd(t_cmd *cmd, char **envp)
{
	if (cmd->argc == 0)
		return 0;

	// 如果是 cd 内建命令
	if (strcmp(cmd->argv[0], "cd") == 0)
	{
		if (cmd->argc != 2)
		{
			write(2, "error: cd: bad arguments\n", 25);
			return 1;
		}
		if (chdir(cmd->argv[1]) != 0)
		{
			write(2, "error: cd: cannot change directory to ", 39);
			write(2, cmd->argv[1], strlen(cmd->argv[1]));
			write(2, "\n", 1);
			return 1;
		}
		return 0;
	}

	// 外部命令
	pid_t pid = fork();
	if (pid < 0)
	{
		write(2, "error: fatal\n", 13);
		exit(1);
	}
	if (pid == 0)
	{
		char *path;
		char *cmd_path = cmd->argv[0];
		if (strchr(cmd_path, '/') == NULL) // 没有 /，尝试 /bin/
		{
			
			path = ft_strjoin("/bin/", cmd_path);
		}
		execve(path, cmd->argv, envp);
		free(path);
		write(2, "error: cannot execute ", 22);
		write(2, cmd->argv[0], strlen(cmd->argv[0]));
		write(2, "\n", 1);
		exit(1);
	}
	else
	{
		waitpid(pid, NULL, 0);
	}
	return 0;
}

void free_cmd(t_cmd *cmd)
{
	if (!cmd)
		return;

	// 只释放 argv 数组本身，而不是里面的字符串
	free(cmd->argv);
	cmd->argv = NULL;

	free(cmd);
}

t_cmd **split_by_pipe(char **argv, int start, int end, int *count)
{
	t_cmd *cmd;
	t_cmd **cmds;
	int k;
	int j;

	k = start;
	j = 0;
	int n_pipe = 0;
	for (int i = start; i <= end; i++)
		if (strcmp(argv[i], "|") == 0)
			n_pipe++;
	cmds = malloc((n_pipe + 1) * sizeof(t_cmd *));
	if (!cmds)
		return NULL;

	while (k <= end)
	{
		if (strcmp(argv[k], "|") == 0)
		{
			cmd = malloc(sizeof(t_cmd));
			if (!cmd)
				return (NULL);
			cmd->argc = k - start;
			cmd->argv = malloc((cmd->argc + 1) * sizeof(char *));
			int i = 0;
			while (i < (cmd->argc))
			{
				cmd->argv[i] = argv[start + i];
				i++;
			}
			cmd->argv[cmd->argc] = NULL;
			cmds[j] = cmd;
			start = k + 1;
			j++;
		}
		k++;
	}
	if (start <= end)
	{
		cmd = malloc(sizeof(t_cmd));
		cmd->argc = end - start + 1;
		cmd->argv = malloc((cmd->argc + 1) * sizeof(char *));
		for (int i = 0; i < cmd->argc; i++)
			cmd->argv[i] = argv[start + i];
		cmd->argv[cmd->argc] = NULL;
		cmds[j++] = cmd;
	}
	*count = j;
	return cmds;
}

int **create_pipes(int n_cmds)
{
	int i;
	int **fds;

	i = 0;
	if (n_cmds < 2)
		return NULL;
	fds = malloc(sizeof(int *) * (n_cmds - 1));
	if (!fds)
		return NULL;
	while (i < n_cmds - 1)
	{
		fds[i] = malloc(sizeof(int) * 2);
		if (!fds[i] || pipe(fds[i]) < 0)
		{
			// 释放已经分配的内存
			for (int j = 0; j < i; j++)
				free(fds[j]);
			free(fds);
			return NULL;
		}
		i++;
	}
	return fds;
}

int run_piped_cmds(t_cmd **cmds, int n_cmds, char **envp)
{
	int i;
	int **fds;
	pid_t *pids;

	i = 0;
	if (n_cmds == 1)
	{
		return (run_cmd(cmds[0], envp));
	}
	fds = create_pipes(n_cmds);
	if (!fds)
		return 1;
	pids = malloc(sizeof(pid_t) * (n_cmds));
	if (!pids)
	{
		free_fds(fds, n_cmds);
		return 1;
	}
	while (i < n_cmds)
	{
		pid_t pid;
		pid = fork();
		if (pid == 0)
		{
			if (i == 0)
			{
				if (dup2(fds[0][1], STDOUT_FILENO) < 0)
				{
					perror("dup2");
					exit(1);
				}
			}
			else if (i == n_cmds - 1)
			{
				if (dup2(fds[n_cmds - 2][0], STDIN_FILENO) < 0)
				{
					perror("dup2");
					exit(1);
				}
			}
			else
			{
				if (dup2(fds[i - 1][0], STDIN_FILENO) < 0)
				{
					perror("dup2");
					exit(1);
				}
				if (dup2(fds[i][1], STDOUT_FILENO) < 0)
				{
					perror("dup2");
					exit(1);
				}
			}
			int j = 0;
			while (j < n_cmds - 1)
			{
				close(fds[j][0]);
				close(fds[j][1]);
				j++;
			}
			char *cmd_path = cmds[i]->argv[0];
			char *path = ft_strjoin("/bin/", cmd_path);
			execve(path, cmds[i]->argv, envp);
			free(path);
			write(2, "error: cannot execute command\n", 30);
			exit(1);
		}
		pids[i] = pid;
		i++;
	}
	int j = 0;
	while (j < n_cmds - 1)
	{
		close(fds[j][0]);
		close(fds[j][1]);
		j++;
	}
	int k = 0;
	while (k < n_cmds)
	{
		waitpid(pids[k], NULL, 0);
		k++;
	}
	free(pids);
	free_fds(fds, n_cmds);
	return 0;
}

int has_pipe(char **argv, int start, int end)
{
	int i;

	i = start;
	while (i <= end)
	{
		if (strcmp(argv[i], "|") == 0)
			return 1;
		i++;
	}
	return 0;
}
int run_all_cmds(char **argv, char **envp)
{
	int start;
	int i;
	int end;

	start = 0;
	i = 0;
	while (argv[i])
	{
		if (strcmp(argv[i], ";") == 0 || argv[i + 1] == NULL)
		{
			if (strcmp(argv[i], ";") == 0)
				end = i - 1;
			else if (argv[i + 1] == NULL)
				end = i;
			if (!has_pipe(argv, start, end))
			{
				t_cmd *cmd = build_single_cmd(argv, start, end);
				run_cmd(cmd, envp);
			}
			else
			{
				t_cmd **cmds;
				int count;

				cmds = split_by_pipe(argv, start, end, &count);
				run_piped_cmds(cmds, count, envp);
				for (int k = 0; k < count; k++)
					free_cmd(cmds[k]);
				free(cmds);
			}
			start = i + 1;
		}
		i++;
	}
	return 0;
}

int main(int argc, char **argv, char **envp)
{
	if (argc > 1)
	{
		// 批量模式：./microshell 命令 ...
		run_all_cmds(argv + 1, envp);
	}
	else
	{
		// 交互模式
		char *line = NULL;
		size_t len = 0;

		while (1)
		{
			write(1, "$> ", 3); // 提示符
			if (getline(&line, &len, stdin) == -1)
				break; // EOF 或 Ctrl+D

			// 简单分割空格，构建 argv
			char *argv_buf[1024];
			int argc_buf = 0;
			char *token = strtok(line, " \n");
			while (token)
			{
				argv_buf[argc_buf++] = token;
				token = strtok(NULL, " \n");
			}
			argv_buf[argc_buf] = NULL;

			run_all_cmds(argv_buf, envp);
		}

		free(line);
	}

	return 0;
}
