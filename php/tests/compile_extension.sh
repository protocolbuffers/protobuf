#!/bin/bash

set -e

cd $(dirname $0)

pushd  ../ext/google/protobuf > /dev/null

CONFIGURE_OPTIONS=("./configure" "--with-php-config=$(which php-config)")

if [ "$1" != "--release" ]; then
  CONFIGURE_OPTIONS+=("CFLAGS=-g -O0 -Wall -DPBPHP_ENABLE_ASSERTS")
fi

FINGERPRINT="$(sha256sum $(which php)) ${CONFIGURE_OPTIONS[@]}"

# If the PHP interpreter we are building against or the arguments
# have changed, we must regenerated the Makefile.
if [[ ! -f BUILD_STAMP ]] || [[ "$(cat BUILD_STAMP)" != "$FINGERPRINT" ]]; then
  phpize --clean
  rm -f configure.in configure.ac
  phpize
  "${CONFIGURE_OPTIONS[@]}"
  echo "$FINGERPRINT" > BUILD_STAMP
fi

make
popd > /dev/null
