#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

void fatal(void)
{
	write(2, "error: fatal\n", 13);
	exit(1);
}

int cd(char **argv, int argc)
{
	if (argc != 2)
	{
		write(2, "error: cd: bad arguments\n", 25);
		return 1;
	}
	if (chdir(argv[1]) != 0)
	{
		write(2, "error: cd: cannot change directory to ", 38);
		write(2, argv[1], strlen(argv[1]));
		write(2, "\n", 1);
		return 1;
	}
	return 0;
}

void wait_all(pid_t *pids, int *count)
{
	for (int i = 0; i < *count; i++)
		waitpid(pids[i], NULL, 0);
	*count = 0;
}

// fork 并执行命令（不 wait）
pid_t exec_cmd_fork(char **argv, int argc, int input_fd, int output_fd, char **envp)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		fatal();
	if (pid == 0)
	{
		argv[argc] = NULL;
		if (input_fd != STDIN_FILENO)
		{
			if (dup2(input_fd, STDIN_FILENO) < 0)
				fatal();
			close(input_fd);
		}
		if (output_fd != STDOUT_FILENO)
		{
			if (dup2(output_fd, STDOUT_FILENO) < 0)
				fatal();
			close(output_fd);
		}
		execve(argv[0], argv, envp);
		write(2, "error: cannot execute ", 22);
		write(2, argv[0], strlen(argv[0]));
		write(2, "\n", 1);
		exit(1);
	}
	return pid; // 父进程返回子进程 PID
}

int main(int argc, char **argv, char **envp)
{
	int i = 1;
	int tmp_input = STDIN_FILENO;

	pid_t pids[256]; // 保存所有 fork 的 PID
	int pids_count = 0;

	while (argv[i])
	{
		int j = i;
		while (j < argc && strcmp(argv[j], ";") && strcmp(argv[j], "|"))
			j++;

		int is_pipe = (j < argc && strcmp(argv[j], "|") == 0);
		int cmd_len = j - i;

		if (cmd_len > 0)
		{
			if (strcmp(argv[i], "cd") == 0)
			{
				if (is_pipe)
					write(2, "error: cd: cannot be piped\n", 27);
				else
					cd(&argv[i], cmd_len);
			}
			else
			{
				int fd[2];
				int out_fd = STDOUT_FILENO;

				if (is_pipe)
				{
					if (pipe(fd) < 0)
						fatal();
					out_fd = fd[1];
				}

				// fork 并返回 PID
				pid_t pid = exec_cmd_fork(&argv[i], cmd_len, tmp_input, out_fd, envp);
				pids[pids_count++] = pid;

				if (is_pipe)
				{
					close(fd[1]); // 父进程关闭写端
					if (tmp_input != STDIN_FILENO)
						close(tmp_input); // 关闭上一条管道读端
					tmp_input = fd[0];	  // 下一条命令的输入
				}
				else
				{
					if (tmp_input != STDIN_FILENO)
						close(tmp_input);
					tmp_input = STDIN_FILENO;
				}
			}
		}

		if (j < argc && strcmp(argv[j], ";") == 0)
		{
			if (tmp_input != STDIN_FILENO)
			{
				close(tmp_input);
				tmp_input = STDIN_FILENO;
			}
			wait_all(pids, &pids_count);
		}
		i = j + 1;
	}
	// 等待所有 fork 的子进程结束
	wait_all(pids, &pids_count);
	return 0;
}