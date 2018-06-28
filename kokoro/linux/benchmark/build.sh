#!/bin/bash
#
# Change to repo root
cd $(dirname $0)/../../..

export OUTPUT_DIR=testoutput
oldpwd=`pwd`

# tcmalloc
if [ ! -f gperftools/.libs/libtcmalloc.so ]; then
  git clone https://github.com/gperftools/gperftools.git
  cd gperftools
  ./autogen.sh
  ./configure
  make -j8
  cd ..
fi

# download datasets for benchmark
cd benchmarks
./download_data.sh
datasets=`find . -type f -name "dataset.*.pb"`
cd $oldpwd

# build Python protobuf
./autogen.sh
./configure CXXFLAGS="-fPIC -O2"
make -j8
cd python
python setup.py build --cpp_implementation
pip install . --user


# build and run Python benchmark
cd ../benchmarks
make python-pure-python-benchmark
make python-cpp-reflection-benchmark
make -j8 python-cpp-generated-code-benchmark
echo "[" > tmp/python_result.json
echo "benchmarking pure python..."
./python-pure-python-benchmark --json --behavior_prefix="pure-python-benchmark" $datasets  >> tmp/python_result.json
echo "," >> "tmp/python_result.json"
echo "benchmarking python cpp reflection..."
env LD_PRELOAD="$oldpwd/gperftools/.libs/libtcmalloc.so" LD_LIBRARY_PATH="$oldpwd/src/.libs" ./python-cpp-reflection-benchmark --json --behavior_prefix="cpp-reflection-benchmark" $datasets  >> tmp/python_result.json
echo "," >> "tmp/python_result.json"
echo "benchmarking python cpp generated code..."
env LD_PRELOAD="$oldpwd/gperftools/.libs/libtcmalloc.so" LD_LIBRARY_PATH="$oldpwd/src/.libs" ./python-cpp-generated-code-benchmark --json --behavior_prefix="cpp-generated-code-benchmark" $datasets >> tmp/python_result.json
echo "]" >> "tmp/python_result.json"
cd $oldpwd

# build CPP protobuf
./configure
make clean && make -j8

# build Java protobuf
cd java
mvn package
cd ..

# build CPP benchmark
cd benchmarks
mv tmp/python_result.json . && make clean && make -j8 cpp-benchmark && mv python_result.json tmp
echo "benchmarking cpp..."
env LD_PRELOAD="$oldpwd/gperftools/.libs/libtcmalloc.so" ./cpp-benchmark --benchmark_min_time=5.0 --benchmark_out_format=json --benchmark_out="tmp/cpp_result.json" $datasets
cd $oldpwd

# build go protobuf 
export PATH="`pwd`/src:$PATH"
export GOPATH="$HOME/gocode"
mkdir -p "$GOPATH/src/github.com/google"
rm -f "$GOPATH/src/github.com/google/protobuf"
ln -s "`pwd`" "$GOPATH/src/github.com/google/protobuf"
export PATH="$GOPATH/bin:$PATH"
go get github.com/golang/protobuf/protoc-gen-go

# build go benchmark
cd benchmarks
make go-benchmark
echo "benchmarking go..."
./go-benchmark $datasets > tmp/go_result.txt

# build java benchmark
make java-benchmark
echo "benchmarking java..."
./java-benchmark -Cresults.file.options.file="tmp/java_result.json" $datasets

# upload result to bq
make python_add_init
env LD_LIBRARY_PATH="$oldpwd/src/.libs" python util/run_and_upload.py -cpp="../tmp/cpp_result.json" -java="../tmp/java_result.json" \
    -python="../tmp/python_result.json" -go="../tmp/go_result.txt"

cd $oldpwd
