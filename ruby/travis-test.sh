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
       gem install bundler && bundle && \
       rake test"
  else
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       gem install bundler && bundle && \
       rake test &&
       cd ../conformance && make test_ruby"
  fi
}

test_version $1
