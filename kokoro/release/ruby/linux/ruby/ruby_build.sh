#!/bin/bash

set -ex

# Build protoc
use_bazel.sh 4.0.0
bazel build //:protoc
ln -s $PWD/bazel-bin/protoc src/protoc

umask 0022
pushd ruby
gem install bundler -v 2.1.4
bundle update && bundle exec rake gem:native
ls pkg
mv pkg/* $ARTIFACT_DIR
popd
