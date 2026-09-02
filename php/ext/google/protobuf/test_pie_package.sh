#!/bin/bash
# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

set -ex

# Ensure we are running from inside the php/ directory
if [ ! -f "ext/google/protobuf/package_mirror.py" ]; then
  echo "This script must be run from inside the php/ directory."
  exit 1
fi

PIE_PHAR_URL="https://github.com/php/pie/releases/latest/download/pie.phar"
TEST_DIR=$(mktemp -d -t pie_test_XXXXXX)
PIE_BIN="$TEST_DIR/pie.phar"

export XDG_CONFIG_HOME="$TEST_DIR/config"
export COMPOSER_HOME="$TEST_DIR/composer"
export PIE_HOME="$TEST_DIR/pie"
mkdir -p "$XDG_CONFIG_HOME" "$COMPOSER_HOME" "$PIE_HOME"

cleanup() {
  rm -rf "$TEST_DIR"
}
trap cleanup EXIT

echo "Downloading latest PIE installer..."
curl -sSL -o "$PIE_BIN" "$PIE_PHAR_URL"

echo "Syncing package mirror structure into temporary test directory..."
python3 ext/google/protobuf/package_mirror.py .. "$TEST_DIR/mirror"

echo "Adding local path repository to PIE..."
php "$PIE_BIN" repository:add path "$TEST_DIR/mirror"

echo "Testing PIE build for google/protobuf-ext..."
php "$PIE_BIN" build google/protobuf-ext:@dev

echo "PIE package build test completed successfully!"
