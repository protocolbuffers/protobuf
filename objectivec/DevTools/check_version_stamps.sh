#!/bin/bash -eu

# This script checks that the runtime version number constant in the compiler
# source and in the runtime source is the same.
#
# We don't really want the generator sources directly referencing the runtime
# or the reverse, so they both have the same constant defined, and this script
# is used in a test to ensure the values stay in sync.

die() {
    echo "Error: $1"
    exit 1
}

readonly GeneratorSrc="src/google/protobuf/compiler/objectivec/file.cc"
readonly RuntimeSrc="objectivec/GPBBootstrap.h"

if [[ ! -e "${GeneratorSrc}" ]] ; then
  die "Failed to find generator file: ${GeneratorSrc}"
fi
if [[ ! -e "${RuntimeSrc}" ]] ; then
  die "Failed to find runtime file: ${RuntimeSrc}"
fi

check_constant() {
  local ConstantName="$1"

  # Collect version from generator sources.
  local GeneratorVersion=$( \
      cat "${GeneratorSrc}" \
          | sed -n -e "s:const int32_t ${ConstantName} = \([0-9]*\);:\1:p"
  )
  if [[ -z "${GeneratorVersion}" ]] ; then
      die "Failed to find ${ConstantName} in the generator source (${GeneratorSrc})."
  fi

  # Collect version from runtime sources.
  local RuntimeVersion=$( \
      cat "${RuntimeSrc}" \
          | sed -n -e "s:#define ${ConstantName} \([0-9]*\):\1:p"
  )
  if [[ -z "${RuntimeVersion}" ]] ; then
      die "Failed to find ${ConstantName} in the runtime source (${RuntimeSrc})."
  fi

  # Compare them.
  if [[ "${GeneratorVersion}" != "${RuntimeVersion}" ]] ; then
      die "${ConstantName} values don't match!
  Generator: ${GeneratorVersion} from ${GeneratorSrc}
    Runtime: ${RuntimeVersion} from ${RuntimeSrc}
"
  fi
}

# Do the check.
check_constant GOOGLE_PROTOBUF_OBJC_VERSION

# Success
