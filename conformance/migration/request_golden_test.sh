#!/bin/bash
# Copyright (c) 2026, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Fails if the set of requests the conformance runner sends to a testee
# differs from conformance_requests.golden (--maximum_edition 2023) or
# conformance_requests_default_edition.golden (proto2/proto3 only).  See
# README.md.

set -euo pipefail

readonly RUNFILES="${TEST_SRCDIR}/${TEST_WORKSPACE}"
readonly PKG="${RUNFILES}/third_party/protobuf/conformance/migration"
readonly GOLDEN="${PKG}/conformance_requests.golden"
readonly DEFAULT_EDITION_GOLDEN="${PKG}/conformance_requests_default_edition.golden"
readonly ACTUAL="${TEST_TMPDIR}/conformance_requests.actual"
readonly DEFAULT_EDITION_ACTUAL="${TEST_TMPDIR}/conformance_requests_default_edition.actual"

source "${PKG}/record_requests_lib.sh"

record_conformance_requests "${RUNFILES}" "${TEST_TMPDIR}" "${ACTUAL}" \
  "${DEFAULT_EDITION_ACTUAL}"
echo "Recorded ${RECORDED_NORMAL_LINES} normal + ${RECORDED_PERF_LINES}" \
  "performance requests (--maximum_edition 2023) and" \
  "${RECORDED_DEFAULT_EDITION_NORMAL_LINES} normal +" \
  "${RECORDED_DEFAULT_EDITION_PERF_LINES} performance requests" \
  "(default edition)." >&2

# Keep the actual outputs around as Sponge artifacts for debugging.
if [[ -n "${TEST_UNDECLARED_OUTPUTS_DIR:-}" ]]; then
  cp "${ACTUAL}" "${DEFAULT_EDITION_ACTUAL}" "${TEST_UNDECLARED_OUTPUTS_DIR}/"
fi

#######################################
# Fails if any test name appears more than once in a recording.  The runner
# rejects duplicate test names within a suite, and the normal and --performance
# runs execute disjoint sets of tests, so every test name must be unique across
# suites and modes.  The recording is sorted, so duplicate names are adjacent.
# Arguments:
#   actual: path to a sorted recording.
# Outputs:
#   The duplicated test names to STDERR.
# Returns:
#   0 if all test names are unique; 1 otherwise.
#######################################
check_unique_test_names() {
  local -r actual="$1"
  local duplicates
  duplicates="$(cut -d' ' -f1 "${actual}" | uniq -d)"
  if [[ -n "${duplicates}" ]]; then
    echo "ERROR: duplicate test names recorded in ${actual}:" >&2
    echo "${duplicates}" >&2
    return 1
  fi
}

check_unique_test_names "${ACTUAL}"
check_unique_test_names "${DEFAULT_EDITION_ACTUAL}"

#######################################
# Diffs a golden against the matching recording.
# Arguments:
#   golden: path to the checked-in golden.
#   actual: path to the recording.
# Outputs:
#   A unified diff to STDOUT if they differ.
# Returns:
#   0 if identical; 1 otherwise.
#######################################
check_golden() {
  local -r golden="$1"
  local -r actual="$2"
  if diff -u "${golden}" "${actual}"; then
    return 0
  fi
  echo "FAIL: $(basename "${golden}") differs from the actual recording" >&2
  return 1
}

failed=0
check_golden "${GOLDEN}" "${ACTUAL}" || failed=1
check_golden "${DEFAULT_EDITION_GOLDEN}" "${DEFAULT_EDITION_ACTUAL}" || failed=1

if (( failed )); then
  cat >&2 <<'EOF'

FAIL: the requests sent by the conformance runner differ from the golden(s) in
third_party/protobuf/conformance/migration/ (see the diffs above; lines are
"<test_name> <request_size> <fnv1a64_of_canonical_request>").

These goldens pin the requests the conformance suites send
(go/modernizing-conformance-tests): a test that is moved or rewritten must
keep sending exactly the same request.  If it does, there is no diff; if you
see one, the change altered behaviour and should be fixed.

ONLY if the change is intentional (e.g. a test case was deliberately added,
removed, or its input changed), regenerate the goldens with:

  blaze run //third_party/protobuf/conformance/migration:regenerate_golden

and explain the change in your CL description.
EOF
  exit 1
fi

echo "PASS"
