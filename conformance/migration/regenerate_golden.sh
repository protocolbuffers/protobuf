#!/bin/bash
# Copyright (c) 2026, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Regenerates conformance_requests.golden and
# conformance_requests_default_edition.golden in the user's workspace:
#
#   blaze run //third_party/protobuf/conformance/migration:regenerate_golden
#
# Only do this when a change to the recorded requests is intentional; see
# README.md.

set -euo pipefail

if [[ -z "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
  echo "ERROR: this script must be executed via 'blaze run'." >&2
  exit 1
fi

if [[ -d "$0.runfiles/google3" ]]; then
  readonly RUNFILES="$0.runfiles/google3"
elif [[ -n "${RUNFILES_DIR:-}" && -d "${RUNFILES_DIR}/google3" ]]; then
  readonly RUNFILES="${RUNFILES_DIR}/google3"
else
  echo "ERROR: cannot locate the runfiles directory for $0." >&2
  exit 1
fi
readonly PKG="${RUNFILES}/third_party/protobuf/conformance/migration"
readonly GOLDEN_DIR="${BUILD_WORKSPACE_DIRECTORY}/third_party/protobuf/conformance/migration"
readonly GOLDEN="${GOLDEN_DIR}/conformance_requests.golden"
readonly DEFAULT_EDITION_GOLDEN="${GOLDEN_DIR}/conformance_requests_default_edition.golden"

source "${PKG}/record_requests_lib.sh"

SCRATCH_DIR="$(mktemp -d)"
readonly SCRATCH_DIR
trap 'rm -rf "${SCRATCH_DIR}"' EXIT

readonly NEW_GOLDEN="${SCRATCH_DIR}/conformance_requests.golden"
readonly NEW_DEFAULT_EDITION_GOLDEN="${SCRATCH_DIR}/conformance_requests_default_edition.golden"
record_conformance_requests "${RUNFILES}" "${SCRATCH_DIR}" "${NEW_GOLDEN}" \
  "${NEW_DEFAULT_EDITION_GOLDEN}"

#######################################
# Replaces a checked-in golden with a freshly recorded one, if it changed.
# Arguments:
#   golden: path to the checked-in golden in the user's workspace.
#   new_golden: path to the freshly recorded golden.
# Outputs:
#   Whether the golden was rewritten or unchanged, to STDOUT.
#######################################
install_golden() {
  local -r golden="$1"
  local -r new_golden="$2"
  if [[ -f "${golden}" ]] && cmp -s "${golden}" "${new_golden}"; then
    echo "Golden is unchanged: ${golden}"
  else
    cp -f "${new_golden}" "${golden}"
    echo "Wrote ${golden}"
  fi
  echo "  ($(wc -l < "${golden}") lines)"
}

install_golden "${GOLDEN}" "${NEW_GOLDEN}"
echo "  (${RECORDED_NORMAL_LINES} normal + ${RECORDED_PERF_LINES} performance" \
  "requests with --maximum_edition 2023)"
install_golden "${DEFAULT_EDITION_GOLDEN}" "${NEW_DEFAULT_EDITION_GOLDEN}"
echo "  (${RECORDED_DEFAULT_EDITION_NORMAL_LINES} normal +" \
  "${RECORDED_DEFAULT_EDITION_PERF_LINES} performance requests with the" \
  "runner's default edition)"
