#!/bin/bash -ex

VERSION=$1
DIR=python/dist/registry/modules/python-$VERSION/$VERSION

rm -rf $DIR
mkdir -p $DIR
cd $DIR

# Calculate the sha256 of the downloaded tar
URL=https://www.python.org/ftp/python/$VERSION/Python-$VERSION.tgz
wget $URL
REPO_SHA=$(sha256sum Python-$VERSION.tgz | cut -d ' ' -f 1 | xxd -r -p | base64)
rm Python-$VERSION.tgz

# Create pyconfig.h overlay
mkdir -p overlay/Python-$VERSION/Include
PYCONFIG=overlay/Python-$VERSION/Include/pyconfig.h
cat <<EOF >$PYCONFIG
#define SIZEOF_WCHAR_T 4
EOF
PYCONFIG_SHA=$(sha256sum $PYCONFIG | cut -d ' ' -f 1 | xxd -r -p | base64)

# Create BUILD.bazel overlay
BUILD=overlay/BUILD.bazel
cat <<EOF >$BUILD
cc_library(
    name = "python_headers",
    hdrs = glob(["**/Include/**/*.h"]),
    strip_include_prefix = "Python-$VERSION/Include",
    visibility = ["//visibility:public"],
)
EOF
BUILD_SHA=$(sha256sum $BUILD | cut -d ' ' -f 1 | xxd -r -p | base64)

cat <<EOF >source.json
{
  "url": "$URL",
  "integrity": "sha256-$REPO_SHA",
  "overlay": {
    "BUILD.bazel": "sha256-$BUILD_SHA",
    "Python-3.9.0/Include/pyconfig.h": "sha256-$PYCONFIG_SHA"
  }
}
EOF

cat <<EOF >MODULE.bazel
# Generated with ./python/dist/make_python_source_archive.sh $VERSION
module(
    name = "python-$VERSION",
    version = "$VERSION",
    compatibility_level = 1,
)
EOF
