// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBUnknownFields.h"
#import "GPBUnknownFields_PackagePrivate.h"

#import <Foundation/Foundation.h>

#import "GPBCodedInputStream.h"
#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBDescriptor.h"
#import "GPBMessage.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUnknownField.h"
#import "GPBUnknownFieldSet.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUnknownField_PackagePrivate.h"
#import "GPBWireFormat.h"

#define CHECK_FIELD_NUMBER(number)                                                      \
  if (number <= 0) {                                                                    \
    [NSException raise:NSInvalidArgumentException format:@"Not a valid field number."]; \
  }

// TODO: Consider using on other functions to reduce bloat when
// some compiler optimizations are enabled.
#define GPB_NOINLINE __attribute__((noinline))

@interface GPBUnknownFields () {
 @package
  NSMutableArray<GPBUnknownField *> *fields_;
}
@end

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

GPB_NOINLINE
static size_t ComputeSerializeSize(GPBUnknownFields *_Nonnull self) {
  size_t result = 0;
  for (GPBUnknownField *field in self->fields_) {
    uint32_t fieldNumber = field->number_;
    switch (field->type_) {
      case GPBUnknownFieldTypeVarint:
        result += GPBComputeUInt64Size(fieldNumber, field->storage_.intValue);
        break;
      case GPBUnknownFieldTypeFixed32:
        result += GPBComputeFixed32Size(fieldNumber, (uint32_t)field->storage_.intValue);
        break;
      case GPBUnknownFieldTypeFixed64:
        result += GPBComputeFixed64Size(fieldNumber, field->storage_.intValue);
        break;
      case GPBUnknownFieldTypeLengthDelimited:
        result += GPBComputeBytesSize(fieldNumber, field->storage_.lengthDelimited);
        break;
      case GPBUnknownFieldTypeGroup:
        result +=
            (GPBComputeTagSize(fieldNumber) * 2) + ComputeSerializeSize(field->storage_.group);
        break;
      case GPBUnknownFieldTypeLegacy:
#if defined(DEBUG) && DEBUG
        NSCAssert(NO, @"Internal error within the library");
#endif
        break;
    }
  }
  return result;
}

GPB_NOINLINE
static void WriteToCoddedOutputStream(GPBUnknownFields *_Nonnull self,
                                      GPBCodedOutputStream *_Nonnull output) {
  for (GPBUnknownField *field in self->fields_) {
    uint32_t fieldNumber = field->number_;
    switch (field->type_) {
      case GPBUnknownFieldTypeVarint:
        [output writeUInt64:fieldNumber value:field->storage_.intValue];
        break;
      case GPBUnknownFieldTypeFixed32:
        [output writeFixed32:fieldNumber value:(uint32_t)field->storage_.intValue];
        break;
      case GPBUnknownFieldTypeFixed64:
        [output writeFixed64:fieldNumber value:field->storage_.intValue];
        break;
      case GPBUnknownFieldTypeLengthDelimited:
        [output writeBytes:fieldNumber value:field->storage_.lengthDelimited];
        break;
      case GPBUnknownFieldTypeGroup:
        [output writeRawVarint32:GPBWireFormatMakeTag(fieldNumber, GPBWireFormatStartGroup)];
        WriteToCoddedOutputStream(field->storage_.group, output);
        [output writeRawVarint32:GPBWireFormatMakeTag(fieldNumber, GPBWireFormatEndGroup)];
        break;
      case GPBUnknownFieldTypeLegacy:
#if defined(DEBUG) && DEBUG
        NSCAssert(NO, @"Internal error within the library");
#endif
        break;
    }
  }
}

GPB_NOINLINE
static BOOL MergeFromInputStream(GPBUnknownFields *self, GPBCodedInputStream *input,
                                 uint32_t endTag) {
#if defined(DEBUG) && DEBUG
  NSCAssert(endTag == 0 || GPBWireFormatGetTagWireType(endTag) == GPBWireFormatEndGroup,
            @"Internal error:Invalid end tag: %u", endTag);
#endif
  GPBCodedInputStreamState *state = &input->state_;
  NSMutableArray<GPBUnknownField *> *fields = self->fields_;
  @try {
    while (YES) {
      uint32_t tag = GPBCodedInputStreamReadTag(state);
      if (tag == endTag) {
        return YES;
      }
      if (tag == 0) {
        // Reached end of input without finding the end tag.
        return NO;
      }
      GPBWireFormat wireType = GPBWireFormatGetTagWireType(tag);
      int32_t fieldNumber = GPBWireFormatGetTagFieldNumber(tag);
      switch (wireType) {
        case GPBWireFormatVarint: {
          uint64_t value = GPBCodedInputStreamReadInt64(state);
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                                    varint:value];
          [fields addObject:field];
          [field release];
          break;
        }
        case GPBWireFormatFixed32: {
          uint32_t value = GPBCodedInputStreamReadFixed32(state);
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                                   fixed32:value];
          [fields addObject:field];
          [field release];
          break;
        }
        case GPBWireFormatFixed64: {
          uint64_t value = GPBCodedInputStreamReadFixed64(state);
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                                   fixed64:value];
          [fields addObject:field];
          [field release];
          break;
        }
        case GPBWireFormatLengthDelimited: {
          NSData *data = GPBCodedInputStreamReadRetainedBytes(state);
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                           lengthDelimited:data];
          [fields addObject:field];
          [field release];
          [data release];
          break;
        }
        case GPBWireFormatStartGroup: {
          GPBUnknownFields *group = [[GPBUnknownFields alloc] init];
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber group:group];
          [fields addObject:field];
          [field release];
          [group release];  // Still will be held in the field/fields.
          uint32_t endGroupTag = GPBWireFormatMakeTag(fieldNumber, GPBWireFormatEndGroup);
          if (MergeFromInputStream(group, input, endGroupTag)) {
            GPBCodedInputStreamCheckLastTagWas(state, endGroupTag);
          } else {
            [NSException
                 raise:NSInternalInconsistencyException
                format:@"Internal error: Unknown field data for nested group was malformed."];
          }
          break;
        }
        case GPBWireFormatEndGroup:
          [NSException raise:NSInternalInconsistencyException
                      format:@"Unexpected end group tag: %u", tag];
          break;
      }
    }
  } @catch (NSException *exception) {
#if defined(DEBUG) && DEBUG
    NSLog(@"%@: Internal exception while parsing unknown data, this shouldn't happen!: %@",
          [self class], exception);
#endif
  }
}

