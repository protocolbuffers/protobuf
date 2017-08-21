@echo off
REM The test.bat is unavailable yet.

set tests=array_test.php encode_decode_test.php generated_class_test.php generated_phpdoc_test.php map_field_test.php well_known_test.php generated_service_test.php descriptors_test.php

(for %%a in (%tests%) do (
  echo ****************************
  echo *    %%a
  echo ****************************
  php -dextension=%PHP_BINARY_PATH%\ext\php_mbstring.dll -dextension=%PHP_RELEASE_PATH%\php_protobuf.dll %PHP_RELEASE_PATH%\phpunit.phar --bootstrap autoload.php %%a
))

exit /b 0

REM export ZEND_DONT_UNLOAD_MODULES=1
REM export USE_ZEND_ALLOC=0
REM valgrind --leak-check=yes php -dextension=../ext/google/protobuf/modules/protobuf.so memory_leak_test.php
