#!/bin/bash

# Bare build: no dependencies installed, no JIT enabled.
bare_install() {
  :
}
bare_script() {
  make -j12 tests && make test
}

# Bare JIT build: no dependencies installed, but JIT enabled.
barejit_install() {
  :
}
barejit_script() {
  make -j12 tests WITH_JIT=yes && make test
}

# Build with Google protobuf support and tests (with JIT).
withprotobuf_install() {
  sudo apt-get update -qq
  sudo apt-get install protobuf-compiler libprotobuf-dev
}
withprotobuf_script() {
  make -j12 tests googlepbtests WITH_JIT=yes
  make test
}

set -e
set -x
eval ${UPB_TRAVIS_BUILD}_${1}

