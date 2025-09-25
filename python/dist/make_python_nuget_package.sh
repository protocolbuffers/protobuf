#!/bin/bash -ex

ARCH=$1
VERSION=$2
DIR=python/dist/registry/modules/nuget_python_${ARCH}_$VERSION/$VERSION

IFS='.' read -ra VERSION_ARRAY <<< "$VERSION"
FULL_API="${VERSION_ARRAY[0]}${VERSION_ARRAY[1]}"
LIMITED_API=${VERSION_ARRAY[0]}

rm -rf $DIR
mkdir -p $DIR
cd $DIR

FOLDER_NAME="python"
if [[ $ARCH == "i686" ]]; then
  FOLDER_NAME="pythonx86"
fi

# Calculate the sha256 of the downloaded tar
URL=https://www.nuget.org/api/v2/package/$FOLDER_NAME/$VERSION
wget $URL
REPO_SHA=$(sha256sum $VERSION | cut -d ' ' -f 1 | xxd -r -p | base64)
rm $VERSION

mkdir -p overlay

# Create BUILD.bazel overlay
BUILD=overlay/BUILD.bazel
cat <<EOF >$BUILD
cc_import(
    name = "python_full_api_lib",
    shared_library = "python${FULL_API}.dll",
    interface_library = "libs/python${FULL_API}.lib",
)

cc_library(
    name = "python_full_api",
    hdrs = glob(["**/*.h"], allow_empty=True),
    strip_include_prefix = "include",
    deps = [":python_full_api_lib"],
    visibility = ["//visibility:public"],
)

cc_import(
    name = "python_limited_api_lib",
    shared_library = "python${LIMITED_API}.dll",
    interface_library = "libs/python${LIMITED_API}.lib",
)

cc_library(
    name = "python_limited_api",
    hdrs = glob(["**/*.h"], allow_empty=True),
    strip_include_prefix = "include",
    deps = [":python_limited_api_lib"],
    visibility = ["//visibility:public"],
)

EOF
BUILD_SHA=$(sha256sum $BUILD | cut -d ' ' -f 1 | xxd -r -p | base64)

cat <<EOF >source.json
{
  "url": "$URL",
  "archive_type": "zip",
  "integrity": "sha256-$REPO_SHA",
  "strip_prefix": "tools",
  "overlay": {
    "BUILD.bazel": "sha256-$BUILD_SHA"
  }
}
EOF

cat <<EOF >MODULE.bazel
# Generated with ./python/dist/make_python_nuget_package.sh $ARCH $VERSION
module(
    name = "nuget_python_${ARCH}_$VERSION",
    version = "$VERSION",
    compatibility_level = 1,
)
EOF
