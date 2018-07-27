/*
 * Copyright (c) 2015 Moritz Bitsch <moritzbitsch@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdlib.h>
#include <unistd.h>

#include "coioimpl.h"
#include "coro.h"

#if defined(__APPLE__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

static coro_context _sched_ctx;
static unsigned long _taskcount = 0;

CoioTaskList 	coio_ready = {0, 0};
CoioTaskList 	coio_sleeping = {0, 0};
CoioTask       *coio_current;

static void
_process_events()
{
	uvlong 		now;
	int 		ms = 5;
	CoioTask       *t;

	if ((t = coio_sleeping.head) != NULL && t->timeout != 0) {
		now = coio_now();
		if (now >= t->timeout) {
			ms = 0;
		} else {
			ms = (t->timeout - now) / 1000000;
		}
	}
	/* TODO:do I/O polling instead of usleep */
	usleep(ms * 1000);

	/* handle CLOCK_MONOTONIC bugs (VirtualBox anyone?) */
	while (!coio_ready.head) {
		/* wake up timed out tasks */
		now = coio_now();
		while ((t = coio_sleeping.head) && now >= t->timeout) {
			coio_rdy(t);
		}
	}
}

int
coio_main()
{
	/* initialize empty ctx for scheduler */
	coro_create(&_sched_ctx, NULL, NULL, NULL, 0);

	/* scheduler mainloop */
	for (;;) {
		if (!coio_ready.head && coio_sleeping.head)
			_process_events();

		if (!coio_ready.head)
			break;

		coio_current = coio_ready.head;
		coio_del(&coio_ready, coio_current);
		coro_transfer(&_sched_ctx, &coio_current->ctx);

		if (coio_current->done) {
			_taskcount--;
			coro_stack_free(&coio_current->stk);
			free(coio_current);
		}
		coio_current = NULL;
	}

	if (_taskcount) {
		return -1;
	}
	return 0;
}

static void
_coio_entry(void *arg)
{
	CoioTask       *task = (CoioTask *) arg;

	task->func(task->arg);

	task->done = 1;
	coro_transfer(&coio_current->ctx, &_sched_ctx);
}

int
coio_create(coio_func f, void *arg, unsigned int stacksize)
{
	CoioTask       *task;

	task = calloc(1, sizeof(*task));
	if (!task)
		return -1;

	if (!coro_stack_alloc(&task->stk, stacksize / sizeof(void *))) {
		free(task);
		return -1;
	}
	task->func = f;
	task->arg = arg;

	coro_create(&task->ctx, _coio_entry, task, task->stk.sptr, task->stk.ssze);

	coio_add(&coio_ready, task);
	_taskcount++;

	return 0;
}

uvlong
coio_timeout(CoioTask * task, int ms)
{
	CoioTask       *t;

	if (ms > 0)
		task->timeout = coio_now() + (ms * 1000000);

	for (t = coio_sleeping.head; t != NULL && t->timeout && t->timeout < task->timeout; t = t->next);

	if (t) {
		task->prev = t->prev;
		task->next = t;
	} else {
		task->prev = coio_sleeping.tail;
		task->next = NULL;
	}

	t = coio_current;

	if (t->prev) {
		t->prev->next = t;
	} else {
		coio_sleeping.head = t;
	}

	if (t->next) {
		t->next->prev = t;
	} else {
		coio_sleeping.tail = t;
	}

	return task->timeout;
}

int
coio_delay(int ms)
{
	uvlong 		when;
	when = coio_timeout(coio_current, ms);
	coio_transfer();
	return (coio_now() - when) / 1000000;
}

void
coio_yield()
{
	coio_rdy(coio_current);
	coio_transfer();
}

void
coio_add(CoioTaskList * lst, CoioTask * task)
{
	if (lst->tail) {
		lst->tail->next = task;
		task->prev = lst->tail;
	} else {
		lst->head = task;
		task->prev = NULL;
	}
	lst->tail = task;
	task->next = NULL;
}

void
coio_del(CoioTaskList * lst, CoioTask * task)
{
	if (task->prev) {
		task->prev->next = task->next;
	} else {
		lst->head = task->next;
	}

	if (task->next) {
		task->next->prev = task->prev;
	} else {
		lst->tail = task->prev;
	}
}

void
coio_rdy(CoioTask * task)
{
	task->timeout = 0;
	coio_del(&coio_sleeping, task);
	coio_add(&coio_ready, task);
}

void
coio_transfer()
{
	coro_transfer(&coio_current->ctx, &_sched_ctx);
}

uvlong
coio_now()
{
#if defined(__APPLE__)
	clock_serv_t cclock;
	mach_timespec_t mts;

	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);

	return (uvlong) mts.tv_sec * 1000 * 1000 * 1000 + mts.tv_nsec;
#else
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return -1;

	return (uvlong) ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
#endif
}
