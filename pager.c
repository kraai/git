#include "cache.h"
#include "spawn-pipe.h"

/*
 * This is split up from the rest of git so that we might do
 * something different on Windows, for example.
 */

#ifndef __MINGW32__
static void run_pager(const char *pager)
{
	execlp(pager, pager, NULL);
	execl("/bin/sh", "sh", "-c", pager, NULL);
}
#endif

void setup_pager(void)
{
#ifndef __MINGW32__
	pid_t pid;
#else
	const char *pager_argv[] = { "sh", "-c", NULL, NULL };
#endif
	int fd[2];
	const char *pager = getenv("GIT_PAGER");

	if (!isatty(1))
		return;
	if (!pager)
		pager = getenv("PAGER");
	if (!pager)
		pager = "less";
	else if (!*pager || !strcmp(pager, "cat"))
		return;

	pager_in_use = 1; /* means we are emitting to terminal */

	if (pipe(fd) < 0)
		return;
#ifndef __MINGW32__
	pid = fork();
	if (pid < 0) {
		close(fd[0]);
		close(fd[1]);
		return;
	}

	/* return in the child */
	if (!pid) {
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		return;
	}

	/* The original process turns into the PAGER */
	dup2(fd[0], 0);
	close(fd[0]);
	close(fd[1]);

	setenv("LESS", "FRSX", 0);
	run_pager(pager);
	die("unable to execute pager '%s'", pager);
	exit(255);
#else
	/* spawn the pager */
	pager_argv[2] = pager;
	if (spawnvpe_pipe(pager_argv[0], pager_argv, environ, fd, NULL) < 0)
		return;

	/* original process continues, but writes to the pipe */
	dup2(fd[1], 1);
	close(fd[1]);
#endif
}
