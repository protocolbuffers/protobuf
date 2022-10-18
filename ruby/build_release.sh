#!/bin/bash

set -ex

WORKING_DIR=$(dirname $(readlink $BASH_SOURCE))

# Move all files to the correct directory
cp src/google/protobuf/*.rb $WORKING_DIR/lib/google/protobuf
cp ruby/protobuf_java_release-project.jar $WORKING_DIR/lib/google/protobuf_java.jar
UTF8_DIR=$WORKING_DIR/ext/google/protobuf_c/third_party/utf8_range
mkdir -p $UTF8_DIR
cp external/utf8_range/LICENSE $UTF8_DIR/LICENSE
cp external/utf8_range/naive.c $UTF8_DIR
cp external/utf8_range/range2-neon.c $UTF8_DIR
cp external/utf8_range/range2-sse.c $UTF8_DIR
cp external/utf8_range/utf8_range.h $UTF8_DIR

# Make all files global readable/writable/executable
cd $WORKING_DIR
chmod -R 777 ./

# Build gem
bash --login -c "rvm use system && gem build google-protobuf.gemspec"
# Build jruby gem
bash --login -c "rvm use jruby && gem build google-protobuf.gemspec"
