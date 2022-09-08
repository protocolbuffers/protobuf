#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

# Install Dependencies
brew cleanup
brew install coreutils php@8.0

# Configure path
PHP_FOLDER=$(find $HOMEBREW_PREFIX -type d -regex ".*php.*/8.0.[0-9_.]*" | sort -n | tail -n 1)
test ! -z "$PHP_FOLDER"
export PATH="$PHP_FOLDER/bin:$PATH"

# Test
kokoro/macos/test_php.sh
