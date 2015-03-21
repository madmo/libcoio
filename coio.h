#ifndef COIO_H
#define COIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoioTask CoioTask;

int  coio_main();
int  coio_create(void (*f)(void *arg), void *arg, unsigned int stacksize);
void coio_yield();

#ifdef __cplusplus
}
#endif

#endif
