#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "pull request" project:
#
# This script selects a specific Dockerfile (for building a Docker image) and
# a script to run inside that image.  Then we delegate to the general
# build_and_run_docker.sh script.

# Change to repo root
cd $(dirname $0)/../../..

export OUTPUT_DIR=testoutput

./tests.sh python py27-python
./tests.sh python_cpp py27-cpp
./tests.sh golang

if [ ! -f gperftools/.libs/libtcmalloc.so ]; then
  git clone https://github.com/gperftools/gperftools.git
  cd gperftools
  ./autogen.sh
  ./configure
  make -j2
  cd ..
fi

oldpwd=`pwd`
cd benchmarks
if [[ $(type cmake 2>/dev/null) ]]; then
  make -j2 cpp-benchmark
fi
make java-benchmark
make python-pure-python-benchmark
make python-cpp-reflection-benchmark
make -j2 python-cpp-generated-code-benchmark
make go-benchmark

./download_data.sh
datasets=`find . -type f -name "dataset.*.pb"`
echo "benchmarking cpp..."
env LD_PRELOAD="$oldpwd/gperftools/.libs/libtcmalloc.so" ./cpp-benchmark --benchmark_min_time=5.0 $datasets > tmp/cpp_result.txt
echo "benchmarking java..."
./java-benchmark $datasets > tmp/java_result.txt
echo "benchmarking pure python..."
sudo ./python-pure-python-benchmark $datasets > tmp/python_result.txt
echo "benchmarking python cpp reflection..."
sudo env LD_PRELOAD="$oldpwd/gperftools/.libs/libtcmalloc.so" ./python-cpp-reflection-benchmark $datasets >> tmp/python_result.txt
echo "benchmarking python cpp generated code..."
sudo env LD_PRELOAD="$oldpwd/gperftools/.libs/libtcmalloc.so" ./python-cpp-generated-code-benchmark $datasets >> tmp/python_result.txt
echo "benchmarking go..."
./go-benchmark $datasets > tmp/go_result.txt

cd run_and_upload_result
python run_and_upload.py -cpp="../tmp/cpp_result.txt" -java="../tmp/java_result.txt" \
    -python="../tmp/python_result.txt" -go="../tmp/go_result.txt"

cd $oldpwd
