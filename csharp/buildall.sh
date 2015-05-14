#!/bin/bash
# Use mono to build solution and run all tests.

# Adjust these to reflect the location of nunit-console in your system.
NUNIT_CONSOLE=nunit-console

# The rest you can leave intact
CONFIG=Release
KEYFILE=../keys/Google.ProtocolBuffers.snk  # TODO(jtattermusch): signing!
SRC=$(dirname $0)/src

set -ex

echo Building the solution.
xbuild /p:Configuration=$CONFIG $SRC/ProtocolBuffers.sln

echo Running tests.
$NUNIT_CONSOLE $SRC/ProtocolBuffers.Test/bin/$CONFIG/Google.ProtocolBuffers.Test.dll

$NUNIT_CONSOLE $SRC/ProtocolBuffersLite.Test/bin/$CONFIG/Google.ProtocolBuffersLite.Test.dll

$NUNIT_CONSOLE $SRC/ProtocolBuffersLite.Test/bin/$CONFIG/Google.ProtocolBuffersMixedLite.Test.dll
