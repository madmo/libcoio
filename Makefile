LIB = libcoio.a

OBJS = \
	coro.o \
	coio.o \
	coio_glib.o

all: $(LIB)

$(OBJS): coio.h coro.h

.c.o:
	$(CC) $(shell pkg-config --cflags glib-2.0) $(CFLAGS) -W -Wall -Wextra -Werror -c $*.c

$(LIB): $(OBJS)
	$(AR) rvc $(LIB) $?

testyield: testyield.c $(LIB)
	$(CC) $(CFLAGS) -o $@ testyield.c $(LIB)

testdelay: testdelay.c $(LIB)
	$(CC) $(CFLAGS) -o $@ testdelay.c $(LIB)

test: testyield testdelay

clean:
	rm -f $(OBJS)
	rm -f $(LIB)
	rm -f testyield testdelay

.PHONY: all clean test
.SUFFIXES: .c .o
