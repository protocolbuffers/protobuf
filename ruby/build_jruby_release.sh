#!/bin/bash
# This file should be executed with jruby

set -ex

WORKING_DIR=$(dirname $(readlink $BASH_SOURCE))

# Move all files to the correct directory
cp src/google/protobuf/*.rb $WORKING_DIR/lib/google/protobuf
cp ruby/protobuf_java_release-project.jar $WORKING_DIR/lib/google/protobuf_java.jar

# Make all files global readable/writable/executable
cd $WORKING_DIR
chmod -R 777 ./

# Build gem
gem build google-protobuf.gemspec
