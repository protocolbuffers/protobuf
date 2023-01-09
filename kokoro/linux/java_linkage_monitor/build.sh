#!/bin/bash
#
# This is the top-level script we give to Kokoro as the entry point for
# running the "pull request" project:
#
# This script selects a specific Dockerfile (for building a Docker image) and
# a script to run inside that image.

set -eux

use_bazel.sh 4.2.2

# Change to repo root
cd $(dirname $0)/../../..

sudo ./kokoro/common/setup_kokoro_environment.sh
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
