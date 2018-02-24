#ifndef VIS_SUBPROCESS_H
#define VIS_SUBPROCESS_H
#include "vis-core.h"

struct Process {
	const char *name;
	FILE *outfd;
	FILE *errfd;
	FILE **infd;
	struct Process *next;
};

typedef struct Process Process;
typedef enum { STDOUT, STDERR } ResponceType;

void vis_process_communicate(Vis *, const char *command, const char *name,
                             FILE **input);
void vis_process_tick(Vis *);
#endif
