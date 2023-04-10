#!/bin/bash

function run_test() {
  # Generate test proto files.
  $1 -Iprotos/src -I../../../src/ --csharp_out=src/Google.Protobuf.Test \
    --csharp_opt=base_namespace=Google.Protobuf \
    protos/src/google/protobuf/unittest_import_proto3.proto \
    protos/src/google/protobuf/unittest_import_public_proto3.proto \
    protos/src/google/protobuf/unittest_well_known_types.proto

  $1 -Iprotos/csharp --csharp_out=src/Google.Protobuf.Test \
    --csharp_opt=base_namespace=UnitTest.Issues \
    protos/csharp/protos/unittest_issues.proto

  $2 -Iprotos/src --csharp_out=src/Google.Protobuf.Test \
    --csharp_opt=base_namespace=Google.Protobuf \
    protos/src/google/protobuf/unittest_proto3.proto \
    protos/src/google/protobuf/map_unittest_proto3.proto

  # Build and test.
  dotnet restore src/Google.Protobuf/Google.Protobuf.csproj
  dotnet restore src/Google.Protobuf.Test/Google.Protobuf.Test.csproj
  dotnet build -c Release src/Google.Protobuf/Google.Protobuf.csproj
  dotnet build -c Release src/Google.Protobuf.Test/Google.Protobuf.Test.csproj
  dotnet run -c Release -f net6.0 -p src/Google.Protobuf.Test/Google.Protobuf.Test.csproj
}

set -ex

PROTOC=$(realpath ${2:-../../../bazel-bin/protoc})

# Change to the script's directory.
cd $(dirname $0)

# Version of the tests (i.e., the version of protobuf from where we extracted
# these tests).
TEST_VERSION=3.0.0

# The old version of protobuf that we are testing compatibility against. This
# is usually the same as TEST_VERSION (i.e., we use the tests extracted from
# that version to test compatibility of the newest runtime against it), but it
# is also possible to use this same test set to test the compatibility of the
# latest version against other versions.
OLD_VERSION=$1
OLD_VERSION_PROTOC=https://repo1.maven.org/maven2/com/google/protobuf/protoc/$OLD_VERSION/protoc-$OLD_VERSION-linux-x86_64.exe

echo "Running compatibility tests with $OLD_VERSION"

# Check protoc
[ -f $PROTOC ] || {
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

# Test A.1:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use old version
run_test "./old_protoc" "./old_protoc"

# Test A.2:
#   proto set 1: use new version
#   proto set 2 which may import protos in set 1: use old version
run_test "$PROTOC" "./old_protoc"

# Test A.3:
#   proto set 1: use old version
#   proto set 2 which may import protos in set 1: use new version
run_test "./old_protoc" "$PROTOC"

rm old_protoc
rm keys -r
rm src/Google.Protobuf -r
