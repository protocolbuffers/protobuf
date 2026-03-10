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
# The objc unittest specific proto files.

OBJC_TEST_PROTO_FILES=(
  objectivec/Tests/any_test.proto
  objectivec/Tests/map_proto2_unittest.proto
  objectivec/Tests/map_unittest.proto
  objectivec/Tests/unittest_cycle.proto
  objectivec/Tests/unittest_deprecated_file.proto
  objectivec/Tests/unittest_deprecated.proto
  objectivec/Tests/unittest_extension_chain_a.proto
  objectivec/Tests/unittest_extension_chain_b.proto
  objectivec/Tests/unittest_extension_chain_c.proto
  objectivec/Tests/unittest_extension_chain_d.proto
  objectivec/Tests/unittest_extension_chain_e.proto
  objectivec/Tests/unittest_extension_chain_f.proto
  objectivec/Tests/unittest_extension_chain_g.proto
  objectivec/Tests/unittest_import_public.proto
  objectivec/Tests/unittest_import.proto
  objectivec/Tests/unittest_mset.proto
  objectivec/Tests/unittest_objc_options.proto
  objectivec/Tests/unittest_objc_startup.proto
  objectivec/Tests/unittest_objc.proto
  objectivec/Tests/unittest_preserve_unknown_enum.proto
  objectivec/Tests/unittest_runtime_proto2.proto
  objectivec/Tests/unittest_runtime_proto3.proto
  objectivec/Tests/unittest.proto
)

OBJC_EXTENSIONS=( .pbobjc.h .pbobjc.m )

# -----------------------------------------------------------------------------
# Ensure the output dir exists
mkdir -p "${OUTPUT_DIR}"

# -----------------------------------------------------------------------------
# Move to the top of the protobuf directories and ensure there is a protoc
# binary to use.
cd "${SRCROOT}/.."
readonly PROTOC="bazel-bin/protoc"
[[ -x "${PROTOC}" ]] || \
  die "Could not find the protoc binary; make sure you have built it (objectivec/DevTools/full_mac_build.sh -h)."

# -----------------------------------------------------------------------------
RUN_PROTOC=no

# Check to if all the output files exist (in case a new one got added).

for PROTO_FILE in "${OBJC_TEST_PROTO_FILES[@]}"; do
  DIR=${PROTO_FILE%/*}
  BASE_NAME=${PROTO_FILE##*/}
  # Drop the extension
  BASE_NAME=${BASE_NAME%.*}
  OBJC_NAME=$(echo "${BASE_NAME}" | awk -F _ '{for(i=1; i<=NF; i++) printf "%s", toupper(substr($i,1,1)) substr($i,2);}')

  for EXT in "${OBJC_EXTENSIONS[@]}"; do
    if [[ ! -f "${OUTPUT_DIR}/${DIR}/${OBJC_NAME}${EXT}" ]]; then
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
     objectivec/Tests/*.proto "${PROTOC}" \
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
# Generate the Objective C specific testing protos.

"${PROTOC}"                                                                 \
  --objc_out="${OUTPUT_DIR}"                                                \
  --objc_opt=expected_prefixes_path=objectivec/Tests/expected_prefixes.txt  \
  --objc_opt=prefixes_must_be_registered=yes                                \
  --objc_opt=require_prefixes=yes                                           \
  --proto_path=.                                                            \
  --proto_path=src                                                          \
  "${OBJC_TEST_PROTO_FILES[@]}"
