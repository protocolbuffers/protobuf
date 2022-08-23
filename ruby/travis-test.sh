#!/usr/bin/env bash

# Exit on any error.
set -ex

test_version() {
  version=$1
  bazel_args=" \
    -k --test_output=streamed \
    --action_env=PATH \
    --action_env=GEM_PATH \
    --action_env=GEM_HOME \
    --test_env=KOKORO_RUBY_VERSION=$version"

  if [[ $version == jruby-9* ]] ; then
    RUBY_PLATFORM=java
  else
    RUBY_PLATFORM=c
  fi
  bash --login -c \
    "rvm install $version && rvm use $version && \
     which ruby && \
     git clean -f && \
     gem install --no-document bundler && bundle && \
     bazel test //ruby/... $bazel_args --define=ruby_platform=$RUBY_PLATFORM"
}

test_version $1
