#!/bin/bash
#
# Install Bazel 4.0.0.
use_bazel.sh 4.0.0

# Change to repo root
cd $(dirname $0)/../../..
SCRIPT_ROOT=$(pwd)

set -ex

export OUTPUT_DIR=testoutput
repo_root="$(pwd)"

# Setup python environment.
pyenv install -v 3.9.5 -s
pyenv global 3.9.5
pyenv versions
python --version
python -m venv "venv"
source "venv/bin/activate"

# TODO(jtattermusch): Add back support for benchmarking with tcmalloc for C++ and python.
# This feature was removed since it used to use tcmalloc from https://github.com/gperftools/gperftools.git
# which is very outdated. See https://github.com/protocolbuffers/protobuf/issues/8725.

# download datasets for benchmark
pushd benchmarks
datasets=$(for file in $(find . -type f -name "dataset.*.pb" -not -path "./tmp/*"); do echo "$(pwd)/$file"; done | xargs)
echo $datasets
popd

# build and run Python benchmark
echo "benchmarking pure python..."
${SCRIPT_ROOT}/kokoro/common/bazel_wrapper.sh run //benchmarks/python:python_benchmark -- \
	--json --behavior_prefix="pure-python-benchmark" $datasets > /tmp/python1.json
echo "benchmarking python cpp reflection..."
${SCRIPT_ROOT}/kokoro/common/bazel_wrapper.sh run //benchmarks/python:python_benchmark --define=use_fast_cpp_protos=true -- \
	--json --behavior_prefix="cpp-reflection-benchmark" $datasets > /tmp/python2.json
echo "benchmarking python cpp generated code..."
${SCRIPT_ROOT}/kokoro/common/bazel_wrapper.sh run //benchmarks/python:python_benchmark --define=use_fast_cpp_protos=true -- \
	--json --cpp_generated --behavior_prefix="cpp-generated-code-benchmark" $datasets >> /tmp/python3.json

jq -s . /tmp/python1.json /tmp/python2.json /tmp/python3.json > python_result.json

# build and run C++ benchmark
echo "benchmarking cpp..."
${SCRIPT_ROOT}/kokoro/common/bazel_wrapper.sh run //benchmarks/cpp:cpp_benchmark -- \
	--benchmark_min_time=5.0 --benchmark_out_format=json --benchmark_out="${repo_root}/cpp_result.json" $datasets

# build and run java benchmark (java 11 is required)
echo "benchmarking java..."
${SCRIPT_ROOT}/kokoro/common/bazel_wrapper.sh run //benchmarks/java:java_benchmark -- \
	-Cresults.file.options.file="${repo_root}/java_result.json" $datasets

# persist raw the results in the build job log (for better debuggability)
cat cpp_result.json
cat java_result.json
cat python_result.json

# print the postprocessed results to the build job log
# TODO(jtattermusch): re-enable uploading results to bigquery (it is currently broken)
bazel run //benchmarks/util:result_parser -- \
	-cpp="${repo_root}/cpp_result.json" \
	-java="${repo_root}/java_result.json" \
	-python="${repo_root}/python_result.json"
