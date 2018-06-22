#!/bin/bash

set -ex

# change to repo root
cd $(dirname $0)/../../..

# all artifacts come here
mkdir artifacts
export ARTIFACT_DIR=$(pwd)/artifacts

# ruby environment
bash kokoro/release/macos/ruby/ruby_build_environment.sh

gem install rubygems-update
update_rubygems

# build artifacts
bash kokoro/release/macos/ruby/ruby_build.sh
