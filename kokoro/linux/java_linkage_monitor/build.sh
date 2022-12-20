#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "pull request" project:
#
# This script selects a specific Dockerfile (for building a Docker image) and
# a script to run inside that image.

set -eux

use_bazel.sh 4.2.2

# Upgrade to a supported gcc version
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get -y update && \
  sudo apt-get install -y \
    gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100 --slave /usr/bin/g++ g++ /usr/bin/g++-7
sudo update-alternatives --set gcc /usr/bin/gcc-7

# Change to repo root
cd $(dirname $0)/../../..

bazel build //:protoc

# The java build setup expects protoc in the root directory.
cp bazel-bin/protoc .

cd java
# Installs the snapshot version locally
mvn -e -B -Dhttps.protocols=TLSv1.2 install -Dmaven.test.skip=true

# Linkage Monitor uses the snapshot versions installed in $HOME/.m2 to verify compatibility
JAR=linkage-monitor-latest-all-deps.jar
curl -v -O "https://storage.googleapis.com/cloud-opensource-java-linkage-monitor/${JAR}"
# Fails if there's new linkage errors compared with baseline
java -jar $JAR com.google.cloud:libraries-bom
