#!/bin/bash

set -e -o pipefail

echo "Running Linkage Monitor check"

echo "Maven command: $(which mvn)"
mvn --version

# This script runs within the Bazel's sandbox directory and uses protoc
# generated within the Bazel project.
protoc_location=$(realpath "${RUNFILES_DIR}/_main/protoc")
if [ ! -x "${protoc_location}" ]; then
  echo "${protoc_location} is not found or not executable"
  exit 1
fi

cd java


echo `pwd`
echo `ls -l core/core_mvn-pom.xml`
echo `ls -l core/lite_mvn-pom.xml`
echo `ls -l kotlin/kotlin_mvn-pom.xml`
echo `ls -l kotlin-lite/kotlin-lite_mvn-pom.xml`
echo `ls -l util/util_mvn-pom.xml`


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

curl -O "https://github.com/GoogleCloudPlatform/cloud-opensource-java/releases/download/dependencies-v1.5.15/linkage-monitor-latest-all-deps.jar"

echo "Running linkage-monitor-latest-all-deps.jar."

# The libraries in the BOM would detect incompatible changes via Linkage Monitor
java -Xmx2048m -jar linkage-monitor-latest-all-deps.jar \
    com.google.protobuf.test:linkage-monitor-check-bom

echo "Finished running Linkage Monitor check"
