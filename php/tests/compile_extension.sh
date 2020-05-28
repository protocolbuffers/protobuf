#!/bin/bash

VERSION=$2

export PATH=/usr/local/php-$VERSION/bin:$PATH

pushd  ../ext/google/protobuf
make clean || true
set -e
# Add following in configure for debug: --enable-debug CFLAGS='-g -O0'
phpize && ./configure --with-php-config=`which php-config` CFLAGS='-g -O0 -Wall' && make
popd
