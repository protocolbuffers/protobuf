#!/bin/bash

# This script will generate the common descriptors needed by the Objective C
# runtime.

# HINT:  Flags passed to generate_descriptor_proto.sh will be passed directly
#   to make when building protoc.  This is particularly useful for passing
#   -j4 to run 4 jobs simultaneously.

set -eu

readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")
readonly ProtoRootDir="${ScriptDir}/../.."
readonly ProtoC="${ProtoRootDir}/src/protoc"

pushd "${ProtoRootDir}" > /dev/null

# Compiler build fails if config.h hasn't been made yet (even if configure/etc.
# have been run, so get that made first).
make $@ config.h

# Make sure the compiler is current.
cd src
make $@ protoc

# These really should only be run when the inputs or compiler are newer than
# the outputs.

# Needed by the runtime.
./protoc --objc_out="${ProtoRootDir}/objectivec" google/protobuf/descriptor.proto

# Well known types that the library provides helpers for.
./protoc --objc_out="${ProtoRootDir}/objectivec" google/protobuf/timestamp.proto
./protoc --objc_out="${ProtoRootDir}/objectivec" google/protobuf/duration.proto

popd > /dev/null
