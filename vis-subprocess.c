#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include "vis-lua.h"
#include "vis-subprocess.h"

#define MAXBUFFER 1024


static Process *process_pull;

Process *new_in_pull() {
  Process *newprocess = (Process *)malloc(sizeof(Process));
  newprocess->next = process_pull;
  process_pull = newprocess;
  return newprocess;
}

Process *vis_process_communicate(Vis *vis, const char *name,
                                 const char *command, void **invalidator) {
  int pin[2], pout[2], perr[2];
  pid_t pid = (pid_t)-1;
  if (pipe(perr) == -1) goto closeerr;
  if (pipe(pout) == -1) goto closeouterr;
  if (pipe(pin) == -1) goto closeall;
  pid = fork();
  if (pid == -1)
    vis_info_show(vis, "fork failed: %s", strerror(errno));
  else if (pid == 0){ /* child process */
    dup2(pin[0], STDIN_FILENO);
    dup2(pout[1], STDOUT_FILENO);
    dup2(perr[1], STDERR_FILENO);
  }
  else { /* main process */
    close(pin[0]);
    close(pout[1]);
    close(perr[1]);
    Process *new = new_in_pull();
    new->name = name;
    new->outfd = pout[0];
    new->errfd = perr[0];
    new->inpfd = pin[1];
    new->pid = pid;
    new->invalidator = invalidator;
    return new;
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
  else
    vis_info_show(vis, "process creation failed: %s", strerror(errno));
}

void destroy(Process **pointer) {
  Process *target = *pointer;
  if (target->outfd != -1) close(target->outfd);
  if (target->errfd != -1) close(target->errfd);
  if (target->inpfd != -1) close(target->inpfd);
  /* marking stream as closed for lua */
  if (target->invalidator) *(target->invalidator) = NULL;
  *pointer = target->next;
  free(target);
}

int vis_process_before_tick(fd_set *readfds) {
  Process **pointer = &process_pull;
  int maxfd = 0;
  while (*pointer) {
    Process *current = *pointer;
    if (current->outfd != -1) {
      FD_SET(current->outfd, readfds);
      maxfd = maxfd < current->outfd ? current->outfd : maxfd;
    }
    if (current->errfd != -1) {
      FD_SET(current->errfd, readfds);
      maxfd = maxfd < current->errfd ? current->errfd : maxfd;
    }
    pointer = &current->next;
  }
  return maxfd;
}

void read_and_fire(Vis* vis, int fd, const char *name, ResponceType iserr) {
  static char buffer[MAXBUFFER];
  size_t obtained = read(fd, &buffer, MAXBUFFER-1);
  if (obtained > 0)
    vis_lua_process_responce(vis, name, buffer, obtained, iserr);
}

void vis_process_tick(Vis *vis, fd_set *readfds) {
  Process **pointer = &process_pull;
  while (*pointer) {
    Process *current = *pointer;
    if (current->outfd != -1 && FD_ISSET(current->outfd, readfds))
      read_and_fire(vis, current->outfd, current->name, STDOUT);
    if (current->errfd != -1 && FD_ISSET(current->errfd, readfds))
      read_and_fire(vis, current->errfd, current->name, STDERR);
    pid_t wpid = waitpid(current->pid, NULL, WNOHANG);
    if (wpid == -1)
      vis_message_show(vis, strerror(errno));
    else if (wpid == current->pid)
      goto just_destroy;
    else if(current->outfd == -1 || current->errfd == -1 ||
            current->inpfd == -1)
      goto kill_and_destroy;
    pointer = &current->next;
    continue;
kill_and_destroy:
    kill(current->pid, SIGTERM);
    waitpid(current->pid, NULL, 0);
just_destroy:
    destroy(pointer);
  }
}
