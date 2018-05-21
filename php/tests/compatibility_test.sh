#!/bin/bash

function use_php() {
  VERSION=$1
  PHP=`which php`
  PHP_CONFIG=`which php-config`
  PHPIZE=`which phpize`
  ln -sfn "/usr/local/php-${VERSION}/bin/php" $PHP
  ln -sfn "/usr/local/php-${VERSION}/bin/php-config" $PHP_CONFIG
  ln -sfn "/usr/local/php-${VERSION}/bin/phpize" $PHPIZE
}

function generate_proto() {
  PROTOC1=$1
  PROTOC2=$2

  rm -rf generated
  mkdir generated

  $PROTOC1 --php_out=generated proto/test_include.proto
  $PROTOC2 --php_out=generated proto/test.proto proto/test_no_namespace.proto proto/test_prefix.proto
  pushd ../../src
  $PROTOC2 --php_out=../php/tests/generated -I../php/tests -I. ../php/tests/proto/test_import_descriptor_proto.proto
  popd
}

# Remove tests to expect error. These were added to API tests by mistake.
function remove_error_test() {
  local TEMPFILE=`tempfile`
  cat $1 | \
  awk -v file=`basename $1` -v dir=`basename $(dirname $1)` '
    BEGIN {
      show = 1
    }
    /@expectedException PHPUnit_Framework_Error/ { show = 0; next; }
    / *\*\//                                     { print; next; }
    / *}/ {
      if (!show) {
        show = 1;
        next;
      }
    }
    show { print }
  ' > $TEMPFILE
  cp $TEMPFILE $1
}

set -ex

# Change to the script's directory.
cd $(dirname $0)

# The old version of protobuf that we are testing compatibility against.
case "$1" in
  ""|3.3.0)
    OLD_VERSION=3.3.0
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.3.0/protoc-3.3.0-linux-x86_64.exe
    ;;
  *)
    echo "[ERROR]: Unknown version number: $1"
    exit 1
    ;;
esac

# Extract the latest protobuf version number.
VERSION_NUMBER=`grep "PHP_PROTOBUF_VERSION" ../ext/google/protobuf/protobuf.h | sed "s|#define PHP_PROTOBUF_VERSION \"\(.*\)\"|\1|"`

echo "Running compatibility tests between $VERSION_NUMBER and $OLD_VERSION"

# Check protoc
[ -f ../../src/protoc ] || {
  echo "[ERROR]: Please build protoc first."
  exit 1
}

# Download old test.
rm -rf protobuf
git clone https://github.com/google/protobuf.git
pushd protobuf
git checkout v$OLD_VERSION
popd

# Build and copy the new runtime
use_php 5.5
pushd ../ext/google/protobuf
make clean || true
phpize && ./configure && make
popd

rm -rf protobuf/php/ext
rm -rf protobuf/php/src
cp -r ../ext protobuf/php/ext/
cp -r ../src protobuf/php/src/

# Download old version protoc compiler (for linux)
wget $OLD_VERSION_PROTOC -O old_protoc
chmod +x old_protoc

NEW_PROTOC=`pwd`/../../src/protoc
OLD_PROTOC=`pwd`/old_protoc
cd protobuf/php
cp -r /usr/local/vendor-5.5 vendor
wget https://phar.phpunit.de/phpunit-4.8.0.phar -O /usr/bin/phpunit

# Remove implementation detail tests.
tests=( array_test.php encode_decode_test.php generated_class_test.php map_field_test.php well_known_test.php )
sed -i.bak '/php_implementation_test.php/d' phpunit.xml
for t in "${tests[@]}"
do
  remove_error_test tests/$t
done

cd tests

# Test A.1:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use old version
generate_proto $OLD_PROTOC $OLD_PROTOC
./test.sh
pushd ..
phpunit
popd

# Test A.2:
#   proto set 1: use new version
#   proto set 2 which may import protos in set 1: use old version
generate_proto $NEW_PROTOC $OLD_PROTOC
./test.sh
pushd ..
phpunit
popd

# Test A.3:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use new version
generate_proto $OLD_PROTOC $NEW_PROTOC
./test.sh
pushd ..
phpunit
popd
