#!/usr/bin/env bash

# Exit on any error.
set -e

test_version() {
  version=$1

  RUBY_CONFORMANCE=test_ruby

  if [ "$version" == "jruby-1.7" ] ; then
    # No conformance tests yet -- JRuby is too broken to run them.
    bash --login -c \
      "rvm install $version && rvm use $version && rvm get head && \
       which ruby && \
       git clean -f && \
       gem install bundler && bundle && \
       rake test"
  elif [ "$version" == "ruby-2.6.0" -o "$version" == "ruby-2.7.0" ] ; then
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       git clean -f && \
       gem install bundler -v 1.17.3 && bundle && \
       rake test &&
       rake gc_test &&
       cd ../conformance && make ${RUBY_CONFORMANCE} &&
       cd ../ruby/compatibility_tests/v3.0.0 &&
       cp -R ../../lib lib && ./test.sh"
  else
    # Recent versions of OSX have deprecated OpenSSL, so we have to explicitly
    # provide a path to the OpenSSL directory installed via Homebrew.
    bash --login -c \
      "rvm install $version --with-openssl-dir=`brew --prefix openssl` && \
       rvm use $version && \
       which ruby && \
       git clean -f && \
       gem install bundler -v 1.17.3 && bundle && \
       rake test &&
       rake gc_test &&
       cd ../conformance && make ${RUBY_CONFORMANCE} &&
       cd ../ruby/compatibility_tests/v3.0.0 && ./test.sh"
  fi
}

test_version $1
