CC = gcc
CPPFLAGS = -g -O3 -Wall -march=native

OBJS = main.o iconv.o naive.o

utf8to16: ${OBJS}
	gcc $^ -o $@

.PHONY: clean
clean:
	rm -f utf8to16 *.o
