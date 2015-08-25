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
#include <stdio.h>
#include "coio.h"

void
_coro_c(coroutine_context* ctx, char* name)
{
	coroutine_data
	{
		coroutine_context ctx;
		int ctr;
	} coroutine_data_end(ctx, d);
	reenter(ctx)
	{
entry:
		yield printf("Coroutine %s started\n", name);

		for (; d->ctr < 5;)
		{
			printf("%s first %d\n", __func__, d->ctr++);
			coio_yield();
			printf("%s second %d\n", __func__, d->ctr++);
			coio_yield();
		}

		printf("done %s\n", __func__);
		finish(ctx);
	}
}

void
_coro_b(coroutine_context* ctx, char* name)
{
	coroutine_data
	{
		coroutine_context ctx;
		int ctr;
	} coroutine_data_end(ctx, d);
	reenter(ctx)
	{
entry:
		yield printf("Coroutine %s started\n", name);

		coio_await(&d->ctx, _coro_c, "sub sub coro");

		for (; d->ctr < 5;)
		{
			printf("%s first %d\n", __func__, d->ctr++);
			coio_yield();
			printf("%s second %d\n", __func__, d->ctr++);
			coio_yield();
		}

		printf("done %s\n", __func__);
		finish(ctx);
	}
}

void
_t1(coroutine_context* ctx, void* arg)
{
	coroutine_data
	{
		coroutine_context ctx;
	} coroutine_data_end(ctx, d);
	reenter(ctx)
	{
entry:

		printf("Hello 1 from _t1\n");
		/* wait for sub coroutine */
		coio_await(&d->ctx, _coro_b, "subcoro");
		printf("Hello 2 from _t1\n");

		finish(ctx);
	}
}

void
_t2(coroutine_context* ctx, void* arg)
{
	reenter(ctx)
	{
entry:
		printf("Hello 1 from _t2\n");
		coio_yield();
		printf("Hello 2 from _t2\n");
		finish(ctx);
	}
}

int
main(int argc, char** argv)
{
	(void) argc;
	(void) argv;
	coio_create(_t1, NULL);
	coio_create(_t2, NULL);

	if (coio_main() < 0)
	{
		printf("Deadlocked\n");
		return 1;
	}

	return 0;
}
