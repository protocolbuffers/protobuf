#!/bin/bash

set -e

run_tests()
{
  tests=( generated_class_test.php )
  # tests=( array_test.php map_field_test.php generated_class_test.php encode_decode_test.php )
  # tests=( encode_decode_test.php )
  # tests=( array_test.php generated_class_test.php map_field_test.php )
  
  for t in "${tests[@]}"
  do
    echo "****************************"
    echo "* $t"
    echo "****************************"
    if [ $1 == "php" ]; then
      php -dextension=./modules/protobuf.so `which phpunit` --bootstrap autoload.php $t
    else
      hhvm -d extension_dir=. -d hhvm.extensions[]=protobuf.so `which phpunit` --bootstrap autoload.php $t
    fi
  done

  if [ $1 == "php" ]; then
    export ZEND_DONT_UNLOAD_MODULES=1
    export USE_ZEND_ALLOC=0
    valgrind --leak-check=yes php -dextension=./modules/protobuf.so memory_leak_test.php
  else
    echo "no memory leak test for hhvm"
    # valgrind --leak-check=yes hhvm -d extension_dir=. -d hhvm.extensions[]=protobuf.so memory_leak_test.php
    # valgrind --leak-check=yes hhvm memory_leak_test.php
  fi
}

PROTOC=../../../../../src/protoc

# # Test HHVM
# echo "****************************"
# echo "* Test HHVM"
# echo "****************************"
# rm -rf generated
# mkdir generated
# 
# $PROTOC --php_out=hhvm:generated             \
#   proto/test.proto                           \
#   proto/test_include.proto                   \
#   proto/test_no_namespace.proto              \
#   proto/test_prefix.proto                    \
#   proto/test_php_namespace.proto             \
#   proto/test_empty_php_namespace.proto       \
#   proto/test_reserved_enum_lower.proto       \
#   proto/test_reserved_enum_upper.proto       \
#   proto/test_reserved_enum_value_lower.proto \
#   proto/test_reserved_enum_value_upper.proto \
#   proto/test_reserved_message_lower.proto    \
#   proto/test_reserved_message_upper.proto    \
#   proto/test_service.proto                   \
#   proto/test_service_namespace.proto         \
#   proto/test_descriptors.proto
# 
# # make clean; hphpize; cmake . && make && hhvm -d extension_dir=. -d hhvm.extensions[]=protobuf.so test_hhvm.php
# phpunit --version
# make clean; hphpize; cmake . && TMPDIR=~/tmp make
# run_tests hhvm

# Test PHP
rm -rf generated
mkdir generated

$PROTOC --php_out=generated                  \
  proto/test.proto                           \
  proto/test_include.proto                   \
  proto/test_no_namespace.proto              \
  proto/test_prefix.proto                    \
  proto/test_php_namespace.proto             \
  proto/test_empty_php_namespace.proto       \
  proto/test_reserved_enum_lower.proto       \
  proto/test_reserved_enum_upper.proto       \
  proto/test_reserved_enum_value_lower.proto \
  proto/test_reserved_enum_value_upper.proto \
  proto/test_reserved_message_lower.proto    \
  proto/test_reserved_message_upper.proto    \
  proto/test_service.proto                   \
  proto/test_service_namespace.proto         \
  proto/test_descriptors.proto

OLD_PATH=$PATH
OLD_CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH
OLD_C_INCLUDE_PATH=$C_INCLUDE_PATH

# php-5.5
echo "****************************"
echo "* Test PHP5.5"
echo "****************************"
export PATH=/usr/local/php-5.5/bin:$OLD_PATH
export CPLUS_INCLUDE_PATH=/usr/local/php-5.5/include/php/main:/usr/local/php-5.5/include/php/:$OLD_CPLUS_INCLUDE_PATH
export C_INCLUDE_PATH=/usr/local/php-5.5/include/php/main:/usr/local/php-5.5/include/php/:$OLD_C_INCLUDE_PATH

make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && run_tests php

# # php-5.5-zts
# echo "****************************"
# echo "* Test PHP5.5ZTS"
# echo "****************************"
# export PATH=/usr/local/php-5.5-zts/bin:$OLD_PATH
# export CPLUS_INCLUDE_PATH=/usr/local/php-5.5-zts/include/php/main:/usr/local/php-5.5-zts/include/php/:$OLD_CPLUS_INCLUDE_PATH
# export C_INCLUDE_PATH=/usr/local/php-5.5-zts/include/php/main:/usr/local/php-5.5-zts/include/php/:$OLD_C_INCLUDE_PATH
# 
# make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && run_tests php

# # php-7.1
# echo "****************************"
# echo "* Test PHP7.1"
# echo "****************************"
# export PATH=/usr/local/php-7.1/bin:$OLD_PATH
# export CPLUS_INCLUDE_PATH=/usr/local/php-7.1/include/php/main:/usr/local/php-7.1/include/php/:$OLD_CPLUS_INCLUDE_PATH
# export C_INCLUDE_PATH=/usr/local/php-7.1/include/php/main:/usr/local/php-7.1/include/php/:$OLD_C_INCLUDE_PATH
# 
# make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && run_tests php

# # php-7.1-zts
# echo "****************************"
# echo "* Test PHP7.1ZTS"
# echo "****************************"
# export PATH=/usr/local/php-7.1-zts/bin:$OLD_PATH
# export CPLUS_INCLUDE_PATH=/usr/local/php-7.1-zts/include/php/main:/usr/local/php-7.1-zts/include/php/:$OLD_CPLUS_INCLUDE_PATH
# export C_INCLUDE_PATH=/usr/local/php-7.1-zts/include/php/main:/usr/local/php-7.1-zts/include/php/:$OLD_C_INCLUDE_PATH
# 
# make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && run_tests php
# 
# echo "All Tests Passed"

