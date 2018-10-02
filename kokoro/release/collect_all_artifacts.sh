#!/bin/bash

set -ex

# Change to repo root.
cd $(dirname $0)/../..

# Initialize any submodules.
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

# ====================================
# Build Google.Protobuf.Tools C# nuget
# The reason it's being done in this script is that we need access to protoc binaries
# built on multiple platform (the build is performed by the "build artifact" step)
# and adding and extra chained build just for building the Google.Protobuf.Tools
# nuget seems like an overkill.
cd csharp
mkdir -p protoc/windows_x86
mkdir -p protoc/windows_x64
cp ${INPUT_ARTIFACTS_DIR}/build32/Release/protoc.exe protoc/windows_x86/protoc.exe
cp ${INPUT_ARTIFACTS_DIR}/build64/Release/protoc.exe protoc/windows_x64/protoc.exe

mkdir -p protoc/linux_x86
mkdir -p protoc/linux_x64
# Because of maven unrelated reasonse the linux protoc binaries have a dummy .exe extension.
# For the Google.Protobuf.Tools nuget, we don't want that expection, so we just remove it.
cp ${INPUT_ARTIFACTS_DIR}/protoc-artifacts/target/linux/x86_32/protoc.exe protoc/linux_x86/protoc
cp ${INPUT_ARTIFACTS_DIR}/protoc-artifacts/target/linux/x86_64/protoc.exe protoc/linux_x64/protoc

mkdir -p protoc/macosx_x86
mkdir -p protoc/macosx_x64
cp ${INPUT_ARTIFACTS_DIR}/build32/src/protoc protoc/macosx_x86/protoc
cp ${INPUT_ARTIFACTS_DIR}/build64/src/protoc protoc/macosx_x64/protoc

# Install nuget (will also install  mono)
# TODO(jtattermusch): use "mono:5.14" docker image instead so we don't have to apt-get install
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
sudo apt install apt-transport-https
echo "deb https://download.mono-project.com/repo/ubuntu stable-trusty main" | sudo tee /etc/apt/sources.list.d/mono-official-stable.list
sudo apt update
sudo apt-get install -y nuget

nuget pack Google.Protobuf.Tools.nuspec

# Copy the nupkg to the output artifacts
cp Google.Protobuf.Tools.*.nupkg ../artifacts
