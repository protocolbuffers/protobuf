BUILD_PATH=./cpp_benchmark
SRC=proto_bench.cc $(BUILD_PATH)/google_speed.pb.cc $(BUILD_PATH)/google_size.pb.cc
OBJ=$(SRC:.cc=.o)
PROGRAM=$(BUILD_PATH)/cpp_benchmark_prog
CC=g++
INCLUDE = -I$(BUILD_PATH)
CXXFLAGS = -O3 -Wall -std=c++11 -Woverloaded-virtual
LINKPARAM = -lprotobuf

.PHONY : all clean test
all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) -o $(PROGRAM) $(LINKPARAM) $(OBJ)

$(BUILD_PATH):
	mkdir -p $@
	find ./ -name "*.proto" | xargs protoc --cpp_out=$(@)

$(SRC) : $(BUILD_PATH)

.SUFFIXES : .cc
%.o : %.cc
	$(CC) $(INCLUDE) $(CXXFLAGS) -c $< -o $@
 
clean:
	rm -rf $(BUILD_PATH)

test:$(PROGRAM)
	$(PROGRAM) google_message1.dat google_message2.dat
