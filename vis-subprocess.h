#ifndef VIS_SUBPROCESS_H
#define VIS_SUBPROCESS_H
#include "vis-core.h"

struct Process {
	const char *name;
	FILE *outfd;
	FILE *errfd;
	FILE **infd;
	void **invalidator;
	struct Process *next;
};

typedef struct Process Process;
typedef enum { STDOUT, STDERR } ResponceType;

Process *vis_process_communicate(Vis *, const char *command, const char *name,
                                 void **invalidator);
void vis_process_tick(Vis *);
#endif
