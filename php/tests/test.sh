#!/bin/bash

set -ex

cd $(dirname $0)

../generate_test_protos.sh
./compile_extension.sh

PHP_VERSION=$(php -r "echo PHP_VERSION;")

# Each version of PHPUnit supports a fairly narrow range of PHP versions.
case "$PHP_VERSION" in
  7.0.*)
    PHPUNIT=phpunit-6.phar
    ;;
  7.1.*|7.2.*)
    PHPUNIT=phpunit-7.5.0.phar
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

php -dextension=../ext/google/protobuf/modules/protobuf.so $PHPUNIT --bootstrap ../vendor/autoload.php .
php -d protobuf.keep_descriptor_pool_after_request=1 -dextension=../ext/google/protobuf/modules/protobuf.so $PHPUNIT --bootstrap ../vendor/autoload.php .
