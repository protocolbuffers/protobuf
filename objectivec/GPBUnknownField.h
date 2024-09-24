// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBArray.h"
#import "GPBUnknownFieldSet.h"
#import "GPBUnknownFields.h"

@class GPBUnknownFieldSet;
@class GPBUnknownFields;

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(uint8_t, GPBUnknownFieldType) {
  GPBUnknownFieldTypeVarint,
  GPBUnknownFieldTypeFixed32,
  GPBUnknownFieldTypeFixed64,
  GPBUnknownFieldTypeLengthDelimited,  // Length prefixed
  GPBUnknownFieldTypeGroup,            // Tag delimited

  /**
   * This type is only used with fields from `GPBUnknownFieldsSet`. Some methods
   * only work with instances with this type and other apis require the other
   * type(s). It is a programming error to use the wrong methods.
   **/
  GPBUnknownFieldTypeLegacy,
};

/**
 * Store an unknown field. These are used in conjunction with GPBUnknownFields.
 **/
__attribute__((objc_subclassing_restricted))
@interface GPBUnknownField : NSObject<NSCopying>

/** Initialize a field with the given number. */
- (instancetype)initWithNumber:(int32_t)number
    __attribute__((deprecated(
        "Use the apis on GPBUnknownFields to add new fields instead of making them directly.")));

/** The field number the data is stored under. */
@property(nonatomic, readonly, assign) int32_t number;

/** The type of the field. */
@property(nonatomic, readonly, assign) GPBUnknownFieldType type;

/**
 * Fetch the varint value.
 *
 * It is a programming error to call this when the `type` is not a varint.
 */
@property(nonatomic, readonly, assign) uint64_t varint;

/**
 * Fetch the fixed32 value.
 *
 * It is a programming error to call this when the `type` is not a fixed32.
 */
@property(nonatomic, readonly, assign) uint32_t fixed32;

/**
 * Fetch the fixed64 value.
 *
 * It is a programming error to call this when the `type` is not a fixed64.
 */
@property(nonatomic, readonly, assign) uint64_t fixed64;

/**
 * Fetch the length delimited (length prefixed) value.
 *
 * It is a programming error to call this when the `type` is not a length
 * delimited.
 */
@property(nonatomic, readonly, strong, nonnull) NSData *lengthDelimited;

/**
 * Fetch the group (tag delimited) value.
 *
 * It is a programming error to call this when the `type` is not a group.
 */
@property(nonatomic, readonly, strong, nonnull) GPBUnknownFields *group;

/**
 * An array of varint values for this field.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 */
@property(nonatomic, readonly, strong) GPBUInt64Array *varintList
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * An array of fixed32 values for this field.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 */
@property(nonatomic, readonly, strong) GPBUInt32Array *fixed32List
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * An array of fixed64 values for this field.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 */
@property(nonatomic, readonly, strong) GPBUInt64Array *fixed64List
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * An array of data values for this field.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 */
@property(nonatomic, readonly, strong) NSArray<NSData *> *lengthDelimitedList
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * An array of groups of values for this field.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 */
@property(nonatomic, readonly, strong) NSArray<GPBUnknownFieldSet *> *groupList
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * Add a value to the varintList.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 *
 * @param value The value to add.
 **/
- (void)addVarint:(uint64_t)value
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * Add a value to the fixed32List.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 *
 * @param value The value to add.
 **/
- (void)addFixed32:(uint32_t)value
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * Add a value to the fixed64List.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 *
 * @param value The value to add.
 **/
- (void)addFixed64:(uint64_t)value
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * Add a value to the lengthDelimitedList.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 *
 * @param value The value to add.
 **/
- (void)addLengthDelimited:(NSData *)value
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

/**
 * Add a value to the groupList.
 *
 * Only valid for type == GPBUnknownFieldTypeLegacy, it is a programming error
 * to use with any other type.
 *
 * @param value The value to add.
 **/
- (void)addGroup:(GPBUnknownFieldSet *)value
    __attribute__((deprecated("See the new apis on GPBUnknownFields and thus reduced api here.")));

@end

NS_ASSUME_NONNULL_END
