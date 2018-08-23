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

.PHONY: all lib clean tests test descriptorgen amalgamate
.PHONY: clean_leave_profile genfiles

# Prevents the deletion of intermediate files.
.SECONDARY:

UPB_MODULES = upb upb.pb upb.json upb.descriptor
UPB_LIBS = $(patsubst %,lib/lib%.a,$(UPB_MODULES))
UPB_PICLIBS = $(patsubst %,lib/lib%_pic.a,$(UPB_MODULES))

# Default rule: build all upb libraries (but no bindings)
default: $(UPB_LIBS)

# All: build absolutely everything
all: lib tests tools/upbc lua python
testall: test pythontest

# Set this to have user-specific flags (especially things like -O0 and -g).
USER_CPPFLAGS?=

# Build with "make WITH_JIT=yes" (or anything besides "no") to enable the JIT.
WITH_JIT=no

# Build with "make UPB_FAIL_WARNINGS=yes" (or anything besides "no") to turn
# warnings into errors.
UPB_FAIL_WARNINGS?=no

# Basic compiler/flag setup.
# We are C89/C++98, with the one exception that we need stdint and "long long."
CC?=cc
CXX?=c++
AR?=ar
CFLAGS=
CXXFLAGS=
INCLUDE=-I.
CSTD=-std=c89 -pedantic -Wno-long-long
CXXSTD=-std=c++98 -pedantic -Wno-long-long
WARNFLAGS=-Wall -Wextra -Wpointer-arith
WARNFLAGS_CXX=$(WARNFLAGS) -Wno-unused-private-field
CPPFLAGS=$(INCLUDE) -DNDEBUG $(USER_CPPFLAGS)
LUA=lua  # 5.1 and 5.2 should both be supported

ifneq ($(WITH_JIT), no)
  USE_JIT=true
  CPPFLAGS += -DUPB_USE_JIT_X64
  EXTRA_LIBS += -ldl
endif

ifeq ($(CC), clang)
  WARNFLAGS += -Wconditional-uninitialized
endif

ifeq ($(CXX), clang++)
  WARNFLAGS_CXX += -Wconditional-uninitialized
endif

ifneq ($(UPB_FAIL_WARNINGS), no)
  WARNFLAGS += -Werror -Wno-keyword-macro
  WARNFLAGS_CXX += -Werror -Wno-keyword-macro
endif

# Build with "make Q=" to see all commands that are being executed.
Q?=@

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
	@rm -rf obj lib
	@rm -f tests/google_message?.h
	@rm -f tests/json/test.upbdefs.o
	@rm -f $(TESTS) tests/testmain.o tests/t.* tests/conformance_upb
	@rm -rf tools/upbc deps
	@rm -rf upb/bindings/python/build
	@rm -f upb/bindings/ruby/Makefile
	@rm -f upb/bindings/ruby/upb.o
	@rm -f upb/bindings/ruby/upb.so
	@rm -f upb/bindings/ruby/mkmf.log
	@rm -f tests/google_messages.pb.*
	@rm -f tests/google_messages.proto.pb
	@rm -f upb.c upb.h
	@find . | grep dSYM | xargs rm -rf

clean: clean_leave_profile clean_lua
	@rm -rf $(call rwildcard,,*.gcno) $(call rwildcard,,*.gcda)

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
# SECONDEXPANSION: flips on a bit essentially that allows this "secondary
# expansion": it must appear before anything that uses make_objs.
.SECONDEXPANSION:
to_srcs = $(subst .,_,$(1)_SRCS)
pc = %
make_objs = $$(patsubst $$(pc).c,obj/$$(pc).$(1),$$($$(call to_srcs,$$*)))
make_objs_cc = $$(patsubst $$(pc).cc,obj/$$(pc).$(1),$$($$(call to_srcs,$$*)))


# Core libraries (ie. not bindings). ###############################################################

upb_SRCS = \
  google/protobuf/descriptor.upb.c \
  upb/decode.c \
  upb/def.c \
  upb/encode.c \
  upb/handlers.c \
  upb/msg.c \
  upb/msgfactory.c \
  upb/refcounted.c \
  upb/sink.c \
  upb/table.c \
  upb/upb.c \

upb_descriptor_SRCS = \
  upb/descriptor/descriptor.upbdefs.c \
  upb/descriptor/reader.c \

upb_pb_SRCS = \
  upb/pb/compile_decoder.c \
  upb/pb/decoder.c \
  upb/pb/encoder.c \
  upb/pb/glue.c \
  upb/pb/textprinter.c \
  upb/pb/varint.c \

