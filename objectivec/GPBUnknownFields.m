// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBUnknownFields.h"
#import "GPBUnknownField_PackagePrivate.h"

#define CHECK_FIELD_NUMBER(number)                                                      \
  if (number <= 0) {                                                                    \
    [NSException raise:NSInvalidArgumentException format:@"Not a valid field number."]; \
  }

@implementation GPBUnknownFields {
 @package
  NSMutableArray<GPBUnknownField *> *fields_;
}

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

- (instancetype)init {
  self = [super init];
  if (self) {
    fields_ = [[NSMutableArray alloc] init];
  }
  return self;
}

- (id)copyWithZone:(NSZone *)zone {
  GPBUnknownFields *copy = [[GPBUnknownFields alloc] init];
  // Fields are r/o in this api, so just copy the array.
  copy->fields_ = [fields_ mutableCopyWithZone:zone];
  return copy;
}

- (void)dealloc {
  [fields_ release];
  [super dealloc];
}

- (BOOL)isEqual:(id)object {
  if (![object isKindOfClass:[GPBUnknownFields class]]) {
    return NO;
  }
  GPBUnknownFields *ufs = (GPBUnknownFields *)object;
  // The type is defined with order of fields mattering, so just compare the arrays.
  return [fields_ isEqual:ufs->fields_];
}

- (NSUInteger)hash {
  return [fields_ hash];
}

- (NSString *)description {
  return [NSString
      stringWithFormat:@"<%@ %p>: %lu fields", [self class], self, (unsigned long)fields_.count];
}

#pragma mark - Public Methods

- (NSUInteger)count {
  return fields_.count;
}

- (BOOL)empty {
  return fields_.count == 0;
}

- (void)clear {
  [fields_ removeAllObjects];
}

- (NSArray<GPBUnknownField *> *)fields:(int32_t)fieldNumber {
  CHECK_FIELD_NUMBER(fieldNumber);
  NSMutableArray<GPBUnknownField *> *result = [[NSMutableArray alloc] init];
  for (GPBUnknownField *field in fields_) {
    if (field.number == fieldNumber) {
      [result addObject:field];
    }
  }
  if (result.count == 0) {
    [result release];
    return nil;
  }
  return [result autorelease];
}

- (void)addFieldNumber:(int32_t)fieldNumber varint:(uint64_t)value {
  CHECK_FIELD_NUMBER(fieldNumber);
  GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber varint:value];
  [fields_ addObject:field];
  [field release];
}

- (void)addFieldNumber:(int32_t)fieldNumber fixed32:(uint32_t)value {
  CHECK_FIELD_NUMBER(fieldNumber);
  GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber fixed32:value];
  [fields_ addObject:field];
  [field release];
}

- (void)addFieldNumber:(int32_t)fieldNumber fixed64:(uint64_t)value {
  CHECK_FIELD_NUMBER(fieldNumber);
  GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber fixed64:value];
  [fields_ addObject:field];
  [field release];
}

- (void)addFieldNumber:(int32_t)fieldNumber lengthDelimited:(NSData *)value {
  CHECK_FIELD_NUMBER(fieldNumber);
  GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                   lengthDelimited:value];
  [fields_ addObject:field];
  [field release];
}

- (GPBUnknownFields *)addGroupWithFieldNumber:(int32_t)fieldNumber {
  CHECK_FIELD_NUMBER(fieldNumber);
  GPBUnknownFields *group = [[GPBUnknownFields alloc] init];
  GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber group:group];
  [fields_ addObject:field];
  [field release];
  return [group autorelease];
}

#pragma mark - NSFastEnumeration protocol

- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(__unsafe_unretained id _Nonnull *)stackbuf
                                    count:(NSUInteger)len {
  return [fields_ countByEnumeratingWithState:state objects:stackbuf count:len];
}

@end

@implementation GPBUnknownFields (AccessHelpers)

- (BOOL)getFirst:(int32_t)fieldNumber varint:(nonnull uint64_t *)outValue {
  CHECK_FIELD_NUMBER(fieldNumber);
  for (GPBUnknownField *field in fields_) {
    if (field.number == fieldNumber && field.type == GPBUnknownFieldTypeVarint) {
      *outValue = field.varint;
      return YES;
    }
  }
  return NO;
}

- (BOOL)getFirst:(int32_t)fieldNumber fixed32:(nonnull uint32_t *)outValue {
  CHECK_FIELD_NUMBER(fieldNumber);
  for (GPBUnknownField *field in fields_) {
    if (field.number == fieldNumber && field.type == GPBUnknownFieldTypeFixed32) {
      *outValue = field.fixed32;
      return YES;
    }
  }
  return NO;
}

- (BOOL)getFirst:(int32_t)fieldNumber fixed64:(nonnull uint64_t *)outValue {
  CHECK_FIELD_NUMBER(fieldNumber);
  for (GPBUnknownField *field in fields_) {
    if (field.number == fieldNumber && field.type == GPBUnknownFieldTypeFixed64) {
      *outValue = field.fixed64;
      return YES;
    }
  }
  return NO;
}

- (nullable NSData *)firstLengthDelimited:(int32_t)fieldNumber {
  CHECK_FIELD_NUMBER(fieldNumber);
  for (GPBUnknownField *field in fields_) {
    if (field.number == fieldNumber && field.type == GPBUnknownFieldTypeLengthDelimited) {
      return field.lengthDelimited;
    }
  }
  return nil;
}

- (nullable GPBUnknownFields *)firstGroup:(int32_t)fieldNumber {
  CHECK_FIELD_NUMBER(fieldNumber);
  for (GPBUnknownField *field in fields_) {
    if (field.number == fieldNumber && field.type == GPBUnknownFieldTypeGroup) {
      return field.group;
    }
  }
  return nil;
}

#pragma clang diagnostic pop

@end
