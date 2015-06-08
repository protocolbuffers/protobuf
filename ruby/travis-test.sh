#!/usr/bin/env bash

set -e  # exit immediately on error
set -x  # display all commands

# RVM depends on running inside a login shell.
# Re-invoke ourselves under a login shell, but prevent infinite recursion.
if [ "$2" != "login" ]; then
  bash --login $0 $1 login
  exit
fi

test_version() {
  version=$1
  rvm get stable
  rvm reload
  rvm install $version
  rvm use $version
  which ruby
  gem install bundler
  bundle
  rake test
}

test_version $1
