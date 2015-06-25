#!/bin/bash
# Use mono to build solution and run all tests.

# Adjust these to reflect the location of nunit-console in your system.
NUNIT_CONSOLE=nunit-console

# The rest you can leave intact
CONFIG=Release
KEYFILE=../keys/Google.ProtocolBuffers.snk  # TODO(jtattermusch): signing!
SRC=$(dirname $0)/src

set -ex

# echo Building the solution.
# TODO(jonskeet): Re-enable building the whole solution when we have ProtoBench et al
# working again.
# xbuild /p:Configuration=$CONFIG $SRC/ProtocolBuffers.sln

xbuild /p:Configuration=$CONFIG $SRC/ProtocolBuffers/ProtocolBuffers.csproj
xbuild /p:Configuration=$CONFIG $SRC/ProtocolBuffers.Test/ProtocolBuffers.Test.csproj

echo Running tests.
$NUNIT_CONSOLE $SRC/ProtocolBuffers.Test/bin/$CONFIG/Google.Protobuf.Test.dll
