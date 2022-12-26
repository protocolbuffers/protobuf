#!/bin/bash

set -ex

# go to the repo root
cd $(dirname $0)/../../../..
GIT_REPO_ROOT=`pwd`

if [[ -t 0 ]]; then
  DOCKER_TTY_ARGS="-it"
else
  # The input device on kokoro is not a TTY, so -it does not work.
  DOCKER_TTY_ARGS=
fi

# Pin the dockcross image since newer versions of the image break the build
# We use an older version of dockcross image that has gcc4.9.4 because it was built
# before https://github.com/dockcross/dockcross/pull/449
# Thanks to that, wheel build with this image aren't actually
# compliant with manylinux2014, but only with manylinux_2_24
PINNED_DOCKCROSS_IMAGE_VERSION=quay.io/pypa/manylinux2014_aarch64:2022-12-10-a8f854a

# running dockcross image without any arguments generates a wrapper
# scripts that can be used to run commands under the dockcross image
# easily.
# See https://github.com/dockcross/dockcross#usage for details
docker run -v $GIT_REPO_ROOT:/workspace --rm $PINNED_DOCKCROSS_IMAGE_VERSION /bin/bash -c "cd /workspace; git config --global --add safe.directory '*'; $@"
