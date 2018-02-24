#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "vis-lua.h"
#include "vis-subprocess.h"
static Process *process_pull;

void add_to_pull(const char *name, FILE *out, FILE *err, FILE **input) {
	Process *newprocess = (Process *)malloc(sizeof(Process));
	newprocess->name = name;
	newprocess->outfd = out;
	newprocess->errfd = err;
	newprocess->infd = input;
	newprocess->next = process_pull;
	process_pull = newprocess;
}

void vis_process_communicate(Vis *vis, const char *name, const char *command, FILE **inputfd) {
	int pin[2], pout[2], perr[2];
	pid_t pid = (pid_t)-1;
	if (pipe(perr) == -1) goto closeerr;
	if (pipe(pout) == -1) goto closeouterr;
	if (pipe(pin) == -1) goto closeall;
	vis->ui->terminal_save(vis->ui); /* no idea why */
	pid = fork();
	if (pid == -1)
		vis_info_show(vis, "fork failed: %s", strerror(errno));
	else if (pid == 0){ /* child process */
		dup2(pin[0], STDIN_FILENO);
		dup2(pout[1], STDOUT_FILENO);
		dup2(perr[1], STDERR_FILENO);
	}
	else { /* main process */
		if (fcntl(pout[0], F_SETFL, O_NONBLOCK) == -1 ||
			fcntl(perr[0], F_SETFL, O_NONBLOCK) == -1) goto closeall;
		close(pin[0]);
		close(pout[1]);
		close(perr[1]);
		*inputfd = fdopen(pin[1], "w");
		add_to_pull(name, fdopen(pout[0], "r"), fdopen(perr[0], "r"), inputfd);
		return;
  }
closeall:
	close(pin[0]);
	close(pin[1]);
closeouterr:
	close(pout[0]);
	close(pout[1]);
closeerr:
	close(perr[0]);
	close(perr[1]);
	if (pid == 0) { /* start command in child process */
		execlp(vis->shell, vis->shell, "-c", command, (char*)NULL);
	}
}


#define FIRE_IF_READY(fd, iserr) \
	if (FD_ISSET(fd, &reads) && \
			getline(&buffer, &length, current->fd) > 0) { \
	vis_lua_process_responce(vis, current->name, buffer, length, iserr); \
	free(buffer); \
	buffer = NULL; \
	length = 0; \
}

void vis_process_tick(Vis *vis) {
	fd_set reads;
	Process **pointer = &process_pull;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;
	while (*pointer) {
		Process *current = *pointer;
		int outfd = fileno(current->outfd);
		int errfd = fileno(current->errfd);
		if (outfd == -1 || errfd == -1 ||
			*(current->infd) == NULL) {
			/* remove from chain and invalidate handles */
			if (outfd != -1) fclose(current->outfd);
			if (errfd != -1) fclose(current->errfd);
			if (*(current->infd) != NULL) {
			fclose(*(current->infd));
			*(current->infd) = NULL;
		}
		*pointer = current->next;
		free(current);
	}
	FD_ZERO(&reads);
	FD_SET(outfd, &reads);
	FD_SET(errfd, &reads);
	if (select(FD_SETSIZE, &reads, NULL, NULL, &timeout) == -1) {
		/* error handling */
		vis_info_show(vis, "select failed: %s", strerror(errno));
	}
	else {
		char *buffer = NULL;
		size_t length = 0;
		FIRE_IF_READY(outfd, STDOUT)
		FIRE_IF_READY(errfd, STDERR)
	}
		pointer = &current->next;
	}
}
