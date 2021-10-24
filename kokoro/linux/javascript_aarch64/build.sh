#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "continuous" and "presubmit" jobs.

set -ex

# Change to repo root
cd $(dirname $0)/../../..

# Initialize any submodules.
git submodule update --init --recursive

kokoro/linux/aarch64/qemu_helpers/prepare_qemu.sh

kokoro/linux/aarch64/test_javascript_aarch64.sh
