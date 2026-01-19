#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct s_cmd
{
	char **argv;
	int argc;
} t_cmd;

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
		execve(cmd->argv[0], cmd->argv, envp);
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

void run_cmds_continue(char **argv, char **envp)
{
	int i = 0;
	int start = 0;

	while (argv[i])
	{
		if (strcmp(argv[i], ";") == 0 || argv[i + 1] == NULL)
		{
			int end = (strcmp(argv[i], ";") == 0) ? i - 1 : i;

			if (end >= start)
			{
				t_cmd *cmd = build_single_cmd(argv, start, end);
				run_cmd(cmd, envp);
				free_cmd(cmd);
			}

			start = i + 1;
		}
		i++;
	}
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
