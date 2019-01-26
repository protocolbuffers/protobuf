#!/bin/bash

set -ex
CXXFLAGS_COMMON="-std=c++14 -DNDEBUG -mmacosx-version-min=10.9"

cd github/protobuf
./autogen.sh

mkdir build32 && cd build32
export CXXFLAGS="$CXXFLAGS_COMMON -m32"
../configure --disable-shared
make -j4
file src/protoc
otool -L src/protoc | grep dylib
cd ..

mkdir build64 && cd build64
export CXXFLAGS="$CXXFLAGS_COMMON -m64"
../configure --disable-shared
make -j4
file src/protoc
otool -L src/protoc | grep dylib
cd ..
