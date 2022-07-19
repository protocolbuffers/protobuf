#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

# Install Dependencies
brew install coreutils php@7.4

# Configure path
HOMEBREW_PREFIX=$(brew --prefix)
PHP_FOLDER=$(find $HOMEBREW_PREFIX -type d -regex ".*php.*/7.4.[0-9]*")
test ! -z "$PHP_FOLDER"
export PATH="$PHP_FOLDER/bin:$PATH"

# Fix missing pcre2.h in homebrew's PHP
#ln -s $(find $HOMEBREW_PREFIX/Cellar/pcre2 -name pcre2.h) $PHP_FOLDER/include/php/ext/pcre/pcre2.h

# Test
./tests.sh php_mac
