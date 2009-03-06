
.PHONY: all clean
CFLAGS=-std=c99 -O3 -Wall -Wextra -pedantic
all: pbstream.o tests
clean:
	rm -f pbstream.o pbstruct.o tests

pbstream.o: pbstream.c pbstream.h
	gcc $(CFLAGS) -o pbstream.o -c pbstream.c

tests: tests.c pbstream.c pbstream.h
	gcc $(CFLAGS) -o tests tests.c
