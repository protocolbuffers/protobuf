#!/bin/bash

# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google LLC nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This script updates checked-in generated files (currently CMakeLists.txt,
# descriptor.upb.h, and descriptor.upb.c), commits the resulting change, and
# pushes it. This does not do anything useful when run manually, but should be
# run by our GitHub action instead.

set -ex

# Exit early if the previous commit was made by the bot. This reduces the risk
# of a bug causing an infinite loop of auto-generated commits.
if (git log -1 --pretty=format:'%an' | grep -q "Protobuf Team Bot"); then
  echo "Previous commit was authored by bot"
  exit 0
fi

cd $(dirname -- "$0")/..
bazel test //cmake:test_generated_files || bazel-bin/cmake/test_generated_files --fix

# Try to determine the most recent pull request number.
title=$(git log -1 --pretty='%s')
pr_from_merge=$(echo "$title" | sed -n 's/^Merge pull request #\([0-9]\+\).*/\1/p')
pr_from_squash=$(echo "$title" | sed -n 's/^.*(#\([0-9]\+\))$/\1/p')

pr_number=""
if [ ! -z "$pr_from_merge" ]; then
  pr_number="$pr_from_merge"
elif [ ! -z "$pr_from_squash" ]; then
  pr_number="$pr_from_squash"
fi

if [ ! -z "$pr_number" ]; then
  commit_message="Auto-generate CMake file lists after PR #$pr_number"
else
  # If we are unable to determine the pull request number, we fall back on this
  # default commit message. Typically this should not occur, but could happen
  # if a pull request was merged via a rebase.
  commit_message="Auto-generate CMake file lists"
fi

git add -A
git diff --staged --quiet || git commit -am "$commit_message"
git push
