#!/bin/bash

# Invoked by the Xcode projects to build the protos needed for the unittests.

set -eu

readonly OUTPUT_DIR="${PROJECT_DERIVED_FILE_DIR}/protos"

# Helper for bailing.
die() {
  echo "Error: $1"
  exit 2
}

# What to do.
case "${ACTION}" in
  "")
    # Build, fall thru
    ;;
  "clean")
    rm -rf "${OUTPUT_DIR}"
    exit 0
    ;;
  *)
    die "Unknown action requested: ${ACTION}"
    ;;
esac

# Move to the top of the protobuf directories.
cd "${SRCROOT}/.."

[[ -x src/protoc ]] || \
  die "Could not find the protoc binary; make sure you have built it (objectivec/DevTools/full_mac_build.sh -h)."

RUN_PROTOC=no
if [[ ! -d "${OUTPUT_DIR}" ]] ; then
  RUN_PROTOC=yes
else
  # Find the newest input file (protos, compiler, and this script).
  # (these patterns catch some extra stuff, but better to over sample than
  # under)
  readonly NewestInput=$(find \
     src/google/protobuf/*.proto \
     objectivec/Tests/*.proto \
     src/.libs src/*.la src/protoc \
     objectivec/DevTools/compile_testing_protos.sh \
        -type f -print0 \
        | xargs -0 stat -f "%m %N" \
        | sort -n | tail -n1 | cut -f2- -d" ")
  # Find the oldest output file.
  readonly OldestOutput=$(find \
        "${OUTPUT_DIR}" \
        -type f -print0 \
        | xargs -0 stat -f "%m %N" \
        | sort -n -r | tail -n1 | cut -f2- -d" ")
  # If the newest input is newer than the oldest output, regenerate.
  if [[ "${NewestInput}" -nt "${OldestOutput}" ]] ; then
    RUN_PROTOC=yes
  fi
fi

if [[ "${RUN_PROTOC}" != "yes" ]] ; then
  # Up to date.
  exit 0
fi

# Ensure the output dir exists
mkdir -p "${OUTPUT_DIR}/google/protobuf"

CORE_PROTO_FILES=(
  src/google/protobuf/unittest_arena.proto
  src/google/protobuf/unittest_custom_options.proto
  src/google/protobuf/unittest_enormous_descriptor.proto
  src/google/protobuf/unittest_embed_optimize_for.proto
  src/google/protobuf/unittest_empty.proto
  src/google/protobuf/unittest_import.proto
  src/google/protobuf/unittest_import_lite.proto
  src/google/protobuf/unittest_lite.proto
  src/google/protobuf/unittest_mset.proto
  src/google/protobuf/unittest_mset_wire_format.proto
  src/google/protobuf/unittest_no_arena.proto
  src/google/protobuf/unittest_no_arena_import.proto
  src/google/protobuf/unittest_no_generic_services.proto
  src/google/protobuf/unittest_optimize_for.proto
  src/google/protobuf/unittest.proto
  src/google/protobuf/unittest_import_public.proto
  src/google/protobuf/unittest_import_public_lite.proto
  src/google/protobuf/unittest_drop_unknown_fields.proto
  src/google/protobuf/unittest_preserve_unknown_enum.proto
  src/google/protobuf/map_lite_unittest.proto
  src/google/protobuf/map_proto2_unittest.proto
  src/google/protobuf/map_unittest.proto
)

# The unittest_custom_options.proto extends the messages in descriptor.proto
# so we build it in to test extending in general. The library doesn't provide
# a descriptor as it doesn't use the classes/enums.
CORE_PROTO_FILES+=(
  src/google/protobuf/descriptor.proto
)

compile_proto() {
  src/protoc                                   \
    --objc_out="${OUTPUT_DIR}/google/protobuf" \
    --proto_path=src/google/protobuf/          \
    --proto_path=src                           \
    $*
}

for a_proto in "${CORE_PROTO_FILES[@]}" ; do
  compile_proto "${a_proto}"
done

OBJC_PROTO_FILES=(
  objectivec/Tests/unittest_cycle.proto
  objectivec/Tests/unittest_runtime_proto2.proto
  objectivec/Tests/unittest_runtime_proto3.proto
  objectivec/Tests/unittest_objc.proto
  objectivec/Tests/unittest_objc_startup.proto
)

for a_proto in "${OBJC_PROTO_FILES[@]}" ; do
  compile_proto --proto_path="objectivec/Tests" "${a_proto}"
done
