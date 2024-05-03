#!/bin/bash
#
# Helper to do build so you don't have to remember all the steps/args.

set -eu

# Some base locations.
readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")
readonly ProtoRootDir="${ScriptDir}/../.."
readonly BazelFlags="${BAZEL_FLAGS:---announce_rc --macos_minimum_os=10.13}"

# Invoke with BAZEL=bazelisk to use that instead.
readonly BazelBin="${BAZEL:-bazel}"

printUsage() {
  NAME=$(basename "${0}")
  cat << EOF
usage: ${NAME} [OPTIONS]

This script does the common build steps needed.

OPTIONS:

 General:

   -h, --help
         Show this message
   -c, --clean
         Issue a clean before the normal build.
   --full-build
         By default only protoc is built within protobuf, this option will
         enable a full build/test of the entire protobuf project.
   --skip-xcode
         Skip the invoke of Xcode to test the runtime on both iOS and OS X.
   --skip-xcode-ios
         Skip the invoke of Xcode to test the runtime on iOS.
   --skip-xcode-debug
         Skip the Xcode Debug configuration.
   --skip-xcode-release
         Skip the Xcode Release configuration.
   --skip-xcode-osx | --skip-xcode-macos
         Skip the invoke of Xcode to test the runtime on OS X.
   --skip-xcode-tvos
         Skip the invoke of Xcode to test the runtime on tvOS.
   --skip-objc-conformance
         Skip the Objective C conformance tests (run on OS X).
   --skip-xcpretty
         By default, if xcpretty is installed, it will be used, this option will
         skip it even it it is installed.
   --xcode-quiet
         Pass -quiet to xcodebuild.

EOF
}

header() {
  echo ""
  echo "========================================================================"
  echo "    ${@}"
  echo "========================================================================"
}

xcodebuild_xcpretty() {
  set -o pipefail && xcodebuild "${@}" | xcpretty
}

if hash xcpretty >/dev/null 2>&1 ; then
  XCODEBUILD=xcodebuild_xcpretty
else
  XCODEBUILD=xcodebuild
fi

DO_CLEAN=no
FULL_BUILD=no
DO_XCODE_IOS_TESTS=yes
DO_XCODE_OSX_TESTS=yes
DO_XCODE_TVOS_TESTS=yes
DO_XCODE_DEBUG=yes
DO_XCODE_RELEASE=yes
DO_OBJC_CONFORMANCE_TESTS=yes
XCODE_QUIET=no
while [[ $# != 0 ]]; do
  case "${1}" in
    -h | --help )
      printUsage
      exit 0
      ;;
    -c | --clean )
      DO_CLEAN=yes
      ;;
    --full-build )
      FULL_BUILD=yes
      ;;
    --skip-xcode )
      DO_XCODE_IOS_TESTS=no
      DO_XCODE_OSX_TESTS=no
      DO_XCODE_TVOS_TESTS=no
      ;;
    --skip-xcode-ios )
      DO_XCODE_IOS_TESTS=no
      ;;
    --skip-xcode-osx | --skip-xcode-macos)
      DO_XCODE_OSX_TESTS=no
      ;;
    --skip-xcode-tvos )
      DO_XCODE_TVOS_TESTS=no
      ;;
    --skip-xcode-debug )
      DO_XCODE_DEBUG=no
      ;;
    --skip-xcode-release )
      DO_XCODE_RELEASE=no
      ;;
    --skip-objc-conformance )
      DO_OBJC_CONFORMANCE_TESTS=no
      ;;
    --skip-xcpretty )
      XCODEBUILD=xcodebuild
      ;;
    --xcode-quiet )
      XCODE_QUIET=yes
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

# Into the proto dir.
cd "${ProtoRootDir}"

if [[ "${DO_CLEAN}" == "yes" ]] ; then
  header "Cleaning"
  ${BazelBin} clean
  if [[ "${DO_XCODE_IOS_TESTS}" == "yes" ]] ; then
    XCODEBUILD_CLEAN_BASE_IOS=(
      xcodebuild
        -project objectivec/ProtocolBuffers_iOS.xcodeproj
        -scheme ProtocolBuffers
    )
    if [[ "${DO_XCODE_DEBUG}" == "yes" ]] ; then
      "${XCODEBUILD_CLEAN_BASE_IOS[@]}" -configuration Debug clean
    fi
    if [[ "${DO_XCODE_RELEASE}" == "yes" ]] ; then
      "${XCODEBUILD_CLEAN_BASE_IOS[@]}" -configuration Release clean
    fi
  fi
  if [[ "${DO_XCODE_OSX_TESTS}" == "yes" ]] ; then
    XCODEBUILD_CLEAN_BASE_OSX=(
      xcodebuild
        -project objectivec/ProtocolBuffers_OSX.xcodeproj
        -scheme ProtocolBuffers
    )
    if [[ "${DO_XCODE_DEBUG}" == "yes" ]] ; then
      "${XCODEBUILD_CLEAN_BASE_OSX[@]}" -configuration Debug clean
    fi
    if [[ "${DO_XCODE_RELEASE}" == "yes" ]] ; then
      "${XCODEBUILD_CLEAN_BASE_OSX[@]}" -configuration Release clean
    fi
  fi
  if [[ "${DO_XCODE_TVOS_TESTS}" == "yes" ]] ; then
    XCODEBUILD_CLEAN_BASE_OSX=(
      xcodebuild
        -project objectivec/ProtocolBuffers_tvOS.xcodeproj
        -scheme ProtocolBuffers
    )
    if [[ "${DO_XCODE_DEBUG}" == "yes" ]] ; then
      "${XCODEBUILD_CLEAN_BASE_OSX[@]}" -configuration Debug clean
    fi
    if [[ "${DO_XCODE_RELEASE}" == "yes" ]] ; then
      "${XCODEBUILD_CLEAN_BASE_OSX[@]}" -configuration Release clean
    fi
  fi
