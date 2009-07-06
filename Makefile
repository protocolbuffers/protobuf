
.PHONY: all clean
CC=gcc
CXX=g++
CFLAGS=-std=c99
CPPFLAGS=-Wall -Wextra -pedantic -g -DUPB_UNALIGNED_READS_OK -fomit-frame-pointer
OBJ=upb_parse.o upb_table.o upb_msg.o upb_enum.o upb_context.o upb_string.o descriptor.o
all: $(OBJ) test_table tests upbc
clean:
	rm -f *.o test_table tests

libupb.a: $(OBJ)
	ar rcs libupb.a $(OBJ)
test_table: libupb.a
upbc: libupb.a

-include deps
deps: *.c *.h
	gcc -MM *.c > deps
