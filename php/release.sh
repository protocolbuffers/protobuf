#!/bin/bash

set -ex

# Make sure we are in a protobuf source tree.
[ -f "php/release.sh" ] || {
  echo "This script must be ran under root of protobuf source tree."
  exit 1
}

VERSION=$1

rm -rf protobuf-php
git clone https://github.com/protocolbuffers/protobuf-php.git

# Clean old files
rm -rf protobuf-php/src

# Copy files
cp -r php/src protobuf-php
cp php/composer.json.dist protobuf-php/composer.json

cd protobuf-php
git add .
git commit -m "$VERSION"
if [ $(git tag -l "$VERSION") ]; then
  echo "tag $VERSION already exists"
else
  git tag "$VERSION"
fi
