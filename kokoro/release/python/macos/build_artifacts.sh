#!/bin/bash

set -ex

# change to repo root
pushd $(dirname $0)/../../../..

export REPO_DIR=protobuf
export BUILD_VERSION=`grep -i "version" python/google/protobuf/__init__.py | grep -o "'.*'" | tr -d "'"`
export BUILD_COMMIT=v$BUILD_VERSION
export PLAT=x86_64
export UNICODE_WIDTH=32
export MACOSX_DEPLOYMENT_TARGET=10.9
export TRAVIS_OS_NAME="osx"

mkdir artifacts
export ARTIFACT_DIR=$(pwd)/artifacts

git clone https://github.com/matthew-brett/multibuild.git
cp kokoro/release/python/linux/config.sh config.sh

OLD_PATH=$PATH

build_artifact_version() {
  MB_PYTHON_VERSION=$1

  # Clean up env
  rm -rf venv
  sudo rm -rf protobuf
  git clone https://github.com/google/protobuf.git
  export PATH=$OLD_PATH

  source multibuild/common_utils.sh
  source multibuild/travis_steps.sh
  before_install

  clean_code $REPO_DIR $BUILD_COMMIT

  build_wheel $REPO_DIR/python $PLAT

  mv wheelhouse/* $ARTIFACT_DIR
}

build_artifact_version 2.7
build_artifact_version 3.4
build_artifact_version 3.5
build_artifact_version 3.6
build_artifact_version 3.7
