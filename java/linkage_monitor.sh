#!/bin/bash

set -e

cd java

echo "list of files here:"

ls -alt

echo "list of files in core:"

ls -alt core/

echo "Maven command: $(which mvn)"

protoc_location="${RUNFILES_DIR}/com_google_protobuf/protoc"
if [ ! -x "${protoc_location}" ]; then
  echo "${protoc_location} is not found or not executable"
  exit 1
fi

mvn --projects "bom,core,util" -e -B -Dhttps.protocols=TLSv1.2 install \
    -Dmaven.test.skip=true \
    -Dprotobuf.basedir="../" \
    -Dprotoc="${protoc_location}"

curl -v -O "https://storage.googleapis.com/cloud-opensource-java-linkage-monitor/linkage-monitor-latest-all-deps.jar"

java -Xmx2048m -jar linkage-monitor-latest-all-deps.jar com.google.cloud:first-party-dependencies
