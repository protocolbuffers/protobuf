
.PHONY: all clean
CC=gcc
CXX=g++
CFLAGS=-std=c99
CPPFLAGS=-O3 -Wall -Wextra -pedantic -g -DUPB_UNALIGNED_READS_OK -fomit-frame-pointer -Idescriptor -Isrc
OBJ=src/upb_parse.o src/upb_table.o src/upb_msg.o src/upb_enum.o src/upb_context.o \
    src/upb_string.o descriptor/descriptor.o
ALL=$(OBJ) src/libupb.a tests/test_table tests/tests tools/upbc
all: $(ALL)
clean:
	rm -f $(ALL) deps

src/libupb.a: $(OBJ)
	ar rcs src/libupb.a $(OBJ)
tests/test_table: src/libupb.a
tools/upbc: src/libupb.a
benchmark/benchmark: src/libupb.a -lm

-include deps
deps: src/*.c src/*.h descriptor/*.c descriptor/*.h tests/*.c tests/*.h tools/*.c tools/*.h
	gcc -MM *.c > deps
