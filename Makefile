#
# This Makefile builds the upb library as well as associated tests, tools, and
# language extensions.
#
# It does not use autoconf/automake/libtool in order to stay lightweight and
# avoid the need for running ./configure.
#
# Summary of compiler flags you may want to use:
#
# * -UNDEBUG: enables assertions() (makes binary larger and slower)
# * -O0: disable optimizations
# * -g: enable debug symbols
# * -fomit-frame-pointer: makes code smaller and faster by freeing up a reg.
#
# Threading:
# * -DUPB_THREAD_UNSAFE: remove all thread-safety.

.PHONY: all lib clean tests test benchmarks benchmark descriptorgen
.PHONY: clean_leave_profile

# Prevents the deletion of intermediate files.
.SECONDARY:

UPB_MODULES = upb upb.pb upb.descriptor
UPB_LIBS = $(patsubst %,lib/lib%.a,$(UPB_MODULES))
UPB_PICLIBS = $(patsubst %,lib/lib%_pic.a,$(UPB_MODULES))

# Default rule: build all upb libraries (but no bindings)
default: $(UPB_LIBS)

# All: build absolutely everything
all: lib tests benchmarks tools/upbc lua python
testall: test pythontest

# Set this to have user-specific flags (especially things like -O0 and -g).
USER_CPPFLAGS=

# Build with "make WITH_JIT=yes" (or anything besides "no") to enable the JIT.
WITH_JIT=no

# Basic compiler/flag setup.
CC=cc
CXX=c++
CFLAGS=-std=c99
CXXFLAGS=-Wno-unused-private-field $(USER_CXXFLAGS)
INCLUDE=-I.
CPPFLAGS=$(INCLUDE) -DNDEBUG -Wall -Wextra -Wno-sign-compare $(USER_CPPFLAGS)
LDLIBS=-lpthread upb/libupb.a
LUA=lua  # 5.1 and 5.2 should both be supported

ifneq ($(WITH_JIT), no)
  USE_JIT=true
  CPPFLAGS += -DUPB_USE_JIT_X64
endif

# Build with "make Q=" to see all commands that are being executed.
Q=@

# Function to expand a wildcard pattern recursively.
rwildcard=$(strip $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d)))

ifeq ($(Q), @)
  E=@echo
else
  E=@:
endif

install:
	test -f upb/bindings/ruby/upb.so && cd upb/bindings/ruby && make install

# Dependency generating. #######################################################

-include deps
# Unfortuantely we can't easily generate deps for benchmarks or tests because
# of the scheme we use that compiles the same source file multiple times with
# different -D options, which can include different header files.
dep:
	$(E) Regenerating dependencies for upb/...
	@set -e
	@rm -f deps
	@for file in $$(find . -name '*.c'); do \
	  gcc -MM $$file -MT $${file%.*}.o $(CPPFLAGS) -I. >> deps 2> /dev/null; \
	done


