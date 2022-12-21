#!/bin/bash

set -ex

use_bazel.sh 5.1.1
  
# change to repo root
cd $(dirname $0)/../../../..

# all artifacts come here
mkdir artifacts
export ARTIFACT_DIR=$(pwd)/artifacts

# ruby environment
bash kokoro/release/ruby/macos/ruby/ruby_build_environment.sh

# build artifacts
bash kokoro/release/ruby/macos/ruby/ruby_build.sh
