#!/bin/bash

set -ex
  
# change to repo root
cd $(dirname $0)/../../../..

use_bazel.sh 5.1.1

# all artifacts come here
mkdir artifacts
export ARTIFACT_DIR=$(pwd)/artifacts

# ruby environment
bash kokoro/release/ruby/macos/ruby/ruby_build_environment.sh

# build artifacts
bash kokoro/release/ruby/macos/ruby/ruby_build.sh
