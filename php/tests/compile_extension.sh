#!/bin/bash

EXTENSION_PATH=$1

pushd $EXTENSION_PATH
make clean || true
set -e
# Add following in configure for debug: --enable-debug CFLAGS='-g -O0'
phpize && ./configure CFLAGS='-g -O0' && make
popd
