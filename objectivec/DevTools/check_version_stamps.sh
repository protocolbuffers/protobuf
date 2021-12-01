#!/bin/bash -eu

# This script checks that the runtime version number constant in the compiler
# source and in the runtime source is the same.
#
# A distro can be made of the protobuf sources with only a subset of the
# languages, so if the compiler depended on the Objective C runtime, those
# builds would break. At the same time, we don't want the runtime source
# depending on the compiler sources; so two copies of the constant are needed.

readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")
readonly ProtoRootDir="${ScriptDir}/../.."

die() {
    echo "Error: $1"
    exit 1
}

readonly GeneratorSrc="${ProtoRootDir}/src/google/protobuf/compiler/objectivec/objectivec_file.cc"
readonly RuntimeSrc="${ProtoRootDir}/objectivec/GPBBootstrap.h"

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
