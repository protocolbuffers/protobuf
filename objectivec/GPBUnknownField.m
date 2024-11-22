// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBUnknownField.h"
#import "GPBUnknownField_PackagePrivate.h"

#import "GPBCodedOutputStream.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBUnknownFields.h"
#import "GPBUnknownFields_PackagePrivate.h"
#import "GPBWireFormat.h"

#define ASSERT_FIELD_TYPE(type)                               \
  if (type_ != type) {                                        \
    [NSException raise:NSInternalInconsistencyException       \
                format:@"GPBUnknownField is the wrong type"]; \
  }

@implementation GPBUnknownField

@synthesize number = number_;
@synthesize type = type_;

- (instancetype)initWithNumber:(int32_t)number varint:(uint64_t)varint {
  if ((self = [super init])) {
    number_ = number;
    type_ = GPBUnknownFieldTypeVarint;
    storage_.intValue = varint;
  }
  return self;
}

- (instancetype)initWithNumber:(int32_t)number fixed32:(uint32_t)fixed32 {
  if ((self = [super init])) {
    number_ = number;
    type_ = GPBUnknownFieldTypeFixed32;
    storage_.intValue = fixed32;
  }
  return self;
}

- (instancetype)initWithNumber:(int32_t)number fixed64:(uint64_t)fixed64 {
  if ((self = [super init])) {
    number_ = number;
    type_ = GPBUnknownFieldTypeFixed64;
    storage_.intValue = fixed64;
  }
  return self;
}

- (instancetype)initWithNumber:(int32_t)number lengthDelimited:(nonnull NSData *)data {
  if ((self = [super init])) {
    number_ = number;
    type_ = GPBUnknownFieldTypeLengthDelimited;
    storage_.lengthDelimited = [data copy];
  }
  return self;
}

- (instancetype)initWithNumber:(int32_t)number group:(nonnull GPBUnknownFields *)group {
  if ((self = [super init])) {
    number_ = number;
    type_ = GPBUnknownFieldTypeGroup;
    // Taking ownership of the group; so retain, not copy.
    storage_.group = [group retain];
  }
  return self;
}

- (void)dealloc {
  switch (type_) {
    case GPBUnknownFieldTypeVarint:
    case GPBUnknownFieldTypeFixed32:
    case GPBUnknownFieldTypeFixed64:
      break;
    case GPBUnknownFieldTypeLengthDelimited:
      [storage_.lengthDelimited release];
      break;
    case GPBUnknownFieldTypeGroup:
      [storage_.group release];
      break;
  }

  [super dealloc];
}

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

- (uint64_t)varint {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeVarint);
  return storage_.intValue;
}

- (uint32_t)fixed32 {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeFixed32);
  return (uint32_t)storage_.intValue;
}

- (uint64_t)fixed64 {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeFixed64);
  return storage_.intValue;
}

- (NSData *)lengthDelimited {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLengthDelimited);
  return storage_.lengthDelimited;
}

- (GPBUnknownFields *)group {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeGroup);
  return storage_.group;
}

- (id)copyWithZone:(NSZone *)zone {
  switch (type_) {
    case GPBUnknownFieldTypeVarint:
    case GPBUnknownFieldTypeFixed32:
    case GPBUnknownFieldTypeFixed64:
    case GPBUnknownFieldTypeLengthDelimited:
      // In these modes, the object isn't mutable, so just return self.
      return [self retain];
    case GPBUnknownFieldTypeGroup: {
      // The `GPBUnknownFields` for the group is mutable, so a new instance of this object and
      // the group is needed.
      GPBUnknownFields *copyGroup = [storage_.group copyWithZone:zone];
      GPBUnknownField *copy = [[GPBUnknownField allocWithZone:zone] initWithNumber:number_
                                                                             group:copyGroup];
      [copyGroup release];
      return copy;
    }
  }
}

- (BOOL)isEqual:(id)object {
  if (self == object) return YES;
  if (![object isKindOfClass:[GPBUnknownField class]]) return NO;
  GPBUnknownField *field = (GPBUnknownField *)object;
  if (number_ != field->number_) return NO;
  if (type_ != field->type_) return NO;
  switch (type_) {
    case GPBUnknownFieldTypeVarint:
    case GPBUnknownFieldTypeFixed32:
    case GPBUnknownFieldTypeFixed64:
      return storage_.intValue == field->storage_.intValue;
    case GPBUnknownFieldTypeLengthDelimited:
      return [storage_.lengthDelimited isEqual:field->storage_.lengthDelimited];
    case GPBUnknownFieldTypeGroup:
      return [storage_.group isEqual:field->storage_.group];
  }
}

- (NSUInteger)hash {
  const int prime = 31;
  NSUInteger result = prime * number_ + type_;
  switch (type_) {
    case GPBUnknownFieldTypeVarint:
    case GPBUnknownFieldTypeFixed32:
    case GPBUnknownFieldTypeFixed64:
      result = prime * result + (NSUInteger)storage_.intValue;
      break;
    case GPBUnknownFieldTypeLengthDelimited:
      result = prime * result + [storage_.lengthDelimited hash];
      break;
    case GPBUnknownFieldTypeGroup:
      result = prime * result + [storage_.group hash];
  }
  return result;
}

- (NSString *)description {
  NSMutableString *description =
      [NSMutableString stringWithFormat:@"<%@ %p>: Field: %d", [self class], self, number_];
  switch (type_) {
    case GPBUnknownFieldTypeVarint:
      [description appendFormat:@" varint: %llu", storage_.intValue];
      break;
    case GPBUnknownFieldTypeFixed32:
      [description appendFormat:@" fixed32: %u", (uint32_t)storage_.intValue];
      break;
    case GPBUnknownFieldTypeFixed64:
      [description appendFormat:@" fixed64: %llu", storage_.intValue];
      break;
    case GPBUnknownFieldTypeLengthDelimited:
      [description appendFormat:@" fixed64: %@", storage_.lengthDelimited];
      break;
    case GPBUnknownFieldTypeGroup:
      [description appendFormat:@" group: %@", storage_.group];
      break;
  }
  return description;
}

#pragma clang diagnostic pop

@end
