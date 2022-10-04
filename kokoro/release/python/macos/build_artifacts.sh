#!/bin/bash

set -ex

# Remove any pre-existing protobuf installation.
brew uninstall -f protobuf

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
export TRAVIS_OS_NAME="osx"

rm -rf artifacts/
rm -rf multibuild/
mkdir artifacts
export ARTIFACT_DIR=$(pwd)/artifacts

git clone https://github.com/matthew-brett/multibuild.git
# Pin multibuild scripts at a known commit to avoid potentially unwanted future changes from
# silently creeping in (see https://github.com/protocolbuffers/protobuf/issues/9180).
# IMPORTANT: always pin multibuild at the same commit for:
# - linux/build_artifacts.sh
# - linux/build_artifacts.sh
# - windows/build_artifacts.bat
(cd multibuild; git checkout b89bb903e94308be79abefa4f436bf123ebb1313)
cp kokoro/release/python/macos/config.sh config.sh

OLD_PATH=$PATH

build_artifact_version() {
  MB_PYTHON_VERSION=$1

  # Clean up env
  rm -rf venv
  sudo rm -rf $REPO_DIR
  cp -R $STAGE_DIR $REPO_DIR
  export PATH=$OLD_PATH

  source multibuild/common_utils.sh
  source multibuild/travis_steps.sh
  before_install

  clean_code $REPO_DIR $BUILD_COMMIT

  build_wheel $REPO_DIR/python $PLAT

  mv wheelhouse/* $ARTIFACT_DIR
}

export MB_PYTHON_OSX_VER=10.9
build_artifact_version 3.7
build_artifact_version 3.8
build_artifact_version 3.9
build_artifact_version 3.10
