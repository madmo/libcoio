/* Copyright (c) 2015 , Moritz Bitsch
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdlib.h>

typedef struct _coroutine_context
{
	int step;
	void *data;
} coroutine_context;

/* hack to fix MSVC edit and continue */
#ifdef _MSC_VER
#define __CORO_LINE_NO __COUNTER__ + 1
#else
#define __CORO_LINE_NO __LINE__
#endif

#define coroutine_data struct
#define coroutine_data_end(c, name) *name = c->data; if (name == NULL) c->data = name = calloc(1, sizeof(*name))

#define reenter(c) \
	coroutine_context* __coro_ctx = c; \
	switch (c->step)

#define entry \
	extern void _coroutine_entry_label_enforcer(); \
	default: \
	abort(); \
	case -1: \
	coroutine_bail_out: break; \
	case 0

#define yield \
	(void)&_coroutine_entry_label_enforcer; \
	__coro_ctx->step = __CORO_LINE_NO; \
	goto coroutine_bail_out; \
	case __CORO_LINE_NO:

#define finish(c) do { (c)->step = -1; if ((c)->data) { free((c)->data); (c)->data = NULL; } } while (0)

#endif
