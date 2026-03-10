#!/bin/sh
#
# Protocol Buffers - Google's data interchange format
# Copyright 2009 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Author: kenton@google.com (Kenton Varda)
#
# Test protoc's zip output mode.

fail() {
  echo "$@" >&2
  exit 1
}

TEST_TMPDIR=.
PROTOC=./protoc
JAR=jar
UNZIP=unzip

echo '
  syntax = "proto2";
  option java_multiple_files = true;
  option java_package = "test.jar";
  option java_outer_classname = "Outer";
  message Foo {}
  message Bar {}
' > $TEST_TMPDIR/testzip.proto

$PROTOC \
    --cpp_out=$TEST_TMPDIR/testzip.zip --python_out=$TEST_TMPDIR/testzip.zip \
    --java_out=$TEST_TMPDIR/testzip.jar -I$TEST_TMPDIR testzip.proto \
    || fail 'protoc failed.'

echo "Testing output to zip..."
if $UNZIP -h > /dev/null; then
  $UNZIP -t $TEST_TMPDIR/testzip.zip > $TEST_TMPDIR/testzip.list \
    || fail 'unzip failed.'

  grep 'testing: testzip\.pb\.cc *OK$' $TEST_TMPDIR/testzip.list > /dev/null \
    || fail 'testzip.pb.cc not found in output zip.'
  grep 'testing: testzip\.pb\.h *OK$' $TEST_TMPDIR/testzip.list > /dev/null \
    || fail 'testzip.pb.h not found in output zip.'
  grep 'testing: testzip_pb2\.py *OK$' $TEST_TMPDIR/testzip.list > /dev/null \
    || fail 'testzip_pb2.py not found in output zip.'
  grep -i 'manifest' $TEST_TMPDIR/testzip.list > /dev/null \
    && fail 'Zip file contained manifest.'
else
  echo "Warning:  'unzip' command not available.  Skipping test."
fi

echo "Testing output to jar..."
if $JAR c $TEST_TMPDIR/testzip.proto > /dev/null; then
  $JAR tf $TEST_TMPDIR/testzip.jar > $TEST_TMPDIR/testzip.list \
    || fail 'jar failed.'

  # Check that -interface.jar timestamps are normalized:
  if [[ "$(TZ=UTC $JAR tvf $TEST_TMPDIR/testzip.jar)" != *'Tue Jan 01 00:00:00 UTC 1980'* ]]; then
    fail 'Zip did not contain normalized timestamps'
  fi

  grep '^test/jar/Foo\.java$' $TEST_TMPDIR/testzip.list > /dev/null \
    || fail 'Foo.java not found in output jar.'
  grep '^test/jar/Bar\.java$' $TEST_TMPDIR/testzip.list > /dev/null \
    || fail 'Bar.java not found in output jar.'
  grep '^test/jar/Outer\.java$' $TEST_TMPDIR/testzip.list > /dev/null \
    || fail 'Outer.java not found in output jar.'
  grep '^META-INF/MANIFEST\.MF$' $TEST_TMPDIR/testzip.list > /dev/null \
    || fail 'Manifest not found in output jar.'
else
  echo "Warning:  'jar' command not available.  Skipping test."
fi

echo PASS
