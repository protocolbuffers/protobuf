#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "continuous" and "presubmit" jobs.

set -ex

# Change to repo root
cd $(dirname $0)/../../..

# Upgrade to a supported gcc version
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get -y update && \
  sudo apt-get install -y \
    gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100 --slave /usr/bin/g++ g++ /usr/bin/g++-7
sudo update-alternatives --set gcc /usr/bin/gcc-7

# Initialize any submodules and regenerate files.
git submodule update --init --recursive
use_bazel.sh 5.1.1
./regenerate_stale_files.sh

kokoro/linux/aarch64/qemu_helpers/prepare_qemu.sh

kokoro/linux/aarch64/test_php_aarch64.sh
