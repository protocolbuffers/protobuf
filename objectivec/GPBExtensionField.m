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

#import "GPBExtensionField_PackagePrivate.h"

#import <objc/runtime.h>

#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

GPB_INLINE size_t TypeSize(GPBType type) {
  switch (type) {
    case GPBTypeBool:
      return 1;
    case GPBTypeFixed32:
    case GPBTypeSFixed32:
    case GPBTypeFloat:
      return 4;
    case GPBTypeFixed64:
    case GPBTypeSFixed64:
    case GPBTypeDouble:
      return 8;
    default:
      return 0;
  }
}

GPB_INLINE BOOL ExtensionIsRepeated(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionRepeated) != 0;
}

GPB_INLINE BOOL ExtensionIsPacked(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionPacked) != 0;
}

GPB_INLINE BOOL ExtensionIsWireFormat(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionSetWireFormat) != 0;
}

static size_t ComputePBSerializedSizeNoTagOfObject(GPBType type, id object) {
#define FIELD_CASE(TYPE, ACCESSOR) \
  case GPBType##TYPE:              \
    return GPBCompute##TYPE##SizeNoTag([(NSNumber *)object ACCESSOR]);
#define FIELD_CASE2(TYPE) \
  case GPBType##TYPE:     \
    return GPBCompute##TYPE##SizeNoTag(object);
  switch (type) {
    FIELD_CASE(Bool, boolValue)
    FIELD_CASE(Float, floatValue)
    FIELD_CASE(Double, doubleValue)
    FIELD_CASE(Int32, intValue)
    FIELD_CASE(SFixed32, intValue)
    FIELD_CASE(SInt32, intValue)
    FIELD_CASE(Enum, intValue)
    FIELD_CASE(Int64, longLongValue)
    FIELD_CASE(SInt64, longLongValue)
    FIELD_CASE(SFixed64, longLongValue)
    FIELD_CASE(UInt32, unsignedIntValue)
    FIELD_CASE(Fixed32, unsignedIntValue)
    FIELD_CASE(UInt64, unsignedLongLongValue)
    FIELD_CASE(Fixed64, unsignedLongLongValue)
    FIELD_CASE2(Data)
    FIELD_CASE2(String)
    FIELD_CASE2(Message)
    FIELD_CASE2(Group)
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static size_t ComputeSerializedSizeIncludingTagOfObject(
    GPBExtensionDescription *description, id object) {
#define FIELD_CASE(TYPE, ACCESSOR)                          \
  case GPBType##TYPE:                                       \
    return GPBCompute##TYPE##Size(description->fieldNumber, \
                                  [(NSNumber *)object ACCESSOR]);
#define FIELD_CASE2(TYPE) \
  case GPBType##TYPE:     \
    return GPBCompute##TYPE##Size(description->fieldNumber, object);
  switch (description->type) {
    FIELD_CASE(Bool, boolValue)
    FIELD_CASE(Float, floatValue)
    FIELD_CASE(Double, doubleValue)
    FIELD_CASE(Int32, intValue)
    FIELD_CASE(SFixed32, intValue)
    FIELD_CASE(SInt32, intValue)
    FIELD_CASE(Enum, intValue)
    FIELD_CASE(Int64, longLongValue)
    FIELD_CASE(SInt64, longLongValue)
    FIELD_CASE(SFixed64, longLongValue)
    FIELD_CASE(UInt32, unsignedIntValue)
    FIELD_CASE(Fixed32, unsignedIntValue)
    FIELD_CASE(UInt64, unsignedLongLongValue)
    FIELD_CASE(Fixed64, unsignedLongLongValue)
    FIELD_CASE2(Data)
    FIELD_CASE2(String)
    FIELD_CASE2(Group)
    case GPBTypeMessage:
      if (ExtensionIsWireFormat(description)) {
        return GPBComputeMessageSetExtensionSize(description->fieldNumber,
                                                 object);
      } else {
        return GPBComputeMessageSize(description->fieldNumber, object);
      }
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static size_t ComputeSerializedSizeIncludingTagOfArray(
    GPBExtensionDescription *description, NSArray *values) {
  if (ExtensionIsPacked(description)) {
    size_t size = 0;
    size_t typeSize = TypeSize(description->type);
    if (typeSize != 0) {
      size = values.count * typeSize;
    } else {
      for (id value in values) {
        size += ComputePBSerializedSizeNoTagOfObject(description->type, value);
      }
    }
    return size + GPBComputeTagSize(description->fieldNumber) +
           GPBComputeRawVarint32SizeForInteger(size);
  } else {
    size_t size = 0;
    for (id value in values) {
      size += ComputeSerializedSizeIncludingTagOfObject(description, value);
    }
    return size;
  }
}

static void WriteObjectIncludingTagToCodedOutputStream(
    id object, GPBExtensionDescription *description,
    GPBCodedOutputStream *output) {
#define FIELD_CASE(TYPE, ACCESSOR)                      \
  case GPBType##TYPE:                                   \
    [output write##TYPE:description->fieldNumber        \
                  value:[(NSNumber *)object ACCESSOR]]; \
    return;
#define FIELD_CASE2(TYPE)                                       \
  case GPBType##TYPE:                                           \
    [output write##TYPE:description->fieldNumber value:object]; \
    return;
  switch (description->type) {
    FIELD_CASE(Bool, boolValue)
    FIELD_CASE(Float, floatValue)
    FIELD_CASE(Double, doubleValue)
    FIELD_CASE(Int32, intValue)
    FIELD_CASE(SFixed32, intValue)
    FIELD_CASE(SInt32, intValue)
    FIELD_CASE(Enum, intValue)
    FIELD_CASE(Int64, longLongValue)
    FIELD_CASE(SInt64, longLongValue)
    FIELD_CASE(SFixed64, longLongValue)
    FIELD_CASE(UInt32, unsignedIntValue)
    FIELD_CASE(Fixed32, unsignedIntValue)
    FIELD_CASE(UInt64, unsignedLongLongValue)
    FIELD_CASE(Fixed64, unsignedLongLongValue)
    FIELD_CASE2(Data)
    FIELD_CASE2(String)
    FIELD_CASE2(Group)
    case GPBTypeMessage:
      if (ExtensionIsWireFormat(description)) {
        [output writeMessageSetExtension:description->fieldNumber value:object];
      } else {
        [output writeMessage:description->fieldNumber value:object];
      }
      return;
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static void WriteObjectNoTagToCodedOutputStream(
    id object, GPBExtensionDescription *description,
    GPBCodedOutputStream *output) {
#define FIELD_CASE(TYPE, ACCESSOR)                             \
  case GPBType##TYPE:                                          \
    [output write##TYPE##NoTag:[(NSNumber *)object ACCESSOR]]; \
    return;
#define FIELD_CASE2(TYPE)               \
  case GPBType##TYPE:                   \
    [output write##TYPE##NoTag:object]; \
    return;
  switch (description->type) {
    FIELD_CASE(Bool, boolValue)
    FIELD_CASE(Float, floatValue)
    FIELD_CASE(Double, doubleValue)
    FIELD_CASE(Int32, intValue)
    FIELD_CASE(SFixed32, intValue)
    FIELD_CASE(SInt32, intValue)
    FIELD_CASE(Enum, intValue)
    FIELD_CASE(Int64, longLongValue)
    FIELD_CASE(SInt64, longLongValue)
    FIELD_CASE(SFixed64, longLongValue)
    FIELD_CASE(UInt32, unsignedIntValue)
    FIELD_CASE(Fixed32, unsignedIntValue)
    FIELD_CASE(UInt64, unsignedLongLongValue)
    FIELD_CASE(Fixed64, unsignedLongLongValue)
    FIELD_CASE2(Data)
    FIELD_CASE2(String)
    FIELD_CASE2(Message)
    case GPBTypeGroup:
      [output writeGroupNoTag:description->fieldNumber value:object];
      return;
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static void WriteArrayIncludingTagsToCodedOutputStream(
    NSArray *values, GPBExtensionDescription *description,
    GPBCodedOutputStream *output) {
  if (ExtensionIsPacked(description)) {
    [output writeTag:description->fieldNumber
              format:GPBWireFormatLengthDelimited];
    size_t dataSize = 0;
    size_t typeSize = TypeSize(description->type);
    if (typeSize != 0) {
      dataSize = values.count * typeSize;
    } else {
      for (id value in values) {
        dataSize +=
            ComputePBSerializedSizeNoTagOfObject(description->type, value);
      }
    }
    [output writeRawVarintSizeTAs32:dataSize];
    for (id value in values) {
      WriteObjectNoTagToCodedOutputStream(value, description, output);
    }
  } else {
    for (id value in values) {
      WriteObjectIncludingTagToCodedOutputStream(value, description, output);
    }
  }
}

@implementation GPBExtensionField {
  GPBExtensionDescription *description_;
  GPBExtensionDescriptor *descriptor_;
  GPBValue defaultValue_;
}

@synthesize containingType = containingType_;
@synthesize descriptor = descriptor_;

- (instancetype)init {
  // Throw an exception if people attempt to not use the designated initializer.
  self = [super init];
  if (self != nil) {
    [self doesNotRecognizeSelector:_cmd];
    self = nil;
  }
  return self;
}

- (instancetype)initWithDescription:(GPBExtensionDescription *)description {
  if ((self = [super init])) {
    description_ = description;
    if (description->extendedClass) {
      Class containingClass = objc_lookUpClass(description->extendedClass);
      NSAssert1(containingClass, @"Class %s not defined",
                description->extendedClass);
      containingType_ = [containingClass descriptor];
    }
#if DEBUG
    const char *className = description->messageOrGroupClassName;
    if (className) {
      NSAssert1(objc_lookUpClass(className) != Nil, @"Class %s not defined",
                className);
    }
#endif
    descriptor_ = [[GPBExtensionDescriptor alloc]
        initWithExtensionDescription:description];
    GPBType type = description_->type;
    if (type == GPBTypeData) {
      // Data stored as a length prefixed c-string in descriptor records.
      const uint8_t *bytes =
          (const uint8_t *)description->defaultValue.valueData;
      if (bytes) {
        uint32_t length = *((uint32_t *)bytes);
        // The length is stored in network byte order.
        length = ntohl(length);
        bytes += sizeof(length);
        defaultValue_.valueData =
            [[NSData alloc] initWithBytes:bytes length:length];
      }
    } else if (type == GPBTypeMessage || type == GPBTypeGroup) {
      // The default is looked up in -defaultValue instead since extensions
      // aren't
      // common, we avoid the hit startup hit and it avoid initialization order
      // issues.
    } else {
      defaultValue_ = description->defaultValue;
    }
  }
  return self;
}

- (void)dealloc {
  if ((description_->type == GPBTypeData) &&
      !ExtensionIsRepeated(description_)) {
    [defaultValue_.valueData release];
  }
  [descriptor_ release];
  [super dealloc];
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> FieldNumber:%d ContainingType:%@",
                                    [self class], self, self.fieldNumber,
                                    self.containingType];
}

- (id)copyWithZone:(NSZone *)__unused zone {
  return [self retain];
}

#pragma mark Properties

- (int32_t)fieldNumber {
  return description_->fieldNumber;
}

- (GPBWireFormat)wireType {
  return GPBWireFormatForType(description_->type,
                              ExtensionIsPacked(description_));
}

- (BOOL)isRepeated {
  return ExtensionIsRepeated(description_);
}

- (id)defaultValue {
  if (ExtensionIsRepeated(description_)) {
    return nil;
  }

  switch (description_->type) {
    case GPBTypeBool:
      return @(defaultValue_.valueBool);
    case GPBTypeFloat:
      return @(defaultValue_.valueFloat);
    case GPBTypeDouble:
      return @(defaultValue_.valueDouble);
    case GPBTypeInt32:
    case GPBTypeSInt32:
    case GPBTypeEnum:
    case GPBTypeSFixed32:
      return @(defaultValue_.valueInt32);
    case GPBTypeInt64:
    case GPBTypeSInt64:
    case GPBTypeSFixed64:
      return @(defaultValue_.valueInt64);
    case GPBTypeUInt32:
    case GPBTypeFixed32:
      return @(defaultValue_.valueUInt32);
    case GPBTypeUInt64:
    case GPBTypeFixed64:
      return @(defaultValue_.valueUInt64);
    case GPBTypeData:
      // Like message fields, the default is zero length data.
      return (defaultValue_.valueData ? defaultValue_.valueData
                                      : GPBEmptyNSData());
    case GPBTypeString:
      // Like message fields, the default is zero length string.
      return (defaultValue_.valueString ? defaultValue_.valueString : @"");
    case GPBTypeGroup:
    case GPBTypeMessage:
      NSAssert(0, @"Shouldn't get here");
      return nil;
  }
}

#pragma mark Internals

- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input
                extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                          message:(GPBMessage *)message {
  GPBCodedInputStreamState *state = &input->state_;
  if (ExtensionIsPacked(description_)) {
    int32_t length = GPBCodedInputStreamReadInt32(state);
    size_t limit = GPBCodedInputStreamPushLimit(state, length);
    while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
      id value = [self newSingleValueFromCodedInputStream:input
                                        extensionRegistry:extensionRegistry
                                            existingValue:nil];
      [message addExtension:self value:value];
      [value release];
    }
    GPBCodedInputStreamPopLimit(state, limit);
  } else {
    id existingValue = nil;
    BOOL isRepeated = ExtensionIsRepeated(description_);
    if (!isRepeated && GPBTypeIsMessage(description_->type)) {
      existingValue = [message getExistingExtension:self];
    }
    id value = [self newSingleValueFromCodedInputStream:input
                                      extensionRegistry:extensionRegistry
                                          existingValue:existingValue];
    if (isRepeated) {
      [message addExtension:self value:value];
    } else {
      [message setExtension:self value:value];
    }
    [value release];
  }
}

- (void)writeValue:(id)value
    includingTagToCodedOutputStream:(GPBCodedOutputStream *)output {
  if (ExtensionIsRepeated(description_)) {
    WriteArrayIncludingTagsToCodedOutputStream(value, description_, output);
  } else {
    WriteObjectIncludingTagToCodedOutputStream(value, description_, output);
  }
}

- (size_t)computeSerializedSizeIncludingTag:(id)value {
  if (ExtensionIsRepeated(description_)) {
    return ComputeSerializedSizeIncludingTagOfArray(description_, value);
  } else {
    return ComputeSerializedSizeIncludingTagOfObject(description_, value);
  }
}

// Note that this returns a retained value intentionally.
- (id)newSingleValueFromCodedInputStream:(GPBCodedInputStream *)input
                        extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                            existingValue:(GPBMessage *)existingValue {
  GPBCodedInputStreamState *state = &input->state_;
  switch (description_->type) {
    case GPBTypeBool:     return [[NSNumber alloc] initWithBool:GPBCodedInputStreamReadBool(state)];
    case GPBTypeFixed32:  return [[NSNumber alloc] initWithUnsignedInt:GPBCodedInputStreamReadFixed32(state)];
    case GPBTypeSFixed32: return [[NSNumber alloc] initWithInt:GPBCodedInputStreamReadSFixed32(state)];
    case GPBTypeFloat:    return [[NSNumber alloc] initWithFloat:GPBCodedInputStreamReadFloat(state)];
    case GPBTypeFixed64:  return [[NSNumber alloc] initWithUnsignedLongLong:GPBCodedInputStreamReadFixed64(state)];
    case GPBTypeSFixed64: return [[NSNumber alloc] initWithLongLong:GPBCodedInputStreamReadSFixed64(state)];
    case GPBTypeDouble:   return [[NSNumber alloc] initWithDouble:GPBCodedInputStreamReadDouble(state)];
    case GPBTypeInt32:    return [[NSNumber alloc] initWithInt:GPBCodedInputStreamReadInt32(state)];
    case GPBTypeInt64:    return [[NSNumber alloc] initWithLongLong:GPBCodedInputStreamReadInt64(state)];
    case GPBTypeSInt32:   return [[NSNumber alloc] initWithInt:GPBCodedInputStreamReadSInt32(state)];
    case GPBTypeSInt64:   return [[NSNumber alloc] initWithLongLong:GPBCodedInputStreamReadSInt64(state)];
    case GPBTypeUInt32:   return [[NSNumber alloc] initWithUnsignedInt:GPBCodedInputStreamReadUInt32(state)];
    case GPBTypeUInt64:   return [[NSNumber alloc] initWithUnsignedLongLong:GPBCodedInputStreamReadUInt64(state)];
    case GPBTypeData:     return GPBCodedInputStreamReadRetainedData(state);
    case GPBTypeString:   return GPBCodedInputStreamReadRetainedString(state);
    case GPBTypeEnum:     return [[NSNumber alloc] initWithInt:GPBCodedInputStreamReadEnum(state)];
    case GPBTypeGroup:
    case GPBTypeMessage: {
      GPBMessage *message;
      if (existingValue) {
        message = [existingValue retain];
      } else {
        GPBDescriptor *decriptor = [descriptor_.msgClass descriptor];
        message = [[decriptor.messageClass alloc] init];
      }

      if (description_->type == GPBTypeGroup) {
        [input readGroup:description_->fieldNumber
                 message:message
            extensionRegistry:extensionRegistry];
      } else {
        // description_->type == GPBTypeMessage
        if (ExtensionIsWireFormat(description_)) {
          // For MessageSet fields the message length will have already been
          // read.
          [message mergeFromCodedInputStream:input
                           extensionRegistry:extensionRegistry];
        } else {
          [input readMessage:message extensionRegistry:extensionRegistry];
        }
      }

      return message;
    }
  }

  return nil;
}

- (NSComparisonResult)compareByFieldNumber:(GPBExtensionField *)other {
  int32_t selfNumber = description_->fieldNumber;
  int32_t otherNumber = other->description_->fieldNumber;
  if (selfNumber < otherNumber) {
    return NSOrderedAscending;
  } else if (selfNumber == otherNumber) {
    return NSOrderedSame;
  } else {
    return NSOrderedDescending;
  }
}

@end
