#!/bin/bash

# Adjust these to reflect the location of NUnit in your system,
# and how you want NUnit to run
NUNIT=~/protobuf/NUnit-2.5.0.9122/bin/net-2.0/nunit-console.exe
NUNIT_OPTIONS=-noshadow

# The rest should be okay.

SRC=../src
LIB=../lib
KEYFILE=../keys/Google.ProtocolBuffers.snk

rm -rf bin
mkdir bin

# Running the unit tests requires the dependencies are
# in the bin directory too
cp -f $LIB/{Rhino.Mocks.dll,nunit.framework.dll} bin

echo Building main library
gmcs -target:library -out:bin/Google.ProtocolBuffers.dll `find $SRC/ProtocolBuffers -name '*.cs'` -keyfile:$KEYFILE

echo Building main library tests
gmcs -target:library -out:bin/Google.ProtocolBuffers.Test.dll `find $SRC/ProtocolBuffers.Test -name '*.cs'` -keyfile:$KEYFILE -r:bin/Google.ProtocolBuffers.dll -r:$LIB/nunit.framework.dll -r:$LIB/Rhino.Mocks.dll

echo Running main library tests
mono $NUNIT bin/Google.ProtocolBuffers.Test.dll $NUNIT_OPTIONS

echo Building ProtoGen
gmcs -target:exe -out:bin/ProtoGen.exe `find $SRC/ProtoGen -name '*.cs'` -keyfile:$KEYFILE -r:bin/Google.ProtocolBuffers.dll

echo Building ProtoGen tests
gmcs -target:library -out:bin/Google.ProtocolBuffers.ProtoGen.Test.dll `find $SRC/ProtoGen.Test -name '*.cs'` -keyfile:$KEYFILE -r:bin/Google.ProtocolBuffers.dll -r:$LIB/nunit.framework.dll -r:bin/ProtoGen.exe

echo Running ProtoGen tests
mono $NUNIT bin/Google.ProtocolBuffers.ProtoGen.Test.dll $NUNIT_OPTIONS
