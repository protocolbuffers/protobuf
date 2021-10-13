#!/bin/bash
#
# Builds protobuf python including the C++ extension with aarch64 crosscompiler.
# The outputs of this script are laid out so that we can later test them under an aarch64 emulator.
# NOTE: This script is expected to run under the dockcross/manylinux2014-aarch64 docker image.

set -ex

PYTHON="/opt/python/cp38-cp38/bin/python"

./autogen.sh
CXXFLAGS="-fPIC -g -O2" ./configure --host=aarch64
make -j8

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
