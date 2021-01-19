#!/bin/bash

# Print usage and fail.
function usage() {
  echo "Usage: protobuf_optimized_pip.sh PROTOBUF_VERSION PYPI_USERNAME PYPI_PASSWORD" >&2
  exit 1   # Causes caller to exit because we use -e.
}

# Validate arguments.
if [ $0 != ./build_wheel_manylinux.sh ]; then
  echo "Please run this script from the directory in which it is located." >&2
  exit 1
fi

if [ $# -lt 3 ]; then
  usage
  exit 1
fi

ARCH=`uname -m`
PROTOBUF_VERSION=$1
PYPI_USERNAME=$2
PYPI_PASSWORD=$3

docker rmi protobuf-python-wheel
if [ "$ARCH" == "aarch64" ]; then
	docker build arm64/ -t protobuf-python-wheel
else
	docker build x86_64/ -t protobuf-python-wheel
fi
docker run --rm protobuf-python-wheel ./protobuf_optimized_pip.sh $PROTOBUF_VERSION $PYPI_USERNAME $PYPI_PASSWORD
docker rmi protobuf-python-wheel
