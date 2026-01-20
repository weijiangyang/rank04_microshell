#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

/*
** 致命错误：fork / pipe / dup 失败时
** 考试要求直接输出并退出
*/
void fatal(void)
{
	write(2, "error: fatal\n", 13);
	exit(1);
}

/*
** 内建命令 cd
** argv: 当前命令起始位置
** i: 参数个数（直到 ; 或 |）
*/
int cd(char **argv, int i)
{
	// cd 参数数量必须是 2：cd + path
	if (i != 2)
		return (write(2, "error: cd: bad arguments\n", 25), 1);

	// chdir 失败
	if (chdir(argv[1]) != 0)
	{
		write(2, "error: cd: cannot change directory to ", 39);
		write(2, argv[1], strlen(argv[1]));
		write(2, "\n", 1);
	}
	return 1;
}

/*
** 执行一个外部命令
** argv      : 当前命令数组起始
** i         : 参数个数
** has_pipe  : 后面是否接 |
** fd        : 管道 fd（或 stdin/stdout 备份）
*/
int exec_cmd(char **argv, int i, int has_pipe, int *fd, char **envp)
{
	pid_t pid;

	// 创建子进程
	if ((pid = fork()) < 0)
		fatal();

	// 子进程
	if (pid == 0)
	{
		// 把 argv[i] 置 NULL，截断命令参数
		argv[i] = NULL;

		// 如果有 pipe，把 stdout 重定向到管道写端
		if (has_pipe && dup2(fd[1], 1) < 0)
			fatal();

		// 关闭不需要的 fd
		close(fd[0]);
		close(fd[1]);

		// 执行命令（考试保证 argv[0] 是完整路径）
		execve(argv[0], argv, envp);

		// execve 失败
		write(2, "error: cannot execute ", 22);
		write(2, argv[0], strlen(argv[0]));
		write(2, "\n", 1);
		exit(1);
	}

	// 父进程等待子进程
	waitpid(pid, NULL, 0);
	return 1;
}

int main(int argc, char **argv, char **envp)
{
	int i = 0;
	int fd[2];
	int tmp = dup(0); // 保存最初的 stdin

	(void)argc;

	// 主循环：逐段处理 ; 和 |
	while (argv[i] && argv[i + 1])
	{
		// argv 指针向前推进到当前命令起点
		argv = &argv[i + 1];
		i = 0;

		// 找到 ; 或 | 或结尾
		while (argv[i] && strcmp(argv[i], ";") && strcmp(argv[i], "|"))
			i++;

		// 内建命令 cd（只能在父进程执行）
		if (strcmp(argv[0], "cd") == 0)
			cd(argv, i);

		// 外部命令
		else if (i != 0)
		{
			// 如果后面是 |
			if (argv[i] && strcmp(argv[i], "|") == 0)
			{
				// 创建管道
				if (pipe(fd) < 0)
					fatal();

				// 执行当前命令，stdout -> 管道
				exec_cmd(argv, i, 1, fd, envp);

				// stdin 指向管道读端，供下一个命令使用
				dup2(fd[0], 0);

				close(fd[0]);
				close(fd[1]);
			}
			// 没有 pipe（结尾或 ;）
			else
			{
				// 备份 stdin / stdout
				fd[0] = dup(0);
				fd[1] = dup(1);

				// 正常执行命令
				exec_cmd(argv, i, 0, fd, envp);

				// 恢复 stdin
				dup2(tmp, 0);

				close(fd[0]);
				close(fd[1]);
			}
		}
	}
	close(tmp);
	return 0;
}
