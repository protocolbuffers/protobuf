#!/bin/bash

set +ex
[[ -s /etc/profile.d/rvm.sh ]] && . /etc/profile.d/rvm.sh
set -e  # rvm commands are very verbose
rvm --default use ruby-2.4.1
# Install the bundler version specified in Gemfile.lock.
# This command can be replaced with "gem install bundler --update" 
# if we stop specifying the bundler version in Gemfile.lock.
gem install bundler -v "$(grep -A 1 "BUNDLED WITH" Gemfile.lock | tail -n 1)"
set -ex
