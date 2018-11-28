#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc
g++ --version

export MACOSX_DEPLOYMENT_TARGET=10.9
./tests.sh python_cpp
