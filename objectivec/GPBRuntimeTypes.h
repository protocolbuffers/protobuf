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

#import "GPBBootstrap.h"

@class GPBEnumDescriptor;
@class GPBMessage;
@class GPBInt32Array;

/**
 * Verifies that a given value can be represented by an enum type.
 * */
typedef BOOL (*GPBEnumValidationFunc)(int32_t);

/**
 * Fetches an EnumDescriptor.
 * */
typedef GPBEnumDescriptor *(*GPBEnumDescriptorFunc)(void);

/**
 * Magic value used at runtime to indicate an enum value that wasn't know at
 * compile time.
 * */
enum {
  kGPBUnrecognizedEnumeratorValue = (int32_t)0xFBADBEEF,
};

/**
 * A union for storing all possible Protobuf values. Note that owner is
 * responsible for memory management of object types.
 * */
typedef union {
  BOOL valueBool;
  int32_t valueInt32;
  int64_t valueInt64;
  uint32_t valueUInt32;
  uint64_t valueUInt64;
  float valueFloat;
  double valueDouble;
  GPB_UNSAFE_UNRETAINED NSData *valueData;
  GPB_UNSAFE_UNRETAINED NSString *valueString;
  GPB_UNSAFE_UNRETAINED GPBMessage *valueMessage;
  int32_t valueEnum;
} GPBGenericValue;

/**
 * Enum listing the possible data types that a field can contain.
 *
 * @note Do not change the order of this enum (or add things to it) without
 *       thinking about it very carefully. There are several things that depend
 *       on the order.
 * */
typedef NS_ENUM(uint8_t, GPBDataType) {
  /** Field contains boolean value(s). */
  GPBDataTypeBool = 0,
  /** Field contains unsigned 4 byte value(s). */
  GPBDataTypeFixed32,
  /** Field contains signed 4 byte value(s). */
  GPBDataTypeSFixed32,
  /** Field contains float value(s). */
  GPBDataTypeFloat,
  /** Field contains unsigned 8 byte value(s). */
  GPBDataTypeFixed64,
  /** Field contains signed 8 byte value(s). */
  GPBDataTypeSFixed64,
  /** Field contains double value(s). */
  GPBDataTypeDouble,
  /**
   * Field contains variable length value(s). Inefficient for encoding negative
   * numbers – if your field is likely to have negative values, use
   * GPBDataTypeSInt32 instead.
   **/
  GPBDataTypeInt32,
  /**
   * Field contains variable length value(s). Inefficient for encoding negative
   * numbers – if your field is likely to have negative values, use
   * GPBDataTypeSInt64 instead.
   **/
  GPBDataTypeInt64,
  /** Field contains signed variable length integer value(s). */
  GPBDataTypeSInt32,
  /** Field contains signed variable length integer value(s). */
  GPBDataTypeSInt64,
  /** Field contains unsigned variable length integer value(s). */
  GPBDataTypeUInt32,
  /** Field contains unsigned variable length integer value(s). */
  GPBDataTypeUInt64,
  /** Field contains an arbitrary sequence of bytes. */
  GPBDataTypeBytes,
  /** Field contains UTF-8 encoded or 7-bit ASCII text. */
  GPBDataTypeString,
  /** Field contains message type(s). */
  GPBDataTypeMessage,
  /** Field contains message type(s). */
  GPBDataTypeGroup,
  /** Field contains enum value(s). */
  GPBDataTypeEnum,
};

enum {
  /**
   * A count of the number of types in GPBDataType. Separated out from the
   * GPBDataType enum to avoid warnings regarding not handling GPBDataType_Count
   * in switch statements.
   **/
  GPBDataType_Count = GPBDataTypeEnum + 1
};

/** An extension range. */
typedef struct GPBExtensionRange {
  /** Inclusive. */
  uint32_t start;
  /** Exclusive. */
  uint32_t end;
} GPBExtensionRange;

/**
 A type to represent an Objective C class.
 This is actually an `objc_class` but the runtime headers will not allow us to
 reference `objc_class`, so we have defined our own.
*/
typedef struct GPBObjcClass_t GPBObjcClass_t;
