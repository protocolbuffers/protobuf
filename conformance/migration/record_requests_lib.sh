# shellcheck shell=bash
# Copyright (c) 2026, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Shared by request_golden_test.sh and regenerate_golden.sh so that the test
# and the regeneration script can never disagree about how requests are
# recorded.  Source this file; do not execute it.
#
# Both callers are expected to have `set -euo pipefail` in effect.

#######################################
# Records every request the conformance runner sends to the skipping
# testee (`--record_requests`).
#
# The runner is invoked four times, all with `--enforce_recommended` and empty
# failure lists:
#   1. `--maximum_edition 2023`, normal mode;
#   2. `--maximum_edition 2023`, `--performance`;
#   3. runner default edition (proto2/proto3 only), normal mode;
#   4. runner default edition (proto2/proto3 only), `--performance`.
# Recordings 1+2 are sorted together into `out_max_2023`; recordings 3+4 are
# sorted together into `out_default_edition`.  Sorting makes the goldens
# independent of test execution order.
#
# Globals:
#   RECORDED_NORMAL_LINES: set to the number of requests in recording 1.
#   RECORDED_PERF_LINES: set to the number of requests in recording 2.
#   RECORDED_DEFAULT_EDITION_NORMAL_LINES: set to the number of requests in
#     recording 3.
#   RECORDED_DEFAULT_EDITION_PERF_LINES: set to the number of requests in
#     recording 4.
# Arguments:
#   runfiles: google3 runfiles root (e.g. "${TEST_SRCDIR}/${TEST_WORKSPACE}").
#   workdir: scratch directory for intermediate files.
#   out_max_2023: output file for the `--maximum_edition 2023` recordings.
#   out_default_edition: output file for the default-edition recordings.
# Outputs:
#   Progress and error messages to STDERR.
# Returns:
#   0 on success; 1 if any runner invocation fails or records nothing.
#######################################
record_conformance_requests() {
  local -r runfiles="$1"
  local -r workdir="$2"
  local -r out_max_2023="$3"
  local -r out_default_edition="$4"

  local -r conformance_dir="${runfiles}/third_party/protobuf/conformance"
  local -r runner="${conformance_dir}/conformance_test_runner"
  local -r testee="${conformance_dir}/migration/skipping_testee"

  # Empty failure lists: with a testee that skips everything nothing can fail,
  # and we do not want any real implementation's failure list to influence
  # which requests get recorded.
  local -r empty_failure_list="${workdir}/empty_failure_list.txt"
  local -r empty_text_format_failure_list="${workdir}/empty_text_format_failure_list.txt"
  : > "${empty_failure_list}"
  : > "${empty_text_format_failure_list}"

  local -r normal="${workdir}/requests_normal.txt"
  local -r perf="${workdir}/requests_performance.txt"
  local -r default_normal="${workdir}/requests_default_edition_normal.txt"
  local -r default_perf="${workdir}/requests_default_edition_performance.txt"

  local -r common_args=(
    --enforce_recommended
    --failure_list "${empty_failure_list}"
    --text_format_failure_list "${empty_text_format_failure_list}"
    --output_dir "${workdir}"
  )
  local -r max_2023_args=(--maximum_edition 2023)

  # The runner exits 0 here (nothing fails, nothing is unexpectedly listed),
  # so a non-zero status means the recording itself is broken.
  echo "Recording requests (--maximum_edition 2023, normal mode)..." >&2
  "${runner}" "${common_args[@]}" "${max_2023_args[@]}" \
    --record_requests "${normal}" "${testee}" \
    || { echo "ERROR: conformance_test_runner failed (--maximum_edition 2023," \
           "normal mode)" >&2; return 1; }

  echo "Recording requests (--maximum_edition 2023, --performance mode)..." >&2
  "${runner}" "${common_args[@]}" "${max_2023_args[@]}" --performance \
    --record_requests "${perf}" "${testee}" \
    || { echo "ERROR: conformance_test_runner failed (--maximum_edition 2023," \
           "--performance mode)" >&2; return 1; }

  # Without --maximum_edition the runner only runs the proto2/proto3 tests;
  # these recordings pin the edition gating.
  echo "Recording requests (default edition, normal mode)..." >&2
  "${runner}" "${common_args[@]}" \
    --record_requests "${default_normal}" "${testee}" \
    || { echo "ERROR: conformance_test_runner failed (default edition," \
           "normal mode)" >&2; return 1; }

  echo "Recording requests (default edition, --performance mode)..." >&2
  "${runner}" "${common_args[@]}" --performance \
    --record_requests "${default_perf}" "${testee}" \
    || { echo "ERROR: conformance_test_runner failed (default edition," \
           "--performance mode)" >&2; return 1; }

  RECORDED_NORMAL_LINES="$(wc -l < "${normal}")"
  RECORDED_PERF_LINES="$(wc -l < "${perf}")"
  RECORDED_DEFAULT_EDITION_NORMAL_LINES="$(wc -l < "${default_normal}")"
  RECORDED_DEFAULT_EDITION_PERF_LINES="$(wc -l < "${default_perf}")"
  if (( RECORDED_NORMAL_LINES == 0 || RECORDED_PERF_LINES == 0
        || RECORDED_DEFAULT_EDITION_NORMAL_LINES == 0
        || RECORDED_DEFAULT_EDITION_PERF_LINES == 0 )); then
    echo "ERROR: no requests were recorded" \
      "(max_2023: normal=${RECORDED_NORMAL_LINES}," \
      "performance=${RECORDED_PERF_LINES};" \
      "default edition: normal=${RECORDED_DEFAULT_EDITION_NORMAL_LINES}," \
      "performance=${RECORDED_DEFAULT_EDITION_PERF_LINES})" >&2
    return 1
  fi

  LC_ALL=C sort "${normal}" "${perf}" > "${out_max_2023}"
  LC_ALL=C sort "${default_normal}" "${default_perf}" > "${out_default_edition}"
}
