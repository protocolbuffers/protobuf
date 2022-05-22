#!/bin/bash
#
# Builds protobuf python including the C++ extension with aarch64 crosscompiler.
# The outputs of this script are laid out so that we can later test them under an aarch64 emulator.
# NOTE: This script is expected to run under the dockcross/manylinux2014-aarch64 docker image.

set -ex

PYTHON="/opt/python/cp38-cp38/bin/python"

# Build protoc and libprotobuf
#ls /usr/bin
#ls /usr/lib
#ls /usr/local/bin
#ls /usr/local/lib
#ls /usr/include
#ls /usr/include/c++/*
#ls /usr/lib/clang/*
find /usr -name zlib
ls /opt/rh/devtoolset-9/root/usr/*
ls /opt/rh/devtoolset-9/root/usr/local/*
#bazel --bazelrc toolchain/toolchains.bazelrc build --config=linux-aarch_64 //:protoc  --sandbox_debug
#export PROTOC=bazel-bin/protoc
# Initialize any submodules.
#git submodule update --init --recursive

# the build commands are expected to run under dockcross docker image
# where the CC, CXX and other toolchain variables already point to the crosscompiler
gcc --version
g++ --version
cmake --version
which gcc
which g++
which cmake
mkdir -p cmake/crossbuild_aarch64
cd cmake/crossbuild_aarch64
echo $CC
echo $CXX
echo $LD_LIBRARY_PATH
#export PATH=/opt/rh/devtoolset-9/root/usr/bin:$PATH
#export CC=/opt/rh/devtoolset-9/root/usr/bin/gcc
#export CXX=/opt/rh/devtoolset-9/root/usr/bin/g++
#export LD_LIBRARY_PATH=/opt/rh/devtoolset-9/root/usr/lib
#exit
#cmake -Dprotobuf_WITH_ZLIB=0 -Dprotobuf_VERBOSE=1 -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DBUILD_SHARED_LIBS=OFF ..
#cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-static" -Dprotobuf_WITH_ZLIB=0 -Dprotobuf_VERBOSE=1 ..
#cmake -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/g++\
#        -DCMAKE_CXX_COMPILER_AR=/opt/rh/devtoolset-9/root/usr/bin/gcc-ar \
#        -DCMAKE_CXX_COMPILER_RANLIB=/opt/rh/devtoolset-9/root/usr/bin/gcc-ranlib \
#        -DCMAKE_C_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/gcc \
#        -DCMAKE_C_COMPILER_AR=/opt/rh/devtoolset-9/root/usr/bin/gcc-ar \
#        -DCMAKE_C_COMPILER_RANLIB=/opt/rh/devtoolset-9/root/usr/bin/gcc-ranlib \
#        -DCMAKE_CXX_COMPILER_WORKS=1 -Dprotobuf_WITH_ZLIB=0 -Dprotobuf_VERBOSE=1 ..
#cmake -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/g++\
#        -DCMAKE_C_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/gcc \
#        -DCMAKE_AR=/opt/rh/devtoolset-9/root/usr/bin/gcc-ar \
#        -DCMAKE_RANLIB=/opt/rh/devtoolset-9/root/usr/bin/gcc-ranlib \
#        -Dprotobuf_WITH_ZLIB=0 -Dprotobuf_VERBOSE=1 ..
#cmake -DCMAKE_CXX_COMPILER=/opt/rh/devtoolset-9/root/usr/bin/g++ -Dprotobuf_WITH_ZLIB=0 -Dprotobuf_VERBOSE=1 ..

ls /usr/xcc/aarch64-unknown-linux-gnueabi/*
ls /usr/xcc/aarch64-unknown-linux-gnueabi/aarch64-unknown-linux-gnueabi/*
ls /usr/xcc/aarch64-unknown-linux-gnueabi/aarch64-unknown-linux-gnueabi/sysroot/*
#export LD_LIBRARY_PATH=/usr/xcc/aarch64-unknown-linux-gnueabi/lib:/usr/xcc/aarch64-unknown-linux-gnueabi/aarch64-unknown-linux-gnueabi/sysroot/lib
#cmake -DCMAKE_CXX_COMPILER=/usr/xcc/aarch64-unknown-linux-gnueabi/bin/aarch64-unknown-linux-gnueabi-g++\
#        -DCMAKE_CXX_COMPILER_AR=/usr/xcc/aarch64-unknown-linux-gnueabi/bin/aarch64-unknown-linux-gnueabi-ar \
#        -DCMAKE_CXX_COMPILER_RANLIB=/usr/xcc/aarch64-unknown-linux-gnueabi/bin/aarch64-unknown-linux-gnueabi-ranlib \
#        -DCMAKE_C_COMPILER=/usr/xcc/aarch64-unknown-linux-gnueabi/bin/aarch64-unknown-linux-gnueabi-gcc \
#        -DCMAKE_C_COMPILER_AR=/usr/xcc/aarch64-unknown-linux-gnueabi/bin/aarch64-unknown-linux-gnueabi-ar \
#        -DCMAKE_C_COMPILER_RANLIB=/usr/xcc/aarch64-unknown-linux-gnueabi/bin/aarch64-unknown-linux-gnueabi-ranlib \
#        -DCMAKE_EXE_LINKER_FLAGS=-static
#cmake -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-lstdc++ -lgcc -lc" -Dprotobuf_WITH_ZLIB=0 -Dprotobuf_VERBOSE=1 ..
cmake -DCMAKE_EXE_LINKER_FLAGS=-static -Dprotobuf_WITH_ZLIB=0 -Dprotobuf_VERBOSE=1 ..

make VERBOSE=1 -j8
export PROTOC=cmake/protoc

# create a simple shell wrapper that runs crosscompiled protoc under qemu
echo '#!/bin/bash' >protoc_qemu_wrapper.sh
echo 'exec qemu-aarch64 "../src/protoc" "$@"' >>protoc_qemu_wrapper.sh
chmod ugo+x protoc_qemu_wrapper.sh

# PROTOC variable is by build_py step that runs under ./python directory
export PROTOC=../protoc_qemu_wrapper.sh

pushd python

# NOTE: this step will use protoc_qemu_wrapper.sh to generate protobuf files.
${PYTHON} setup.py build_py

# when crosscompiling for aarch64, --plat-name needs to be set explicitly
# to end up with correctly named wheel file
# the value should be manylinuxABC_ARCH and dockcross docker image
# conveniently provides the value in the AUDITWHEEL_PLAT env
plat_name_flag="--plat-name=$AUDITWHEEL_PLAT"

# override the value of EXT_SUFFIX to make sure the crosscompiled .so files in the wheel have the correct filename suffix
export PROTOCOL_BUFFERS_OVERRIDE_EXT_SUFFIX="$(${PYTHON} -c 'import sysconfig; print(sysconfig.get_config_var("EXT_SUFFIX").replace("-x86_64-linux-gnu.so", "-aarch64-linux-gnu.so"))')"

# Build the python extension inplace to be able to python unittests later
${PYTHON} setup.py build_ext --cpp_implementation --compile_static_extension --inplace

# Build the binary wheel (to check it with auditwheel)
${PYTHON} setup.py bdist_wheel --cpp_implementation --compile_static_extension $plat_name_flag
