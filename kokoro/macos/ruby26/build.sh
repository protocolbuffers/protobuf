#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
KOKORO_INSTALL_RVM=yes
source kokoro/macos/prepare_build_macos_rc

./tests.sh ruby26
