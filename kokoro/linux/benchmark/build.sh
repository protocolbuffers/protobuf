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
datasets=$(for file in $(find . -type f -name "dataset.*.pb" -not -path "./tmp/*"); do echo "$(pwd)/$file"; done | xargs)
echo $datasets
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
rm -f "$GOPATH/src/github.com/protocolbuffers/protobuf"
ln -s "`pwd`" "$GOPATH/src/github.com/protocolbuffers/protobuf"
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

make js-benchmark
echo "benchmarking js..."
./js-benchmark $datasets  --json_output=$(pwd)/tmp/node_result.json

make -j8 generate_proto3_data
proto3_datasets=$(for file in $datasets; do echo $(pwd)/tmp/proto3_data/${file#$(pwd)}; done | xargs)
echo $proto3_datasets

# build php benchmark
make -j8 php-benchmark
echo "benchmarking php..."
./php-benchmark $proto3_datasets --json --behavior_prefix="php" > tmp/php_result.json
make -j8 php-c-benchmark
echo "benchmarking php_c..."
./php-c-benchmark $proto3_datasets --json --behavior_prefix="php_c" > tmp/php_c_result.json

# upload result to bq
make python_add_init
env LD_LIBRARY_PATH="$oldpwd/src/.libs" python -m util.result_uploader -php="../tmp/php_result.json" -php_c="../tmp/php_c_result.json"  \
	-cpp="../tmp/cpp_result.json" -java="../tmp/java_result.json" -go="../tmp/go_result.txt" -python="../tmp/python_result.json" -node="../tmp/node_result.json"
cd $oldpwd
