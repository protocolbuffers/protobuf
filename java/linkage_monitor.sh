#!/bin/bash

set -e -o pipefail

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

# Install the test BOM for Linkage Monitor
pushd test/linkage-monitor-check-bom
mvn -e -B install
popd

# Linkage Monitor requires the artifacts to be available in local Maven
# repository.
mvn -e -B clean generate-sources install  \
    -Dhttps.protocols=TLSv1.2 \
    -Dmaven.test.skip=true \
    -Dprotobuf.basedir="../.." \
    -Dprotoc="${protoc_location}"

echo "Installed the artifacts to local Maven repository"

curl -O "https://storage.googleapis.com/cloud-opensource-java-linkage-monitor/linkage-monitor-latest-all-deps.jar"

echo "Running linkage-monitor-latest-all-deps.jar."

# The libraries in the BOM would detect incompatible changes via Linkage Monitor
java -Xmx2048m -jar linkage-monitor-latest-all-deps.jar \
    com.google.protobuf.test:linkage-monitor-check-bom

echo "Finished running Linkage Monitor check"
