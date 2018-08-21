#!/bin/bash

install_protoc() {
  sudo apt-get install protobuf-compiler
  protoc --version || true
}

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

# Build with strict warnings.
warnings_install() {
  :
}
warnings_script() {
  make -j12 default WITH_MAX_WARNINGS=yes
  make -j12 tests WITH_MAX_WARNINGS=yes
  make test
}

# A 32-bit build.  Can only test the core because any dependencies
# need to be available as 32-bit libs also, which gets hairy fast.
# Can't enable the JIT because it only supports x64.
core32_install() {
  sudo apt-get update -qq
  sudo apt-get install libc6-dev-i386 g++-multilib
}
core32_script() {
  make -j12 tests USER_CPPFLAGS="$USER_CPPFLAGS -m32"
  make test
}

# A build of Lua and running of Lua tests.
lua_install() {
  sudo apt-get update -qq
  sudo apt-get install lua5.2 liblua5.2-dev
}
lua_script() {
  make -j12 testlua USER_CPPFLAGS="$USER_CPPFLAGS `pkg-config lua5.2 --cflags`"
}

# Test that generated files don't need to be regenerated.
#
# We would include the Ragel output here too, but we can't really guarantee
# that its output will be stable for multiple versions of the tool, and we
# don't want the test to be brittle.
genfiles_install() {
  sudo apt-get update -qq
  sudo apt-get install lua5.2 liblua5.2-dev

  # Need a recent version of protoc to compile proto3 files.
  # .travis.yml will add this to our path
  mkdir protoc
  cd protoc
  wget https://github.com/google/protobuf/releases/download/v3.0.0-beta-2/protoc-3.0.0-beta-2-linux-x86_64.zip
  unzip protoc-3.0.0-beta-2-linux-x86_64.zip
  cd ..
}
genfiles_script() {
  protoc --version || true

  # Avoid regenerating descriptor.pb, since its output can vary based on the
  # version of protoc.
  touch upb/descriptor/descriptor.pb

  make -j12 genfiles USER_CPPFLAGS="$USER_CPPFLAGS `pkg-config lua5.2 --cflags`"
  # Will fail if any differences were observed.
  git diff --exit-code
}

# Tests the ndebug build.
ndebug_install() {
  sudo apt-get update -qq
  sudo apt-get install lua5.2 liblua5.2-dev libprotobuf-dev
  install_protoc
}
ndebug_script() {
  # Override of USER_CPPFLAGS removes -UNDEBUG.
  export USER_CPPFLAGS="`pkg-config lua5.2 --cflags` -g -fomit-frame-pointer"
  make -j12 tests testlua WITH_JIT=yes
  make test
}

# Tests the amalgamated build (this ensures that the different .c files
# don't have symbols or macros that conflict with each other.
amalgamated_install() {
  :
}
amalgamated_script() {
  # Override of USER_CPPFLAGS removes -UNDEBUG.
  export USER_CPPFLAGS="-UNDEBUG"
  make amalgamated
}

# A run that executes with coverage support and uploads to coveralls.io
coverage_install() {
  sudo apt-get update -qq
  sudo apt-get install libprotobuf-dev lua5.2 liblua5.2-dev
  install_protoc
  sudo pip install cpp-coveralls
}
coverage_script() {
  export USER_CPPFLAGS="--coverage -O0 `pkg-config lua5.2 --cflags`"
  make -j12 tests testlua WITH_JIT=yes
  make test
}
coverage_after_success() {
  coveralls --exclude dynasm --exclude tests --exclude upb/bindings/linux --gcov-options '\-lp'
}

set -e
set -x

if [ "$1" == "local" ]; then
  run_config() {
    make clean
    echo
    echo "travis.sh: TESTING CONFIGURATION $1 ==============================="
    echo
    UPB_TRAVIS_BUILD=$1 ./travis.sh script
  }
  # Run all configurations serially locally to test before pushing a pull
  # request.
  export CC=gcc
  export CXX=g++
  run_config "bare"
  run_config "barejit"
  run_config "core32"
  run_config "lua"
  run_config "ndebug"
  run_config "genfiles"
  run_config "amalgamated"
  exit
fi

$CC --version
$CXX --version

# Uncomment to enable uploading failure logs to S3.
# UPLOAD_TO_S3=true

if [ "$1" == "after_failure" ] && [ "$UPLOAD_TO_S3" == "true" ]; then
  # Upload failing tree to S3.
  curl -sL https://raw.githubusercontent.com/travis-ci/artifacts/master/install | bash
  PATH="$PATH:$HOME/bin"
  export ARTIFACTS_BUCKET=haberman-upb-travis-artifacts2
  ARCHIVE=failing-artifacts.tar.gz
  tar zcvf $ARCHIVE $(git ls-files -o)
  artifacts upload $ARCHIVE
  exit
fi

if [ "$1" == "after_success" ] && [ "$UPB_TRAVIS_BUILD" != "coverage" ]; then
  # after_success is only used for coverage.
  exit
fi

if [ "$CC" != "gcc" ] && [ "$UPB_TRAVIS_BUILD" == "coverage" ]; then
  # coverage build only works for GCC.
  exit
fi

# Enable asserts and ref debugging (though some configurations override this).
export USER_CPPFLAGS="-UNDEBUG -DUPB_DEBUG_REFS -DUPB_THREAD_UNSAFE -DUPB_DEBUG_TABLE -g"

if [ "$CC" == "gcc" ]; then
  # For the GCC build test loading JIT code via SO.  For the Clang build test
  # loading it in the normal way.
  export USER_CPPFLAGS="$USER_CPPFLAGS -DUPB_JIT_LOAD_SO"
fi

# TODO(haberman): Test UPB_DUMP_BYTECODE?  We don't right now because it is so
# noisy.

# Enable verbose build.
export Q=

# Make any compiler warning fail the build.
export UPB_FAIL_WARNINGS=true

eval ${UPB_TRAVIS_BUILD}_${1}
