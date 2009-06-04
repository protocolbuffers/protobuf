
.PHONY: all clean
CC=gcc
CFLAGS=-std=c99 -O3 -Wall -Wextra -pedantic
OBJ=upb_parse.o upb_table.o upb_struct.o descriptor.o
all: $(OBJ)
clean:
	rm -f $(OBJ) tests

upb_parse.o: upb_parse.c upb_parse.h
	$(CC) $(CFLAGS) -o upb_parse.o -c upb_parse.c

upb_table.o: upb_table.c upb_table.h
	$(CC) $(CFLAGS) -o upb_table.o -c upb_table.c

upb_struct.o: upb_struct.c upb_struct.h
	$(CC) $(CFLAGS) -o upb_struct.o -c upb_struct.c

descriptor.o: descriptor.c descriptor.h
	$(CC) $(CFLAGS) -o descriptor.o -c descriptor.c

tests: tests.c upb_parse.c upb_parse.h
	$(CC) $(CFLAGS) -o tests tests.c
