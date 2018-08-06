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

#ifdef __cplusplus
extern 		"C" {
#endif

typedef struct CoioTask CoioTask;
typedef void (*coio_func)(void* arg);
typedef unsigned long long uvlong;

extern CoioTask* coio_current;

int 		coio_main();
int 		coio_create(const char* name, coio_func f, void* arg, unsigned int stacksize);
void 		coio_yield();
uvlong 		coio_now();
int 		coio_delay(int ms);
void 		coio_ready(CoioTask* task);

/* channel communication from libtask */
typedef struct CoioAlt CoioAlt;
typedef struct Altarray Altarray;
typedef struct CoioChannel CoioChannel;

enum
{
	COIO_CHANEND,
	COIO_CHANSND,
	COIO_CHANRCV,
	COIO_CHANNOP,
	COIO_CHANNOBLK,
};

struct CoioAlt
{
	CoioChannel*	c;
	void*		v;
	unsigned int	op;
	CoioTask*	task;
	CoioAlt*		xalt;
};

struct Altarray
{
	CoioAlt**		a;
	unsigned int	n;
	unsigned int	m;
};

struct CoioChannel
{
	unsigned int	bufsize;
	unsigned int	elemsize;
	unsigned char*	buf;
	unsigned int	nbuf;
	unsigned int	off;
	Altarray	asend;
	Altarray	arecv;
	char*		name;
};

int		coio_chanalt(CoioAlt* alts);
CoioChannel*	coio_chancreate(int elemsize, int elemcnt);
void		coio_chanfree(CoioChannel* c);
int		coio_channbrecv(CoioChannel* c, void* v);
void*		coio_channbrecvp(CoioChannel* c);
unsigned long	coio_channbrecvul(CoioChannel* c);
int		coio_channbsend(CoioChannel* c, void* v);
int		coio_channbsendp(CoioChannel* c, void* v);
int		coio_channbsendul(CoioChannel* c, unsigned long v);
int		coio_chanrecv(CoioChannel* c, void* v);
void*		coio_chanrecvp(CoioChannel* c);
unsigned long	coio_chanrecvul(CoioChannel* c);
int		coio_chansend(CoioChannel* c, void* v);
int		coio_chansendp(CoioChannel* c, void* v);
int		coio_chansendul(CoioChannel* c, unsigned long v);

#ifdef __cplusplus
}
#endif

#endif
