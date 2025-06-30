#!/bin/bash
set -eux

# Print the current directory for debugging
pwd
ls -la

# Find the setuptools directory and add it to PYTHONPATH
SETUPTOOLS_PATH=$(find $PWD -name "setuptools" -type d | grep site-packages | head -1)
if [ -z "$SETUPTOOLS_PATH" ]; then
  echo "Error: Could not find setuptools directory"
  find $PWD -type d | grep setuptools
  exit 1
fi

SITE_PACKAGES_DIR=$(dirname "$SETUPTOOLS_PATH")
echo "Using setuptools from: $SITE_PACKAGES_DIR"
export PYTHONPATH="$SITE_PACKAGES_DIR"

# Run setup.py with the arguments passed to this script
cd protobuf/
python3 setup.py "$@"
