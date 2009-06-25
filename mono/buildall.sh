#!/bin/bash

export SRC=../src
export LIB=../lib

echo Building main library
gmcs -target:library -out:Google.ProtocolBuffers.dll `find $SRC/ProtocolBuffers -name '*.cs'` -keyfile:$SRC/ProtocolBuffers/Properties/Google.ProtocolBuffers.snk

echo Building main library tests
gmcs -target:library -out:Google.ProtocolBuffers.Test.dll `find $SRC/ProtocolBuffers.Test -name '*.cs'` -keyfile:$SRC/ProtocolBuffers.Test/Properties/Google.ProtocolBuffers.Test.snk -r:Google.ProtocolBuffers.dll -r:$LIB/nunit.framework.dll -r:$LIB/Rhino.Mocks.dll

echo Running main library tests
mono ~/protobuf/NUnit-2.5.0.9122/bin/net-2.0/nunit-console.exe Google.ProtocolBuffers.Test.dll -noshadow

echo Building ProtoGen
gmcs -target:exe -out:ProtoGen.exe `find $SRC/ProtoGen -name '*.cs'` -keyfile:$SRC/ProtoGen/Properties/Google.ProtocolBuffers.ProtoGen.snk -r:Google.ProtocolBuffers.dll

echo Building ProtoGen tests
gmcs -target:library -out:Google.ProtocolBuffers.ProtoGen.Test.dll `find $SRC/ProtoGen.Test -name '*.cs'` -keyfile:$SRC/ProtoGen.Test/Properties/Google.ProtocolBuffers.ProtoGen.Test.snk -r:Google.ProtocolBuffers.dll -r:$LIB/nunit.framework.dll -r:ProtoGen.exe

echo Running ProtoGen tests
mono ~/protobuf/NUnit-2.5.0.9122/bin/net-2.0/nunit-console.exe Google.ProtocolBuffers.ProtoGen.Test.dll -noshadow

