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

#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestCycle.pbobjc.h"
#import "objectivec/Tests/UnittestDeprecated.pbobjc.h"
#import "objectivec/Tests/UnittestDeprecatedFile.pbobjc.h"
#import "objectivec/Tests/UnittestImport.pbobjc.h"
#import "objectivec/Tests/UnittestImportPublic.pbobjc.h"
#import "objectivec/Tests/UnittestMset.pbobjc.h"
#import "objectivec/Tests/UnittestObjc.pbobjc.h"
#import "objectivec/Tests/UnittestObjcOptions.pbobjc.h"
#import "objectivec/Tests/UnittestObjcStartup.pbobjc.h"
#import "objectivec/Tests/UnittestPreserveUnknownEnum.pbobjc.h"
#import "objectivec/Tests/UnittestRuntimeProto2.pbobjc.h"
#import "objectivec/Tests/UnittestRuntimeProto3.pbobjc.h"
