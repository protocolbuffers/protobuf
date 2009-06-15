
.PHONY: all clean
CC=gcc
CXX=g++
CFLAGS=-std=c99 -fgnu89-inline
CPPFLAGS=-O3 -Wall -Wextra -pedantic -g
OBJ=upb_parse.o upb_table.o upb_struct.o descriptor.o
all: $(OBJ) test_table
clean:
	rm -f $(OBJ) tests

upb_parse.o: upb_parse.c upb_parse.h
upb_table.o: upb_table.c upb_table.h
upb_struct.o: upb_struct.c upb_struct.h
descriptor.o: descriptor.c descriptor.h
test_table: test_table.cc upb_table.o

tests: tests.c upb_parse.c upb_parse.h
	$(CC) $(CFLAGS) -o tests tests.c
