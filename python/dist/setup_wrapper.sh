#!/bin/bash
# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

set -eux

# Find the setuptools directory and add it to PYTHONPATH
SETUPTOOLS_PATH=$(find $PWD -name "setuptools" -type d | grep site-packages | head -1)
if [ -z "$SETUPTOOLS_PATH" ]; then
  echo "Warning: Could not find setuptools directory"
else
  SITE_PACKAGES_DIR=$(dirname "$SETUPTOOLS_PATH")
  echo "Using setuptools from: $SITE_PACKAGES_DIR"
  export PYTHONPATH="$SITE_PACKAGES_DIR"
fi

# Run setup.py with the arguments passed to this script
cd protobuf/
python3 setup.py "$@"
