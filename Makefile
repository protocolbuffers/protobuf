#
# This Makefile builds the upb library as well as associated tests, tools, and
# language extensions.
#
# It does not use autoconf/automake/libtool in order to stay lightweight and
# avoid the need for running ./configure.
#
# Summary of compiler flags you may want to use:
#
# * -DNDEBUG: makes binary smaller and faster by removing sanity checks.
# * -O3: optimize for maximum speed
# * -fomit-frame-pointer: makes code smaller and faster by freeing up a reg.
#
# Threading:
# * -DUPB_USE_PTHREADS: configures upb to use pthreads r/w lock.
# * -DUPB_THREAD_UNSAFE: remove all thread-safety.
# * -pthread: required on GCC to enable pthreads (but what does it do?)
#
# Other:
# * -DUPB_UNALIGNED_READS_OK: makes code smaller, but not standard compliant

.PHONY: all lib clean tests test benchmarks benchmark descriptorgen
.PHONY: clean_leave_profile

# Default rule: just build libupb.
default: lib

# All: build absolutely everything
all: lib tests benchmarks tools/upbc lua python
testall: test pythontest

# User-specified CFLAGS.
USER_CFLAGS=$(strip $(shell test -f perf-cppflags && cat perf-cppflags))

# If the user doesn't specify an -O setting, we default to -O3, except
# for def which gets -Os.
ifeq (, $(findstring -O, $(USER_CFLAGS)))
  USER_CFLAGS += -O3
  DEF_OPT = -Os
endif

ifneq (, $(findstring DUPB_USE_JIT_X64, $(USER_CFLAGS)))
  USE_JIT=true
endif

# Basic compiler/flag setup.
CC=gcc
CXX=g++
CFLAGS=-std=gnu99
CXXFLAGS=-Ibindings/cpp
INCLUDE=-Itests -I.
CPPFLAGS=$(INCLUDE) -Wall -Wextra $(USER_CFLAGS)
LDLIBS=-lpthread upb/libupb.a
LUA=lua5.1  # 5.1 and 5.2 should both be supported

# Build with "make Q=" to see all commands that are being executed.
Q=@

ifeq ($(Q), @)
  E=@echo
else
  E=@:
endif

# Dependency generating. #######################################################

-include deps
# Unfortuantely we can't easily generate deps for benchmarks or tests because
# of the scheme we use that compiles the same source file multiple times with
# different -D options, which can include different header files.
ALLSRC=$(shell find . -name '*.c' -print | grep -v perf-tmp)
deps: Makefile $(ALLSRC)
	$(E) Regenerating dependencies for upb/...
	@set -e
	@rm -f deps
	@for file in $(ALLSRC); do \
	  gcc -MM $$file -MT $${file%.*}.o $(CPPFLAGS) -I. >> deps 2> /dev/null; \
	done


# Source files. ###############################################################

# The core library.
CORE= \
  upb/bytestream.c \
  upb/def.c \
  upb/descriptor/reader.c \
  upb/descriptor/descriptor.upb.c \
  upb/google/bridge.cc \
  upb/google/proto2.cc \
  upb/handlers.c \
  upb/refcounted.c \
  upb/sink.c \
  upb/symtab.c \
  upb/table.c \
  upb/upb.c \

# TODO: the proto2 bridge should be built as a separate library.

# Library for the protocol buffer format (both text and binary).
PB= \
  upb/pb/decoder.c \
  upb/pb/glue.c \
  upb/pb/textprinter.c \
  upb/pb/varint.c \


# Rules. #######################################################################

