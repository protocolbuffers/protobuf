#!/usr/bin/env bash

# Exit on any error.
set -e

test_version() {
  version=$1
  if [ "$version" == "jruby-1.7" ] ; then
    # No conformance tests yet -- JRuby is too broken to run them.
    bash --login -c \
      "rvm install $version && rvm use $version && rvm get head && \
       which ruby && \
       git clean -f && \
       gem install bundler && bundle && \
       rake test"
  elif [ "$version" == "ruby-2.6.0" ] ; then
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       git clean -f && \
       gem install bundler -v 1.17.3 && bundle && \
       rake test &&
       rake gc_test &&
       cd ../conformance && make test_ruby &&
       cd ../ruby/compatibility_tests/v3.0.0 &&
       cp -R ../../lib lib && ./test.sh"
  else
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       git clean -f && \
       gem install bundler -v 1.17.3 && bundle && \
       rake test &&
       rake gc_test &&
       cd ../conformance && make test_ruby &&
       cd ../ruby/compatibility_tests/v3.0.0 && ./test.sh"
  fi
}

test_version $1
