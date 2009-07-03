
.PHONY: all clean
CC=gcc
CXX=g++
CFLAGS=-std=c99
CPPFLAGS=-O0 -Wall -Wextra -pedantic -g -DUPB_UNALIGNED_READS_OK -fomit-frame-pointer
OBJ=upb_parse.o upb_table.o upb_msg.o upb_context.o descriptor.o
all: $(OBJ) test_table tests
clean:
	rm -f *.o test_table tests

libupb.a: $(OBJ)
	ar rcs libupb.a $(OBJ)
test_table: libupb.a

-include deps
deps: *.c *.h
	gcc -MM *.c > deps
