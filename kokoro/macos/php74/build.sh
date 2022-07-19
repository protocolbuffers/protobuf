#!/bin/bash
#
# Build file to set up and run tests

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

# Install PHP + Composer
brew install php@7.4 composer pcre2

# Configure path
PHP_FOLDER=$(find /opt/homebrew -type d -regex ".*php.*/7.4.[0-9]*")
test ! -z "$PHP_FOLDER"
export PATH="$PHP_FOLDER/bin:$PATH"

# Fix missing pcre2.h in homebrew's PHP
ln -s $(find /opt/homebrew/Cellar/pcre2 -name pcre2.h) $PHP_FOLDER/include/php/ext/pcre/pcre2.h

# Test
./tests.sh php_mac
