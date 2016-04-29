#!/usr/bin/env bash

# Exit on any error.
set -ex

test_version() {
  echo $PATH
  which ruby
  ruby --version
  version=$1
  if [ "$version" == "jruby" ] ; then
    # No conformance tests yet -- JRuby is too broken to run them.
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       gem install bundler && bundle && \
       rake test"
  else
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       gem install bundler && bundle && \
       rake test &&
       RUBYLIB=lib:../conformance ../conformance/conformance-test-runner --failure_list ../conformance/failure_list_ruby.txt ../conformance/conformance_ruby.rb"
  fi
}

test_version $1