clean_leave_profile:
	rm -rf $(LIBUPB) $(LIBUPB_PIC)
	rm -rf $(call rwildcard,,*.o) $(call rwildcard,,*.lo) $(call rwildcard,,*.dSYM)
	rm -rf upb/pb/decoder_x64.h
	rm -rf benchmark/google_messages.proto.pb benchmark/google_messages.pb.* benchmarks/b.* benchmarks/*.pb*
	rm -rf upb/pb/jit_debug_elf_file.o
	rm -rf upb/pb/jit_debug_elf_file.h
	rm -rf $(TESTS) tests/t.*
	rm -rf upb/descriptor.pb
	rm -rf tools/upbc deps
	rm -rf bindings/lua/upb.so
	rm -rf bindings/python/build

clean: clean_leave_profile
	rm -rf $(call rwildcard,,*.gcno) $(call rwildcard,,*.gcda)

# Core library (libupb.a).
SRC=$(CORE) $(PB)
LIBUPB=upb/libupb.a
LIBUPB_PIC=upb/libupb_pic.a
lib: $(LIBUPB)


OBJ=$(patsubst %.c,%.o,$(SRC)) $(patsubst %.cc,%.o,$(SRC))
PICOBJ=$(patsubst %.c,%.lo,$(SRC)) $(patsubst %.cc,%.lo,$(SRC))


ifdef USE_JIT
upb/pb/decoder.o upb/pb/decoder.lo: upb/pb/decoder_x64.h
endif
$(LIBUPB): $(OBJ)
	$(E) AR $(LIBUPB)
	$(Q) ar rcs $(LIBUPB) $(OBJ)
$(LIBUPB_PIC): $(PICOBJ)
	$(E) AR $(LIBUPB_PIC)
	$(Q) ar rcs $(LIBUPB_PIC) $(PICOBJ)

%.o : %.c
	$(E) CC $<
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.o : %.cc
	$(E) CXX $<
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

%.lo : %.c
	$(E) 'CC -fPIC' $<
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

%.o : %.cc
	$(E) CXX $<
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC

# Override the optimization level for def.o, because it is not in the
# critical path but gets very large when -O3 is used.
upb/def.o: upb/def.c
	$(E) CC $<
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) $(DEF_OPT) -c -o $@ $<

upb/def.lo: upb/def.c
	$(E) 'CC -fPIC' $<
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) $(DEF_OPT) -c -o $@ $< -fPIC

upb/pb/decoder_x64.h: upb/pb/decoder_x64.dasc
	$(E) DYNASM $<
	$(Q) $(LUA) dynasm/dynasm.lua upb/pb/decoder_x64.dasc > upb/pb/decoder_x64.h

ifneq ($(shell uname), Darwin)
upb/pb/jit_debug_elf_file.o: upb/pb/jit_debug_elf_file.s
	$(E) GAS $<
	$(Q) gcc -c upb/pb/jit_debug_elf_file.s -o upb/pb/jit_debug_elf_file.o

upb/pb/jit_debug_elf_file.h: upb/pb/jit_debug_elf_file.o
	$(E) XXD $<
	$(Q) xxd -i < upb/pb/jit_debug_elf_file.o > upb/pb/jit_debug_elf_file.h
upb/pb/decoder_x64.h: upb/pb/jit_debug_elf_file.h
endif

# Function to expand a wildcard pattern recursively.
rwildcard=$(strip $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d)))

# Regenerating the auto-generated files in upb/.
upb/descriptor.pb: upb/descriptor.proto
	@# TODO: replace with upbc
	protoc upb/descriptor.proto -oupb/descriptor.pb

descriptorgen: upb/descriptor.pb tools/upbc
	@# Regenerate descriptor_const.h
	./tools/upbc -o upb/descriptor upb/descriptor.pb

tools/upbc: tools/upbc.c $(LIBUPB)
	$(E) CC $<
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBUPB)

examples/msg: examples/msg.c $(LIBUPB)
	$(E) CC $<
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBUPB)

# Tests. #######################################################################

tests/test.proto.pb: tests/test.proto
	@# TODO: replace with upbc
	protoc tests/test.proto -otests/test.proto.pb

SIMPLE_TESTS= \
  tests/test_def \
  tests/test_varint \

SIMPLE_CXX_TESTS= \
  tests/test_cpp \

  # The build process for this test is complicated and hasn't been
  # ported to the open-source Makefile yet.
  # tests/test_decoder \

VARIADIC_TESTS= \
  tests/t.test_vs_proto2.googlemessage1 \
  tests/t.test_vs_proto2.googlemessage2 \

TESTS=$(SIMPLE_TESTS) $(SIMPLE_CXX_TESTS) $(VARIADIC_TESTS) tests/test_table


tests: $(TESTS) $(INTERACTIVE_TESTS)
$(TESTS): $(LIBUPB)
tests/test_def: tests/test.proto.pb

tests/testmain.o: tests/testmain.cc
	$(E) CXX $<
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(SIMPLE_TESTS): tests/testmain.o
$(SIMPLE_TESTS): % : %.c
	$(E) CC $<
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tests/testmain.o $< $(LIBUPB)

$(SIMPLE_CXX_TESTS): tests/testmain.o
$(SIMPLE_CXX_TESTS): % : %.cc
	$(E) CXX $<
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ tests/testmain.o $< $(LIBUPB)

VALGRIND=valgrind --leak-check=full --error-exitcode=1
test: tests
	@echo Running all tests under valgrind.
	@set -e  # Abort on error.
	@for test in $(SIMPLE_TESTS) $(SIMPLE_CXX_TESTS); do \
	  if [ -x ./$$test ] ; then \
	    echo !!! $(VALGRIND) ./$$test; \
	    $(VALGRIND) ./$$test tests/test.proto.pb || exit 1; \
	  fi \
	done;
	@$(VALGRIND) ./tests/t.test_vs_proto2.googlemessage1 benchmarks/google_message1.dat || exit 1;
	@$(VALGRIND) ./tests/t.test_vs_proto2.googlemessage2 benchmarks/google_message2.dat || exit 1;
	@echo "All tests passed!"

tests/t.test_vs_proto2.googlemessage1 \
tests/t.test_vs_proto2.googlemessage2: \
    tests/test_vs_proto2.cc $(LIBUPB) benchmarks/google_messages.proto.pb \
    benchmarks/google_messages.pb.cc
	$(E) CXX $< '(benchmarks::SpeedMessage1)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o tests/t.test_vs_proto2.googlemessage1 $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"../benchmarks/google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"../benchmarks/google_message1.dat\" \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_HFILE=\"../benchmarks/google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc tests/testmain.o -lprotobuf -lpthread $(LIBUPB)
	$(E) CXX $< '(benchmarks::SpeedMessage2)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o tests/t.test_vs_proto2.googlemessage2 $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"../benchmarks/google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"../benchmarks/google_message2.dat\" \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_HFILE=\"../benchmarks/google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc tests/testmain.o -lprotobuf -lpthread $(LIBUPB)
tests/test_table: tests/test_table.cc
	@# Includes <hash_set> which is a deprecated header.
	$(E) CXX $<
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -Wno-deprecated -o $@ $< tests/testmain.o $(LIBUPB)

tests/tests: upb/libupb.a


# Benchmarks. ##################################################################

# Benchmarks
UPB_BENCHMARKS=benchmarks/b.parsestream_googlemessage1.upb_table \
               benchmarks/b.parsestream_googlemessage2.upb_table \

ifdef USE_JIT
UPB_BENCHMARKS += \
               benchmarks/b.parsestream_googlemessage1.upb_jit \
               benchmarks/b.parsestream_googlemessage2.upb_jit \
               benchmarks/b.parsetoproto2_googlemessage1.upb_jit \
               benchmarks/b.parsetoproto2_googlemessage2.upb_jit
endif

BENCHMARKS=$(UPB_BENCHMARKS) \
           benchmarks/b.parsetostruct_googlemessage1.proto2_table \
           benchmarks/b.parsetostruct_googlemessage2.proto2_table \
           benchmarks/b.parsetostruct_googlemessage1.proto2_compiled \
           benchmarks/b.parsetostruct_googlemessage2.proto2_compiled \
           benchmarks/b.parsetoproto2_googlemessage1.upb \
           benchmarks/b.parsetoproto2_googlemessage2.upb

upb_benchmarks: $(UPB_BENCHMARKS)
benchmarks: $(BENCHMARKS)
upb_benchmark: $(UPB_BENCHMARKS)
	@rm -f benchmarks/results
	@rm -rf benchmarks/*.dSYM
	@for test in benchmarks/b.* ; do ./$$test ; done
benchmark: $(BENCHMARKS)
	@rm -f benchmarks/results
	@rm -rf benchmarks/*.dSYM
	@for test in benchmarks/b.* ; do ./$$test ; done

benchmarks/google_messages.proto.pb: benchmarks/google_messages.proto
	@# TODO: replace with upbc.
	protoc benchmarks/google_messages.proto -obenchmarks/google_messages.proto.pb

benchmarks/google_messages.pb.cc: benchmarks/google_messages.proto
	protoc benchmarks/google_messages.proto --cpp_out=.

# This basic idea is useful (varying the benchmarks' setup by recompiling
# several times with different #defines) but the implementation in this
# Makefile is verbose and error-prone.  Open to better ideas here.  I don't
# want to make these command-line parameters -- it makes it more annoying to
# debug or profile them.

benchmarks/b.parsetostruct_googlemessage1.upb_table \
benchmarks/b.parsetostruct_googlemessage2.upb_table: \
    benchmarks/parsetostruct.upb.c $(LIBUPB) benchmarks/google_messages.proto.pb
	$(E) 'CC benchmarks/parsetostruct.upb.c (benchmarks.SpeedMessage1, nojit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.upb_table $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DJIT=false $(LIBUPB)
	$(E) 'CC benchmarks/parsetostruct.upb.c (benchmarks.SpeedMessage2, nojit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.upb_table $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DJIT=false $(LIBUPB)

benchmarks/b.parsestream_googlemessage1.upb_table \
benchmarks/b.parsestream_googlemessage2.upb_table: \
    benchmarks/parsestream.upb.c $(LIBUPB) benchmarks/google_messages.proto.pb
	$(E) 'CC benchmarks/parsestream.upb.c (benchmarks.SpeedMessage1, nojit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsestream_googlemessage1.upb_table $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" -DJIT=false \
	  $(LIBUPB)
	$(E) 'CC benchmarks/parsestream.upb.c (benchmarks.SpeedMessage2, nojit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsestream_googlemessage2.upb_table $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" -DJIT=false \
	  $(LIBUPB)

ifdef USE_JIT
benchmarks/b.parsetostruct_googlemessage1.upb_jit \
benchmarks/b.parsetostruct_googlemessage2.upb_jit: \
    benchmarks/parsetostruct.upb.c $(LIBUPB) benchmarks/google_messages.proto.pb
	$(E) 'CC benchmarks/parsetostruct.upb.c (benchmarks.SpeedMessage1, jit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.upb_jit $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" -DJIT=true \
	  -DJIT=true $(LIBUPB)
	$(E) 'CC benchmarks/parsetostruct.upb.c (benchmarks.SpeedMessage2, jit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.upb_jit $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" -DJIT=true \
	  -DJIT=true $(LIBUPB)

benchmarks/b.parsestream_googlemessage1.upb_jit \
benchmarks/b.parsestream_googlemessage2.upb_jit: \
    benchmarks/parsestream.upb.c $(LIBUPB) benchmarks/google_messages.proto.pb
	$(E) 'CC benchmarks/parsestream.upb.c (benchmarks.SpeedMessage1, jit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsestream_googlemessage1.upb_jit $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" -DJIT=true \
	  $(LIBUPB)
	$(E) 'CC benchmarks/parsestream.upb.c (benchmarks.SpeedMessage2, jit)'
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsestream_googlemessage2.upb_jit $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" -DJIT=true \
	  $(LIBUPB)

benchmarks/b.parsetoproto2_googlemessage1.upb_jit \
benchmarks/b.parsetoproto2_googlemessage2.upb_jit: \
    benchmarks/parsetoproto2.upb.cc benchmarks/google_messages.pb.cc $(LIBUPB) benchmarks/google_messages.proto.pb
	$(E) 'CXX benchmarks/parsetoproto2.upb.cc (benchmarks.SpeedMessage1, jit)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetoproto2_googlemessage1.upb_jit $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" -DJIT=true \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread $(LIBUPB)
	$(E) 'CXX benchmarks/parsetoproto2.upb.cc (benchmarks.SpeedMessage2, jit)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetoproto2_googlemessage2.upb_jit $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" -DJIT=true \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread $(LIBUPB)
endif

benchmarks/b.parsetostruct_googlemessage1.proto2_table \
benchmarks/b.parsetostruct_googlemessage2.proto2_table: \
    benchmarks/parsetostruct.proto2_table.cc benchmarks/google_messages.pb.cc
	$(E) 'CXX benchmarks/parsetostruct.proto2_table.cc (benchmarks.SpeedMessage1)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.proto2_table $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread
	$(E) 'CXX benchmarks/parsetostruct.proto2_table.cc (benchmarks.SpeedMessage2)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.proto2_table $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread

benchmarks/b.parsetostruct_googlemessage1.proto2_compiled \
benchmarks/b.parsetostruct_googlemessage2.proto2_compiled: \
    benchmarks/parsetostruct.proto2_compiled.cc benchmarks/google_messages.pb.cc
	$(E) 'CXX benchmarks/parsetostruct.proto2_compiled.cc (benchmarks.SpeedMessage1)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.proto2_compiled $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread
	$(E) 'CXX benchmarks/parsetostruct.proto2_compiled.cc (benchmarks.SpeedMessage2)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.proto2_compiled $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread

benchmarks/b.parsetoproto2_googlemessage1.upb \
benchmarks/b.parsetoproto2_googlemessage2.upb: \
    benchmarks/parsetoproto2.upb.cc benchmarks/google_messages.pb.cc $(LIBUPB) benchmarks/google_messages.proto.pb
	$(E) 'CXX benchmarks/parsetoproto2.upb.cc (benchmarks.SpeedMessage1, nojit)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetoproto2_googlemessage1.upb $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" -DJIT=false \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread $(LIBUPB)
	$(E) 'CXX benchmarks/parsetoproto2.upb.cc (benchmarks.SpeedMessage2, nojit)'
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetoproto2_googlemessage2.upb $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" -DJIT=false \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread $(LIBUPB)


# Lua extension ##################################################################

LUA_CPPFLAGS = $(strip $(shell pkg-config --silence-errors --cflags lua || pkg-config --silence-errors --cflags lua5.1 || echo '-I/usr/local/include'))
ifeq ($(shell uname), Darwin)
  LUA_LDFLAGS = -undefined dynamic_lookup
else
  LUA_LDFLAGS =
endif

LUAEXT=bindings/lua/upb.so
lua: $(LUAEXT)
bindings/lua/upb.so: bindings/lua/upb.c $(LIBUPB_PIC)
	$(E) CC bindings/lua/upb.c
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_CPPFLAGS) -fpic -shared -o $@ $< upb/libupb_pic.a $(LUA_LDFLAGS)


# Python extension #############################################################

PYTHON=python
PYTHONEXT=bindings/python/build/install/lib/python/upb/__init__.so
python: $(PYTHONEXT)
$(PYTHONEXT): $(LIBUPB_PIC) bindings/python/upb.c
	$(E) PYTHON bindings/python/upb.c
	$(Q) cd bindings/python && $(PYTHON) setup.py build --debug install --home=build/install

pythontest: $(PYTHONEXT)
	cd bindings/python && cp test.py build/install/lib/python && valgrind $(PYTHON) ./build/install/lib/python/test.py
