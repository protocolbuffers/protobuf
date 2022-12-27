#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Fix locale issues in Monterey.
export LC_ALL=en_US.UTF-8

bash -l kokoro/macos/test_ruby.sh ruby-3.1.0
