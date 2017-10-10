#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

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

// Makes sure all the generated headers compile with ARC on.

// The unittest_custom_options.proto extends the messages in descriptor.proto
// so we build it in to test extending in general. The library doesn't provide
// a descriptor as it doesn't use the classes/enums.
#import "google/protobuf/Descriptor.pbobjc.h"

#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestCustomOptions.pbobjc.h"
#import "google/protobuf/UnittestCycle.pbobjc.h"
#import "google/protobuf/UnittestDeprecated.pbobjc.h"
#import "google/protobuf/UnittestDeprecatedFile.pbobjc.h"
#import "google/protobuf/UnittestDropUnknownFields.pbobjc.h"
#import "google/protobuf/UnittestEmbedOptimizeFor.pbobjc.h"
#import "google/protobuf/UnittestEmpty.pbobjc.h"
#import "google/protobuf/UnittestEnormousDescriptor.pbobjc.h"
#import "google/protobuf/UnittestImport.pbobjc.h"
#import "google/protobuf/UnittestImportLite.pbobjc.h"
#import "google/protobuf/UnittestImportPublic.pbobjc.h"
#import "google/protobuf/UnittestImportPublicLite.pbobjc.h"
#import "google/protobuf/UnittestLite.pbobjc.h"
#import "google/protobuf/UnittestMset.pbobjc.h"
#import "google/protobuf/UnittestNoGenericServices.pbobjc.h"
#import "google/protobuf/UnittestObjc.pbobjc.h"
#import "google/protobuf/UnittestObjcStartup.pbobjc.h"
#import "google/protobuf/UnittestOptimizeFor.pbobjc.h"
#import "google/protobuf/UnittestPreserveUnknownEnum.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto2.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto3.pbobjc.h"
