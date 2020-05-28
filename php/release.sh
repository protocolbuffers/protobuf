#!/bin/bash

set -ex

# Make sure we are in a protobuf source tree.
[ -f "php/release.sh" ] || {
  echo "This script must be ran under root of protobuf source tree."
  exit 1
}

VERSION=$1

git clone git@github.com:protocolbuffers/protobuf-php.git
git clone git@github.com:protocolbuffers/protobuf.git

# Clean old files
pushd protobuf-php
rm -rf src
popd

# Checkout the target version
pushd protobuf/php
git checkout -b $VERSION
popd

# Copy files
pushd protobuf-php
mv ../protobuf/php/src src
mv ../protobuf/composer.json composer.json
sed -i 's|php/src|src|g' composer.json
git add .
git commit -m "$VERSION"
git tag "$VERSION"
popd

# Clean up
rm -rf protobuf
