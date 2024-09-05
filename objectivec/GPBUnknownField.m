// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBUnknownField.h"
#import "GPBUnknownField_PackagePrivate.h"

#import "GPBArray.h"
#import "GPBCodedOutputStream.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBUnknownFieldSet.h"
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

- (instancetype)initWithNumber:(int32_t)number {
  if ((self = [super init])) {
    number_ = number;
    type_ = GPBUnknownFieldTypeLegacy;
  }
  return self;
}

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
    case GPBUnknownFieldTypeLegacy:
      [storage_.legacy.mutableVarintList release];
      [storage_.legacy.mutableFixed32List release];
      [storage_.legacy.mutableFixed64List release];
      [storage_.legacy.mutableLengthDelimitedList release];
      [storage_.legacy.mutableGroupList release];
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

- (GPBUInt64Array *)varintList {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  return storage_.legacy.mutableVarintList;
}

- (GPBUInt32Array *)fixed32List {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  return storage_.legacy.mutableFixed32List;
}

- (GPBUInt64Array *)fixed64List {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  return storage_.legacy.mutableFixed64List;
}

- (NSArray<NSData *> *)lengthDelimitedList {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  return storage_.legacy.mutableLengthDelimitedList;
}

- (NSArray<GPBUnknownFieldSet *> *)groupList {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  return storage_.legacy.mutableGroupList;
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
    case GPBUnknownFieldTypeLegacy: {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      GPBUnknownField *result = [[GPBUnknownField allocWithZone:zone] initWithNumber:number_];
      result->storage_.legacy.mutableFixed32List =
          [storage_.legacy.mutableFixed32List copyWithZone:zone];
      result->storage_.legacy.mutableFixed64List =
          [storage_.legacy.mutableFixed64List copyWithZone:zone];
      result->storage_.legacy.mutableLengthDelimitedList =
          [storage_.legacy.mutableLengthDelimitedList mutableCopyWithZone:zone];
      result->storage_.legacy.mutableVarintList =
          [storage_.legacy.mutableVarintList copyWithZone:zone];
      if (storage_.legacy.mutableGroupList.count) {
        result->storage_.legacy.mutableGroupList = [[NSMutableArray allocWithZone:zone]
            initWithCapacity:storage_.legacy.mutableGroupList.count];
        for (GPBUnknownFieldSet *group in storage_.legacy.mutableGroupList) {
          GPBUnknownFieldSet *copied = [group copyWithZone:zone];
          [result->storage_.legacy.mutableGroupList addObject:copied];
          [copied release];
        }
      }
#pragma clang diagnostic pop
      return result;
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
    case GPBUnknownFieldTypeLegacy: {
      BOOL equalVarint =
          (storage_.legacy.mutableVarintList.count == 0 &&
           field->storage_.legacy.mutableVarintList.count == 0) ||
          [storage_.legacy.mutableVarintList isEqual:field->storage_.legacy.mutableVarintList];
      if (!equalVarint) return NO;
      BOOL equalFixed32 =
          (storage_.legacy.mutableFixed32List.count == 0 &&
           field->storage_.legacy.mutableFixed32List.count == 0) ||
          [storage_.legacy.mutableFixed32List isEqual:field->storage_.legacy.mutableFixed32List];
      if (!equalFixed32) return NO;
      BOOL equalFixed64 =
          (storage_.legacy.mutableFixed64List.count == 0 &&
           field->storage_.legacy.mutableFixed64List.count == 0) ||
          [storage_.legacy.mutableFixed64List isEqual:field->storage_.legacy.mutableFixed64List];
      if (!equalFixed64) return NO;
      BOOL equalLDList = (storage_.legacy.mutableLengthDelimitedList.count == 0 &&
                          field->storage_.legacy.mutableLengthDelimitedList.count == 0) ||
                         [storage_.legacy.mutableLengthDelimitedList
                             isEqual:field->storage_.legacy.mutableLengthDelimitedList];
      if (!equalLDList) return NO;
      BOOL equalGroupList =
          (storage_.legacy.mutableGroupList.count == 0 &&
           field->storage_.legacy.mutableGroupList.count == 0) ||
          [storage_.legacy.mutableGroupList isEqual:field->storage_.legacy.mutableGroupList];
      if (!equalGroupList) return NO;
      return YES;
    }
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
    case GPBUnknownFieldTypeLegacy:
      result = prime * result + [storage_.legacy.mutableVarintList hash];
      result = prime * result + [storage_.legacy.mutableFixed32List hash];
      result = prime * result + [storage_.legacy.mutableFixed64List hash];
      result = prime * result + [storage_.legacy.mutableLengthDelimitedList hash];
      result = prime * result + [storage_.legacy.mutableGroupList hash];
      break;
  }
  return result;
}

