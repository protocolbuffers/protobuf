// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBUnknownFields.h"

#import <Foundation/Foundation.h>

#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBMessage.h"
#import "GPBUnknownField.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUnknownField_PackagePrivate.h"
#import "GPBUnknownFields_PackagePrivate.h"
#import "GPBWireFormat.h"

#define CHECK_FIELD_NUMBER(number)                                                      \
  if (number <= 0) {                                                                    \
    [NSException raise:NSInvalidArgumentException format:@"Not a valid field number."]; \
  }

@interface GPBUnknownFields ()
- (BOOL)mergeFromInputStream:(nonnull GPBCodedInputStream *)input endTag:(uint32_t)endTag;
@end

@implementation GPBUnknownFields {
 @package
  NSMutableArray<GPBUnknownField *> *fields_;
}

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

- (instancetype)initFromMessage:(nonnull GPBMessage *)message {
  self = [super init];
  if (self) {
    fields_ = [[NSMutableArray alloc] init];
    // TODO: b/349146447 - Move off the legacy class and directly to the data once Message is
    // updated.
    GPBUnknownFieldSet *legacyUnknownFields = [message unknownFields];
    if (legacyUnknownFields) {
      GPBCodedInputStream *input =
          [[GPBCodedInputStream alloc] initWithData:[legacyUnknownFields data]];
      // Parse until the end of the data (tag will be zero).
      if (![self mergeFromInputStream:input endTag:0]) {
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

#pragma mark - Internal Methods

- (size_t)serializedSize {
  size_t result = 0;
  for (GPBUnknownField *field in self->fields_) {
    result += [field serializedSize];
  }
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)output {
  for (GPBUnknownField *field in fields_) {
    [field writeToOutput:output];
  }
}

- (NSData *)serializeAsData {
  if (fields_.count == 0) {
    return [NSData data];
  }
  size_t expectedSize = [self serializedSize];
  NSMutableData *data = [NSMutableData dataWithLength:expectedSize];
  GPBCodedOutputStream *stream = [[GPBCodedOutputStream alloc] initWithData:data];
  @try {
    [self writeToCodedOutputStream:stream];
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

- (BOOL)mergeFromInputStream:(nonnull GPBCodedInputStream *)input endTag:(uint32_t)endTag {
#if defined(DEBUG) && DEBUG
  NSAssert(endTag == 0 || GPBWireFormatGetTagWireType(endTag) == GPBWireFormatEndGroup,
           @"Internal error:Invalid end tag: %u", endTag);
#endif
  GPBCodedInputStreamState *state = &input->state_;
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
          [fields_ addObject:field];
          [field release];
          break;
        }
        case GPBWireFormatFixed32: {
          uint32_t value = GPBCodedInputStreamReadFixed32(state);
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                                   fixed32:value];
          [fields_ addObject:field];
          [field release];
          break;
        }
        case GPBWireFormatFixed64: {
          uint64_t value = GPBCodedInputStreamReadFixed64(state);
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                                   fixed64:value];
          [fields_ addObject:field];
          [field release];
          break;
        }
        case GPBWireFormatLengthDelimited: {
          NSData *data = GPBCodedInputStreamReadRetainedBytes(state);
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber
                                                           lengthDelimited:data];
          [fields_ addObject:field];
          [field release];
          [data release];
          break;
        }
        case GPBWireFormatStartGroup: {
          GPBUnknownFields *group = [[GPBUnknownFields alloc] init];
          GPBUnknownField *field = [[GPBUnknownField alloc] initWithNumber:fieldNumber group:group];
          [fields_ addObject:field];
          [field release];
          [group release];  // Still will be held in the field/fields_.
          uint32_t endGroupTag = GPBWireFormatMakeTag(fieldNumber, GPBWireFormatEndGroup);
          if ([group mergeFromInputStream:input endTag:endGroupTag]) {
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
