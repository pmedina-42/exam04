#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int haspipe;
int ret;
char **e;
int stdIn;

size_t	len(char *s)
{
	size_t i = 0;

	while (s[i++]);
	return i;
}

void	fatal_error()
{
	char *s = "error: fatal\n";
	write(2, s, len(s));
	exit(1);
}

void	exe_error(char *arg)
{
	char *s = "error: cannot execute ";
	write(2, s, len(s));
	write(2, arg, len(arg));
	write(2, "\n", 1);
	exit(1);
}

void	cd_error(char *msg, char *arg)
{
	char *s = "error: cd: ";
	write(2, s, len(s));
	write(2, msg, len(msg));
	if (arg != NULL)
		write(2, arg, len(arg));
	write(2, "\n", 1);
}

char	**get_next(char **argv, char *sep)
{
	while (argv && *argv)
	{
		if (!strcmp(*argv, sep))
		{
			if (!strcmp(sep, "|"))
				haspipe = 1;
			*argv = NULL;
			return ++argv;
		}
		argv++;
	}
	return NULL;
}

void	open_pipes(int fd[2])
{
	if (haspipe)
	{
		if (close(fd[0]) == -1)
			fatal_error();
		if (dup2(fd[1], STDOUT_FILENO) == -1)
			fatal_error();
		if (close(fd[1]) == -1)
			fatal_error();
	}
}

void	close_pipes(int fd[2])
{
	if (haspipe)
	{
		if (close(fd[1]) == -1)
			fatal_error();
		if (dup2(fd[0], STDIN_FILENO) == -1)
			fatal_error();
		if (close(fd[0]) == -1)
			fatal_error();
	}
}

void	cd_builtin(char **argv)
{
	if (!argv || *(argv + 1))
	{
		ret = 1;
		cd_error("bad arguments", NULL);
		return ;
	}
	if (chdir(*argv) == -1)
	{
		ret = 1;
		cd_error("cannot change directory to ", *argv);
	}
}

void	exe_cmd(char **argv)
{
	pid_t pid;
	int fd[2];

	if (!strcmp(*argv, "cd"))
		cd_builtin(++argv);
	else
	{
		if (haspipe)
			if (pipe(fd) == -1)
				fatal_error();
		pid = fork();
		if (pid == -1)
			fatal_error();
		if (pid == 0)
		{
			open_pipes(fd);
			if (execve(*argv, argv, e) == -1)
				exe_error(*argv);
		}
		else
			close_pipes(fd);
	}
}

void	parse_cmd(char **argv)
{
	char **next_cmd;
	int nproc = 1;

	while (argv && *argv)
	{
		haspipe = 0;
		next_cmd = get_next(argv, "|");
		if (next_cmd != NULL)
			nproc++;
		exe_cmd(argv);
		argv = next_cmd;
	}
	//printf("nproc = %d\n", nproc);
	while (nproc-- > 0)
		waitpid(-1, 0, 0);
}

void	restore_fd()
{
	int tmp = dup(STDIN_FILENO);
	if (dup2(stdIn, STDIN_FILENO) == -1)
		fatal_error();
	if (close(tmp) == -1)
		fatal_error();
}

int		main(int argc, char **argv, char **env)
{
	char **next_pipeline;

	stdIn = dup(STDIN_FILENO);
	argv++;
	e = env;
	while (argc && argv && *argv)
	{
		next_pipeline = get_next(argv, ";");
		parse_cmd(argv);
		argv = next_pipeline;
		restore_fd();
	}
	return ret;
}
