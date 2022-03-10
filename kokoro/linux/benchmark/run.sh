#!/bin/bash
#
# Change to repo root
cd $(dirname $0)/../../..

set -ex

export OUTPUT_DIR=testoutput
repo_root="$(pwd)"

# TODO(jtattermusch): Add back support for benchmarking with tcmalloc for C++ and python.
# This feature was removed since it used to use tcmalloc from https://github.com/gperftools/gperftools.git
# which is very outdated. See https://github.com/protocolbuffers/protobuf/issues/8725.

# download datasets for benchmark
pushd benchmarks
datasets=$(for file in $(find . -type f -name "dataset.*.pb" -not -path "./tmp/*"); do echo "$(pwd)/$file"; done | xargs)
echo $datasets
popd

# build Python protobuf
./autogen.sh
./configure CXXFLAGS="-fPIC -O2"
make -j8
pushd python
python3 -m venv env
source env/bin/activate
python3 setup.py build --cpp_implementation
pip3 install --install-option="--cpp_implementation" .
popd

# build and run Python benchmark
# We do this before building protobuf C++ since C++ build
# will rewrite some libraries used by protobuf python.
pushd benchmarks
make python-pure-python-benchmark
make python-cpp-reflection-benchmark
make -j8 python-cpp-generated-code-benchmark
echo "[" > tmp/python_result.json
echo "benchmarking pure python..."
./python-pure-python-benchmark --json --behavior_prefix="pure-python-benchmark" $datasets  >> tmp/python_result.json
echo "," >> "tmp/python_result.json"
echo "benchmarking python cpp reflection..."
env LD_LIBRARY_PATH="${repo_root}/src/.libs" ./python-cpp-reflection-benchmark --json --behavior_prefix="cpp-reflection-benchmark" $datasets  >> tmp/python_result.json
echo "," >> "tmp/python_result.json"
echo "benchmarking python cpp generated code..."
env LD_LIBRARY_PATH="${repo_root}/src/.libs" ./python-cpp-generated-code-benchmark --json --behavior_prefix="cpp-generated-code-benchmark" $datasets >> tmp/python_result.json
echo "]" >> "tmp/python_result.json"
popd

# build CPP protobuf
./configure
make clean && make -j8

pushd java
mvn package -B -Dmaven.test.skip=true
popd

pushd benchmarks

# build and run C++ benchmark
# "make clean" deletes the contents of the tmp/ directory, so we move it elsewhere and then restore it once build is done.
# TODO(jtattermusch): find a less clumsy way of protecting python_result.json contents
mv tmp/python_result.json . && make clean && make -j8 cpp-benchmark && mv python_result.json tmp
echo "benchmarking cpp..."
env ./cpp-benchmark --benchmark_min_time=5.0 --benchmark_out_format=json --benchmark_out="tmp/cpp_result.json" $datasets

# TODO(jtattermusch): add benchmarks for https://github.com/protocolbuffers/protobuf-go.
# The original benchmarks for https://github.com/golang/protobuf were removed
# because:
# * they were broken and haven't been producing results for a long time
# * the https://github.com/golang/protobuf implementation has been superseded by
#   https://github.com/protocolbuffers/protobuf-go

# build and run java benchmark (java 11 is required)
make java-benchmark
echo "benchmarking java..."
./java-benchmark -Cresults.file.options.file="tmp/java_result.json" $datasets

# TODO(jtattermusch): re-enable JS benchmarks once https://github.com/protocolbuffers/protobuf/issues/8747 is fixed.
# build and run js benchmark
# make js-benchmark
# echo "benchmarking js..."
# ./js-benchmark $datasets  --json_output=$(pwd)/tmp/node_result.json

# TODO(jtattermusch): add php-c-benchmark. Currently its build is broken.

# persist raw the results in the build job log (for better debuggability)
cat tmp/cpp_result.json
cat tmp/java_result.json
cat tmp/python_result.json

# print the postprocessed results to the build job log
# TODO(jtattermusch): re-enable uploading results to bigquery (it is currently broken)
make python_add_init
env LD_LIBRARY_PATH="${repo_root}/src/.libs" python3 -m util.result_parser \
	-cpp="../tmp/cpp_result.json" -java="../tmp/java_result.json" -python="../tmp/python_result.json"
popd

