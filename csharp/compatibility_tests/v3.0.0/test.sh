#!/bin/bash

function run_test() {
  # Generate test proto files.
  ./protoc_1 -Iprotos/src -I../../../src/ --csharp_out=src/Google.Protobuf.Test \
    --csharp_opt=base_namespace=Google.Protobuf \
    protos/src/google/protobuf/unittest_import_proto3.proto \
    protos/src/google/protobuf/unittest_import_public_proto3.proto \
    protos/src/google/protobuf/unittest_well_known_types.proto

  ./protoc_1 -Iprotos/csharp --csharp_out=src/Google.Protobuf.Test \
    --csharp_opt=base_namespace=UnitTest.Issues \
    protos/csharp/protos/unittest_issues.proto

  ./protoc_2 -Iprotos/src --csharp_out=src/Google.Protobuf.Test \
    --csharp_opt=base_namespace=Google.Protobuf \
    protos/src/google/protobuf/unittest_proto3.proto \
    protos/src/google/protobuf/map_unittest_proto3.proto

  # Build and test.
  dotnet build -c release src/Google.Protobuf src/Google.Protobuf.Test
  dotnet test -c release -f netcoreapp1.0 src/Google.Protobuf.Test
}

set -ex

# Change to the script's directory.
cd $(dirname $0)

# Version of the tests (i.e., the version of protobuf from where we extracted
# these tests).
TEST_VERSION=3.0.0

# The old version of protobuf that we are testing compatibility against. This
# is usually the same as TEST_VERSION (i.e., we use the tests extracted from
# that version to test compatibility of the newest runtime against it), but it
# is also possible to use this same test set to test the compatibiilty of the
# latest version against other versions.
case "$1" in
  ""|3.0.0)
    OLD_VERSION=3.0.0
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0/protoc-3.0.0-linux-x86_64.exe
    ;;
  3.0.2)
    OLD_VERSION=3.0.2
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.2/protoc-3.0.2-linux-x86_64.exe
    ;;
  3.1.0)
    OLD_VERSION=3.1.0
    OLD_VERSION_PROTOC=http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.1.0/protoc-3.1.0-linux-x86_64.exe
    ;;
  *)
    echo "[ERROR]: Unknown version number: $1"
    exit 1
    ;;
esac

echo "Running compatibility tests with $OLD_VERSION"

# Check protoc
[ -f ../../../src/protoc ] || {
  echo "[ERROR]: Please build protoc first."
  exit 1
}

# Download old version protoc compiler (for linux).
wget $OLD_VERSION_PROTOC -O old_protoc
chmod +x old_protoc

# Test source compatibility. In these tests we recompile everything against
# the new runtime (including old version generated code).
# Copy the new runtime and keys.
cp ../../src/Google.Protobuf src/Google.Protobuf -r
cp ../../keys . -r
dotnet restore

# Test A.1:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use old version
cp old_protoc protoc_1
cp old_protoc protoc_2
run_test

# Test A.2:
#   proto set 1: use new version
#   proto set 2 which may import protos in set 1: use old version
cp ../../../src/protoc protoc_1
cp old_protoc protoc_2
run_test

# Test A.3:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use new version
cp old_protoc protoc_1
cp ../../../src/protoc protoc_2
run_test

rm protoc_1
rm protoc_2
rm old_protoc
rm keys -r
rm src/Google.Protobuf -r
