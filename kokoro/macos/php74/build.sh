#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

# Install Dependencies
# PHP 7.4 is already installed on the machine
brew cleanup
brew install coreutils

# Configure path
PHP_FOLDER=$(find $HOMEBREW_PREFIX -type d -regex ".*php.*/7.4.[0-9_.]*" | sort -n | tail -n 1)
test ! -z "$PHP_FOLDER"
export PATH="$PHP_FOLDER/bin:$PATH"

# Test
kokoro/macos/test_php.sh