# If the JIT is enabled we include its source.
# If Lua is present we can use DynASM to regenerate the .h file.
ifdef USE_JIT
upb_pb_SRCS += upb/pb/compile_decoder_x64.c

# The JIT can't compile with -Wpedantic, since it does some inherently
# platform-specific things like casting between data pointers and function
# pointers.  Also DynASM emits some GNU extensions.
obj/upb/pb/compile_decoder_x64.o : CSTD = -std=gnu89
obj/upb/pb/compile_decoder_x64.lo : CSTD = -std=gnu89

upb/pb/compile_decoder_x64.h: upb/pb/compile_decoder_x64.dasc
	$(E) DYNASM $<
	$(Q) $(LUA) third_party/dynasm/dynasm.lua -c upb/pb/compile_decoder_x64.dasc > upb/pb/compile_decoder_x64.h || (rm upb/pb/compile_decoder_x64.h ; false)
endif

upb_json_SRCS = \
  upb/json/parser.c \
  upb/json/printer.c \

# Ideally we could keep this uncommented, but Git apparently sometimes skews
# timestamps slightly at "clone" time, which makes "Make" think that it needs
# to rebuild upb/json/parser.c when it actually doesn't.  This would be harmless
# except that the user might not have Ragel installed.
#
# So instead we require an excplicit "make ragel" to rebuild this (for now).
# More pain for people developing upb/json/parser.rl, but less pain for everyone
# else.
#
# upb/json/parser.c: upb/json/parser.rl
# 	$(E) RAGEL $<
# 	$(Q) ragel -C -o upb/json/parser.c upb/json/parser.rl
ragel:
	$(E) RAGEL upb/json/parser.rl
	$(Q) ragel -C -o upb/json/parser.c upb/json/parser.rl

# If the user doesn't specify an -O setting, we use -O3 for critical-path
# code and -Os for the rest.
ifeq (, $(findstring -O, $(USER_CPPFLAGS)))
OPT = -O3
lib/libupb.a : OPT = -Os
lib/libupb.descriptor.a : OPT = -Os
obj/upb/pb/compile_decoder.o : OPT = -Os
obj/upb/pb/compile_decoder_64.o : OPT = -Os

ifdef USE_JIT
obj/upb/pb/compile_decoder_x64.o: OPT=-Os
endif

endif

$(UPB_PICLIBS): lib/lib%_pic.a: $(call make_objs,lo)
	$(E) AR $@
	$(Q) mkdir -p lib && $(AR) rcs $@ $^

$(UPB_LIBS): lib/lib%.a: $(call make_objs,o)
	$(E) AR $@
	$(Q) mkdir -p lib && $(AR) rcs $@ $^


