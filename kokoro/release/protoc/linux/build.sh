#!/bin/bash

# This is not the source of truth for release protoc executables, and will soon
# be deprecated.

set -ex

# Change to repo root.
cd $(dirname $0)/../../../..
GIT_REPO_ROOT=$(pwd)

# Initialize any submodules.
git submodule update --init --recursive

# Cross-build for aarch64, ppc64le and s390x. Note: we do these builds first to avoid
# file permission issues. The Docker builds will create directories owned by
# root, which causes problems if we try to add new artifacts to those
# directories afterward.


kokoro/release/protoc/build-protoc.sh linux aarch_64 protoc

kokoro/release/protoc/build-protoc.sh linux ppcle_64 protoc

kokoro/release/protoc/build-protoc.sh linux s390_64 protoc

kokoro/release/protoc/build-protoc.sh linux x86_64 protoc

kokoro/release/protoc/build-protoc.sh linux x86_32 protoc
