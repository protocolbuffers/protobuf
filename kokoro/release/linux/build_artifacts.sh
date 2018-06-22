#!/bin/bash

set -ex

# change to repo root
cd $(dirname $0)/../../..

source kokoro/release/linux/prepare_build.sh

# ruby environment
bash kokoro/release/linux/ruby/ruby_build_environment.sh

gem install rubygems-update
update_rubygems

# build artifacts
bash kokoro/release/macos/ruby/ruby_build.sh