- (void)writeToOutput:(GPBCodedOutputStream *)output {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  NSUInteger count = storage_.legacy.mutableVarintList.count;
  if (count > 0) {
    [output writeUInt64Array:number_ values:storage_.legacy.mutableVarintList tag:0];
  }
  count = storage_.legacy.mutableFixed32List.count;
  if (count > 0) {
    [output writeFixed32Array:number_ values:storage_.legacy.mutableFixed32List tag:0];
  }
  count = storage_.legacy.mutableFixed64List.count;
  if (count > 0) {
    [output writeFixed64Array:number_ values:storage_.legacy.mutableFixed64List tag:0];
  }
  count = storage_.legacy.mutableLengthDelimitedList.count;
  if (count > 0) {
    [output writeBytesArray:number_ values:storage_.legacy.mutableLengthDelimitedList];
  }
  count = storage_.legacy.mutableGroupList.count;
  if (count > 0) {
    [output writeUnknownGroupArray:number_ values:storage_.legacy.mutableGroupList];
  }
#pragma clang diagnostic pop
}

- (size_t)serializedSize {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  __block size_t result = 0;
  int32_t number = number_;
  [storage_.legacy.mutableVarintList
      enumerateValuesWithBlock:^(uint64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
        result += GPBComputeUInt64Size(number, value);
      }];

  [storage_.legacy.mutableFixed32List
      enumerateValuesWithBlock:^(uint32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
        result += GPBComputeFixed32Size(number, value);
      }];

  [storage_.legacy.mutableFixed64List
      enumerateValuesWithBlock:^(uint64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
        result += GPBComputeFixed64Size(number, value);
      }];

  for (NSData *data in storage_.legacy.mutableLengthDelimitedList) {
    result += GPBComputeBytesSize(number, data);
  }

  for (GPBUnknownFieldSet *set in storage_.legacy.mutableGroupList) {
    result += GPBComputeUnknownGroupSize(number, set);
  }
#pragma clang diagnostic pop

  return result;
}

- (void)writeAsMessageSetExtensionToOutput:(GPBCodedOutputStream *)output {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  for (NSData *data in storage_.legacy.mutableLengthDelimitedList) {
    [output writeRawMessageSetExtension:number_ value:data];
  }
}

- (size_t)serializedSizeAsMessageSetExtension {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  size_t result = 0;
  for (NSData *data in storage_.legacy.mutableLengthDelimitedList) {
    result += GPBComputeRawMessageSetExtensionSize(number_, data);
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
    case GPBUnknownFieldTypeLegacy:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      [description appendString:@" {\n"];
      [storage_.legacy.mutableVarintList
          enumerateValuesWithBlock:^(uint64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
            [description appendFormat:@"\t%llu\n", value];
          }];
      [storage_.legacy.mutableFixed32List
          enumerateValuesWithBlock:^(uint32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
            [description appendFormat:@"\t%u\n", value];
          }];
      [storage_.legacy.mutableFixed64List
          enumerateValuesWithBlock:^(uint64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
            [description appendFormat:@"\t%llu\n", value];
          }];
      for (NSData *data in storage_.legacy.mutableLengthDelimitedList) {
        [description appendFormat:@"\t%@\n", data];
      }
      for (GPBUnknownFieldSet *set in storage_.legacy.mutableGroupList) {
        [description appendFormat:@"\t%@\n", set];
      }
      [description appendString:@"}"];
#pragma clang diagnostic pop
      break;
  }
  return description;
}

