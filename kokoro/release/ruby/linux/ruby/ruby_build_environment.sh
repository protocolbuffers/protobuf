#!/bin/bash

set +ex
[[ -s /etc/profile.d/rvm.sh ]] && . /etc/profile.d/rvm.sh
set -e  # rvm commands are very verbose
rvm --default use ruby-2.4.1
# The version needs to be updated if the version specified in Gemfile.lock is changed
gem install bundler -v '1.17.3'
set -ex
