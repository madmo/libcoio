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
#ifndef COIO_H
#define COIO_H

#include "coroutine.h"

#ifdef __cplusplus
extern      "C" {
#endif

#define coio_yield() do { coio_yield_impl(); yield{}; } while (0)
#define coio_delay(ms) do { coio_delay_impl(ms); yield{}; } while (0)
#define coio_await(c, f, ...) while ((c)->step != -1 ) { f((c), ##__VA_ARGS__); coio_yield(); }

typedef struct CoioTask CoioTask;
typedef void (*coio_func)(coroutine_context* ctx, void* arg);
typedef unsigned long long uvlong;

int coio_main();
int coio_create(coio_func f, void* arg);
void coio_yield_impl();
uvlong coio_now();
int coio_delay_impl(int ms);

#ifdef __cplusplus
}
#endif

#endif
