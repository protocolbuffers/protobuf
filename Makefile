
.PHONY: all clean
all: pbstream.o pbstruct.o tests
clean:
	rm -f pbstream.o pbstruct.o tests

pbstream.o: pbstream.c pbstream.h
	gcc -std=c99 -O3 -Wall -o pbstream.o -c pbstream.c

pbstruct.o: pbstruct.c pbstruct.h
	gcc -std=c99 -O3 -Wall -o pbstruct.o -c pbstruct.c

tests: tests.c pbstream.c pbstream.h
	gcc -std=c99 -O3 -Wall -o tests tests.c
