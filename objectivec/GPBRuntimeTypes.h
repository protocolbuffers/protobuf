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

// Function used to verify that a given value can be represented by an
// enum type.
typedef BOOL (*GPBEnumValidationFunc)(int32_t);

// Function used to fetch an EnumDescriptor.
typedef GPBEnumDescriptor *(*GPBEnumDescriptorFunc)(void);

// Magic values used for when an the at runtime to indicate an enum value
// that wasn't know at compile time.
enum {
  kGPBUnrecognizedEnumeratorValue = (int32_t)0xFBADBEEF,
};

// A union for storing all possible Protobuf values.
// Note that owner is responsible for memory management of object types.
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

// Do not change the order of this enum (or add things to it) without thinking
// about it very carefully. There are several things that depend on the order.
typedef enum {
  GPBDataTypeBool = 0,
  GPBDataTypeFixed32,
  GPBDataTypeSFixed32,
  GPBDataTypeFloat,
  GPBDataTypeFixed64,
  GPBDataTypeSFixed64,
  GPBDataTypeDouble,
  GPBDataTypeInt32,
  GPBDataTypeInt64,
  GPBDataTypeSInt32,
  GPBDataTypeSInt64,
  GPBDataTypeUInt32,
  GPBDataTypeUInt64,
  GPBDataTypeBytes,
  GPBDataTypeString,
  GPBDataTypeMessage,
  GPBDataTypeGroup,
  GPBDataTypeEnum,
} GPBDataType;

enum {
  // A count of the number of types in GPBDataType. Separated out from the
  // GPBDataType enum to avoid warnings regarding not handling
  // GPBDataType_Count in switch statements.
  GPBDataType_Count = GPBDataTypeEnum + 1
};

// An extension range.
typedef struct GPBExtensionRange {
  uint32_t start;  // inclusive
  uint32_t end;    // exclusive
} GPBExtensionRange;
