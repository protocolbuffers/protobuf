#!/bin/bash

set -e

run_tests()
{
  tests=( encode_decode_test.php )
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

  # if [ $1 == "php" ]; then
  #   export ZEND_DONT_UNLOAD_MODULES=1
  #   export USE_ZEND_ALLOC=0
  #   valgrind --leak-check=yes php -dextension=./modules/protobuf.so memory_leak_test.php
  # else
  #   # valgrind --leak-check=yes hhvm -d extension_dir=. -d hhvm.extensions[]=protobuf.so memory_leak_test.php
  #   valgrind --leak-check=yes hhvm memory_leak_test.php
  # fi
}

PROTOC=../../../../../src/protoc

# Test HHVM
$PROTOC --php_out=hhvm:generated test.proto
# make clean; hphpize; cmake . && make && hhvm -d extension_dir=. -d hhvm.extensions[]=protobuf.so test_hhvm.php
phpunit --version
make clean; hphpize; cmake . && make
run_tests hhvm

# # Test PHP
# $PROTOC --php_out=generated test.proto
# OLD_PATH=$PATH
# OLD_CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH
# OLD_C_INCLUDE_PATH=$C_INCLUDE_PATH
# 
# # php-5.5
# export PATH=/usr/local/php-5.5/bin:$OLD_PATH
# export CPLUS_INCLUDE_PATH=/usr/local/php-5.5/include/php/main:/usr/local/php-5.5/include/php/:$OLD_CPLUS_INCLUDE_PATH
# export C_INCLUDE_PATH=/usr/local/php-5.5/include/php/main:/usr/local/php-5.5/include/php/:$OLD_C_INCLUDE_PATH
# 
# # make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && php -dextension=./modules/protobuf.so test_php.php
# make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && run_tests php

# # php-5.5-zts
# export PATH=/usr/local/php-5.5-zts/bin:$OLD_PATH
# export CPLUS_INCLUDE_PATH=/usr/local/php-5.5-zts/include/php/main:/usr/local/php-5.5-zts/include/php/:$OLD_CPLUS_INCLUDE_PATH
# export C_INCLUDE_PATH=/usr/local/php-5.5-zts/include/php/main:/usr/local/php-5.5-zts/include/php/:$OLD_C_INCLUDE_PATH
# 
# make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && run_tests php

# # php-7.1
# export PATH=/usr/local/php-7.1/bin:$OLD_PATH
# export CPLUS_INCLUDE_PATH=/usr/local/php-7.1/include/php/main:/usr/local/php-7.1/include/php/:$OLD_CPLUS_INCLUDE_PATH
# export C_INCLUDE_PATH=/usr/local/php-7.1/include/php/main:/usr/local/php-7.1/include/php/:$OLD_C_INCLUDE_PATH
# 
# make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && run_tests php

# # php-7.1-zts
# export PATH=/usr/local/php-7.1-zts/bin:$OLD_PATH
# export CPLUS_INCLUDE_PATH=/usr/local/php-7.1-zts/include/php/main:/usr/local/php-7.1-zts/include/php/:$OLD_CPLUS_INCLUDE_PATH
# export C_INCLUDE_PATH=/usr/local/php-7.1-zts/include/php/main:/usr/local/php-7.1-zts/include/php/:$OLD_C_INCLUDE_PATH
# 
# make clean; phpize && ./configure CFLAGS='-g -O0' CXXFLAGS='-g -O0' && make && php -dextension=./modules/protobuf.so test_php.php

echo "All Tests Passed"

