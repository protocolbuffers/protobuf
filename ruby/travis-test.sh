#!/usr/bin/env bash

# Exit on any error.
set -ex

test_version() {
  version=$1

  RUBY_CONFORMANCE=test_ruby

  if [[ $version == jruby-9* ]] ; then
    bash --login -c \
      "rvm install $version && rvm use $version && rvm get head && \
       which ruby && \
       git clean -f && \
       gem install --no-document bundler && bundle && \
       rake test && \
       rake gc_test && \
       cd ../conformance && make test_jruby && \
       cd ../ruby/compatibility_tests/v3.0.0 && ./test.sh"
  else
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       git clean -f && \
       gem install --no-document bundler -v 1.17.3 && bundle && \
       rake test && \
       rake gc_test && \
       cd ../conformance && make ${RUBY_CONFORMANCE} && \
       cd ../ruby/compatibility_tests/v3.0.0 && \
       cp -R ../../lib lib && ./test.sh"
  fi
}

test_version $1
