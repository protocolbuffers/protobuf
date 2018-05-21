// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Collects all the compiled protos into one file and compiles them to make sure
// the compiler is generating valid code.

// The unittest_custom_options.proto extends the messages in descriptor.proto
// so we build it in to test extending in general. The library doesn't provide
// a descriptor as it doesn't use the classes/enums.
#import "google/protobuf/Descriptor.pbobjc.m"

#import "google/protobuf/AnyTest.pbobjc.m"
#import "google/protobuf/MapProto2Unittest.pbobjc.m"
#import "google/protobuf/MapUnittest.pbobjc.m"
#import "google/protobuf/Unittest.pbobjc.m"
#import "google/protobuf/UnittestArena.pbobjc.m"
#import "google/protobuf/UnittestCustomOptions.pbobjc.m"
#import "google/protobuf/UnittestCycle.pbobjc.m"
#import "google/protobuf/UnittestDeprecated.pbobjc.m"
#import "google/protobuf/UnittestDeprecatedFile.pbobjc.m"
#import "google/protobuf/UnittestDropUnknownFields.pbobjc.m"
#import "google/protobuf/UnittestEmbedOptimizeFor.pbobjc.m"
#import "google/protobuf/UnittestEmpty.pbobjc.m"
#import "google/protobuf/UnittestEnormousDescriptor.pbobjc.m"
#import "google/protobuf/UnittestImport.pbobjc.m"
#import "google/protobuf/UnittestImportLite.pbobjc.m"
#import "google/protobuf/UnittestImportPublic.pbobjc.m"
#import "google/protobuf/UnittestImportPublicLite.pbobjc.m"
#import "google/protobuf/UnittestLite.pbobjc.m"
#import "google/protobuf/UnittestMset.pbobjc.m"
#import "google/protobuf/UnittestMsetWireFormat.pbobjc.m"
#import "google/protobuf/UnittestNoArena.pbobjc.m"
#import "google/protobuf/UnittestNoArenaImport.pbobjc.m"
#import "google/protobuf/UnittestNoGenericServices.pbobjc.m"
#import "google/protobuf/UnittestObjc.pbobjc.m"
#import "google/protobuf/UnittestObjcStartup.pbobjc.m"
#import "google/protobuf/UnittestOptimizeFor.pbobjc.m"
#import "google/protobuf/UnittestPreserveUnknownEnum.pbobjc.m"
#import "google/protobuf/UnittestRuntimeProto2.pbobjc.m"
#import "google/protobuf/UnittestRuntimeProto3.pbobjc.m"

#import "google/protobuf/UnittestExtensionChainA.pbobjc.m"
#import "google/protobuf/UnittestExtensionChainB.pbobjc.m"
#import "google/protobuf/UnittestExtensionChainC.pbobjc.m"
#import "google/protobuf/UnittestExtensionChainD.pbobjc.m"
#import "google/protobuf/UnittestExtensionChainE.pbobjc.m"
// See GPBUnittestProtos2.m for for "UnittestExtensionChainF.pbobjc.m"
#import "google/protobuf/UnittestExtensionChainG.pbobjc.m"
