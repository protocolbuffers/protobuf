// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

@class GPBUnknownField;

NS_ASSUME_NONNULL_BEGIN

/**
 * A collection of unknown fields. Fields parsed from the binary representation
 * of a message that are unknown end up in an instance of this set.
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
