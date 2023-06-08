#!/bin/bash

set -e

echo "Running Linkage Monitor check"

echo "Maven command: $(which mvn)"
mvn --version

# This script runs within the Bazel's sandbox directory and uses protoc
# generated within the Bazel project.
protoc_location=$(realpath "${RUNFILES_DIR}/com_google_protobuf/protoc")
if [ ! -x "${protoc_location}" ]; then
  echo "${protoc_location} is not found or not executable"
  exit 1
fi

cd java

# Linkage Monitor requires the artifacts to be available in local Maven
# repository.
mvn --projects "bom,core,util" -e -B -Dhttps.protocols=TLSv1.2 clean generate-sources install \
    -Dmaven.test.skip=true \
    -Dprotobuf.basedir="../.." \
    -Dprotoc="${protoc_location}"

echo "Installed the artifacts to local Maven repository"

curl -v -O "https://storage.googleapis.com/cloud-opensource-java-linkage-monitor/linkage-monitor-latest-all-deps.jar"

echo "Running linkage-monitor-latest-all-deps.jar."

# The generated libraries in google-cloud-shared-dependencies would detect
# incompatible changes via Linkage Monitor
# https://github.com/googleapis/sdk-platform-java/tree/main/java-shared-dependencies
java -Xmx2048m -jar linkage-monitor-latest-all-deps.jar com.google.cloud:google-cloud-shared-dependencies

echo "Finished running Linkage Monitor check"
