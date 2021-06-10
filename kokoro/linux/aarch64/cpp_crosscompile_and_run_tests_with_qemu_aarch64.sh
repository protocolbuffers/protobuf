#!/bin/bash
#
# Builds protobuf C++ with aarch64 crosscompiler and runs a basic set of tests under an emulator.
# NOTE: This script is expected to run under the dockcross/linux-arm64 docker image.

set -ex

mkdir -p cmake/crossbuild_aarch64
cd cmake/crossbuild_aarch64

# the build commands are expected to run under dockcross docker image
# where the CC, CXX and other toolchain variables already point to the crosscompiler
cmake ..
make -j8

# check that the resulting test binary is indeed an aarch64 ELF
(file ./tests | grep -q "ELF 64-bit LSB executable, ARM aarch64") || (echo "Test binary in not an aarch64 binary"; exit 1)

# run the basic set of C++ tests under QEMU
# there are other tests we could run (e.g. ./lite-test), but this is sufficient as a smoketest
qemu-aarch64 ./tests
