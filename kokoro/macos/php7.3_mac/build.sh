#!/bin/bash
#
# Build file to set up and run tests

set -eux

# Change to repo root
cd $(dirname $0)/../../..

# Prepare worker environment to run tests
source kokoro/macos/prepare_build_macos_rc

brew tap shivammathur/php

# Install dependencies.
brew install \
    shivammathur/php/php@7.3 \
    composer

# "Install" valgrind
echo "#! /bin/bash" > valgrind
chmod ug+x valgrind
sudo mv valgrind /usr/local/bin/valgrind

./tests.sh php7.3_mac
