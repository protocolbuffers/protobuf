
.PHONY: all clean
all: pbstream.o tests
clean:
	rm -f pbstream.o tests

pbstream.o: pbstream.c pbstream.h
	gcc -std=c99 -O3 -Wall -o pbstream.o -c pbstream.c

tests: tests.c pbstream.c pbstream.h
	gcc -std=c99 -O3 -Wall -o tests tests.c
