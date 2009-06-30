
.PHONY: all clean
CC=gcc
CXX=g++
CFLAGS=-std=c99
CPPFLAGS=-O3 -DNDEBUG -Wall -Wextra -pedantic -g -DUPB_UNALIGNED_READS_OK -fomit-frame-pointer
OBJ=upb_parse.o upb_table.o upb_msg.o upb_context.o descriptor.o
all: $(OBJ) test_table tests
clean:
	rm -f $(OBJ) tests

upb_parse.o: upb_parse.c upb_parse.h
upb_table.o: upb_table.c upb_table.h
upb_msg.o: upb_msg.c upb_msg.h
upb_inlinedefs.o: upb_inlinedefs.c upb_msg.h
descriptor.o: descriptor.c descriptor.h
test_table: test_table.cc upb_table.o

tests: tests.c upb_parse.c upb_parse.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o tests tests.c
