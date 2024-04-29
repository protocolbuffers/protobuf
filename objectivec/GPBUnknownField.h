// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
__attribute__((objc_subclassing_restricted))
@interface GPBUnknownField : NSObject<NSCopying>

/** Initialize a field with the given number. */
- (instancetype)initWithNumber:(int32_t)number;

/** The field number the data is stored under. */
@property(nonatomic, readonly, assign) int32_t number;

/** An array of varint values for this field. */
@property(nonatomic, readonly, strong) GPBUInt64Array *varintList;

/** An array of fixed32 values for this field. */
@property(nonatomic, readonly, strong) GPBUInt32Array *fixed32List;

/** An array of fixed64 values for this field. */
@property(nonatomic, readonly, strong) GPBUInt64Array *fixed64List;

/** An array of data values for this field. */
@property(nonatomic, readonly, strong) NSArray<NSData *> *lengthDelimitedList;

/** An array of groups of values for this field. */
@property(nonatomic, readonly, strong) NSArray<GPBUnknownFieldSet *> *groupList;

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
