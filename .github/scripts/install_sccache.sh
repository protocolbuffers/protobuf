#!/bin/bash
# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
# Stopgap for the CMake-under-Bazel CI jobs (test_cpp.yml, linux-cmake-bazel):
# installs sccache into the running container if the docker image does not
# ship it. The CMake images do; the Bazel/monolithic images do not yet. Delete
# this script, and its calls in test_cpp.yml, once the image includes sccache.
#
# Pinned to the release protocolbuffers/protobuf-ci/sccache defaults to, with
# its checksum verified before anything is installed.

set -euo pipefail

if command -v sccache >/dev/null 2>&1; then
  exit 0
fi

SCCACHE_VERSION=v0.5.4
SCCACHE_SHA256=4bf3ce366aa02599019093584a5cbad4df783f8d6e3610548c2044daa595d40b
ARCHIVE="sccache-${SCCACHE_VERSION}-x86_64-unknown-linux-musl"

tmp="$(mktemp -d)"
trap 'rm -rf "${tmp}"' EXIT

curl -sSL -o "${tmp}/${ARCHIVE}.tar.gz" \
  "https://github.com/mozilla/sccache/releases/download/${SCCACHE_VERSION}/${ARCHIVE}.tar.gz"
echo "${SCCACHE_SHA256}  ${tmp}/${ARCHIVE}.tar.gz" | sha256sum -c -
tar -xzf "${tmp}/${ARCHIVE}.tar.gz" -C "${tmp}"
install -m 755 "${tmp}/${ARCHIVE}/sccache" /usr/local/bin/sccache

echo "Installed $(sccache --version) (image does not ship sccache)"
