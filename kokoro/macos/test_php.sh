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
bazel test //conformance:php_c --action_env=PATH --test_env=PATH --test_output=streamed
