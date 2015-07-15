#!/usr/bin/env bash

# Exit on any error.
set -e

test_version() {
  version=$1
  bash --login -c \
    "rvm install $version && rvm use $version && \
     which ruby && \
     gem install bundler && bundle && \
     rake test && \
     cd ../conformance && \
     make test_ruby"
}

test_version $1
