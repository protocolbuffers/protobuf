// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBUnknownField.h"

#import "GPBArray.h"

@class GPBCodedOutputStream;

@interface GPBUnknownField () {
 @package
  int32_t number_;
  GPBUnknownFieldType type_;

  union {
    uint64_t intValue;                 // type == Varint, Fixed32, Fixed64
    NSData *_Nonnull lengthDelimited;  // type == LengthDelimited
    GPBUnknownFields *_Nonnull group;  // type == Group
    struct {                           // type == Legacy
      GPBUInt64Array *_Null_unspecified mutableVarintList;
      GPBUInt32Array *_Null_unspecified mutableFixed32List;
      GPBUInt64Array *_Null_unspecified mutableFixed64List;
      NSMutableArray<NSData *> *_Null_unspecified mutableLengthDelimitedList;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      NSMutableArray<GPBUnknownFieldSet *> *_Null_unspecified mutableGroupList;
#pragma clang diagnostic pop
    } legacy;
  } storage_;
}

- (nonnull instancetype)initWithNumber:(int32_t)number varint:(uint64_t)varint;
- (nonnull instancetype)initWithNumber:(int32_t)number fixed32:(uint32_t)fixed32;
- (nonnull instancetype)initWithNumber:(int32_t)number fixed64:(uint64_t)fixed64;
- (nonnull instancetype)initWithNumber:(int32_t)number lengthDelimited:(nonnull NSData *)data;
- (nonnull instancetype)initWithNumber:(int32_t)number group:(nonnull GPBUnknownFields *)group;

- (void)writeToOutput:(nonnull GPBCodedOutputStream *)output;
- (size_t)serializedSize;

- (void)writeAsMessageSetExtensionToOutput:(nonnull GPBCodedOutputStream *)output;
- (size_t)serializedSizeAsMessageSetExtension;

- (void)mergeFromField:(nonnull GPBUnknownField *)other;

@end
