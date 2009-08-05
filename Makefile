
# Function to expand a wildcard pattern recursively.
rwildcard=$(strip $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d)))

.PHONY: all clean
CC=gcc
CXX=g++
CFLAGS=-std=c99
INCLUDE=-Idescriptor -Isrc -Itests -I.
CPPFLAGS=-O3 -fomit-frame-pointer -Wall -Wextra -g -DNDEBUG $(INCLUDE)

ALL=deps $(OBJ) src/libupb.a tests/test_table tests/tests tools/upbc
all: $(ALL)
clean:
	rm -f $(call rwildcard,,*.o) $(ALL) benchmark/google_messages.proto.pb benchmark/google_messages.pb.* benchmark/b_*

# The core library (src/libupb.a)
OBJ=src/upb_parse.o src/upb_table.o src/upb_msg.o src/upb_enum.o src/upb_context.o \
    src/upb_string.o src/upb_text.o src/upb_serialize.o descriptor/descriptor.o
SRC=$(call rwildcard,,*.c)
HEADERS=$(call rwildcard,,*.h)
src/libupb.a: $(OBJ)
	ar rcs src/libupb.a $(OBJ)

# Tests
test: tests/tests
	./tests/tests
tests/test_table: src/libupb.a
tests/tests: src/libupb.a

# Tools
tools/upbc: src/libupb.a

# Benchmarks

-include deps
deps: $(SRC) $(HEADERS) gen-deps.sh Makefile
	./gen-deps.sh $(SRC)
