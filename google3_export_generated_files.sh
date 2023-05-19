#!/bin/bash

# This script updates all our generated files and exports them to a given
# directory.  This is intended to simplify the process of updating these from
# google3.  After copybara generates a branch for testings, run the following
# from a git clone:
#   git fetch --all
#   git checkout upstream/<copybara branch>
#   ./google3_export_generated_files <ldap>/<citc_workspace>
#
# Note: this is a temporary script and won't be needed once we automatically
# update these.

set -ex

OUTPUT_PATH=/google/src/cloud/$1/google3/third_party/protobuf/github

update_staleness() {
  TARGET_DIR=$1
  TARGET=$2
  GEN_PATH=$3
  GEN_FILES=${@:4}
  bazel build //$TARGET_DIR:$TARGET
  bazel-bin/$TARGET_DIR/$TARGET --fix
  for file in $GEN_FILES; do
    cp $GEN_PATH/$file $OUTPUT_PATH/$GEN_PATH/
  done
}

update_staleness ruby/ext/google/protobuf_c test_amalgamation_staleness ruby/ext/google/protobuf_c ruby-upb.*
update_staleness php test_amalgamation_staleness php/ext/google/protobuf php-upb.*
