
.PHONY: all clean
CFLAGS=-std=c99 -O3 -Wall -Wextra -pedantic
OBJ=upb_parse.o upb_fieldmap.o upb_struct.o
all: $(OBJ) tests
clean:
	rm -f $(OBJ) tests

upb_parse.o: upb_parse.c upb_parse.h
	gcc $(CFLAGS) -o upb_parse.o -c upb_parse.c

upb_fieldmap.o: upb_fieldmap.c upb_fieldmap.h
	gcc $(CFLAGS) -o upb_fieldmap.o -c upb_fieldmap.c

upb_struct.o: upb_struct.c upb_struct.h
	gcc $(CFLAGS) -o upb_struct.o -c upb_struct.c

tests: tests.c upb_parse.c upb_parse.h
	gcc $(CFLAGS) -o tests tests.c
