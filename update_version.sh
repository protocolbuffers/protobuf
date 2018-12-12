#!/bin/bash
# Helper script to update version number in various files.
# Each file has it own string matching syntax to avoid accidentally updating
# the wrong version number of other dependencies.

if [ "$#" -eq 1 ]; then
  VERSION=$1
  IFS='.' read -ra VERSION_INFO <<< "$VERSION"
fi

if [ "$#" -ne 1 ] || [ ${#VERSION_INFO[@]} -ne 3 ]; then
  echo "
Usage: $0 VERSION

Example:
$0 2.1.3
"
  exit 1
fi

update_version() {
  file=$2
  # Replace the version number in the given file.
  sed -ri "$1" $file
  # Verify that the version number is updated successfully.
  if [ $(grep -c $VERSION $file) -eq 0 ]; then
    echo "$file version is not updated successfully. Please verify."
    exit 1
  fi
}

update_version "s/\[Protocol Buffers\],\[.*\],\[protobuf@googlegroups.com\]/[Protocol Buffers],[$VERSION],[protobuf@googlegroups.com]/g" configure.ac
update_version "s/^  <version>.*<\/version>/  <version>$VERSION<\/version>/g" java/pom.xml
update_version "s/^    <version>.*<\/version>/    <version>$VERSION<\/version>/g" java/core/pom.xml
update_version "s/^    <version>.*<\/version>/    <version>$VERSION<\/version>/g" java/util/pom.xml
update_version "s/^  <version>.*<\/version>/  <version>$VERSION<\/version>/g" protoc-artifacts/pom.xml
update_version "s/^  s.version  = '.*'/  s.version  = '$VERSION'/g" Protobuf.podspec
update_version "s/^__version__ = '.*'/__version__ = '$VERSION'/g" python/google/protobuf/__init__.py
update_version "s/^    <VersionPrefix>.*<\/VersionPrefix>/    <VersionPrefix>$VERSION<\/VersionPrefix>/g" csharp/src/Google.Protobuf/Google.Protobuf.csproj
update_version "s/^    <version>.*<\/version>/    <version>$VERSION<\/version>/g" csharp/Google.Protobuf.Tools.nuspec
update_version "s/^  \"version\": \".*\",/  \"version\": \"$VERSION\",/g" js/package.json
update_version "s/^  s.version     = \".*\"/  s.version     = \"$VERSION\"/g" ruby/google-protobuf.gemspec

# Special handling for C++ file, where version is X.Y.Z is transformed to X00Y00Z
CPP_VERSION=${VERSION_INFO[0]}00${VERSION_INFO[1]}00${VERSION_INFO[2]}
sed -ri "s/^#define GOOGLE_PROTOBUF_VERSION .*/#define GOOGLE_PROTOBUF_VERSION $CPP_VERSION/g" src/google/protobuf/stubs/common.h
if [ $(grep -c $CPP_VERSION src/google/protobuf/stubs/common.h) -eq 0 ]; then
  echo "src/google/protobuf/stubs/common.h version is not updated"
  exit 1
fi

# Increment PROTOBUF_VERSION by 1.
sed -ri 's/^(PROTOBUF_VERSION = )([0-9]+)/echo "\1$((\2+1))"/ge' src/Makefile.am