clean_leave_profile:
	rm -rf obj lib
	rm -f benchmark/google_messages.proto.pb benchmark/google_messages.pb.* benchmarks/b.* benchmarks/*.pb*
	rm -f $(TESTS) tests/testmain.o tests/t.*
	rm -f tests/test.proto.pb
	rm -f upb/descriptor.pb
	rm -rf tools/upbc deps
	rm -rf upb/bindings/python/build
	rm -f upb/bindings/ruby/Makefile
	rm -f upb/bindings/ruby/upb.so
	rm -f upb/bindings/ruby/mkmf.log
	find . | grep dSYM | xargs rm -rf

clean: clean_leave_profile
	rm -rf $(call rwildcard,,*.gcno) $(call rwildcard,,*.gcda)

# A little bit of Make voodoo: you can call this from the deps of a patterned
# rule like so:
#
# all: lib/libfoo.bar.a
#
# foo_bar_SRCS = a.c b.c
#
# # This will expand into a.o b.o
# lib/lib%.a: $(call make_objs,o)
#	gcc -c -o $@ $^
#
# SECONDEXPANSION: flips on a bit essentially that allows this "seconary
# expansion": it must appear before anything that uses make_objs.
.SECONDEXPANSION:
to_srcs = $(subst .,_,$(1)_SRCS)
pc = %
make_objs = $$(patsubst upb/$$(pc).c,obj/$$(pc).$(1),$$($$(call to_srcs,$$*)))
make_objs_cc = $$(patsubst upb/$$(pc).cc,obj/$$(pc).$(1),$$($$(call to_srcs,$$*)))


# Core libraries (ie. not bindings). ###############################################################

upb_SRCS = \
  upb/def.c \
  upb/handlers.c \
  upb/refcounted.c \
  upb/shim/shim.c \
  upb/symtab.c \
  upb/table.c \
  upb/upb.c \

upb_descriptor_SRCS = \
  upb/descriptor/reader.c \
  upb/descriptor/descriptor.upb.c \

upb_pb_SRCS = \
  upb/pb/decoder.c \
  upb/pb/compile_decoder.c \
  upb/pb/glue.c \
  upb/pb/varint.c \
  upb/pb/textprinter.c \

# If the JIT is enabled we include its source.
# If Lua is present we can use DynASM to regenerate the .h file.
ifdef USE_JIT
upb_pb_SRCS += upb/pb/compile_decoder_x64.c
obj/pb/compile_decoder_x64.o obj/pb/compile_decoder_x64.lo: upb/pb/compile_decoder_x64.h

upb/pb/compile_decoder_x64.h: upb/pb/compile_decoder_x64.dasc
	$(E) DYNASM $<
	$(Q) $(LUA) dynasm/dynasm.lua upb/pb/compile_decoder_x64.dasc > upb/pb/compile_decoder_x64.h || (rm upb/pb/compile_decoder_x64.h ; false)
endif

upb_json_SRCS = \
  upb/json/typed_printer.c

# If the user doesn't specify an -O setting, we use -O3 for critical-path
# code and -Os for the rest.
ifeq (, $(findstring -O, $(USER_CFLAGS)))
OPT = -O3
lib/libupb.a : OPT = -Os
lib/libupb.descriptor.a : OPT = -Os
endif

$(UPB_PICLIBS): lib/lib%_pic.a: $(call make_objs,lo)
	$(E) AR $@
	$(Q) mkdir -p lib && ar rcs $@ $^

$(UPB_LIBS): lib/lib%.a: $(call make_objs,o)
	$(E) AR $@
	$(Q) mkdir -p lib && ar rcs $@ $^


obj/%.o: upb/%.c | $$(@D)/.
	$(E) CC $<
	$(Q) $(CC) $(OPT) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

obj/%.o: upb/%.cc | $$(@D)/.
	$(E) CXX $<
	$(Q) $(CXX) $(OPT) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

obj/%.lo: upb/%.c | $$(@D)/.
	$(E) 'CC -fPIC' $<
	$(Q) $(CC) $(OPT) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< -fPIC

obj/%.lo: upb/%.cc | $$(@D)/.
	$(E) CXX $<
	$(Q) $(CXX) $(OPT) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $< -fPIC

# Note: mkdir -p is technically susceptible to races when used with make -j.
%/.:
	$(Q) mkdir -p $@

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

# This section contains only the tests that don't depend on any external
# libraries.

C_TESTS = \
  tests/pb/test_varint \
  tests/test_def \
  tests/test_handlers \

CC_TESTS = \
  tests/pb/test_decoder \
  tests/test_cpp \
  tests/test_table \

TESTS=$(C_TESTS) $(CC_TESTS)
tests: $(TESTS)

tests/testmain.o: tests/testmain.cc
	$(E) CXX $<
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(C_TESTS): % : %.c tests/testmain.o $$(LIBS)
	$(E) CC $<
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/testmain.o $< $(LIBS)

$(CC_TESTS): % : %.cc tests/testmain.o $$(LIBS)
	$(E) CXX $<
	$(Q) $(CXX) $(CPPFLAGS) $(CXXFLAGS) -Wno-deprecated -o $@ tests/testmain.o $< $(LIBS)

# Several of these tests don't actually test these libs, but use them
# incidentally to load a descriptor
LOAD_DESCRIPTOR_LIBS = lib/libupb.pb.a lib/libupb.descriptor.a

# Specify which libs each test depends on.
tests/pb/test_varint: LIBS = lib/libupb.pb.a lib/libupb.a
tests/test_def: LIBS = $(LOAD_DESCRIPTOR_LIBS) lib/libupb.a
tests/test_handlers: LIBS = lib/libupb.descriptor.a lib/libupb.a
tests/pb/test_decoder: LIBS = lib/libupb.pb.a lib/libupb.a
tests/test_cpp: LIBS = $(LOAD_DESCRIPTOR_LIBS) lib/libupb.a
tests/test_table: LIBS = lib/libupb.a

tests/test_def: tests/test.proto.pb

tests/test.proto.pb: tests/test.proto
	@# TODO: add .proto file parser to upb so this isn't necessary.
	protoc tests/test.proto -otests/test.proto.pb

VARIADIC_TESTS= \
  tests/t.test_vs_proto2.googlemessage1 \
  tests/t.test_vs_proto2.googlemessage2 \

ifeq ($(RUN_UNDER), valgrind)
RUN_UNDER=valgrind --leak-check=full --error-exitcode=1 --track-origins=yes
endif

test: tests
	@set -e  # Abort on error.
	@find tests -perm -u+x -type f | while read test; do \
	  if [ -x ./$$test ] ; then \
	    echo "RUN $$test"; \
	    $(RUN_UNDER) ./$$test tests/test.proto.pb || exit 1; \
	  fi \
	done;
	@echo "All tests passed!"


# Google protobuf binding ######################################################

upb_bindings_googlepb_SRCS = \
  upb/bindings/googlepb/bridge.cc \
  upb/bindings/googlepb/proto2.cc \

GOOGLEPB_TESTS = \
  tests/bindings/googlepb/test_vs_proto2.googlemessage1 \
  tests/bindings/googlepb/test_vs_proto2.googlemessage2 \

GOOGLEPB_LIB=lib/libupb.bindings.googlepb.a

.PHONY: googlepb clean_googlepb googlepbtest
clean: clean_googlepb
clean_googlepb:
	rm -f tests/bindings/googlepb/test_vs_proto2.googlemessage*
	rm -f $(GOOGLEPB_LIB)
googlepb: default $(GOOGLEPB_LIB)
googlepbtest: $(GOOGLEPB_TESTS)

lib/libupb.bindings.googlepb.a: $(upb_bindings_googlepb_SRCS:upb/%.cc=obj/%.o)
	$(E) AR $@
	$(Q) mkdir -p lib && ar rcs $@ $^

# These generated files live in benchmarks/ but are used by both tests and
# benchmarks.
benchmarks/google_messages.proto.pb: benchmarks/google_messages.proto
	@# TODO: replace with upbc.
	protoc benchmarks/google_messages.proto -obenchmarks/google_messages.proto.pb
benchmarks/google_messages.pb.cc: benchmarks/google_messages.proto
	protoc benchmarks/google_messages.proto --cpp_out=.

tests/bindings/googlepb/test_vs_proto2.googlemessage1: \
    tests/bindings/googlepb/test_vs_proto2.cc $(LIBUPB) benchmarks/google_messages.proto.pb \
    benchmarks/google_messages.pb.cc
	$(E) CXX $< '(benchmarks::SpeedMessage1)'
	$(Q) $(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"../benchmarks/google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"../benchmarks/google_message1.dat\" \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_HFILE=\"../benchmarks/google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc tests/testmain.o -lprotobuf -lpthread \
	  lib/libupb.bindings.googlepb.a lib/libupb.pb.a lib/libupb.descriptor.a lib/libupb.a

tests/bindings/googlepb/test_vs_proto2.googlemessage2: \
    tests/bindings/googlepb/test_vs_proto2.cc $(LIBUPB) benchmarks/google_messages.proto.pb \
    benchmarks/google_messages.pb.cc
	$(E) CXX $< '(benchmarks::SpeedMessage2)'
	$(Q) $(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"../benchmarks/google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"../benchmarks/google_message2.dat\" \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_HFILE=\"../benchmarks/google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc tests/testmain.o -lprotobuf -lpthread \
	  lib/libupb.bindings.googlepb.a lib/libupb.pb.a lib/libupb.descriptor.a lib/libupb.a


# Lua extension ##################################################################

ifeq ($(shell uname), Darwin)
  LUA_LDFLAGS = -undefined dynamic_lookup
else
  LUA_LDFLAGS =
endif

LUAEXTS = \
  upb/bindings/lua/upb.so \
  upb/bindings/lua/upb.pb.so \

.PHONY: clean_lua testlua lua
clean: clean_lua
testlua:
	LUA_PATH=tests/bindings/lua/?.lua LUA_CPATH=upb/bindings/lua/?.so lua tests/bindings/lua/upb.lua

clean_lua:
	rm -f upb/bindings/lua/upb.lua.h
	rm -f upb/bindings/lua/upb.so
	rm -f upb/bindings/lua/upb.pb.so

lua: $(LUAEXTS)

upb/bindings/lua/upb.lua.h:
	$(E) XXD upb/bindings/lua/upb.lua
	$(Q) xxd -i < upb/bindings/lua/upb.lua > upb/bindings/lua/upb.lua.h

upb/bindings/lua/upb.so: upb/bindings/lua/upb.c upb/bindings/lua/upb.lua.h lib/libupb.descriptor_pic.a lib/libupb_pic.a lib/libupb.pb_pic.a
	$(E) CC upb/bindings/lua/upb.c
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) -fpic -shared -o $@ $< lib/libupb.pb_pic.a lib/libupb.descriptor_pic.a lib/libupb_pic.a $(LUA_LDFLAGS)

upb/bindings/lua/upb.pb.so: upb/bindings/lua/upb.pb.c lib/libupb_pic.a lib/libupb.pb_pic.a
	$(E) CC upb/bindings/lua/upb.pb.c
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) -fpic -shared -o $@ $< $(LUA_LDFLAGS)


# Python extension #############################################################

PYTHON=python
PYTHONEXT=bindings/python/build/install/lib/python/upb/__init__.so
python: $(PYTHONEXT)
$(PYTHONEXT): $(LIBUPB_PIC) bindings/python/upb.c
	$(E) PYTHON bindings/python/upb.c
	$(Q) cd bindings/python && $(PYTHON) setup.py build --debug install --home=build/install

pythontest: $(PYTHONEXT)
	cd bindings/python && cp test.py build/install/lib/python && valgrind $(PYTHON) ./build/install/lib/python/test.py

# Ruby extension ###############################################################

RUBY=ruby
RUBYEXT=upb/bindings/ruby/upb.so
ruby: $(RUBYEXT)
upb/bindings/ruby/Makefile: upb/bindings/ruby/extconf.rb lib/libupb_pic.a lib/libupb.pb_pic.a lib/libupb.descriptor_pic.a
	$(E) RUBY upb/bindings/ruby/extconf.rb
	$(Q) cd upb/bindings/ruby && ruby extconf.rb
$(RUBYEXT): upb/bindings/ruby/upb.c upb/bindings/ruby/Makefile
	$(E) CC upb/bindings/ruby/upb.c
	$(Q) cd upb/bindings/ruby && make
