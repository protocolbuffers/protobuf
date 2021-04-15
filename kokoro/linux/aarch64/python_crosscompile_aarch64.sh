#!/bin/bash
#
# Builds protobuf C++ with aarch64 crosscompiler and runs a basic set of tests under an emulator.
# NOTE: This script is expected to run under the dockcross/linux-arm64 docker image.

set -ex

PYTHON="/opt/python/cp38-cp38/bin/python"

./autogen.sh
CXXFLAGS="-fPIC -g -O2" ./configure
make -j8

pushd python

# TODO: currently this step relies on qemu being registered with binfmt_misc so that
# aarch64 binaries are automatically run with an emulator. This works well once
# "sudo apt install qemu-user-static binfmt-support" is installed on the host machine.
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
