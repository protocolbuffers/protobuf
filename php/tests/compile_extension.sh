#!/bin/bash

cd $(dirname $0)

if [ "$1" = "--release"]; then
  CFLAGS="-Wall"
else
  # To get debugging symbols in PHP itself, build PHP with:
  #   $ ./configure --enable-debug CFLAGS='-g -O0'
  CFLAGS="-g -O0 -Wall"
fi

pushd  ../ext/google/protobuf
make clean || true
set -e
phpize && ./configure --with-php-config=$(which php-config) CFLAGS="$CFLAGS" && make
popd
