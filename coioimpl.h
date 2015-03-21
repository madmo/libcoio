#ifndef COIOIMPL_H
#define COIOIMPL_

#include "coro.h"
#include "coio.h"

typedef struct CoioTaskList CoioTaskList;

struct CoioTask {
	coro_context ctx;
	struct coro_stack stk;

	coro_func func;
	void *arg;

	int done;

	/* linked list support */
	CoioTask *next;
	CoioTask *prev;
};

struct CoioTaskList {
	CoioTask *head;
	CoioTask *tail;
};

void coio_add(CoioTaskList *lst, CoioTask *task);
void coio_del(CoioTaskList *lst, CoioTask *task);

#endif
