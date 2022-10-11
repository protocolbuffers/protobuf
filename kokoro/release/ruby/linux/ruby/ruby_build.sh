#!/bin/bash

set -ex

# Build protoc
use_bazel.sh 5.1.1
bazel build //:protoc

# The java build setup expects protoc in the root directory.
cp bazel-bin/protoc .
export PROTOC=$PWD/protoc

# Pull in dependencies.
git submodule update --init --recursive

umask 0022
pushd ruby
gem install bundler -v 2.1.4
bundle update && bundle exec rake gem:native
ls pkg
mv pkg/* $ARTIFACT_DIR
popd
