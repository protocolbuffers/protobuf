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

#import <Foundation/Foundation.h>

@class GPBCodedOutputStream;
@class GPBUInt32Array;
@class GPBUInt64Array;
@class GPBUnknownFieldSet;

NS_ASSUME_NONNULL_BEGIN

@interface GPBUnknownField : NSObject<NSCopying>

@property(nonatomic, readonly, assign) int32_t number;

// Only one of these will be set.
@property(nonatomic, readonly, strong) GPBUInt64Array *varintList;
@property(nonatomic, readonly, strong) GPBUInt32Array *fixed32List;
@property(nonatomic, readonly, strong) GPBUInt64Array *fixed64List;
@property(nonatomic, readonly, strong) NSArray *lengthDelimitedList;  // NSData
@property(nonatomic, readonly, strong) NSArray *groupList;  // GPBUnknownFieldSet

// Only one of these should be used per Field.
- (void)addVarint:(uint64_t)value;
- (void)addFixed32:(uint32_t)value;
- (void)addFixed64:(uint64_t)value;
- (void)addLengthDelimited:(NSData *)value;
- (void)addGroup:(GPBUnknownFieldSet *)value;

@end

NS_ASSUME_NONNULL_END
