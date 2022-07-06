#!/usr/bin/env bash

# Exit on any error.
set -ex

test_version() {
  version=$1

  if [[ $version == jruby-9* ]] ; then
    RUBY_CONFORMANCE=test_jruby
  else
    RUBY_CONFORMANCE=test_ruby
  fi
  bash --login -c \
    "rvm install $version && rvm use $version && \
     which ruby && \
     git clean -f && \
     gem install --no-document bundler && bundle && \
     rake test && \
     rake gc_test && \
     cd ../conformance && make ${RUBY_CONFORMANCE} && \
     cd ../ruby/compatibility_tests/v3.0.0 && \
     cp -R ../../lib lib && cp -R ../../ext ext && ./test.sh"
}

test_version $1