obj/%.o: %.c | $$(@D)/.
	$(E) CC $<
	$(Q) $(CC) $(OPT) $(CSTD) $(WARNFLAGS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

obj/%.o: %.cc | $$(@D)/.
	$(E) CXX $<
	$(Q) $(CXX) $(OPT) $(CXXSTD) $(WARNFLAGS_CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

obj/%.lo: %.c | $$(@D)/.
	$(E) 'CC -fPIC' $<
	$(Q) $(CC) $(OPT) $(CSTD) $(WARNFLAGS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< -fPIC

obj/%.lo: %.cc | $$(@D)/.
	$(E) CXX -fPIC $<
	$(Q) $(CXX) $(OPT) $(CXXSTD) $(WARNFLAGS_CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $< -fPIC

# Note: mkdir -p is technically susceptible to races when used with make -j.
%/.:
	$(Q) mkdir -p $@

# Regenerating the auto-generated files in upb/.
upb/descriptor/descriptor.pb: upb/descriptor/descriptor.proto
	$(E) PROTOC upb/descriptor/descriptor.proto
	$(Q) protoc upb/descriptor/descriptor.proto -oupb/descriptor/descriptor.pb

# "genfiles" includes Proto schemas we need for tests
# For the moment we check in the *.upbdefs.* generated files so that people
# can build and run the tests without Lua as a build dependency.

genfiles: tools/upbc
	@# TODO: replace protoc with upbc when upb can parse .proto files
	$(E) PROTOC upb/descriptor/descriptor.proto
	$(Q) protoc upb/descriptor/descriptor.proto -oupb/descriptor/descriptor.pb
	$(E) PROTOC google/protobuf/descriptor.proto
	$(Q) protoc google/protobuf/descriptor.proto -ogoogle/protobuf/descriptor.pb
	$(E) UPBC upb/descriptor/descriptor.pb
	$(Q) ./tools/upbc --generate-upbdefs upb/descriptor/descriptor.pb
	$(E) UPBC google/protobuf/descriptor.pb
	$(Q) ./tools/upbc google/protobuf/descriptor.pb
	$(E) PROTOC tests/json/test.proto
	$(Q) protoc tests/json/test.proto -otests/json/test.proto.pb
	$(E) UPBC tests/json/test.proto.pb
	$(Q) ./tools/upbc --generate-upbdefs tests/json/test.proto.pb
	$(E) DYNASM upb/pb/compile_decoder_x64.dasc
	$(Q) $(LUA) third_party/dynasm/dynasm.lua -c upb/pb/compile_decoder_x64.dasc > upb/pb/compile_decoder_x64.h || (rm upb/pb/compile_decoder_x64.h ; false)

# upbc depends on these Lua extensions.
UPBC_LUA_EXTS = \
  upb/bindings/lua/upb_c.so \
  upb/bindings/lua/upb/pb_c.so \
  upb/bindings/lua/upb/table_c.so \

tools/upbc: $(UPBC_LUA_EXTS) Makefile
	$(E) ECHO tools/upbc
	$(Q) echo "#!/bin/sh" > tools/upbc
	$(Q) echo 'BASE=`dirname "$$0"`' >> tools/upbc
	$(Q) echo 'export LUA_CPATH="$$BASE/../upb/bindings/lua/?.so"' >> tools/upbc
	$(Q) echo 'export LUA_PATH="$$BASE/?.lua;$$BASE/../upb/bindings/lua/?.lua"' >> tools/upbc
	$(Q) echo 'lua $$BASE/upbc.lua "$$@"' >> tools/upbc
	$(Q) chmod a+x tools/upbc

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
  tests/pb/test_encoder \
  tests/json/test_json \
  tests/test_cpp \
  tests/test_table \

TESTS=$(C_TESTS) $(CC_TESTS)
tests: $(TESTS)

tests/json/test.upbdefs.o: tests/json/test.upbdefs.c
	$(E) CC $<
	$(Q) $(CC) $(OPT) $(CSTD) $(WARNFLAGS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

tests/testmain.o: tests/testmain.cc
	$(E) CXX $<
	$(Q) $(CXX) $(OPT) $(CXXSTD) $(WARNFLAGS_CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(C_TESTS): % : %.c tests/testmain.o $$(LIBS)
	$(E) CC $<
	$(Q) $(CC) $(OPT) $(CSTD) $(WARNFLAGS) $(CPPFLAGS) $(CFLAGS) -o $@ tests/testmain.o $< $(LIBS)

$(CC_TESTS): % : %.cc tests/testmain.o $$(LIBS)
	$(E) CXX $<
	$(Q) $(CXX) $(OPT) $(CXXSTD) $(WARNFLAGS_CXX) $(CPPFLAGS) $(CXXFLAGS) -Wno-deprecated -o $@ tests/testmain.o $< $(LIBS)

# Several of these tests don't actually test these libs, but use them
# incidentally to load a descriptor
LOAD_DESCRIPTOR_LIBS = lib/libupb.pb.a lib/libupb.descriptor.a

# Specify which libs each test depends on.
tests/pb/test_varint: LIBS = lib/libupb.pb.a lib/libupb.a $(EXTRA_LIBS)
tests/test_def: LIBS = $(LOAD_DESCRIPTOR_LIBS) lib/libupb.a $(EXTRA_LIBS)
tests/test_handlers: LIBS = lib/libupb.descriptor.a lib/libupb.a $(EXTRA_LIBS)
tests/pb/test_decoder: LIBS = lib/libupb.pb.a lib/libupb.a $(EXTRA_LIBS)
tests/pb/test_encoder: LIBS = lib/libupb.pb.a lib/libupb.descriptor.a lib/libupb.a $(EXTRA_LIBS)
tests/test_cpp: LIBS = $(LOAD_DESCRIPTOR_LIBS) lib/libupb.a $(EXTRA_LIBS)
tests/test_table: LIBS = lib/libupb.a $(EXTRA_LIBS)
tests/json/test_json: LIBS = lib/libupb.a lib/libupb.json.a tests/json/test.upbdefs.o $(EXTRA_LIBS)

tests/test.proto.pb: tests/test.proto
	@# TODO: add .proto file parser to upb so this isn't necessary.
	protoc tests/test.proto -otests/test.proto.pb

VARIADIC_TESTS= \
  tests/t.test_vs_proto2.googlemessage1 \
  tests/t.test_vs_proto2.googlemessage2 \

ifeq ($(RUN_UNDER), valgrind)
RUN_UNDER=valgrind --leak-check=full --error-exitcode=1 --track-origins=yes
endif

test:
	@set -e  # Abort on error.
	@find tests -perm -u+x -type f | while read test; do \
	  if [ -x ./$$test ] ; then \
	    echo "RUN $$test"; \
	    $(RUN_UNDER) ./$$test tests/test.proto.pb || exit 1; \
	  fi \
	done;
	@echo "All tests passed!"

obj/conformance_protos: obj/conformance_protos.pb tools/upbc
	cd obj && ../tools/upbc conformance_protos.pb && touch conformance_protos

obj/conformance_protos.pb: third_party/protobuf/autogen.sh
	protoc -Ithird_party/protobuf/conformance -Ithird_party/protobuf/src --include_imports \
	  third_party/protobuf/conformance/conformance.proto \
	  third_party/protobuf/src/google/protobuf/test_messages_proto3.proto \
	  -o obj/conformance_protos.pb

third_party/protouf/autogen.sh: .gitmodules
	git submodule init && git submodule update

tests/conformance_upb: tests/conformance_upb.c lib/libupb.a obj/conformance_protos
	$(CC) -o tests/conformance_upb tests/conformance_upb.c -Iobj -I. $(CPPFLAGS) $(CFLAGS) obj/conformance.upb.c obj/google/protobuf/*.upb.c lib/libupb.a


# Lua extension ##################################################################

ifeq ($(shell uname), Darwin)
  LUA_LDFLAGS = -undefined dynamic_lookup -flat_namespace
else
  LUA_LDFLAGS =
endif

LUAEXTS = \
  upb/bindings/lua/upb_c.so \
  upb/bindings/lua/upb/pb_c.so \

LUATESTS = \
  tests/bindings/lua/test_upb.lua \
  tests/bindings/lua/test_upb.pb.lua \

.PHONY: clean_lua testlua lua

testlua: lua
	@set -e; \
	for test in $(LUATESTS) ; do \
	  echo LUA $$test; \
	  LUA_PATH="third_party/lunit/?.lua;upb/bindings/lua/?.lua" \
	    LUA_CPATH=upb/bindings/lua/?.so \
	    $(RUN_UNDER) lua $$test; \
	done

clean: clean_lua
clean_lua:
	@rm -f upb/bindings/lua/upb_c.so
	@rm -f upb/bindings/lua/upb/pb_c.so
	@rm -f upb/bindings/lua/upb/table_c.so

lua: $(LUAEXTS)

# Right now the core upb module depends on all of these.
# It's a TODO to factor this more cleanly in the code.
LUA_LIB_DEPS = \
  lib/libupb.pb_pic.a \
  lib/libupb.descriptor_pic.a \
  lib/libupb_pic.a \

upb/bindings/lua/upb_c.so: upb/bindings/lua/upb.c upb/bindings/lua/def.c upb/bindings/lua/msg.c $(LUA_LIB_DEPS)
	$(E) 'CC upb/bindings/lua/{upb,def,msg}.c'
	$(Q) $(CC) $(OPT) $(CSTD) $(WARNFLAGS) $(CPPFLAGS) $(CFLAGS) -fpic -shared -o $@ $^ $(LUA_LDFLAGS)

upb/bindings/lua/upb/table_c.so: upb/bindings/lua/upb/table.c lib/libupb_pic.a
	$(E) CC upb/bindings/lua/upb/table.c
	$(Q) $(CC) $(OPT) $(CSTD) $(WARNFLAGS) $(CPPFLAGS) $(CFLAGS) -fpic -shared -o $@ $^ $(LUA_LDFLAGS)

upb/bindings/lua/upb/pb_c.so: upb/bindings/lua/upb/pb.c $(LUA_LIB_DEPS)
	$(E) CC upb/bindings/lua/upb/pb.c
	$(Q) $(CC) $(OPT) $(CSTD) $(WARNFLAGS) $(CPPFLAGS) $(CFLAGS) -fpic -shared -o $@ $^ $(LUA_LDFLAGS)

# Amalgamated source (upb.c/upb.h) ############################################

AMALGAMATE_SRCS=$(upb_SRCS) $(upb_descriptor_SRCS) $(upb_pb_SRCS) $(upb_json_SRCS)

amalgamate: upb.c upb.h

upb.c upb.h: $(AMALGAMATE_SRCS)
	$(E) AMALGAMATE $@
	$(Q) ./tools/amalgamate.py "" "" $^

amalgamated: upb.c upb.h
	$(E) CC upb.c
	$(Q) $(CC) -o upb.o -c upb.c $(WARNFLAGS)
