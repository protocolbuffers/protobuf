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

# Install the test BOM for Linkage Monitor
pushd test/linkage-monitor-check-bom
mvn -e -B install
popd

runfiles_dir="$(realpath ${RUNFILES_DIR})"
genfiles_dir=`echo ${RUNFILES_DIR} | sed 's|/[^/]*$||'`

if [ ! -d "${runfiles_dir}/_main/java/lite" ]; then
  mkdir "${runfiles_dir}/_main/java/lite"
fi

cp -f "${genfiles_dir}"/core/core_mvn-pom.xml "${runfiles_dir}"/_main/java/core/pom.xml || true
cp -f "${genfiles_dir}"/core/lite_mvn-pom.xml "${runfiles_dir}"/_main/java/lite/pom.xml || true
cp -f "${genfiles_dir}"/util/util_mvn-pom.xml "${runfiles_dir}"/_main/java/util/pom.xml || true
cp -f "${genfiles_dir}"/kotlin/kotlin_mvn-pom.xml "${runfiles_dir}"/_main/java/kotlin/pom.xml || true
cp -f "${genfiles_dir}"/kotlin-lite/kotlin-lite_mvn-pom.xml "${runfiles_dir}"/_main/java/kotlin-lite/pom.xml || true

# Linkage Monitor requires the artifacts to be available in local Maven
# repository.
mvn -e -B clean generate-sources install  \
    -Dhttps.protocols=TLSv1.2 \
    -Dmaven.test.skip=true \
    -Dprotobuf.basedir="../.." \
    -Dprotoc="${protoc_location}"

echo "Installed the artifacts to local Maven repository"

curl -L -O "https://github.com/GoogleCloudPlatform/cloud-opensource-java/releases/download/dependencies-v1.5.15/linkage-monitor-latest-all-deps.jar"

echo "Running linkage-monitor-latest-all-deps.jar."

# The libraries in the BOM would detect incompatible changes via Linkage Monitor
java -Xmx2048m -jar linkage-monitor-latest-all-deps.jar \
    com.google.protobuf.test:linkage-monitor-check-bom

echo "Finished running Linkage Monitor check"
