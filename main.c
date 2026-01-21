
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

void fatal(void)
{
	write(2, "error: fatal\n", 13);
	exit(1);
}

int cd(char **argv, int argc)
{
	if (argc != 2)
		return (write(2, "error: cd: bad arguments\n", 25), 1);
	if (chdir(argv[1]) != 0)
	{
		write(2, "error: cd: cannot change directory to ", 39);
		write(2, argv[1], strlen(argv[1]));
		write(2, "\n", 1);
		return 1;
	}
	return 0;
}

int exec_cmd(char **argv, int argc, int input_fd, int output_fd, char **envp)
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

	waitpid(pid, NULL, 0);
	return 0;
}

int main(int argc, char **argv, char **envp)
{
	int i = 1;
	int tmp_input = STDIN_FILENO;

	while (i < argc)
	{
		int j = i;
		// 找到 ; 或 | 或结尾
		while (j < argc && strcmp(argv[j], ";") && strcmp(argv[j], "|"))
			j++;

		int is_pipe = (j < argc && strcmp(argv[j], "|") == 0);
		int cmd_len = j - i;
		if (cmd_len > 0)
		{
			if (strcmp(argv[i], "cd") == 0)
				cd(&argv[i], cmd_len);
			else
			{
				int fd[2];
				int output_fd = STDOUT_FILENO;

				if (is_pipe)
				{
					if (pipe(fd) < 0)
						fatal();
					output_fd = fd[1];
				}

				exec_cmd(&argv[i], cmd_len, tmp_input, output_fd, envp);

				if (is_pipe)
				{
					close(fd[1]);
					if (tmp_input != STDIN_FILENO)
						close(tmp_input);
					tmp_input = fd[0]; // 下一条命令从管道读端读取
				}
				else
				{
					if (tmp_input != STDIN_FILENO)
						close(tmp_input);
					tmp_input = STDIN_FILENO; // 重置 stdin
				}
			}
		}

		i = j + 1;
	}

	if (tmp_input != STDIN_FILENO)
		close(tmp_input);

	return 0;
}
