#!/bin/bash

# This script checks that the runtime version number constant in the compiler
# source and in the runtime source is the same.
#
# A distro can be made of the protobuf sources with only a subset of the
# languages, so if the compiler depended on the Objective C runtime, those
# builds would break. At the same time, we don't want the runtime source
# depending on the compiler sources; so two copies of the constant are needed.

set -eu

readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")
readonly ProtoRootDir="${ScriptDir}/../.."

die() {
    echo "Error: $1"
    exit 1
}

readonly ConstantName=GOOGLE_PROTOBUF_OBJC_GEN_VERSION

# Collect version from plugin sources.

readonly PluginSrc="${ProtoRootDir}/src/google/protobuf/compiler/objectivec/objectivec_file.cc"
readonly PluginVersion=$( \
    cat "${PluginSrc}" \
        | sed -n -e "s:const int32 ${ConstantName} = \([0-9]*\);:\1:p"
)

if [[ -z "${PluginVersion}" ]] ; then
    die "Failed to find ${ConstantName} in the plugin source (${PluginSrc})."
fi

# Collect version from runtime sources.

readonly RuntimeSrc="${ProtoRootDir}/objectivec/GPBBootstrap.h"
readonly RuntimeVersion=$( \
    cat "${RuntimeSrc}" \
        | sed -n -e "s:#define ${ConstantName} \([0-9]*\):\1:p"
)

if [[ -z "${RuntimeVersion}" ]] ; then
    die "Failed to find ${ConstantName} in the runtime source (${RuntimeSrc})."
fi

# Compare them.

if [[ "${PluginVersion}" != "${RuntimeVersion}" ]] ; then
    die "Versions don't match!
   Plugin: ${PluginVersion} from ${PluginSrc}
  Runtime: ${RuntimeVersion} from ${RuntimeSrc}
"
fi

# Success
