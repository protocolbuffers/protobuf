#!/bin/bash -eu
# Invoked by the Xcode projects to build the protos needed for the unittests.

readonly OUTPUT_DIR="${PROJECT_DERIVED_FILE_DIR}/protos"

# -----------------------------------------------------------------------------
# Helper for bailing.
die() {
  echo "Error: $1"
  exit 2
}

# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# Reusing a bunch of the protos from the protocolbuffers/protobuf tree, this
# can include some extras as there is no harm in ensuring work for C++
# generation.

CORE_PROTO_FILES=(
  src/google/protobuf/any_test.proto
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
  # The unittest_custom_options.proto extends the messages in descriptor.proto
  # so we build it in to test extending in general. The library doesn't provide
  # a descriptor as it doesn't use the classes/enums.
  src/google/protobuf/descriptor.proto
)

# -----------------------------------------------------------------------------
# The objc unittest specific proto files.

OBJC_TEST_PROTO_FILES=(
  objectivec/Tests/unittest_cycle.proto
  objectivec/Tests/unittest_deprecated.proto
  objectivec/Tests/unittest_deprecated_file.proto
  objectivec/Tests/unittest_extension_chain_a.proto
  objectivec/Tests/unittest_extension_chain_b.proto
  objectivec/Tests/unittest_extension_chain_c.proto
  objectivec/Tests/unittest_extension_chain_d.proto
  objectivec/Tests/unittest_extension_chain_e.proto
  objectivec/Tests/unittest_extension_chain_f.proto
  objectivec/Tests/unittest_extension_chain_g.proto
  objectivec/Tests/unittest_objc.proto
  objectivec/Tests/unittest_objc_startup.proto
  objectivec/Tests/unittest_objc_options.proto
  objectivec/Tests/unittest_runtime_proto2.proto
  objectivec/Tests/unittest_runtime_proto3.proto
)

OBJC_EXTENSIONS=( .pbobjc.h .pbobjc.m )

# -----------------------------------------------------------------------------
# Ensure the output dir exists
mkdir -p "${OUTPUT_DIR}/google/protobuf"

# -----------------------------------------------------------------------------
# Move to the top of the protobuf directories and ensure there is a protoc
# binary to use.
cd "${SRCROOT}/.."
[[ -x src/protoc ]] || \
  die "Could not find the protoc binary; make sure you have built it (objectivec/DevTools/full_mac_build.sh -h)."

# -----------------------------------------------------------------------------
RUN_PROTOC=no

# Check to if all the output files exist (in case a new one got added).

for PROTO_FILE in "${CORE_PROTO_FILES[@]}" "${OBJC_TEST_PROTO_FILES[@]}"; do
  DIR=${PROTO_FILE%/*}
  BASE_NAME=${PROTO_FILE##*/}
  # Drop the extension
  BASE_NAME=${BASE_NAME%.*}
  OBJC_NAME=$(echo "${BASE_NAME}" | awk -F _ '{for(i=1; i<=NF; i++) printf "%s", toupper(substr($i,1,1)) substr($i,2);}')

  for EXT in "${OBJC_EXTENSIONS[@]}"; do
    if [[ ! -f "${OUTPUT_DIR}/google/protobuf/${OBJC_NAME}${EXT}" ]]; then
      RUN_PROTOC=yes
    fi
  done
done

# If we haven't decided to run protoc because of a missing file, check to see if
# an input has changed.
if [[ "${RUN_PROTOC}" != "yes" ]] ; then
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
        -type f -name "*.pbobjc.[hm]" -print0 \
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

# -----------------------------------------------------------------------------
# Prune out all the files from previous generations to ensure we only have
# current ones.
find "${OUTPUT_DIR}" \
    -type f -name "*.pbobjc.[hm]" -print0 \
    | xargs -0 rm -rf

# -----------------------------------------------------------------------------
# Helper to invoke protoc
compile_protos() {
  src/protoc                                   \
    --objc_out="${OUTPUT_DIR}/google/protobuf" \
    --proto_path=src/google/protobuf/          \
    --proto_path=src                           \
    --experimental_allow_proto3_optional       \
    "$@"
}

# -----------------------------------------------------------------------------
# Generate most of the proto files that exist in the C++ src tree.

# Note: there is overlap in package.Message names between some of the test
# files, so they can't be generated all at once. This works because the overlap
# isn't linked into a single binary.
for a_proto in "${CORE_PROTO_FILES[@]}" ; do
  compile_protos "${a_proto}"
done

# -----------------------------------------------------------------------------
# Generate the Objective C specific testing protos.
compile_protos \
  --proto_path="objectivec/Tests" \
  "${OBJC_TEST_PROTO_FILES[@]}"
