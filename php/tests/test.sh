#!/bin/bash

set -ex

cd $(dirname $0)

./generate_protos.sh
./compile_extension.sh

PHP_VERSION=$(php -r "echo PHP_VERSION;")

# Each version of PHPUnit supports a fairly narrow range of PHP versions.
case "$PHP_VERSION" in
  7.0.*|7.1.*|7.2.*)
    # Oddly older than for 5.6. Not sure the reason.
    PHPUNIT=phpunit-5.6.0.phar
    ;;
  7.3.*|7.4.*)
    PHPUNIT=phpunit-8.phar
    ;;
  8.0.*)
    PHPUNIT=phpunit-9.phar
    ;;
  *)
    echo "ERROR: Unsupported PHP version $PHP_VERSION"
    exit 1
    ;;
esac

[ -f $PHPUNIT ] || wget https://phar.phpunit.de/$PHPUNIT

tests=( ArrayTest.php EncodeDecodeTest.php GeneratedClassTest.php MapFieldTest.php WellKnownTest.php DescriptorsTest.php WrapperTypeSettersTest.php)

for t in "${tests[@]}"
do
  echo "****************************"
  echo "* $t"
  echo "****************************"
  php -dextension=../ext/google/protobuf/modules/protobuf.so $PHPUNIT --bootstrap autoload.php $t
  echo ""
done

for t in "${tests[@]}"
do
  echo "****************************"
  echo "* $t persistent"
  echo "****************************"
  php -d protobuf.keep_descriptor_pool_after_request=1 -dextension=../ext/google/protobuf/modules/protobuf.so $PHPUNIT --bootstrap autoload.php $t
  echo ""
done

# # Make sure to run the memory test in debug mode.
# php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php

export ZEND_DONT_UNLOAD_MODULES=1
export USE_ZEND_ALLOC=0
valgrind --suppressions=valgrind.supp --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
valgrind --suppressions=valgrind.supp --leak-check=yes php -d protobuf.keep_descriptor_pool_after_request=1 -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php

# TODO(teboring): Only for debug (phpunit has memory leak which blocks this beging used by
# regression test.)

# for t in "${tests[@]}"
# do
#   echo "****************************"
#   echo "* $t (memory leak)"
#   echo "****************************"
#   valgrind --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so $PHPUNIT --bootstrap autoload.php $t
#   echo ""
# done
