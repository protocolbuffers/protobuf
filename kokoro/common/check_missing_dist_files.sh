#!/bin/bash
#
# Script to compare a distribution archive for expected files based on git.
#
# Usage:
#   check_missing_dist_files.sh path/to/dist_archive.tar.gz

set -eux
set -o pipefail

# By default, look for a git repo based on this script's path.
: ${SOURCE_DIR:=$(cd $(dirname $0)/../.. ; pwd)}

# Use a temporary directory for intermediate files.
# Note that pipelines below use subshells to avoid multiple trap executions.
_workdir=$(mktemp -d)
function cleanup_workdir() { rm -r ${_workdir}; }
trap cleanup_workdir EXIT

# List all the files in the archive.
(
  tar -atf $1 | \
    cut -d/ -f2- | \
    sort
) > ${_workdir}/archive.lst

# List all files in the git repo that should be in the archive.
(
  git -C ${SOURCE_DIR} ls-files | \
  grep "^\(java\|python\|objectivec\|csharp\|ruby\|php\|cmake\|examples\|src/google/protobuf/.*\.proto\)" |\
  grep -v ".gitignore" | \
  grep -v "java/lite/proguard.pgcfg" | \
  grep -v "python/compatibility_tests" | \
  grep -v "python/docs" | \
  grep -v "python/.repo-metadata.json" | \
  grep -v "python/protobuf_distutils" | \
  grep -v "csharp/compatibility_tests" | \
  sort
) > ${_workdir}/expected.lst

# Check for missing files.
MISSING_FILES=( $(cd ${_workdir} && comm -13 archive.lst expected.lst) )
if (( ${#MISSING_FILES[@]} == 0 )); then
  exit 0
fi

(
  set +x
  echo -e "\n\nMissing files from archive:"
  for (( i=0 ; i < ${#MISSING_FILES[@]} ; i++ )); do
    echo "  ${MISSING_FILES[i]}"
  done
  echo -e "\nAdd them to the `pkg_files` rule in corresponding BUILD.bazel.\n"
) >&2
exit 1
