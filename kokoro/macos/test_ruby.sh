#!/bin/bash

set -ex

VERSION=$1

# Prepare worker environment to run tests
KOKORO_INSTALL_RVM=yes
source kokoro/macos/prepare_build_macos_rc

# Setup ruby
rvm install $VERSION
rvm use $VERSION
rvm get head
which ruby
rvm current | grep -qe "${RUBY_VERSION}.*" || exit 1;

# Run tests
bazel test //ruby/... --test_env=KOKORO_RUBY_VERSION=$VERSION
