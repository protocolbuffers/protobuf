#!/bin/bash

set -ex

# change to repo root
pushd $(dirname $0)/../../../..

# Create stage dir
ORIGINAL_DIR=`pwd`
pushd ..
cp -R $ORIGINAL_DIR stage
export STAGE_DIR="`pwd`/stage"
popd

export REPO_DIR=protobuf
export BUILD_VERSION=`grep -i "version" python/google/protobuf/__init__.py | grep -o "'.*'" | tr -d "'"`

export BUILD_COMMIT=`git rev-parse HEAD`
export PLAT=x86_64
export UNICODE_WIDTH=32
export MACOSX_DEPLOYMENT_TARGET=10.9

rm -rf artifacts/
rm -rf multibuild/
mkdir artifacts
export ARTIFACT_DIR=$(pwd)/artifacts

git clone https://github.com/matthew-brett/multibuild.git
cp kokoro/release/python/linux/config.sh config.sh

build_artifact_version() {
  MB_PYTHON_VERSION=$1
  cp -R $STAGE_DIR $REPO_DIR

  source multibuild/common_utils.sh
  source multibuild/travis_steps.sh
  before_install

  clean_code $REPO_DIR $BUILD_COMMIT

  build_wheel $REPO_DIR/python $PLAT

  mv wheelhouse/* $ARTIFACT_DIR

  # Clean up env
  rm -rf venv
  sudo rm -rf $REPO_DIR
}

build_crosscompiled_aarch64_artifact_version() {
  # crosscompilation is only supported with the dockcross manylinux2014 image
  DOCKER_IMAGE=dockcross/manylinux2014-aarch64:20210706-65bf2dd
  PLAT=aarch64

  # TODO(jtatermusch): currently when crosscompiling, "auditwheel repair" will be disabled
  # since auditwheel doesn't work for crosscomiled wheels.
  build_artifact_version $@
}

build_artifact_version 3.6
build_artifact_version 3.7
build_artifact_version 3.8
build_artifact_version 3.9
build_artifact_version 3.10

build_crosscompiled_aarch64_artifact_version 3.7
build_crosscompiled_aarch64_artifact_version 3.8
build_crosscompiled_aarch64_artifact_version 3.9
build_crosscompiled_aarch64_artifact_version 3.10
