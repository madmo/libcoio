#include <stdio.h>
#include "coio.h"

void _t1(void *arg)
{
	printf("Hello 1 from _t1\n");
	coio_yield();
	printf("Hello 2 from _t1\n");
}

void _t2(void *arg)
{
	printf("Hello 1 from _t2\n");
	coio_yield();
	printf("Hello 2 from _t2\n");
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	coio_create(_t1, NULL, 0x8000);
	coio_create(_t2, NULL, 0x8000);

	if (coio_main() < 0)
	{
		printf("Deadlocked\n");
		return 1;
	}

	return 0;
}
