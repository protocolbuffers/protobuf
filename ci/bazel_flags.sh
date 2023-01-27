#!/bin/bash
#
# Helper to construct bazel flags for CI testing.


set -e

printUsage() {
  NAME=$(basename "${0}")
  cat << EOF
usage: ${NAME} [OPTIONS]

This script generates and outputs Bazel flags for CI testing.

OPTIONS:

 General:

   -h, --help
        Show this message
   --cache
        Specify which remote Bazel cache to use.
   --act
        Skip bazel cache for local act runs due to issue with credential
        files and nested docker images.

EOF
}

while [[ $# != 0 ]]; do
  case "${1}" in
    -h | --help )
      printUsage
      exit 0
      ;;
    --cache )
      shift
      CACHE_FOLDER=$1
      ;;
    --act )
      ACT="true"
      ;;
    -*)
      echo "ERROR: Unknown option: ${1}" 1>&2
      printUsage
      exit 1
      ;;
    *)
      echo "ERROR: Unknown argument: ${1}" 1>&2
      printUsage
      exit 1
      ;;
  esac
  shift
done

OUTPUT="--keep_going --test_output=errors --test_timeout=600"
if [ -z $ACT ]; then
  OUTPUT="${OUTPUT} \
    --google_credentials=/workspace/$(basename $GOOGLE_APPLICATION_CREDENTIALS)
    --remote_cache=https://storage.googleapis.com/protobuf-bazel-cache/protobuf/gha/${CACHE_FOLDER}\
    --remote_upload_local_results"
fi

echo $OUTPUT
