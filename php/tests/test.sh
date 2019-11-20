#!/bin/bash

VERSION=$1

export PATH=/usr/local/php-$VERSION/bin:$PATH
export C_INCLUDE_PATH=/usr/local/php-$VERSION/include/php/main:/usr/local/php-$VERSION/include/php:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/usr/local/php-$VERSION/include/php/main:/usr/local/php-$VERSION/include/php:$CPLUS_INCLUDE_PATH

# Compile c extension
/bin/bash ./compile_extension.sh ../ext/google/protobuf

tests=( array_test.php encode_decode_test.php generated_class_test.php map_field_test.php well_known_test.php descriptors_test.php wrapper_type_setters_test.php)

for t in "${tests[@]}"
do
  echo "****************************"
  echo "* $t"
  echo "****************************"
  php -dextension=../ext/google/protobuf/modules/protobuf.so `which phpunit` --bootstrap autoload.php $t
  echo ""
done

for t in "${tests[@]}"
do
  echo "****************************"
  echo "* $t persistent"
  echo "****************************"
  php -d protobuf.keep_descriptor_pool_after_request=1 -dextension=../ext/google/protobuf/modules/protobuf.so `which phpunit` --bootstrap autoload.php $t
  echo ""
done

# # Make sure to run the memory test in debug mode.
# php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php

export ZEND_DONT_UNLOAD_MODULES=1
export USE_ZEND_ALLOC=0
valgrind --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
valgrind --leak-check=yes php -d protobuf.keep_descriptor_pool_after_request=1 -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php

# TODO(teboring): Only for debug (phpunit has memory leak which blocks this beging used by
# regresssion test.)

# for t in "${tests[@]}"
# do
#   echo "****************************"
#   echo "* $t (memory leak)"
#   echo "****************************"
#   valgrind --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so `which phpunit` --bootstrap autoload.php $t
#   echo ""
# done
