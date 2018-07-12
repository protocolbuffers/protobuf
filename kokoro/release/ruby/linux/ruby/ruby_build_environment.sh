#!/bin/bash

set +ex
[[ -s /etc/profile.d/rvm.sh ]] && . /etc/profile.d/rvm.sh
set -e  # rvm commands are very verbose
rvm --default use ruby-2.4.1
gem install bundler --update
set -ex
