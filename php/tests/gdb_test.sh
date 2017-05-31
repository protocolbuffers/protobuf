#!/bin/bash

# gdb --args php -dextension=../ext/google/protobuf/modules/protobuf.so `which
# phpunit` --bootstrap autoload.php tmp_test.php
#
gdb --args php -dextension=../ext/google/protobuf/modules/protobuf.so `which phpunit` --bootstrap autoload.php well_known_test.php
#
# gdb --args php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
#
# USE_ZEND_ALLOC=0 valgrind --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
