#!/bin/bash

set -eux

# Change to repo root
cd $(dirname $0)/../../..

use_php() {
  VERSION=$1
  export PATH=/usr/local/php-${VERSION}/bin:$PATH
}

build_php() {
  use_php $1
  pushd php
  rm -rf vendor
  php -v
  php -m
  composer update
  composer test
  popd
}

test_php_c() {
  pushd php
  rm -rf vendor
  php -v
  php -m
  composer update
  composer test_c
  popd
}

build_php_c() {
  use_php $1
  test_php_c
}

mkdir -p build
pushd build
cmake ..
cmake --build . -- -j20
ctest --verbose --parallel 20
export PROTOC=$(pwd)/protoc
popd

build_php 7.0
build_php 7.1
build_php 7.4
build_php_c 7.0
build_php_c 7.1
build_php_c 7.4
build_php_c 7.1-zts
build_php_c 7.2-zts
build_php_c 7.5-zts

# Cleanup after CMake build
rm -rf build
