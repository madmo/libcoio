LIB = libcoio.a

OBJS = \
	coro.o \
	coio.o

all: $(LIB)

$(OBJS): coio.h coro.h

.o: .c
	$(CC) $(CFLAGS) -W -Wall -Wextra -Werror -c $*.c

$(LIB): $(OBJS)
	$(AR) rvc $(LIB) $?

testyield: testyield.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $^

test: testyield

clean:
	rm -f $(OBJS)
	rm -f $(LIB)
	rm -f testyield

.PHONY: all clean test
