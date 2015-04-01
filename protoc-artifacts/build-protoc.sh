#!/bin/bash

cd $(dirname "$0")
WORKING_DIR=$(pwd)

# Override the default value set in configure.ac that has '-g' which produces
# huge binary.
export CXXFLAGS="-DNDEBUG"

# Statically link libgcc and libstdc++.
# -s to produce stripped binary
export LDFLAGS="-static-libgcc -static-libstdc++ -s"

# Under Cygwin we use MinGW GCC because the executable produced by Cygwin GCC
# depends on Cygwin DLL.
if [[ "$(uname)" == CYGWIN* ]]; then
  export CC=i686-pc-mingw32-gcc
  export CXX=i686-pc-mingw32-c++
  export CXXCPP=i686-pc-mingw32-cpp
fi

cd "$WORKING_DIR"/.. && ./configure --disable-shared && make clean && make &&
  cd "$WORKING_DIR" && mkdir -p target &&
  (cp ../src/protoc target/protoc.exe || cp ../src/protoc.exe target/protoc.exe)
