/* Copyright (c) 2005 Russ Cox, MIT; see COPYRIGHT */
/* Copyright (c) 2019 Moritz Bitsch */

#include "coioimpl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

CoioChannel* coio_chancreate(int elemsize, int bufsize)
{
	CoioChannel* c;

	c = malloc(sizeof *c+bufsize*elemsize);
	if (c == NULL)
	{
		perror("chancreate malloc");
		exit(1);
	}
	memset(c, 0, sizeof *c);
	c->elemsize = elemsize;
	c->bufsize = bufsize;
	c->nbuf = 0;
	c->buf = (unsigned char*)(c+1);
	return c;
}

/* bug - work out races */
void coio_chanfree(CoioChannel* c)
{
	if (c == NULL)
		return;
	free(c->name);
	free(c->arecv.a);
	free(c->asend.a);
	free(c);
}

static void addarray(Altarray* a, CoioAlt* alt)
{
	if (a->n == a->m)
	{
		a->m += 16;
		a->a = realloc(a->a, a->m*sizeof a->a[0]);
	}
	a->a[a->n++] = alt;
}

static void delarray(Altarray* a, int i)
{
	--a->n;
	a->a[i] = a->a[a->n];
}

/*
 * doesn't really work for things other than CHANSND and CHANRCV
 * but is only used as arg to chanarray, which can handle it
 */
#define otherop(op)	(COIO_CHANSND+COIO_CHANRCV-(op))

static Altarray* chanarray(CoioChannel* c, uint op)
{
	switch (op)
	{
	default:
		return NULL;
	case COIO_CHANSND:
		return &c->asend;
	case COIO_CHANRCV:
		return &c->arecv;
	}
}

static int altcanexec(CoioAlt* a)
{
	Altarray* ar;
	CoioChannel* c;

	if (a->op == COIO_CHANNOP)
		return 0;
	c = a->c;
	if (c->bufsize == 0)
	{
		ar = chanarray(c, otherop(a->op));
		return ar && ar->n;
	}
	else
	{
		switch (a->op)
		{
		default:
			return 0;
		case COIO_CHANSND:
			return c->nbuf < c->bufsize;
		case COIO_CHANRCV:
			return c->nbuf > 0;
		}
	}
}

static void altqueue(CoioAlt* a)
{
	Altarray* ar;

	ar = chanarray(a->c, a->op);
	addarray(ar, a);
}

static void altdequeue(CoioAlt* a)
{
	unsigned int i;
	Altarray* ar;

	ar = chanarray(a->c, a->op);
	if (ar == NULL)
	{
		fprintf(stderr, "bad use of altdequeue op=%d\n", a->op);
		abort();
	}

	for (i=0; i<ar->n; i++)
		if (ar->a[i] == a)
		{
			delarray(ar, i);
			return;
		}
	fprintf(stderr, "cannot find self in altdq\n");
	abort();
}

static void altalldequeue(CoioAlt* a)
{
	int i;

	for (i=0; a[i].op!=COIO_CHANEND && a[i].op!=COIO_CHANNOBLK; i++)
		if (a[i].op != COIO_CHANNOP)
			altdequeue(&a[i]);
}

static void amove(void* dst, void* src, uint n)
{
	if (dst)
	{
		if (src == NULL)
			memset(dst, 0, n);
		else
			memmove(dst, src, n);
	}
}

/*
 * Actually move the data around.  There are up to three
 * players: the sender, the receiver, and the channel itself.
 * If the channel is unbuffered or the buffer is empty,
 * data goes from sender to receiver.  If the channel is full,
 * the receiver removes some from the channel and the sender
 * gets to put some in.
 */
static void altcopy(CoioAlt* s, CoioAlt* r)
{
	CoioAlt* t;
	CoioChannel* c;
	unsigned char* cp;

	/*
	 * Work out who is sender and who is receiver
	 */
	if (s == NULL && r == NULL)
		return;
	assert(s != NULL);
	c = s->c;
	if (s->op == COIO_CHANRCV)
	{
		t = s;
		s = r;
		r = t;
	}
	assert(s==NULL || s->op == COIO_CHANSND);
	assert(r==NULL || r->op == COIO_CHANRCV);

	/*
	 * Channel is empty (or unbuffered) - copy directly.
	 */
	if (s && r && c->nbuf == 0)
	{
		amove(r->v, s->v, c->elemsize);
		return;
	}

	/*
	 * Otherwise it's always okay to receive and then send.
	 */
	if (r)
	{
		cp = c->buf + c->off*c->elemsize;
		amove(r->v, cp, c->elemsize);
		--c->nbuf;
		if (++c->off == c->bufsize)
			c->off = 0;
	}
	if (s)
	{
		cp = c->buf + (c->off+c->nbuf)%c->bufsize*c->elemsize;
		amove(cp, s->v, c->elemsize);
		++c->nbuf;
	}
}

