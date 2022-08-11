#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

objectivec/DevTools/full_mac_build.sh \
  --core-only --skip-xcode-osx --skip-xcode-tvos --skip-objc-conformance --skip-xcode-release
