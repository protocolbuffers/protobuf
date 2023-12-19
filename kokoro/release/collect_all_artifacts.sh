#!/bin/bash

set -ex

# Change to repo root.
cd $(dirname $0)/../..

# Initialize any submodules.
git config --global --add safe.directory '*'
git submodule update --init --recursive

# The directory with all resulting artifacts
mkdir -p artifacts

# Artifacts from all predecessor jobs get copied to this directory by kokoro
INPUT_ARTIFACTS_DIR="${KOKORO_GFILE_DIR}/github/protobuf"

# TODO(jtattermusch): remove listing the files, but for now it make it easier
# to iterate on the script.
ls -R ${INPUT_ARTIFACTS_DIR}

# ====================================
# Copy to expose all the artifacts from the predecessor jobs to the output
# TODO(jtattermusch): the directory layout of the artifact builds is pretty messy,
# so will be the output artifacts of this job.
cp -r ${INPUT_ARTIFACTS_DIR}/* artifacts
