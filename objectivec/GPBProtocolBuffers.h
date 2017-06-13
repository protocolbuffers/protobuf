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

#import "GPBBootstrap.h"

#import "GPBArray.h"
#import "GPBCodedInputStream.h"
#import "GPBCodedOutputStream.h"
#import "GPBDescriptor.h"
#import "GPBDictionary.h"
#import "GPBExtensionRegistry.h"
#import "GPBMessage.h"
#import "GPBRootObject.h"
#import "GPBUnknownField.h"
#import "GPBUnknownFieldSet.h"
#import "GPBUtilities.h"
#import "GPBWellKnownTypes.h"
#import "GPBWireFormat.h"

// This CPP symbol can be defined to use imports that match up to the framework
// imports needed when using CocoaPods.
#if !defined(GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS)
 #define GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS 0
#endif

// Well-known proto types
#if GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS
 #import <protobuf/Any.pbobjc.h>
 #import <protobuf/Api.pbobjc.h>
 #import <protobuf/Duration.pbobjc.h>
 #import <protobuf/Empty.pbobjc.h>
 #import <protobuf/FieldMask.pbobjc.h>
 #import <protobuf/SourceContext.pbobjc.h>
 #import <protobuf/Struct.pbobjc.h>
 #import <protobuf/Timestamp.pbobjc.h>
 #import <protobuf/Type.pbobjc.h>
 #import <protobuf/Wrappers.pbobjc.h>
#else
 #import "google/protobuf/Any.pbobjc.h"
 #import "google/protobuf/Api.pbobjc.h"
 #import "google/protobuf/Duration.pbobjc.h"
 #import "google/protobuf/Empty.pbobjc.h"
 #import "google/protobuf/FieldMask.pbobjc.h"
 #import "google/protobuf/SourceContext.pbobjc.h"
 #import "google/protobuf/Struct.pbobjc.h"
 #import "google/protobuf/Timestamp.pbobjc.h"
 #import "google/protobuf/Type.pbobjc.h"
 #import "google/protobuf/Wrappers.pbobjc.h"
#endif
