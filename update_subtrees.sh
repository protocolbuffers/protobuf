#!/bin/bash -eux

set -eux

cd $(dirname $0)

git subtree pull --prefix third_party/utf8_range \
  https://github.com/protocolbuffers/utf8_range.git main --squash
