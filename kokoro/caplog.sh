# Log capturing for the Kokoro runtime environment.
#
# This script should be `source`d from Kokoro build scripts to configure log
# capturing when running under Kokoro.
#
# When not running under Kokoro, no logs will be collected. If you want to run
# locally and collect logs anyway, set the KOKORO_ARTIFACTS_DIR environment
# variable to a directory where the logs should go.
#
# The job `.cfg` file needs the following stanzas to declare the captured logs
# as outputs (yes, these are globs, not regexes):
#
#   action: {
#     define_artifacts: {
#       regex: "**/*sponge_log.log"
#       regex: "**/*sponge_log.xml"
#     }
#   }
#
# Use the provided functions below as build/test fixtures, e.g.:
#
#   source kokoro/capture_logs.sh
#   caplog build/step1 <build command>
#   caplog tests/step2 <test command>
#
# If log capturing is enabled, this script will set some variables that can be
# used if necessary:
#
#   CAPLOG_DIR         is used for logs
#   CAPLOG_CMAKE_ARGS  contains extra cmake args to enable test XML output
#   CAPLOG_CTEST_ARGS  contains extra ctest args to capture combined test logs
#
# For example:
#
#   if [[ -v CAPLOG_DIR_BUILD ]]; then
#     cp extra_diagnostics.log "${CAPLOG_DIR_BUILD}/diagnostics.log"
#   fi
#
#   # Use ${...:-} form under `set -u`:
#   caplog build/01_configure cmake -G Ninja ${CAPLOG_CMAKE_ARGS:-}
#   caplog build/02_build     cmake --build
#   caplog test/03_test       ctest ${CAPLOG_CTEST_ARGS:-}

if [[ -z ${KOKORO_ARTIFACTS_DIR:-} ]]; then
  function caplog() { shift; "$@"; }
else

  CAPLOG_DIR="$(mktemp -d)"
  CAPLOG_CMAKE_ARGS="-Dprotobuf_TEST_XML_OUTDIR=${CAPLOG_DIR}/tests/"
  CAPLOG_CTEST_ARGS="--verbose"

  # Captures the stdout/stderr of a command to a named log file.
  # Usage: caplog NAME COMMAND [ARGS...]
  function caplog() {
    _name="${CAPLOG_DIR}/${1%.log}.log"; shift
    mkdir -p "${_name%/*}"
    date
    time ( "$@" 2>&1 | tee "${_name}" )
    if [[ $? != 0 ]] ; then
      cat "${_name}"
      return 1
    fi
  }

  # Trap handler: renames logs on script exit so they will be found by Kokoro.
  function _caplog_onexit() {
    _rc=$?
    set +x
    echo "Collecting logs [${BASH_SOURCE}]"

    find "${CAPLOG_DIR}" -type f -name '*.log' \
      | while read _textlog; do
      # Ensure an XML file exists for each .log file.
      touch ${_textlog%.log}.xml
    done

    find "${CAPLOG_DIR}" -type f \( -name '*.xml' -or -name '*.log' \) \
      | while read _src; do
      # Move to artifacts dir, preserving the path relative to CAPLOG_DIR.
      # The filename changes from foo/bar.log to foo/bar/sponge_log.log.
      _logfile=${_src/${CAPLOG_DIR}\//}
      _stem=${KOKORO_ARTIFACTS_DIR}/${_logfile%.*}
      _ext=${_logfile##*.}
      mkdir -p ${_stem}
      mv -v "${_src}" "${_stem}/sponge_log.${_ext}"
    done
    rm -rv "${CAPLOG_DIR}"
    exit ${_rc}
  }
  trap _caplog_onexit EXIT

fi
