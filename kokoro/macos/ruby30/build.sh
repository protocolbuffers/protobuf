#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

bash -l kokoro/macos/test_ruby.sh ruby-3.0.2
