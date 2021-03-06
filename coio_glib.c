/*
 * Copyright (c) 2018 Moritz Bitsch <moritzbitsch@gmail.com>
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

#include "coio_glib.h"
#include "coio.h"
#include "coioimpl.h"

#include <glib.h>
#include <gio/gio.h>

struct coio_source {
	GSource base;
};

static gboolean coio_source_prepare(GSource* source, gint* timeout_)
{
	(void)source;
	uvlong now;
	int ms = 5;
	CoioTask* t;

	if (coio_ready_list.head) {
		*timeout_ = 0;

		/* if we return true here our check functions do not get called */
		if (coio_sleeping.head) {
			return FALSE;
		}

		return TRUE;
	}

	if ((t = coio_sleeping.head) != NULL && t->timeout != 0) {
		now = coio_now();
		if (now >= t->timeout) {
			ms = 0;
		} else {
			ms = (t->timeout - now);
		}
	}

	*timeout_ = ms;
	return FALSE;
}

static gboolean coio_source_check(GSource* source)
{
	(void)source;
	CoioTask* t;

	if (coio_sleeping.head) {
		/* wake up timed out tasks */
		uvlong now = coio_now();
		while ((t = coio_sleeping.head) && t->timeout && now >= t->timeout) {
			coio_ready(t);
		}
	}

	if (coio_ready_list.head)
		return TRUE;

	return FALSE;
}

static gboolean coio_source_dispatch(GSource* source, GSourceFunc callback, gpointer user_data)
{
	(void)source;
	CoioTask* last;
	gboolean result = G_SOURCE_CONTINUE;

	if (!coio_ready_list.head)
		return G_SOURCE_CONTINUE;

	last = coio_ready_list.tail;

	do {
		coio_current = coio_ready_list.head;
		coio_current->ready = 0;
		coio_del(&coio_ready_list, coio_current);
		coro_transfer(&coio_sched_ctx, &coio_current->ctx);

		if (coio_current->done) {
			coio_taskcount--;
			coro_stack_free(&coio_current->stk);
			free(coio_current);
		}
	} while (coio_current != last);

	coio_current = NULL;

	if (callback) {
		result = callback(user_data);
	}

	return result;
}

static void coio_source_finalize(GSource* source)
{
	(void)source;
}

GSource* coio_gsource_create()
{
	coro_create(&coio_sched_ctx, NULL, NULL, NULL, 0);

	static GSourceFuncs funcs = {
		coio_source_prepare,
		coio_source_check,
		coio_source_dispatch,
		coio_source_finalize,
		NULL,
		NULL
	};

	return g_source_new(&funcs, sizeof(struct coio_source));
}

gboolean coio_task_wakeup_helper(gpointer task)
{
	coio_ready((CoioTask*)task);
	return G_SOURCE_REMOVE;
}

void coio_gasyncresult_wakeup_helper(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	(void)source_object;
	CoioChannel* ch = (CoioChannel*)user_data;

	coio_channbsendp(ch, g_object_ref(res));
}
