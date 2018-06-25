#!/bin/bash

set -ex

# change to repo root
cd $(dirname $0)/../../..

source kokoro/release/linux/prepare_build.sh

# ruby environment
source kokoro/release/linux/ruby/ruby_build_environment.sh

# build artifacts
bash kokoro/release/linux/ruby/ruby_build.sh
