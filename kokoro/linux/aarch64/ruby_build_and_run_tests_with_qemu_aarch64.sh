#!/bin/bash

set -ex

# go to the repo root
cd $(dirname $0)/../../..

gem install bundler

cd ruby

bundle
rake
rake clobber_package gem

# run all the tests
rake test
