#!/bin/bash

set -ex

# Build protoc
use_bazel.sh 5.1.1
bazel build //:protoc
cp bazel-bin/protoc src/protoc
export PROTOC=$PWD/src/protoc

umask 0022
pushd ruby
gem install bundler -v 2.1.4
bundle update && bundle exec rake gem:native
ls pkg
mv pkg/* $ARTIFACT_DIR
popd