@implementation GPBUnknownFields

- (instancetype)initFromMessage:(nonnull GPBMessage *)message {
  self = [super init];
  if (self) {
    fields_ = [[NSMutableArray alloc] init];
    // This shouldn't happen with the annotations, but just incase something claiming nonnull
    // does return nil, block it.
    if (!message) {
      [self release];
      [NSException raise:NSInvalidArgumentException format:@"Message cannot be nil"];
    }
    NSData *data = GPBMessageUnknownFieldsData(message);
    if (data) {
      GPBCodedInputStream *input = [[GPBCodedInputStream alloc] initWithData:data];
      // Parse until the end of the data (tag will be zero).
      if (!MergeFromInputStream(self, input, 0)) {
        [input release];
        [self release];
        [NSException raise:NSInternalInconsistencyException
                    format:@"Internal error: Unknown field data from message was malformed."];
      }
      [input release];
    }
  }
  return self;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    fields_ = [[NSMutableArray alloc] init];
  }
  return self;
}

- (id)copyWithZone:(NSZone *)zone {
  GPBUnknownFields *copy = [[GPBUnknownFields allocWithZone:zone] init];
  copy->fields_ = [[NSMutableArray allocWithZone:zone] initWithArray:fields_ copyItems:YES];
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

- (GPBUnknownField *)addCopyOfField:(nonnull GPBUnknownField *)field {
  if (field->type_ == GPBUnknownFieldTypeLegacy) {
    [NSException raise:NSInternalInconsistencyException
                format:@"GPBUnknownField is the wrong type"];
  }
  GPBUnknownField *result = [field copy];
  [fields_ addObject:result];
  return [result autorelease];
}

- (void)removeField:(nonnull GPBUnknownField *)field {
  NSUInteger count = fields_.count;
  [fields_ removeObjectIdenticalTo:field];
  if (count == fields_.count) {
    [NSException raise:NSInvalidArgumentException format:@"The field was not present."];
  }
}

- (void)clearFieldNumber:(int32_t)fieldNumber {
  CHECK_FIELD_NUMBER(fieldNumber);
  NSMutableIndexSet *toRemove = nil;
  NSUInteger idx = 0;
  for (GPBUnknownField *field in fields_) {
    if (field->number_ == fieldNumber) {
      if (toRemove == nil) {
        toRemove = [[NSMutableIndexSet alloc] initWithIndex:idx];
      } else {
        [toRemove addIndex:idx];
      }
    }
    ++idx;
  }
  if (toRemove) {
    [fields_ removeObjectsAtIndexes:toRemove];
    [toRemove release];
  }
}

#pragma mark - NSFastEnumeration protocol

- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(__unsafe_unretained id _Nonnull *)stackbuf
                                    count:(NSUInteger)len {
  return [fields_ countByEnumeratingWithState:state objects:stackbuf count:len];
}

#pragma mark - Internal Methods

- (NSData *)serializeAsData {
  if (fields_.count == 0) {
    return [NSData data];
  }
  size_t expectedSize = ComputeSerializeSize(self);
  NSMutableData *data = [NSMutableData dataWithLength:expectedSize];
  GPBCodedOutputStream *stream = [[GPBCodedOutputStream alloc] initWithData:data];
  @try {
    WriteToCoddedOutputStream(self, stream);
    [stream flush];
  } @catch (NSException *exception) {
#if defined(DEBUG) && DEBUG
    NSLog(@"Internal exception while building GPBUnknownFields serialized data: %@", exception);
#endif
  }
#if defined(DEBUG) && DEBUG
  NSAssert([stream bytesWritten] == expectedSize, @"Internal error within the library");
#endif
  [stream release];
  return data;
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

@end

#pragma clang diagnostic pop
