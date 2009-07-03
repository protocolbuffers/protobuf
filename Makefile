
.PHONY: all clean
CC=gcc
CXX=g++
CFLAGS=-std=c99
CPPFLAGS=-Wall -Wextra -pedantic -g -DUPB_UNALIGNED_READS_OK -fomit-frame-pointer
OBJ=upb_parse.o upb_table.o upb_msg.o upb_context.o descriptor.o
all: $(OBJ) test_table tests
clean:
	rm -f $(OBJ) tests

-include deps
deps: *.c *.h
	gcc -MM *.c > deps
