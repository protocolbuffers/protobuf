#!/bin/bash

set -ex

# Build protoc
bazel build //:protoc
export PROTOC=$PWD/bazel-bin/protoc

umask 0022
pushd ruby
bundle update && bundle exec rake gem:native
ls pkg
mv pkg/* $ARTIFACT_DIR
popd
