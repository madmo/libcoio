LIB = libcoio.a

OBJS = \
	coio.o

all: $(LIB)

$(OBJS): coio.h coroutine.h

.c.o:
	$(CC) $(CFLAGS) -W -Wall -Wextra -Werror -c $*.c

$(LIB): $(OBJS)
	$(AR) rvc $(LIB) $?

testyield: testyield.c $(LIB) coio.h coroutine.h
	$(CC) $(CFLAGS) -o $@ testyield.c $(LIB)

testdelay: testdelay.c $(LIB) coio.h coroutine.h
	$(CC) $(CFLAGS) -o $@ testdelay.c $(LIB)

test: testyield testdelay

clean:
	rm -f $(OBJS)
	rm -f $(LIB)
	rm -f testyield testdelay

.PHONY: all clean test
.SUFFIXES: .c .o
