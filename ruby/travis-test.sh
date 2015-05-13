#!/bin/bash

# Exit on any error.
set -e

test_version() {
  version=$1
  bash --login -c \
    "rvm install $version && rvm use $version && \
     which ruby && \
     gem install bundler && bundle && \
     rake test"
}

test_version ruby-1.9
test_version ruby-2.0
test_version ruby-2.1
test_version ruby-2.2
test_version jruby
