// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

#import <Foundation/Foundation.h>

// This CPP symbol can be defined to use imports that match up to the framework
// imports needed when using CocoaPods.
#if !defined(GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS)
 #define GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS 0
#endif

#if GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS
 #import <Protobuf/Duration.pbobjc.h>
 #import <Protobuf/Timestamp.pbobjc.h>
#else
 #import "google/protobuf/Duration.pbobjc.h"
 #import "google/protobuf/Timestamp.pbobjc.h"
#endif

NS_ASSUME_NONNULL_BEGIN

// Extension to GPBTimestamp to work with standard Foundation time/date types.
@interface GPBTimestamp (GBPWellKnownTypes)
@property(nonatomic, readwrite, strong) NSDate *date;
@property(nonatomic, readwrite) NSTimeInterval timeIntervalSince1970;
- (instancetype)initWithDate:(NSDate *)date;
- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970;
@end

// Extension to GPBDuration to work with standard Foundation time type.
@interface GPBDuration (GBPWellKnownTypes)
@property(nonatomic, readwrite) NSTimeInterval timeIntervalSince1970;
- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970;
@end

NS_ASSUME_NONNULL_END
