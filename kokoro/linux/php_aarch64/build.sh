#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "continuous" and "presubmit" jobs.

set -ex

# Change to repo root
cd $(dirname $0)/../../..

# Initialize any submodules and regenerate files.
git submodule update --init --recursive
use_bazel.sh 5.1.1
sudo ./kokoro/common/setup_kokoro_environment.sh
./regenerate_stale_files.sh

kokoro/linux/aarch64/qemu_helpers/prepare_qemu.sh

kokoro/linux/aarch64/test_php_aarch64.sh
