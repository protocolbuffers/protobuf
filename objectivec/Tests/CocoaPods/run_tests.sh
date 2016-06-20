#!/bin/bash
#
# Helper to run the pods tests.

set -eu

readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")

printUsage() {
  NAME=$(basename "${0}")
  cat << EOF
usage: ${NAME} [OPTIONS]

This script runs some test to check the CocoaPods integration.

OPTIONS:

 General:

   -h, --help
         Show this message
   --skip-static
         Skip the static based pods tests.
   --skip-framework
         Skip the framework based pods tests.
   --skip-ios
         Skip the iOS pods tests.
   --skip-osx
         Skip the OS X pods tests.

EOF
}

TEST_MODES=( "static" "framework" )
TEST_NAMES=( "iOSCocoaPodsTester" "OSXCocoaPodsTester" )
while [[ $# != 0 ]]; do
  case "${1}" in
    -h | --help )
      printUsage
      exit 0
      ;;
    --skip-static )
      TEST_MODES=(${TEST_MODES[@]/static})
      ;;
    --skip-framework )
      TEST_MODES=(${TEST_MODES[@]/framework})
      ;;
    --skip-ios )
      TEST_NAMES=(${TEST_NAMES[@]/iOSCocoaPodsTester})
      ;;
    --skip-osx )
      TEST_NAMES=(${TEST_NAMES[@]/OSXCocoaPodsTester})
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

# Sanity check.
if [[ "${#TEST_NAMES[@]}" == 0 ]] ; then
  echo "ERROR: Need to run at least iOS or OS X tests." 1>&2
  exit 2
fi
if [[ "${#TEST_MODES[@]}" == 0 ]] ; then
  echo "ERROR: Need to run at least static or frameworks tests." 1>&2
  exit 2
fi

header() {
  echo ""
  echo "========================================================================"
  echo "    ${@}"
  echo "========================================================================"
  echo ""
}

# Cleanup hook for do_test, assumes directory is correct.
cleanup() {
  local TEST_NAME="$1"

  echo "Cleaning up..."

  # Generally don't let things fail, and eat common stdout, but let stderr show
  # incase something does hiccup.
  xcodebuild -workspace "${TEST_NAME}.xcworkspace" -scheme "${TEST_NAME}" clean > /dev/null || true
  pod deintegrate > /dev/null || true
  # Flush the cache so nothing is left behind.
  pod cache clean --all || true
  # Delete the files left after pod deintegrate.
  rm -f Podfile.lock || true
  rm -rf "${TEST_NAME}.xcworkspace" || true
  git checkout -- "${TEST_NAME}.xcodeproj" || true
  # Remove the Podfile that was put in place.
  rm -f Podfile || true
}

do_test() {
  local TEST_NAME="$1"
  local TEST_MODE="$2"

  header "${TEST_NAME}" - Mode: "${TEST_MODE}"
  cd "${ScriptDir}/${TEST_NAME}"

  # Hook in cleanup for any failures.
  trap "cleanup ${TEST_NAME}" EXIT

  # Ensure nothing is cached by pods to start with that could throw things off.
  pod cache clean --all

  # Put the right Podfile in place.
  cp -f "Podfile-${TEST_MODE}" "Podfile"

  xcodebuild_args=( "-workspace" "${TEST_NAME}.xcworkspace" "-scheme" "${TEST_NAME}" )

  # For iOS, if the SDK is not provided it tries to use iphoneos, and the test
  # fail on Travis since those machines don't have a Code Signing identity.
  if  [[ "${TEST_NAME}" == iOS* ]] ; then
    # Apparently the destination flag is required to avoid "Unsupported architecture"
    # errors.
    xcodebuild_args+=(
        -sdk iphonesimulator ONLY_ACTIVE_ARCH=NO
        -destination "platform=iOS Simulator,name=iPad 2,OS=9.3"
    )
  fi

  # Do the work!
  pod install --verbose

  xcodebuild "${xcodebuild_args[@]}" build

  # Clear the hook and manually run cleanup.
  trap - EXIT
  cleanup "${TEST_NAME}"
}

# Run the tests.
for TEST_NAME in "${TEST_NAMES[@]}" ; do
  for TEST_MODE in "${TEST_MODES[@]}" ; do
    do_test "${TEST_NAME}" "${TEST_MODE}"
  done
done
