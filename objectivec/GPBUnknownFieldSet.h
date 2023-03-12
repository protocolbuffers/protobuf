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

@class GPBUnknownField;

NS_ASSUME_NONNULL_BEGIN

/**
 * A collection of unknown fields. Fields parsed from the binary representation
 * of a message that are unknown end up in an instance of this set. This only
 * applies for files declared with the "proto2" syntax.
 **/
__attribute__((objc_subclassing_restricted))
@interface GPBUnknownFieldSet : NSObject<NSCopying>

/**
 * Tests to see if the given field number has a value.
 *
 * @param number The field number to check.
 *
 * @return YES if there is an unknown field for the given field number.
 **/
- (BOOL)hasField:(int32_t)number;

/**
 * Fetches the GPBUnknownField for the given field number.
 *
 * @param number The field number to look up.
 *
 * @return The GPBUnknownField or nil if none found.
 **/
- (nullable GPBUnknownField *)getField:(int32_t)number;

/**
 * @return The number of fields in this set.
 **/
- (NSUInteger)countOfFields;

/**
 * Adds the given field to the set.
 *
 * @param field The field to add to the set.
 **/
- (void)addField:(GPBUnknownField *)field;

/**
 * @return An array of the GPBUnknownFields sorted by the field numbers.
 **/
- (NSArray<GPBUnknownField *> *)sortedFields;

@end

NS_ASSUME_NONNULL_END
