#!/bin/bash
#
# Build file to set up and run tests
set -eu

# Install Bazel 4.0.0.
use_bazel.sh 4.0.0
bazel version

# Change to repo root
cd $(dirname $0)/../../..

# Get kokoro scripts from repo root by default.
: ${SCRIPT_ROOT:=$(pwd)}
source ${SCRIPT_ROOT}/kokoro/common/pyenv.sh

#  Disabled for now, re-enable if appropriate.
#  //:build_files_updated_unittest \

bazel_args=(
  test
  --keep_going
  --copt=-Werror
  --host_copt=-Werror
  --test_output=errors
  --
  //...
  -//objectivec/...  # only works on macOS
  @com_google_protobuf_examples//...
)

${SCRIPT_ROOT}/kokoro/common/bazel_wrapper.sh "${bazel_args[@]}"

# Verify that we can build successfully from generated tar files.
(
  pyenv versions
  pyenv shell 2.7.9  # python2 required for old googletest autotools support
  git submodule update --init --recursive
  ./autogen.sh && ./configure && make -j$(nproc) dist
)
DIST=`ls *.tar.gz`
tar -xf $DIST
cd ${DIST//.tar.gz}
${SCRIPT_ROOT}/kokoro/common/bazel_wrapper.sh build //:protobuf //:protobuf_java
