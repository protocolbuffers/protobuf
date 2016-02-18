#!/usr/bin/env bash

my_dir="$(dirname "$0")"

source $my_dir/tests.sh

# Note: travis currently does not support testing more than one language so the
# .travis.yml cheats and claims to only be cpp.  If they add multiple language
# support, this should probably get updated to install steps and/or
# rvm/gemfile/jdk/etc. entries rather than manually doing the work.

# .travis.yml uses matrix.exclude to block the cases where app-get can't be
# use to install things.

# -------- main --------

if [ "$#" -ne 1 ]; then
  echo "
Usage: $0 { cpp |
            csharp |
            java_jdk6 |
            java_jdk7 |
            java_oracle7 |
            javanano_jdk6 |
            javanano_jdk7 |
            javanano_oracle7 |
            objectivec_ios |
            objectivec_osx |
            python |
            python_cpp |
            ruby_19 |
            ruby_20 |
            ruby_21 |
            ruby_22 |
            jruby }
"
  exit 1
fi

set -e  # exit immediately on error
set -x  # display all commands
eval "build_$1"
