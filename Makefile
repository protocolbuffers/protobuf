
# Function to expand a wildcard pattern recursively.
rwildcard=$(strip $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d)))

.PHONY: all clean test benchmarks benchmark descriptorgen
CC=gcc
CXX=g++
CFLAGS=-std=c99
INCLUDE=-Idescriptor -Isrc -Itests -I.
CPPFLAGS=-Wall -Wextra -g $(INCLUDE) $(strip $(shell test -f perf-cppflags && cat perf-cppflags))

LIBUPB=src/libupb.a
ALL=deps $(OBJ) $(LIBUPB) tests/test_table tests/tests tools/upbc
all: $(ALL)
clean:
	rm -f $(call rwildcard,,*.o) $(ALL) benchmark/google_messages.proto.pb benchmark/google_messages.pb.* benchmarks/b.* benchmarks/*.pb*
	rm -f descriptor/descriptor.proto.pb

# The core library (src/libupb.a)
OBJ=src/upb_parse.o src/upb_table.o src/upb_msg.o src/upb_enum.o src/upb_context.o \
    src/upb_string.o src/upb_text.o src/upb_serialize.o descriptor/descriptor.o
SRC=$(call rwildcard,,*.c)
HEADERS=$(call rwildcard,,*.h)
$(LIBUPB): $(OBJ)
	ar rcs $(LIBUPB) $(OBJ)

# Regenerating the auto-generated files in descriptor/.
descriptor/descriptor.proto.pb: descriptor/descriptor.proto
	# TODO: replace with upbc
	protoc descriptor/descriptor.proto -odescriptor/descriptor.proto.pb

descriptorgen: descriptor/descriptor.proto.pb tools/upbc
	./tools/upbc -i upb_file_descriptor_set -o descriptor/descriptor descriptor/descriptor.proto.pb

# Tests
test: tests/tests
	./tests/tests
tests/test_table: src/libupb.a
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
deps: $(SRC) $(HEADERS) gen-deps.sh Makefile
	@./gen-deps.sh $(SRC)
