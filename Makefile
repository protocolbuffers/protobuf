
.PHONY: all clean
all: pbstream.o
clean:
	rm -f pbstream.o

pbstream.o: pbstream.c
	gcc -std=c99 -O3 -Wall -o pbstream.o -c pbstream.c
