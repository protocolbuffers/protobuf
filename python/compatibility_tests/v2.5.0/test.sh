#!/bin/bash

set -ex

# Change to the script's directory.
cd $(dirname $0)

# Version of the tests (i.e., the version of protobuf from where we extracted
# these tests).
TEST_VERSION=2.5.0

# The old version of protobuf that we are testing compatibility against. This
# is usually the same as TEST_VERSION (i.e., we use the tests extracted from
# that version to test compatibility of the newest runtime against it), but it
# is also possible to use this same test set to test the compatibiilty of the
# latest version against other versions.
case "$1" in
  ""|2.5.0)
    OLD_VERSION=2.5.0
    OLD_VERSION_PROTOC=https://github.com/xfxyjwf/protobuf-compiler-release/raw/master/v2.5.0/linux/protoc
    ;;
  2.6.1)
    OLD_VERSION=2.6.1
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/2.6.1-build2/protoc-2.6.1-build2-linux-x86_64.exe
    ;;
  3.0.0-beta-1)
    OLD_VERSION=3.0.0-beta-1
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-1/protoc-3.0.0-beta-1-linux-x86_64.exe
    ;;
  3.0.0-beta-2)
    OLD_VERSION=3.0.0-beta-2
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-2/protoc-3.0.0-beta-2-linux-x86_64.exe
    ;;
  3.0.0-beta-3)
    OLD_VERSION=3.0.0-beta-3
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-3/protoc-3.0.0-beta-3-linux-x86_64.exe
    ;;
  3.0.0-beta-4)
    OLD_VERSION=3.0.0-beta-4
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-4/protoc-3.0.0-beta-4-linux-x86_64.exe
    ;;
  *)
    echo "[ERROR]: Unknown version number: $1"
    exit 1
    ;;
esac

# Extract the latest protobuf version number.
VERSION_NUMBER=`grep "^__version__ = '.*'" ../../google/protobuf/__init__.py | sed "s|__version__ = '\(.*\)'|\1|"`

echo "Running compatibility tests between $VERSION_NUMBER and $OLD_VERSION"

# Check protoc
[ -f ../../../src/protoc ] || {
  echo "[ERROR]: Please build protoc first."
  exit 1
}

# Test source compatibility. In these tests we recompile everything against
# the new runtime (including old version generated code).
rm google -f -r
mkdir -p google/protobuf/internal
# Build and copy the new runtime
cd ../../
python setup.py build
cp google/protobuf/*.py compatibility_tests/v2.5.0/google/protobuf/
cp google/protobuf/internal/*.py compatibility_tests/v2.5.0/google/protobuf/internal/
cd compatibility_tests/v2.5.0
cp tests/google/protobuf/internal/test_util.py google/protobuf/internal/
cp google/protobuf/__init__.py google/

# Download old version protoc compiler (for linux)
wget $OLD_VERSION_PROTOC -O old_protoc
chmod +x old_protoc

# Test A.1:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use old version
cp old_protoc protoc_1
cp old_protoc protoc_2
python setup.py build
python setup.py test

# Test A.2:
#   proto set 1: use new version
#   proto set 2 which may import protos in set 1: use old version
cp ../../../src/protoc protoc_1
cp old_protoc protoc_2
python setup.py build
python setup.py test

# Test A.3:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use new version
cp old_protoc protoc_1
cp ../../../src/protoc protoc_2
python setup.py build
python setup.py test

rm google -r -f
rm build -r -f
rm protoc_1
rm protoc_2
rm old_protoc
