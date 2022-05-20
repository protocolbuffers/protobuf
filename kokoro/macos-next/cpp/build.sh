#!/bin/bash -eux
#
# Build file to set up and run tests

# Run from the project root directory.
cd $(dirname $0)/../../..

_invocation_id=$(uuidgen | tr A-Z a-z)

bazel test \
  --bes_backend=${KOKORO_BES_BACKEND_ADDRESS:-buildeventservice.googleapis.com} \
  --bes_instance_name=${KOKORO_BES_PROJECT_ID} \
  --google_auth_scopes=https://www.googleapis.com/auth/cloud-source-tools \
  --google_default_credentials=true \
  --invocation_id=${_invocation_id} \
  --remote_cache=https://storage.googleapis.com/protobuf-bazel-cache \
  -- \
  //...