static void altexec(CoioAlt* a)
{
	int i;
	Altarray* ar;
	CoioAlt* other;
	CoioChannel* c;

	c = a->c;
	ar = chanarray(c, otherop(a->op));
	if (ar && ar->n)
	{
		i = rand()%ar->n;
		other = ar->a[i];
		altcopy(a, other);
		altalldequeue(other->xalt);
		other->xalt[0].xalt = other;
		coio_ready(other->task);
	}
	else
		altcopy(a, NULL);
}

#define dbgalt 0
int coio_chanalt(CoioAlt* a)
{
	int i, j, ncan, n, canblock;
	CoioChannel* c;
	CoioTask* t;

	//needstack(512);
	for (i=0; a[i].op != COIO_CHANEND && a[i].op != COIO_CHANNOBLK; i++)
		;
	n = i;
	canblock = a[i].op == COIO_CHANEND;

	t = coio_current;
	for (i=0; i<n; i++)
	{
		a[i].task = t;
		a[i].xalt = a;
	}
	if (dbgalt) printf("alt ");
	ncan = 0;
	for (i=0; i<n; i++)
	{
		c = a[i].c;
		if (dbgalt) printf(" %c:", "esrnb"[a[i].op]);
		if (dbgalt)
		{
			if (c->name) printf("%s", c->name);
			else printf("%p", c);
		}
		if (altcanexec(&a[i]))
		{
			if (dbgalt) printf("*");
			ncan++;
		}
	}
	if (ncan)
	{
		j = rand()%ncan;
		for (i=0; i<n; i++)
		{
			if (altcanexec(&a[i]))
			{
				if (j-- == 0)
				{
					if (dbgalt)
					{
						c = a[i].c;
						printf(" => %c:", "esrnb"[a[i].op]);
						if (c->name) printf("%s", c->name);
						else printf("%p", c);
						printf("\n");
					}
					altexec(&a[i]);
					return i;
				}
			}
		}
	}
	if (dbgalt)printf("\n");

	if (!canblock)
		return -1;

	for (i=0; i<n; i++)
	{
		if (a[i].op != COIO_CHANNOP)
			altqueue(&a[i]);
	}

	coio_transfer();

	/*
	 * the guy who ran the op took care of dequeueing us
	 * and then set a[0].alt to the one that was executed.
	 */
	return a[0].xalt - a;
}

static int _chanop(CoioChannel* c, int op, void* p, int canblock)
{
	CoioAlt a[2];

	a[0].c = c;
	a[0].op = op;
	a[0].v = p;
	a[1].op = canblock ? COIO_CHANEND : COIO_CHANNOBLK;
	if (coio_chanalt(a) < 0)
		return -1;
	return 1;
}

int coio_chansend(CoioChannel* c, void* v)
{
	return _chanop(c, COIO_CHANSND, v, 1);
}

int coio_channbsend(CoioChannel* c, void* v)
{
	return _chanop(c, COIO_CHANSND, v, 0);
}

int coio_chanrecv(CoioChannel* c, void* v)
{
	return _chanop(c, COIO_CHANRCV, v, 1);
}

int coio_channbrecv(CoioChannel* c, void* v)
{
	return _chanop(c, COIO_CHANRCV, v, 0);
}

int coio_chansendp(CoioChannel* c, void* v)
{
	return _chanop(c, COIO_CHANSND, (void*)&v, 1);
}

void* coio_chanrecvp(CoioChannel* c)
{
	void* v;

	_chanop(c, COIO_CHANRCV, (void*)&v, 1);
	return v;
}

int coio_channbsendp(CoioChannel* c, void* v)
{
	return _chanop(c, COIO_CHANSND, (void*)&v, 0);
}

void* coio_channbrecvp(CoioChannel* c)
{
	void* v;

	_chanop(c, COIO_CHANRCV, (void*)&v, 0);
	return v;
}

int coio_chansendul(CoioChannel* c, ulong val)
{
	return _chanop(c, COIO_CHANSND, &val, 1);
}

ulong coio_chanrecvul(CoioChannel* c)
{
	ulong val;

	_chanop(c, COIO_CHANRCV, &val, 1);
	return val;
}

int coio_channbsendul(CoioChannel* c, ulong val)
{
	return _chanop(c, COIO_CHANSND, &val, 0);
}

ulong coio_channbrecvul(CoioChannel* c)
{
	ulong val;

	_chanop(c, COIO_CHANRCV, &val, 0);
	return val;
}
