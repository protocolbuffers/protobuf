#!/bin/bash

set -eux

pushd php
rm -rf vendor
php -v
php -m
composer update
composer test_c
popd

git clean -fXd
bazel test $(kokoro/common/bazel_flags.sh) \
  --action_env=PATH --test_env=PATH \
  //php:conformance_test_c
