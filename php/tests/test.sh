#!/bin/bash

OLD_PATH=$PATH
OLD_CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH
OLD_C_INCLUDE_PATH=$C_INCLUDE_PATH

export PATH=/usr/local/php-7.1/bin:$OLD_PATH
export CPLUS_INCLUDE_PATH=/usr/local/php-7.1/include/php/main:/usr/local/php-7.1/include/php/:$OLD_CPLUS_INCLUDE_PATH
export C_INCLUDE_PATH=/usr/local/php-7.1/include/php/main:/usr/local/php-7.1/include/php/:$OLD_C_INCLUDE_PATH

# Compile c extension
pushd ../ext/google/protobuf/
make clean || true
set -e
# Add following in configure for debug: --enable-debug CFLAGS='-g -O0'
phpize && ./configure CFLAGS='-g -O0' && make
popd

# tests=( array_test.php encode_decode_test.php generated_class_test.php generated_phpdoc_test.php map_field_test.php well_known_test.php generated_service_test.php descriptors_test.php )
tests=( encode_decode_test2.php )

for t in "${tests[@]}"
do
  echo "****************************"
  echo "* $t"
  echo "****************************"
  php -dextension=../ext/google/protobuf/modules/protobuf.so `which phpunit` --bootstrap autoload.php $t
  echo ""
done

# # Make sure to run the memory test in debug mode.
# php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php

# export ZEND_DONT_UNLOAD_MODULES=1
# export USE_ZEND_ALLOC=0
# valgrind --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
