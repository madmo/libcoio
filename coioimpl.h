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
#ifndef COIOIMPL_H
#define COIOIMPL_H

#include "coroutine.h"
#include "coio.h"

typedef struct CoioTaskList CoioTaskList;

struct CoioTask
{
	coroutine_context ctx;

	coio_func func;
	void* arg;

	uvlong timeout;
	int ready;

	/* linked list support */
	CoioTask* next;
	CoioTask* prev;
};

struct CoioTaskList
{
	CoioTask* head;
	CoioTask* tail;
};

extern CoioTaskList coio_ready;
extern CoioTaskList coio_sleeping;
extern CoioTask* coio_current;

void coio_add(CoioTaskList* lst, CoioTask* task);
void coio_del(CoioTaskList* lst, CoioTask* task);
void coio_rdy(CoioTask* task);
void coio_transfer();

#endif
