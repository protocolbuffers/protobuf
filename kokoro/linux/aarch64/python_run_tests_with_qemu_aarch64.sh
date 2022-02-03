#!/bin/bash

set -ex

# go to the repo root
cd $(dirname $0)/../../..
cd python

PYTHON="/opt/python/cp38-cp38/bin/python"
${PYTHON} -m pip install --user pytest auditwheel

# check that we are really using aarch64 python
(${PYTHON} -c 'import sysconfig; print(sysconfig.get_platform())' | grep -q "linux-aarch64") || (echo "Wrong python platform, needs to be aarch64 python."; exit 1)

# step 1: run all python unittests
# we've built the python extension previously with --inplace option
# so we can just discover all the unittests and run them directly under 
# the python/ directory.
LD_LIBRARY_PATH=../src/.libs PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp ${PYTHON} -m pytest google/protobuf

# step 2: run auditwheel show to check that the wheel is manylinux2014 compatible.
# auditwheel needs to run on wheel's target platform (or under an emulator)
${PYTHON} -m auditwheel show dist/protobuf-*-manylinux2014_aarch64.whl

# step 3: smoketest that the wheel can be installed and run a smokecheck
${PYTHON} -m pip install dist/protobuf-*-manylinux2014_aarch64.whl
# when python cpp extension is on, simply importing a message type will trigger loading the cpp extension
PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp ${PYTHON} -c 'import google.protobuf.timestamp_pb2; print("Successfully loaded the python cpp extension!")'
