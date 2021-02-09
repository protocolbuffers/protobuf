#!/bin/bash

set -ex

cd $(dirname $0)

../prepare_c_extension.sh
pushd  ../ext/google/protobuf
phpize --clean
rm -f configure.in configure.ac
phpize
if [ "$1" = "--release" ]; then
  ./configure --with-php-config=$(which php-config)
else
  # To get debugging symbols in PHP itself, build PHP with:
  #   $ ./configure --enable-debug CFLAGS='-g -O0'
  ./configure --with-php-config=$(which php-config) CFLAGS="-g -O0 -Wall"
fi
make
popd