fi

if [[ "${FULL_BUILD}" == "yes" ]] ; then
  header "Build/Test: everything"
  ${BazelBin} test //:protoc //:protobuf //src/... $BazelFlags
else
  header "Building: protoc"
  ${BazelBin} build //:protoc $BazelFlags
fi

# Ensure the WKT sources checked in are current.
objectivec/generate_well_known_types.sh --check-only

header "Checking on the ObjC Runtime pddm expansions"
${BazelBin} test //objectivec:sources_pddm_expansion_test $BazelFlags

readonly XCODE_VERSION_LINE="$(xcodebuild -version | grep Xcode\  )"
readonly XCODE_VERSION="${XCODE_VERSION_LINE/Xcode /}"  # drop the prefix.

if [[ "${DO_XCODE_IOS_TESTS}" == "yes" ]] ; then
  XCODEBUILD_TEST_BASE_IOS=(
    "${XCODEBUILD}"
      -project objectivec/ProtocolBuffers_iOS.xcodeproj
      -scheme ProtocolBuffers
  )
  if [[ "${XCODE_QUIET}" == "yes" ]] ; then
    XCODEBUILD_TEST_BASE_IOS+=( -quiet )
  fi
  # Don't need to worry about form factors or retina/non retina.
  # NOTE: Different Xcode have different simulated hardware/os support.
  case "${XCODE_VERSION}" in
    [6-9].* | 1[0-2].* )
      echo "ERROR: Xcode 13.3.1 or higher is required." 1>&2
      exit 11
      ;;
    13.* | 14.*)
      XCODEBUILD_TEST_BASE_IOS+=(
          -destination "platform=iOS Simulator,name=iPhone 13,OS=latest"
      )
      ;;
    * )
      echo ""
      echo "ATTENTION: Time to update the simulator targets for Xcode ${XCODE_VERSION}"
      echo ""
      echo "ERROR: Build aborted!"
      exit 2
      ;;
  esac
  if [[ "${DO_XCODE_DEBUG}" == "yes" ]] ; then
    header "Doing Xcode iOS build/tests - Debug"
    "${XCODEBUILD_TEST_BASE_IOS[@]}" -configuration Debug test
  fi
  if [[ "${DO_XCODE_RELEASE}" == "yes" ]] ; then
    header "Doing Xcode iOS build/tests - Release"
    "${XCODEBUILD_TEST_BASE_IOS[@]}" -configuration Release test
  fi
  # Don't leave the simulator in the developer's face.
  killall Simulator 2> /dev/null || true
fi
if [[ "${DO_XCODE_OSX_TESTS}" == "yes" ]] ; then
  XCODEBUILD_TEST_BASE_OSX=(
    "${XCODEBUILD}"
      -project objectivec/ProtocolBuffers_OSX.xcodeproj
      -scheme ProtocolBuffers
      -destination "platform=macOS"
  )
  if [[ "${XCODE_QUIET}" == "yes" ]] ; then
    XCODEBUILD_TEST_BASE_OSX+=( -quiet )
  fi
  case "${XCODE_VERSION}" in
    [6-9].* | 1[0-2].* )
      echo "ERROR: Xcode 13.3.1 or higher is required." 1>&2
      exit 11
      ;;
  esac
  if [[ "${DO_XCODE_DEBUG}" == "yes" ]] ; then
    header "Doing Xcode OS X build/tests - Debug"
    "${XCODEBUILD_TEST_BASE_OSX[@]}" -configuration Debug test
  fi
  if [[ "${DO_XCODE_RELEASE}" == "yes" ]] ; then
    header "Doing Xcode OS X build/tests - Release"
    "${XCODEBUILD_TEST_BASE_OSX[@]}" -configuration Release test
  fi
fi
if [[ "${DO_XCODE_TVOS_TESTS}" == "yes" ]] ; then
  XCODEBUILD_TEST_BASE_TVOS=(
    "${XCODEBUILD}"
      -project objectivec/ProtocolBuffers_tvOS.xcodeproj
      -scheme ProtocolBuffers
  )
  case "${XCODE_VERSION}" in
    [6-9].* | 1[0-2].* )
      echo "ERROR: Xcode 13.3.1 or higher is required." 1>&2
      exit 11
      ;;
    13.* | 14.*)
      XCODEBUILD_TEST_BASE_TVOS+=(
        -destination "platform=tvOS Simulator,name=Apple TV 4K (2nd generation),OS=latest"
      )
      ;;
    * )
      echo ""
      echo "ATTENTION: Time to update the simulator targets for Xcode ${XCODE_VERSION}"
      echo ""
      echo "ERROR: Build aborted!"
      exit 2
      ;;
  esac
  if [[ "${XCODE_QUIET}" == "yes" ]] ; then
    XCODEBUILD_TEST_BASE_TVOS+=( -quiet )
  fi
  if [[ "${DO_XCODE_DEBUG}" == "yes" ]] ; then
    header "Doing Xcode tvOS build/tests - Debug"
    "${XCODEBUILD_TEST_BASE_TVOS[@]}" -configuration Debug test
  fi
  if [[ "${DO_XCODE_RELEASE}" == "yes" ]] ; then
    header "Doing Xcode tvOS build/tests - Release"
    "${XCODEBUILD_TEST_BASE_TVOS[@]}" -configuration Release test
  fi
fi

if [[ "${DO_OBJC_CONFORMANCE_TESTS}" == "yes" ]] ; then
  header "Running ObjC Conformance Tests"
  ${BazelBin} test //objectivec:conformance_test $BazelFlags
fi

echo ""
echo "$(basename "${0}"): Success!"
