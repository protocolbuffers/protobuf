#!/bin/bash

# Bare build: no dependencies installed, no JIT enabled.
bare_install() {
  :
}
bare_script() {
  make -j12 tests
  make test
}

# Bare JIT build: no dependencies installed, but JIT enabled.
barejit_install() {
  :
}
barejit_script() {
  make -j12 tests WITH_JIT=yes
  make test
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

# A 32-bit build.  Can only test the core because any dependencies
# need to be available as 32-bit libs also, which gets hairy fast.
# Can't enable the JIT because it only supports x64.
core32_install() {
  sudo apt-get install libc6-dev-i386
}
core32_script() {
  make -j12 tests USER_CPPFLAGS=-m32
  make test
}

# A build of Lua and running of Lua tests.
lua_install() {
  sudo apt-get update -qq
  sudo apt-get install lua5.2 liblua5.2-dev
}
lua_script() {
  make -j12 testlua USER_CPPFLAGS=`pkg-config lua5.2 --cflags`
}

set -e
set -x
eval ${UPB_TRAVIS_BUILD}_${1}

