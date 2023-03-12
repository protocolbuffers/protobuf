#!/bin/bash

set -ex

cd $(dirname $0)/..

# utf8_range has to live in the base third_party directory.
# We copy it into the ext/google/protobuf directory for the build
# (and for the release to PECL).
rm -rf ext/google/protobuf/third_party
mkdir -p ext/google/protobuf/third_party/utf8_range
cp -r ../third_party/utf8_range/* ext/google/protobuf/third_party/utf8_range

echo "Copied utf8_range from ../third_party -> ext/google/protobuf/third_party"

pushd  ext/google/protobuf > /dev/null

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

make -j8
TEST_PHP_ARGS="-q" make -j8 test
popd > /dev/null
