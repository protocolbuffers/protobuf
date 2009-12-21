#
# This Makefile builds the upb library as well as associated tests, tools, and
# language extensions.
#
# It does not use autoconf/automake/libtool because I can't stomach all the
# cruft.  If you're not compiling for gcc, you may have to change some of the
# options.
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

# Function to expand a wildcard pattern recursively.
rwildcard=$(strip $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d)))

.PHONY: all clean test benchmarks benchmark descriptorgen tests
CC=gcc
CXX=g++
CFLAGS=-std=c99
INCLUDE=-Idescriptor -Isrc -Itests -I.
CPPFLAGS=-Wall -Wextra -g $(INCLUDE) $(strip $(shell test -f perf-cppflags && cat perf-cppflags))
LDLIBS=-lpthread

LIBUPB=src/libupb.a
LIBUPB_PIC=src/libupb_pic.a
LIBUPB_SHARED=src/libupb.so
ALL=deps $(OBJ) $(LIBUPB) $(LIBUPB_PIC) tools/upbc
all: $(ALL)
clean:
	rm -rf $(LIBUPB) $(LIBUPB_PIC)
	rm -rf $(call rwildcard,,*.o) $(call rwildcard,,*.lo)
	rm -rf benchmark/google_messages.proto.pb benchmark/google_messages.pb.* benchmarks/b.* benchmarks/*.pb*
	rm -rf tests/tests tests/t.* tests/test_table
	rm -rf descriptor/descriptor.proto.pb
	rm -rf tools/upbc deps
	cd lang_ext/python && python setup.py clean --all

# The core library (src/libupb.a)
SRC=src/upb.c src/upb_parse.c src/upb_table.c src/upb_def.c src/upb_data.c \
    descriptor/descriptor.c
    #src/upb_serialize.c descriptor/descriptor.c
STATICOBJ=$(patsubst %.c,%.o,$(SRC))
SHAREDOBJ=$(patsubst %.c,%.lo,$(SRC))
# building shared objects is like building static ones, except -fPIC is added.
%.lo : %.c ; $(CC) -fPIC $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(LIBUPB): $(STATICOBJ)
	ar rcs $(LIBUPB) $(STATICOBJ)
$(LIBUPB_PIC): $(SHAREDOBJ)
	ar rcs $(LIBUPB_PIC) $(SHAREDOBJ)
$(LIBUPB_SHARED): $(SHAREDOBJ)
	$(CC) -shared -o $(LIBUPB_SHARED) $(SHAREDOBJ)

# Regenerating the auto-generated files in descriptor/.
descriptor/descriptor.proto.pb: descriptor/descriptor.proto
	# TODO: replace with upbc
	protoc descriptor/descriptor.proto -odescriptor/descriptor.proto.pb

descriptorgen: descriptor/descriptor.proto.pb tools/upbc
	./tools/upbc -i upb_file_descriptor_set -o descriptor/descriptor descriptor/descriptor.proto.pb

# Language extensions.
python: $(LIBUPB_PIC)
	cd lang_ext/python && python setup.py build

# Tests
tests/test.proto.pb: tests/test.proto
	# TODO: replace with upbc
	protoc tests/test.proto -otests/test.proto.pb

tests: tests/tests \
    tests/test_table \
    tests/t.test_vs_proto2.googlemessage1 \
    tests/t.test_vs_proto2.googlemessage2 \
    tests/test.proto.pb

#VALGRIND=valgrind --leak-check=full --error-exitcode=1 
VALGRIND=
test: tests
	@echo Running all tests under valgrind.
	$(VALGRIND) ./tests/tests
#	Needs to be rewritten to separate the benchmark.
#	valgrind --error-exitcode=1 ./tests/test_table
	@for test in tests/t.* ; do \
	  if [ -f ./$$test ] ; then \
	    echo $(VALGRIND) ./$$test: \\c; \
	    $(VALGRIND) ./$$test; \
	  fi \
	done;

tests/t.test_vs_proto2.googlemessage1 \
tests/t.test_vs_proto2.googlemessage2: \
    tests/test_vs_proto2.cc $(LIBUPB) benchmarks/google_messages.proto.pb \
    benchmarks/google_messages.pb.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o tests/t.test_vs_proto2.googlemessage1 $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"../benchmarks/google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"../benchmarks/google_message1.dat\" \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_HFILE=\"../benchmarks/google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread $(LIBUPB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o tests/t.test_vs_proto2.googlemessage2 $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"../benchmarks/google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"../benchmarks/google_message2.dat\" \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_HFILE=\"../benchmarks/google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread $(LIBUPB)
tests/test_table: tests/test_table.cc
	# Includes <hash_set> which is a deprecated header.
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -Wno-deprecated -o $@ $< $(LIBUPB)

tests/tests: src/libupb.a

# Tools
tools/upbc: src/libupb.a

# Benchmarks
UPB_BENCHMARKS=benchmarks/b.parsetostruct_googlemessage1.upb_table_byval \
               benchmarks/b.parsetostruct_googlemessage1.upb_table_byref \
               benchmarks/b.parsetostruct_googlemessage2.upb_table_byval \
               benchmarks/b.parsetostruct_googlemessage2.upb_table_byref

BENCHMARKS=$(UPB_BENCHMARKS) \
           benchmarks/b.parsetostruct_googlemessage1.proto2_table \
           benchmarks/b.parsetostruct_googlemessage2.proto2_table \
           benchmarks/b.parsetostruct_googlemessage1.proto2_compiled \
           benchmarks/b.parsetostruct_googlemessage2.proto2_compiled
upb_benchmarks: $(UPB_BENCHMARKS)
benchmarks: $(BENCHMARKS)
benchmark:
	@rm -f benchmarks/results
	@rm -rf benchmarks/*.dSYM
	@for test in benchmarks/b.* ; do ./$$test ; done

benchmarks/google_messages.proto.pb: benchmarks/google_messages.proto
	# TODO: replace with upbc.
	protoc benchmarks/google_messages.proto -obenchmarks/google_messages.proto.pb

benchmarks/google_messages.pb.cc: benchmarks/google_messages.proto
	protoc benchmarks/google_messages.proto --cpp_out=.

benchmarks/b.parsetostruct_googlemessage1.upb_table_byval \
benchmarks/b.parsetostruct_googlemessage1.upb_table_byref \
benchmarks/b.parsetostruct_googlemessage2.upb_table_byval \
benchmarks/b.parsetostruct_googlemessage2.upb_table_byref: \
    benchmarks/parsetostruct.upb_table.c $(LIBUPB) benchmarks/google_messages.proto.pb
	$(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.upb_table_byval $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DBYREF=false $(LIBUPB)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.upb_table_byref $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DBYREF=true $(LIBUPB)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.upb_table_byval $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DBYREF=false $(LIBUPB)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.upb_table_byref $< \
	  -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
	  -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DBYREF=true $(LIBUPB)

benchmarks/b.parsetostruct_googlemessage1.proto2_table \
benchmarks/b.parsetostruct_googlemessage2.proto2_table: \
    benchmarks/parsetostruct.proto2_table.cc benchmarks/google_messages.pb.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.proto2_table $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.proto2_table $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread

benchmarks/b.parsetostruct_googlemessage1.proto2_compiled \
benchmarks/b.parsetostruct_googlemessage2.proto2_compiled: \
    benchmarks/parsetostruct.proto2_compiled.cc \
    benchmarks/parsetostruct.proto2_table.cc benchmarks/google_messages.pb.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage1.proto2_compiled $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
	  -DMESSAGE_FILE=\"google_message1.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o benchmarks/b.parsetostruct_googlemessage2.proto2_compiled $< \
	  -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
	  -DMESSAGE_FILE=\"google_message2.dat\" \
	  -DMESSAGE_HFILE=\"google_messages.pb.h\" \
	  benchmarks/google_messages.pb.cc -lprotobuf -lpthread

-include deps
deps: gen-deps.sh Makefile $(call rwildcard,,*.c) $(call rwildcard,,*.h)
	@./gen-deps.sh $(SRC)
