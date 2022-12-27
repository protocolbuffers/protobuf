#!/bin/bash

set -eux

VERSION=$1

# Prepare worker environment to run tests
KOKORO_INSTALL_RVM=yes
source kokoro/macos/prepare_build_macos_rc

# wget is required for ruby compatibility tests.
brew install wget

# Configure system ruby.
# We need to disable unbound variable errors, due to a known issue described in
# https://github.com/rvm/rvm/issues/4618.
set +u
rvm install $VERSION
rvm use $VERSION
rvm current | grep -qe "${RUBY_VERSION}.*" || exit 1;
set -u

# Run tests
bazel test //ruby/... --test_env=KOKORO_RUBY_VERSION=$VERSION