- (void)mergeFromField:(GPBUnknownField *)other {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  GPBUInt64Array *otherVarintList = other.varintList;
  if (otherVarintList.count > 0) {
    if (storage_.legacy.mutableVarintList == nil) {
      storage_.legacy.mutableVarintList = [otherVarintList copy];
    } else {
      [storage_.legacy.mutableVarintList addValuesFromArray:otherVarintList];
    }
  }

  GPBUInt32Array *otherFixed32List = other.fixed32List;
  if (otherFixed32List.count > 0) {
    if (storage_.legacy.mutableFixed32List == nil) {
      storage_.legacy.mutableFixed32List = [otherFixed32List copy];
    } else {
      [storage_.legacy.mutableFixed32List addValuesFromArray:otherFixed32List];
    }
  }

  GPBUInt64Array *otherFixed64List = other.fixed64List;
  if (otherFixed64List.count > 0) {
    if (storage_.legacy.mutableFixed64List == nil) {
      storage_.legacy.mutableFixed64List = [otherFixed64List copy];
    } else {
      [storage_.legacy.mutableFixed64List addValuesFromArray:otherFixed64List];
    }
  }

  NSArray *otherLengthDelimitedList = other.lengthDelimitedList;
  if (otherLengthDelimitedList.count > 0) {
    if (storage_.legacy.mutableLengthDelimitedList == nil) {
      storage_.legacy.mutableLengthDelimitedList = [otherLengthDelimitedList mutableCopy];
    } else {
      [storage_.legacy.mutableLengthDelimitedList addObjectsFromArray:otherLengthDelimitedList];
    }
  }

  NSArray *otherGroupList = other.groupList;
  if (otherGroupList.count > 0) {
    if (storage_.legacy.mutableGroupList == nil) {
      storage_.legacy.mutableGroupList =
          [[NSMutableArray alloc] initWithCapacity:otherGroupList.count];
    }
    // Make our own mutable copies.
    for (GPBUnknownFieldSet *group in otherGroupList) {
      GPBUnknownFieldSet *copied = [group copy];
      [storage_.legacy.mutableGroupList addObject:copied];
      [copied release];
    }
  }
#pragma clang diagnostic pop
}

- (void)addVarint:(uint64_t)value {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  if (storage_.legacy.mutableVarintList == nil) {
    storage_.legacy.mutableVarintList = [[GPBUInt64Array alloc] initWithValues:&value count:1];
  } else {
    [storage_.legacy.mutableVarintList addValue:value];
  }
}

- (void)addFixed32:(uint32_t)value {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  if (storage_.legacy.mutableFixed32List == nil) {
    storage_.legacy.mutableFixed32List = [[GPBUInt32Array alloc] initWithValues:&value count:1];
  } else {
    [storage_.legacy.mutableFixed32List addValue:value];
  }
}

- (void)addFixed64:(uint64_t)value {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  if (storage_.legacy.mutableFixed64List == nil) {
    storage_.legacy.mutableFixed64List = [[GPBUInt64Array alloc] initWithValues:&value count:1];
  } else {
    [storage_.legacy.mutableFixed64List addValue:value];
  }
}

- (void)addLengthDelimited:(NSData *)value {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  if (storage_.legacy.mutableLengthDelimitedList == nil) {
    storage_.legacy.mutableLengthDelimitedList = [[NSMutableArray alloc] initWithObjects:&value
                                                                                   count:1];
  } else {
    [storage_.legacy.mutableLengthDelimitedList addObject:value];
  }
}

- (void)addGroup:(GPBUnknownFieldSet *)value {
  ASSERT_FIELD_TYPE(GPBUnknownFieldTypeLegacy);
  if (storage_.legacy.mutableGroupList == nil) {
    storage_.legacy.mutableGroupList = [[NSMutableArray alloc] initWithObjects:&value count:1];
  } else {
    [storage_.legacy.mutableGroupList addObject:value];
  }
}

#pragma clang diagnostic pop

@end
