#!/bin/bash

source googletest.sh || exit 1

script=${TEST_SRCDIR}/google3/third_party/protobuf/github/validate_yaml

$script || die "Failed to execute $script"

echo "PASS"
