#!/bin/bash
#
# Crosscompiles protobuf C++ under dockcross docker image and runs the tests under an emulator.

set -e

# go to the repo root
cd $(dirname $0)/../../..

# Initialize any submodules.
git submodule update --init --recursive

# run the C++ build and test script under dockcross/linux-arm64 image
kokoro/linux/aarch64/dockcross_helpers/run_dockcross_linux_aarch64.sh kokoro/linux/aarch64/cpp_crosscompile_and_run_tests_with_qemu_aarch64.sh
