#!/usr/bin/env bash

# Exit on any error.
set -ex

test_version() {
  version=$1
  bazel_args="\
    $(../kokoro/common/bazel_flags.sh) \
    --action_env=PATH \
    --action_env=GEM_PATH \
    --action_env=GEM_HOME \
    --test_env=KOKORO_RUBY_VERSION=$version"

  if [[ $version == jruby-9* ]] ; then
    bash --login -c \
      "rvm install $version && rvm use $version && rvm get head && \
       which ruby && \
       git clean -f && \
       gem install --no-document bundler && bundle && \
       bazel test //ruby/... $bazel_args --define=ruby_platform=java"
  else
    bash --login -c \
      "rvm install $version && rvm use $version && \
       which ruby && \
       git clean -f && \
       gem install --no-document bundler -v 1.17.3 && bundle && \
       bazel test //ruby/... $bazel_args --define=ruby_platform=c"
  fi
}

test_version $1
