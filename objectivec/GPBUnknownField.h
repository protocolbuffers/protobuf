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
/**
 * Store an unknown field. These are used in conjunction with
 * GPBUnknownFieldSet.
 **/
@interface GPBUnknownField : NSObject<NSCopying>

/** The field number the data is stored under. */
@property(nonatomic, readonly, assign) int32_t number;

/** An array of varint values for this field. */
@property(nonatomic, readonly, strong) GPBUInt64Array *varintList;

/** An array of fixed32 values for this field. */
@property(nonatomic, readonly, strong) GPBUInt32Array *fixed32List;

/** An array of fixed64 values for this field. */
@property(nonatomic, readonly, strong) GPBUInt64Array *fixed64List;

/** An array of data values for this field. */
@property(nonatomic, readonly, strong) NSArray<NSData*> *lengthDelimitedList;

/** An array of groups of values for this field. */
@property(nonatomic, readonly, strong) NSArray<GPBUnknownFieldSet*> *groupList;

/**
 * Add a value to the varintList.
 *
 * @param value The value to add.
 **/
- (void)addVarint:(uint64_t)value;
/**
 * Add a value to the fixed32List.
 *
 * @param value The value to add.
 **/
- (void)addFixed32:(uint32_t)value;
/**
 * Add a value to the fixed64List.
 *
 * @param value The value to add.
 **/
- (void)addFixed64:(uint64_t)value;
/**
 * Add a value to the lengthDelimitedList.
 *
 * @param value The value to add.
 **/
- (void)addLengthDelimited:(NSData *)value;
/**
 * Add a value to the groupList.
 *
 * @param value The value to add.
 **/
- (void)addGroup:(GPBUnknownFieldSet *)value;

@end

NS_ASSUME_NONNULL_END
