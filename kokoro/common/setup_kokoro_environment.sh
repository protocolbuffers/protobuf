#!/bin/bash

set -eux

# Upgrade to a supported gcc version
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get -y update && \
  sudo apt-get install --no-install-recommends  -y --fix-missing --option Acquire::Retries=10 --option Acquire::http::Timeout="1800" \
    gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100 --slave /usr/bin/g++ g++ /usr/bin/g++-7
sudo update-alternatives --set gcc /usr/bin/gcc-7

# TODO(b/265232445) Remove this once the patch is upstreamed
git submodule update --init --recursive
sed -i 's:random/mock_helpers.h:random/internal/mock_helpers.h:' third_party/abseil-cpp/CMake/AbseilDll.cmake
sed -i 's:ABSL_INTERNA_TEST_DLL_TARGETS:ABSL_INTERNAL_TEST_DLL_TARGETS:' third_party/abseil-cpp/CMake/AbseilDll.cmake
