#!/bin/bash

VERSION=$1

export PATH=/usr/local/php-$VERSION/bin:$PATH
export C_INCLUDE_PATH=/usr/local/php-$VERSION/include/php/main:/usr/local/php-$VERSION/include/php:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/usr/local/php-$VERSION/include/php/main:/usr/local/php-$VERSION/include/php:$CPLUS_INCLUDE_PATH

php -i | grep "Configuration"

# gdb --args php -dextension=../ext/google/protobuf/modules/protobuf.so `which
# phpunit` --bootstrap autoload.php tmp_test.php
#
gdb --args php -dextension=../ext/google/protobuf/modules/protobuf.so `which phpunit` --bootstrap autoload.php generated_class_test.php
#
# gdb --args php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
#
# USE_ZEND_ALLOC=0 valgrind --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
