#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "continuous" and "presubmit" jobs.

set -ex

# Change to repo root
cd $(dirname $0)/../../..

kokoro/linux/aarch64/qemu_helpers/prepare_qemu.sh

kokoro/linux/aarch64/test_python_aarch64.sh
