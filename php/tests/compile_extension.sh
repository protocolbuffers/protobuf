#!/bin/bash

set -e

cd $(dirname $0)

../prepare_c_extension.sh
pushd  ../ext/google/protobuf > /dev/null

CONFIGURE_OPTIONS=("./configure" "--with-php-config=$(which php-config)")

if [ "$1" != "--release" ]; then
  CONFIGURE_OPTIONS+=("CFLAGS=-g -O0 -Wall")
fi

# If the PHP interpreter we are building against or the arguments
# have changed, we must regenerated the Makefile.
if [[ ! -f Makefile ]] || [[ "$(grep '  \$ ./configure' config.log)" != "  $ ${CONFIGURE_OPTIONS[@]}" ]]; then
  phpize --clean
  rm -f configure.in configure.ac
  phpize
  "${CONFIGURE_OPTIONS[@]}"
fi

make
popd > /dev/null
