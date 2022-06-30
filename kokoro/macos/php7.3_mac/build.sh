#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

# TODO(mkruskal) Re-enable this once we can get a working PHP 7.0 installed.
#./tests.sh php7.3_mac
