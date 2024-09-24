// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBMessage.h"
#import "GPBMessage_PackagePrivate.h"

#import <Foundation/Foundation.h>
#import <objc/message.h>
#import <objc/runtime.h>
#import <os/lock.h>
#import <stdatomic.h>

#import "GPBArray.h"
#import "GPBArray_PackagePrivate.h"
#import "GPBCodedInputStream.h"
#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBDescriptor.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBDictionary.h"
#import "GPBDictionary_PackagePrivate.h"
#import "GPBExtensionInternals.h"
#import "GPBExtensionRegistry.h"
#import "GPBRootObject.h"
#import "GPBRootObject_PackagePrivate.h"
#import "GPBUnknownField.h"
#import "GPBUnknownFieldSet.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUnknownFields.h"
#import "GPBUnknownFields_PackagePrivate.h"
#import "GPBUtilities.h"
#import "GPBUtilities_PackagePrivate.h"

// TODO: Consider using on other functions to reduce bloat when
// some compiler optimizations are enabled.
#define GPB_NOINLINE __attribute__((noinline))

// Returns a new instance that was automatically created by |autocreator| for
// its field |field|.
static GPBMessage *GPBCreateMessageWithAutocreator(Class msgClass, GPBMessage *autocreator,
                                                   GPBFieldDescriptor *field)
    __attribute__((ns_returns_retained));

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

NSString *const GPBMessageErrorDomain = GPBNSStringifySymbol(GPBMessageErrorDomain);

NSString *const GPBErrorReasonKey = @"Reason";

static NSString *const kGPBDataCoderKey = @"GPBData";

// Length-delimited has a max size of 2GB, and thus messages do also.
// src/google/protobuf/message_lite also does this enforcement on the C++ side. Validation for
// parsing is done with GPBCodedInputStream; but for messages, it is less checks to do it within
// the message side since the input stream code calls these same bottlenecks.
// https://protobuf.dev/programming-guides/encoding/#cheat-sheet
static const size_t kMaximumMessageSize = 0x7fffffff;

NSString *const GPBMessageExceptionMessageTooLarge =
    GPBNSStringifySymbol(GPBMessageExceptionMessageTooLarge);

//
// PLEASE REMEMBER:
//
// This is the base class for *all* messages generated, so any selector defined,
// *public* or *private* could end up colliding with a proto message field. So
// avoid using selectors that could match a property, use C functions to hide
// them, etc.
//

@interface GPBMessage () {
 @package
  // Only one of these two is ever set, GPBUnknownFieldSet is being deprecated and will
  // eventually be removed, but because that api support mutation, once the property is
  // fetch it must continue to be used so any mutations will be honored in future operations
  // on the message.
  // Read only operations that access these two/cause things to migration between them should
  // be protected with an @synchronized(self) block (that way the code also doesn't have to
  // worry about throws).
  NSMutableData *unknownFieldData_;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  GPBUnknownFieldSet *unknownFields_;
#pragma clang diagnostic pop

  NSMutableDictionary *extensionMap_;
  // Readonly access to autocreatedExtensionMap_ is protected via readOnlyLock_.
  NSMutableDictionary *autocreatedExtensionMap_;

  // If the object was autocreated, we remember the creator so that if we get
  // mutated, we can inform the creator to make our field visible.
  GPBMessage *autocreator_;
  GPBFieldDescriptor *autocreatorField_;
  GPBExtensionDescriptor *autocreatorExtension_;

  // Messages can only be mutated from one thread. But some *readonly* operations modify internal
  // state because they autocreate things. The autocreatedExtensionMap_ is one such structure.
  // Access during readonly operations is protected via this lock.
  //
  // Long ago, this was an OSSpinLock, but then it came to light that there were issues for that on
  // iOS:
  //   http://mjtsai.com/blog/2015/12/16/osspinlock-is-unsafe/
  //   https://lists.swift.org/pipermail/swift-dev/Week-of-Mon-20151214/000372.html
  // It was changed to a dispatch_semaphore_t, but that has potential for priority inversion issues.
  // The minOS versions are now high enough that os_unfair_lock can be used, and should provide
  // all the support we need. For more information in the concurrency/locking space see:
  //   https://gist.github.com/tclementdev/6af616354912b0347cdf6db159c37057
  //   https://developer.apple.com/library/archive/documentation/Performance/Conceptual/EnergyGuide-iOS/PrioritizeWorkWithQoS.html
  //   https://developer.apple.com/videos/play/wwdc2017/706/
  os_unfair_lock readOnlyLock_;
}
@end

static id CreateArrayForField(GPBFieldDescriptor *field, GPBMessage *autocreator)
    __attribute__((ns_returns_retained));
static id GetOrCreateArrayIvarWithField(GPBMessage *self, GPBFieldDescriptor *field);
static id GetArrayIvarWithField(GPBMessage *self, GPBFieldDescriptor *field);
static id CreateMapForField(GPBFieldDescriptor *field, GPBMessage *autocreator)
    __attribute__((ns_returns_retained));
static id GetOrCreateMapIvarWithField(GPBMessage *self, GPBFieldDescriptor *field);
static id GetMapIvarWithField(GPBMessage *self, GPBFieldDescriptor *field);
static NSMutableDictionary *CloneExtensionMap(NSDictionary *extensionMap, NSZone *zone)
    __attribute__((ns_returns_retained));

#if defined(DEBUG) && DEBUG
static NSError *MessageError(NSInteger code, NSDictionary *userInfo) {
  return [NSError errorWithDomain:GPBMessageErrorDomain code:code userInfo:userInfo];
}
#endif

static NSError *ErrorFromException(NSException *exception) {
  NSError *error = nil;

  if ([exception.name isEqual:GPBCodedInputStreamException]) {
    NSDictionary *exceptionInfo = exception.userInfo;
    error = exceptionInfo[GPBCodedInputStreamUnderlyingErrorKey];
  }

  if (!error) {
    NSString *reason = exception.reason;
    NSDictionary *userInfo = nil;
    if ([reason length]) {
      userInfo = @{GPBErrorReasonKey : reason};
    }

    error = [NSError errorWithDomain:GPBMessageErrorDomain
                                code:GPBMessageErrorCodeOther
                            userInfo:userInfo];
  }
  return error;
}

// Helper to encode varints onto the mutable data, the max size need is 10 bytes.
GPB_NOINLINE
static uint8_t *EncodeVarintU64(uint64_t val, uint8_t *ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

// Helper to encode varints onto the mutable data, the max size need is 5 bytes.
GPB_NOINLINE
static uint8_t *EncodeVarintU32(uint32_t val, uint8_t *ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

// Helper to encode signed int32 values as varints onto the mutable data, the max size need is 10
// bytes.
GPB_NOINLINE
static uint8_t *EncodeVarintS32(int32_t val, uint8_t *ptr) {
  if (val >= 0) {
    return EncodeVarintU32((uint32_t)val, ptr);
  } else {
    // Must sign-extend
    int64_t extended = val;
    return EncodeVarintU64((uint64_t)extended, ptr);
  }
}

GPB_NOINLINE
static void AddUnknownFieldVarint32(GPBMessage *self, uint32_t fieldNumber, int32_t value) {
  if (self->unknownFields_) {
    [self->unknownFields_ mergeVarintField:fieldNumber value:value];
    return;
  }
  uint8_t buf[20];
  uint8_t *ptr = buf;
  ptr = EncodeVarintU32(GPBWireFormatMakeTag(fieldNumber, GPBWireFormatVarint), ptr);
  ptr = EncodeVarintS32(value, ptr);

  if (self->unknownFieldData_ == nil) {
    self->unknownFieldData_ = [[NSMutableData alloc] initWithCapacity:ptr - buf];
    GPBBecomeVisibleToAutocreator(self);
  }
  [self->unknownFieldData_ appendBytes:buf length:ptr - buf];
}

GPB_NOINLINE
static void AddUnknownFieldLengthDelimited(GPBMessage *self, uint32_t fieldNumber, NSData *value) {
  if (self->unknownFields_) {
    [self->unknownFields_ mergeLengthDelimited:fieldNumber value:value];
    return;
  }
  uint8_t buf[20];
  uint8_t *ptr = buf;
  ptr = EncodeVarintU32(GPBWireFormatMakeTag(fieldNumber, GPBWireFormatLengthDelimited), ptr);
  ptr = EncodeVarintU64((uint64_t)value.length, ptr);

  if (self->unknownFieldData_ == nil) {
    self->unknownFieldData_ = [[NSMutableData alloc] initWithCapacity:(ptr - buf) + value.length];
    GPBBecomeVisibleToAutocreator(self);
  }
  [self->unknownFieldData_ appendBytes:buf length:ptr - buf];
  [self->unknownFieldData_ appendData:value];
}

GPB_NOINLINE
static void AddUnknownMessageSetEntry(GPBMessage *self, uint32_t typeId, NSData *value) {
  if (self->unknownFields_) {
    // Legacy Set does this odd storage for MessageSet.
    [self->unknownFields_ mergeLengthDelimited:typeId value:value];
    return;
  }

  uint8_t buf[60];
  uint8_t *ptr = buf;
  ptr = EncodeVarintU32(GPBWireFormatMessageSetItemTag, ptr);
  ptr = EncodeVarintU32(GPBWireFormatMessageSetTypeIdTag, ptr);
  ptr = EncodeVarintU32(typeId, ptr);
  ptr = EncodeVarintU32(GPBWireFormatMessageSetMessageTag, ptr);
  ptr = EncodeVarintU64((uint64_t)value.length, ptr);
  uint8_t *split = ptr;

  ptr = EncodeVarintU32(GPBWireFormatMessageSetItemEndTag, ptr);
  uint8_t *end = ptr;

  if (self->unknownFieldData_ == nil) {
    self->unknownFieldData_ = [[NSMutableData alloc] initWithCapacity:(end - buf) + value.length];
    GPBBecomeVisibleToAutocreator(self);
  }
  [self->unknownFieldData_ appendBytes:buf length:split - buf];
  [self->unknownFieldData_ appendData:value];
  [self->unknownFieldData_ appendBytes:split length:end - split];
}

GPB_NOINLINE
static void ParseUnknownField(GPBMessage *self, uint32_t tag, GPBCodedInputStream *input) {
  if (self->unknownFields_) {
    if (![self->unknownFields_ mergeFieldFrom:tag input:input]) {
      GPBRaiseStreamError(GPBCodedInputStreamErrorInvalidTag, @"Unexpected end-group tag");
    }
    return;
  }

  uint8_t buf[20];
  uint8_t *ptr = buf;
  ptr = EncodeVarintU32(tag, ptr);  // All will need the tag
  NSData *bytesToAppend = nil;

  GPBCodedInputStreamState *state = &input->state_;

  switch (GPBWireFormatGetTagWireType(tag)) {
    case GPBWireFormatVarint: {
      ptr = EncodeVarintU64(GPBCodedInputStreamReadUInt64(state), ptr);
      break;
    }
    case GPBWireFormatFixed64: {
      uint64_t value = GPBCodedInputStreamReadFixed64(state);
      *(ptr++) = (uint8_t)(value) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 8) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 16) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 24) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 32) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 40) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 48) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 56) & 0xFF;
      break;
    }
    case GPBWireFormatLengthDelimited: {
      bytesToAppend = GPBCodedInputStreamReadRetainedBytes(state);
      ptr = EncodeVarintU64((uint64_t)bytesToAppend.length, ptr);
      break;
    }
    case GPBWireFormatStartGroup: {
      bytesToAppend = GPBCodedInputStreamReadRetainedBytesToEndGroupNoCopy(
          state, GPBWireFormatGetTagFieldNumber(tag));
      break;
    }
    case GPBWireFormatEndGroup:
      GPBRaiseStreamError(GPBCodedInputStreamErrorInvalidTag, @"Unexpected end-group tag");
      break;
    case GPBWireFormatFixed32: {
      uint32_t value = GPBCodedInputStreamReadFixed32(state);
      *(ptr++) = (uint8_t)(value) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 8) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 16) & 0xFF;
      *(ptr++) = (uint8_t)(value >> 24) & 0xFF;
      break;
    }
  }

  if (self->unknownFieldData_ == nil) {
    self->unknownFieldData_ =
        [[NSMutableData alloc] initWithCapacity:(ptr - buf) + bytesToAppend.length];
    GPBBecomeVisibleToAutocreator(self);
  }

  [self->unknownFieldData_ appendBytes:buf length:ptr - buf];
  if (bytesToAppend) {
    [self->unknownFieldData_ appendData:bytesToAppend];
    [bytesToAppend release];
  }
}

static void CheckExtension(GPBMessage *self, GPBExtensionDescriptor *extension) {
  if (![self isKindOfClass:extension.containingMessageClass]) {
    [NSException raise:NSInvalidArgumentException
                format:@"Extension %@ used on wrong class (%@ instead of %@)",
                       extension.singletonName, [self class], extension.containingMessageClass];
  }
}

static NSMutableDictionary *CloneExtensionMap(NSDictionary *extensionMap, NSZone *zone) {
  if (extensionMap.count == 0) {
    return nil;
  }
  NSMutableDictionary *result =
      [[NSMutableDictionary allocWithZone:zone] initWithCapacity:extensionMap.count];

  for (GPBExtensionDescriptor *extension in extensionMap) {
    id value = [extensionMap objectForKey:extension];
    BOOL isMessageExtension = GPBExtensionIsMessage(extension);

    if (extension.repeated) {
      if (isMessageExtension) {
        NSMutableArray *list = [[NSMutableArray alloc] initWithCapacity:[value count]];
        for (GPBMessage *listValue in value) {
          GPBMessage *copiedValue = [listValue copyWithZone:zone];
          [list addObject:copiedValue];
          [copiedValue release];
        }
        [result setObject:list forKey:extension];
        [list release];
      } else {
        NSMutableArray *copiedValue = [value mutableCopyWithZone:zone];
        [result setObject:copiedValue forKey:extension];
        [copiedValue release];
      }
    } else {
      if (isMessageExtension) {
        GPBMessage *copiedValue = [value copyWithZone:zone];
        [result setObject:copiedValue forKey:extension];
        [copiedValue release];
      } else {
        [result setObject:value forKey:extension];
      }
    }
  }

  return result;
}

static id CreateArrayForField(GPBFieldDescriptor *field, GPBMessage *autocreator) {
  id result;
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
    case GPBDataTypeBool:
      result = [[GPBBoolArray alloc] init];
      break;
    case GPBDataTypeFixed32:
    case GPBDataTypeUInt32:
      result = [[GPBUInt32Array alloc] init];
      break;
    case GPBDataTypeInt32:
    case GPBDataTypeSFixed32:
    case GPBDataTypeSInt32:
      result = [[GPBInt32Array alloc] init];
      break;
    case GPBDataTypeFixed64:
    case GPBDataTypeUInt64:
      result = [[GPBUInt64Array alloc] init];
      break;
    case GPBDataTypeInt64:
    case GPBDataTypeSFixed64:
    case GPBDataTypeSInt64:
      result = [[GPBInt64Array alloc] init];
      break;
    case GPBDataTypeFloat:
      result = [[GPBFloatArray alloc] init];
      break;
    case GPBDataTypeDouble:
      result = [[GPBDoubleArray alloc] init];
      break;

    case GPBDataTypeEnum:
      result = [[GPBEnumArray alloc] initWithValidationFunction:field.enumDescriptor.enumVerifier];
      break;

    case GPBDataTypeBytes:
    case GPBDataTypeGroup:
    case GPBDataTypeMessage:
    case GPBDataTypeString:
      if (autocreator) {
        result = [[GPBAutocreatedArray alloc] init];
      } else {
        result = [[NSMutableArray alloc] init];
      }
      break;
  }

  if (autocreator) {
    if (GPBDataTypeIsObject(fieldDataType)) {
      GPBAutocreatedArray *autoArray = result;
      autoArray->_autocreator = autocreator;
    } else {
      GPBInt32Array *gpbArray = result;
      gpbArray->_autocreator = autocreator;
    }
  }

  return result;
}

static id CreateMapForField(GPBFieldDescriptor *field, GPBMessage *autocreator) {
  id result;
  GPBDataType keyDataType = field.mapKeyDataType;
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  switch (keyDataType) {
    case GPBDataTypeBool:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBBoolBoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBBoolUInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBBoolInt32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBBoolUInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBBoolInt64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBBoolFloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBBoolDoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBBoolEnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBBoolObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeFixed32:
    case GPBDataTypeUInt32:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBUInt32BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBUInt32UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBUInt32Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBUInt32UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBUInt32Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBUInt32FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBUInt32DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBUInt32EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBUInt32ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeInt32:
    case GPBDataTypeSFixed32:
    case GPBDataTypeSInt32:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBInt32BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBInt32UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBInt32Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBInt32UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBInt32Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBInt32FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBInt32DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBInt32EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBInt32ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeFixed64:
    case GPBDataTypeUInt64:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBUInt64BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBUInt64UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBUInt64Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBUInt64UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBUInt64Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBUInt64FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBUInt64DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBUInt64EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBUInt64ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeInt64:
    case GPBDataTypeSFixed64:
    case GPBDataTypeSInt64:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBInt64BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBInt64UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBInt64Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBInt64UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBInt64Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBInt64FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBInt64DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBInt64EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBInt64ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeString:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBStringBoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBStringUInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBStringInt32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBStringUInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBStringInt64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBStringFloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBStringDoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBStringEnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          if (autocreator) {
            result = [[GPBAutocreatedDictionary alloc] init];
          } else {
            result = [[NSMutableDictionary alloc] init];
          }
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;

    case GPBDataTypeFloat:
    case GPBDataTypeDouble:
    case GPBDataTypeEnum:
    case GPBDataTypeBytes:
    case GPBDataTypeGroup:
    case GPBDataTypeMessage:
      NSCAssert(NO, @"shouldn't happen");
      return nil;
  }

  if (autocreator) {
    if ((keyDataType == GPBDataTypeString) && GPBDataTypeIsObject(valueDataType)) {
      GPBAutocreatedDictionary *autoDict = result;
      autoDict->_autocreator = autocreator;
    } else {
      GPBInt32Int32Dictionary *gpbDict = result;
      gpbDict->_autocreator = autocreator;
    }
  }

  return result;
}

#if !defined(__clang_analyzer__)
// These functions are blocked from the analyzer because the analyzer sees the
// GPBSetRetainedObjectIvarWithFieldPrivate() call as consuming the array/map,
// so use of the array/map after the call returns is flagged as a use after
// free.
// But GPBSetRetainedObjectIvarWithFieldPrivate() is "consuming" the retain
// count be holding onto the object (it is transferring it), the object is
// still valid after returning from the call.  The other way to avoid this
// would be to add a -retain/-autorelease, but that would force every
// repeated/map field parsed into the autorelease pool which is both a memory
// and performance hit.

static id GetOrCreateArrayIvarWithField(GPBMessage *self, GPBFieldDescriptor *field) {
  id array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
  if (!array) {
    // No lock needed, this is called from places expecting to mutate
    // so no threading protection is needed.
    array = CreateArrayForField(field, nil);
    GPBSetRetainedObjectIvarWithFieldPrivate(self, field, array);
  }
  return array;
}

// This is like GPBGetObjectIvarWithField(), but for arrays, it should
// only be used to wire the method into the class.
static id GetArrayIvarWithField(GPBMessage *self, GPBFieldDescriptor *field) {
  uint8_t *storage = (uint8_t *)self->messageStorage_;
  _Atomic(id) *typePtr = (_Atomic(id) *)&storage[field->description_->offset];
  id array = atomic_load(typePtr);
  if (array) {
    return array;
  }

  id expected = nil;
  id autocreated = CreateArrayForField(field, self);
  if (atomic_compare_exchange_strong(typePtr, &expected, autocreated)) {
    // Value was set, return it.
    return autocreated;
  }

  // Some other thread set it, release the one created and return what got set.
  if (GPBFieldDataTypeIsObject(field)) {
    GPBAutocreatedArray *autoArray = autocreated;
    autoArray->_autocreator = nil;
  } else {
    GPBInt32Array *gpbArray = autocreated;
    gpbArray->_autocreator = nil;
  }
  [autocreated release];
  return expected;
}

static id GetOrCreateMapIvarWithField(GPBMessage *self, GPBFieldDescriptor *field) {
  id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
  if (!dict) {
    // No lock needed, this is called from places expecting to mutate
    // so no threading protection is needed.
    dict = CreateMapForField(field, nil);
    GPBSetRetainedObjectIvarWithFieldPrivate(self, field, dict);
  }
  return dict;
}

// This is like GPBGetObjectIvarWithField(), but for maps, it should
// only be used to wire the method into the class.
static id GetMapIvarWithField(GPBMessage *self, GPBFieldDescriptor *field) {
  uint8_t *storage = (uint8_t *)self->messageStorage_;
  _Atomic(id) *typePtr = (_Atomic(id) *)&storage[field->description_->offset];
  id dict = atomic_load(typePtr);
  if (dict) {
    return dict;
  }

  id expected = nil;
  id autocreated = CreateMapForField(field, self);
  if (atomic_compare_exchange_strong(typePtr, &expected, autocreated)) {
    // Value was set, return it.
    return autocreated;
  }

  // Some other thread set it, release the one created and return what got set.
  if ((field.mapKeyDataType == GPBDataTypeString) && GPBFieldDataTypeIsObject(field)) {
    GPBAutocreatedDictionary *autoDict = autocreated;
    autoDict->_autocreator = nil;
  } else {
    GPBInt32Int32Dictionary *gpbDict = autocreated;
    gpbDict->_autocreator = nil;
  }
  [autocreated release];
  return expected;
}

#endif  // !defined(__clang_analyzer__)

static void DecodeSingleValueFromInputStream(GPBExtensionDescriptor *extension,
                                             GPBMessage *messageToGetExtension,
                                             GPBCodedInputStream *input,
                                             id<GPBExtensionRegistry> extensionRegistry,
                                             BOOL isRepeated, GPBMessage *targetMessage) {
  GPBExtensionDescription *description = extension->description_;
#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
  if (GPBDataTypeIsMessage(description->dataType)) {
    NSCAssert(targetMessage != nil, @"Internal error: must have a target message");
  } else {
    NSCAssert(targetMessage == nil, @"Internal error: should not have a target message");
  }
#endif
  GPBCodedInputStreamState *state = &input->state_;
  id nsValue;
  switch (description->dataType) {
    case GPBDataTypeBool: {
      BOOL value = GPBCodedInputStreamReadBool(state);
      nsValue = [[NSNumber alloc] initWithBool:value];
      break;
    }
    case GPBDataTypeFixed32: {
      uint32_t value = GPBCodedInputStreamReadFixed32(state);
      nsValue = [[NSNumber alloc] initWithUnsignedInt:value];
      break;
    }
    case GPBDataTypeSFixed32: {
      int32_t value = GPBCodedInputStreamReadSFixed32(state);
      nsValue = [[NSNumber alloc] initWithInt:value];
      break;
    }
    case GPBDataTypeFloat: {
      float value = GPBCodedInputStreamReadFloat(state);
      nsValue = [[NSNumber alloc] initWithFloat:value];
      break;
    }
    case GPBDataTypeFixed64: {
      uint64_t value = GPBCodedInputStreamReadFixed64(state);
      nsValue = [[NSNumber alloc] initWithUnsignedLongLong:value];
      break;
    }
    case GPBDataTypeSFixed64: {
      int64_t value = GPBCodedInputStreamReadSFixed64(state);
      nsValue = [[NSNumber alloc] initWithLongLong:value];
      break;
    }
    case GPBDataTypeDouble: {
      double value = GPBCodedInputStreamReadDouble(state);
      nsValue = [[NSNumber alloc] initWithDouble:value];
      break;
    }
    case GPBDataTypeInt32: {
      int32_t value = GPBCodedInputStreamReadInt32(state);
      nsValue = [[NSNumber alloc] initWithInt:value];
      break;
    }
    case GPBDataTypeInt64: {
      int64_t value = GPBCodedInputStreamReadInt64(state);
      nsValue = [[NSNumber alloc] initWithLongLong:value];
      break;
    }
    case GPBDataTypeSInt32: {
      int32_t value = GPBCodedInputStreamReadSInt32(state);
      nsValue = [[NSNumber alloc] initWithInt:value];
      break;
    }
    case GPBDataTypeSInt64: {
      int64_t value = GPBCodedInputStreamReadSInt64(state);
      nsValue = [[NSNumber alloc] initWithLongLong:value];
      break;
    }
    case GPBDataTypeUInt32: {
      uint32_t value = GPBCodedInputStreamReadUInt32(state);
      nsValue = [[NSNumber alloc] initWithUnsignedInt:value];
      break;
    }
    case GPBDataTypeUInt64: {
      uint64_t value = GPBCodedInputStreamReadUInt64(state);
      nsValue = [[NSNumber alloc] initWithUnsignedLongLong:value];
      break;
    }
    case GPBDataTypeBytes:
      nsValue = GPBCodedInputStreamReadRetainedBytes(state);
      break;
    case GPBDataTypeString:
      nsValue = GPBCodedInputStreamReadRetainedString(state);
      break;
    case GPBDataTypeEnum: {
      int32_t val = GPBCodedInputStreamReadEnum(&input->state_);
      GPBEnumDescriptor *enumDescriptor = extension.enumDescriptor;
      // If run with source generated before the closed enum support, all enums
      // will be considers not closed, so casing to the enum type for a switch
      // could cause things to fall off the end of a switch.
      if (!enumDescriptor.isClosed || enumDescriptor.enumVerifier(val)) {
        nsValue = [[NSNumber alloc] initWithInt:val];
      } else {
        AddUnknownFieldVarint32(messageToGetExtension, extension->description_->fieldNumber, val);
        nsValue = nil;
      }
      break;
    }
    case GPBDataTypeGroup:
    case GPBDataTypeMessage: {
      if (description->dataType == GPBDataTypeGroup) {
        [input readGroup:description->fieldNumber
                      message:targetMessage
            extensionRegistry:extensionRegistry];
      } else {
// description->dataType == GPBDataTypeMessage
#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
        NSCAssert(!GPBExtensionIsWireFormat(description),
                  @"Internal error: got a MessageSet extension when not expected.");
#endif
        [input readMessage:targetMessage extensionRegistry:extensionRegistry];
      }
      // Nothing to add below since the caller provided the message (and added it).
      nsValue = nil;
      break;
    }
  }  // switch

  if (nsValue) {
    if (isRepeated) {
      [messageToGetExtension addExtension:extension value:nsValue];
    } else {
      [messageToGetExtension setExtension:extension value:nsValue];
    }
    [nsValue release];
  }
}

static void ExtensionMergeFromInputStream(GPBExtensionDescriptor *extension, BOOL isPackedOnStream,
                                          GPBCodedInputStream *input,
                                          id<GPBExtensionRegistry> extensionRegistry,
                                          GPBMessage *message) {
  GPBExtensionDescription *description = extension->description_;
  GPBCodedInputStreamState *state = &input->state_;
  if (isPackedOnStream) {
#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
    NSCAssert(GPBExtensionIsRepeated(description), @"How was it packed if it isn't repeated?");
#endif
    int32_t length = GPBCodedInputStreamReadInt32(state);
    size_t limit = GPBCodedInputStreamPushLimit(state, length);
    while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
      DecodeSingleValueFromInputStream(extension, message, input, extensionRegistry,
                                       /*isRepeated=*/YES, nil);
    }
    GPBCodedInputStreamPopLimit(state, limit);
  } else {
    BOOL isRepeated = GPBExtensionIsRepeated(description);
    GPBMessage *targetMessage = nil;
    if (GPBDataTypeIsMessage(description->dataType)) {
      // For messages/groups create the targetMessage out here and add it to the objects graph in
      // advance, that way if DecodeSingleValueFromInputStream() throw for a parsing issue, the
      // object won't be leaked.
      if (isRepeated) {
        GPBDescriptor *descriptor = [extension.msgClass descriptor];
        targetMessage = [[descriptor.messageClass alloc] init];
        [message addExtension:extension value:targetMessage];
        [targetMessage release];
      } else {
        targetMessage = [message getExistingExtension:extension];
        if (!targetMessage) {
          GPBDescriptor *descriptor = [extension.msgClass descriptor];
          targetMessage = [[descriptor.messageClass alloc] init];
          [message setExtension:extension value:targetMessage];
          [targetMessage release];
        }
      }
    }
    DecodeSingleValueFromInputStream(extension, message, input, extensionRegistry, isRepeated,
                                     targetMessage);
  }
}

static GPBMessage *GPBCreateMessageWithAutocreator(Class msgClass, GPBMessage *autocreator,
                                                   GPBFieldDescriptor *field) {
  GPBMessage *message = [[msgClass alloc] init];
  message->autocreator_ = autocreator;
  message->autocreatorField_ = [field retain];
  return message;
}

static GPBMessage *CreateMessageWithAutocreatorForExtension(Class msgClass, GPBMessage *autocreator,
                                                            GPBExtensionDescriptor *extension)
    __attribute__((ns_returns_retained));

static GPBMessage *CreateMessageWithAutocreatorForExtension(Class msgClass, GPBMessage *autocreator,
                                                            GPBExtensionDescriptor *extension) {
  GPBMessage *message = [[msgClass alloc] init];
  message->autocreator_ = autocreator;
  message->autocreatorExtension_ = [extension retain];
  return message;
}

BOOL GPBWasMessageAutocreatedBy(GPBMessage *message, GPBMessage *parent) {
  return (message->autocreator_ == parent);
}

void GPBBecomeVisibleToAutocreator(GPBMessage *self) {
  // Message objects that are implicitly created by accessing a message field
  // are initially not visible via the hasX selector. This method makes them
  // visible.
  if (self->autocreator_) {
    // This will recursively make all parent messages visible until it reaches a
    // super-creator that's visible.
    if (self->autocreatorField_) {
      GPBSetObjectIvarWithFieldPrivate(self->autocreator_, self->autocreatorField_, self);
    } else {
      [self->autocreator_ setExtension:self->autocreatorExtension_ value:self];
    }
  }
}

void GPBAutocreatedArrayModified(GPBMessage *self, id array) {
  // When one of our autocreated arrays adds elements, make it visible.
  GPBDescriptor *descriptor = [[self class] descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (field.fieldType == GPBFieldTypeRepeated) {
      id curArray = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (curArray == array) {
        if (GPBFieldDataTypeIsObject(field)) {
          GPBAutocreatedArray *autoArray = array;
          autoArray->_autocreator = nil;
        } else {
          GPBInt32Array *gpbArray = array;
          gpbArray->_autocreator = nil;
        }
        GPBBecomeVisibleToAutocreator(self);
        return;
      }
    }
  }
  NSCAssert(NO, @"Unknown autocreated %@ for %@.", [array class], self);
}

void GPBAutocreatedDictionaryModified(GPBMessage *self, id dictionary) {
  // When one of our autocreated dicts adds elements, make it visible.
  GPBDescriptor *descriptor = [[self class] descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (field.fieldType == GPBFieldTypeMap) {
      id curDict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (curDict == dictionary) {
        if ((field.mapKeyDataType == GPBDataTypeString) && GPBFieldDataTypeIsObject(field)) {
          GPBAutocreatedDictionary *autoDict = dictionary;
          autoDict->_autocreator = nil;
        } else {
          GPBInt32Int32Dictionary *gpbDict = dictionary;
          gpbDict->_autocreator = nil;
        }
        GPBBecomeVisibleToAutocreator(self);
        return;
      }
    }
  }
  NSCAssert(NO, @"Unknown autocreated %@ for %@.", [dictionary class], self);
}

void GPBClearMessageAutocreator(GPBMessage *self) {
  if ((self == nil) || !self->autocreator_) {
    return;
  }

#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
  // Either the autocreator must have its "has" flag set to YES, or it must be
  // NO and not equal to ourselves.
  BOOL autocreatorHas =
      (self->autocreatorField_ ? GPBGetHasIvarField(self->autocreator_, self->autocreatorField_)
                               : [self->autocreator_ hasExtension:self->autocreatorExtension_]);
  GPBMessage *autocreatorFieldValue =
      (self->autocreatorField_
           ? GPBGetObjectIvarWithFieldNoAutocreate(self->autocreator_, self->autocreatorField_)
           : [self->autocreator_->autocreatedExtensionMap_
                 objectForKey:self->autocreatorExtension_]);
  NSCAssert(autocreatorHas || autocreatorFieldValue != self,
            @"Cannot clear autocreator because it still refers to self, self: %@.", self);

#endif  // DEBUG && !defined(NS_BLOCK_ASSERTIONS)

  self->autocreator_ = nil;
  [self->autocreatorField_ release];
  self->autocreatorField_ = nil;
  [self->autocreatorExtension_ release];
  self->autocreatorExtension_ = nil;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
GPB_NOINLINE
static void MergeUnknownFieldDataIntoFieldSet(GPBMessage *self, NSData *data,
                                              GPBUnknownFieldSet *targetSet) {
  GPBUnknownFieldSet *unknownFields = targetSet ? targetSet : self->unknownFields_;

#if defined(DEBUG) && DEBUG
  NSCAssert(unknownFields != nil, @"Internal error: unknown fields not initialized.");
#endif

  BOOL isMessageSet = self.descriptor.isWireFormat;
  GPBUnknownFieldSet *decodeInto = isMessageSet ? [[GPBUnknownFieldSet alloc] init] : unknownFields;

  GPBCodedInputStream *input = [[GPBCodedInputStream alloc] initWithData:data];
  @try {
    [decodeInto mergeFromCodedInputStream:input];
  } @catch (NSException *exception) {
#if defined(DEBUG) && DEBUG
    NSLog(@"%@: Internal exception while parsing the unknown fields into a Set: %@", [self class],
          exception);
#endif
  }
  [input release];

  if (isMessageSet) {
    // Need to transform the groups back into how Message feeds the data into a MessageSet when
    // doing a full MessageSet based decode.
    GPBUnknownField *groupField = [decodeInto getField:GPBWireFormatMessageSetItem];
    for (GPBUnknownFieldSet *group in groupField.groupList) {
      GPBUnknownField *typeIdField = [group getField:GPBWireFormatMessageSetTypeId];
      GPBUnknownField *messageField = [group getField:GPBWireFormatMessageSetMessage];
      if (typeIdField.varintList.count != 1 || messageField.lengthDelimitedList.count != 1) {
#if defined(DEBUG) && DEBUG
        NSCAssert(NO, @"Internal error: MessageSet group missing typeId or message.");
#endif
        continue;
      }
      int32_t fieldNumber = (int32_t)[typeIdField.varintList valueAtIndex:0];
      GPBUnknownField *messageSetField = [[GPBUnknownField alloc] initWithNumber:fieldNumber];
      [messageSetField addLengthDelimited:messageField.lengthDelimitedList[0]];
      [unknownFields addField:messageSetField];
      [messageSetField release];
    }
    [decodeInto release];
  }
}
#pragma clang diagnostic pop

@implementation GPBMessage

+ (void)initialize {
  Class pbMessageClass = [GPBMessage class];
  if ([self class] == pbMessageClass) {
    // This is here to start up the "base" class descriptor.
    [self descriptor];
    // Message shares extension method resolving with GPBRootObject so insure
    // it is started up at the same time.
    (void)[GPBRootObject class];
  } else if ([self superclass] == pbMessageClass) {
    // This is here to start up all the "message" subclasses. Just needs to be
    // done for the messages, not any of the subclasses.
    // This must be done in initialize to enforce thread safety of start up of
    // the protocol buffer library.
    // Note: The generated code for -descriptor calls
    // +[GPBDescriptor allocDescriptorForClass:...], passing the GPBRootObject
    // subclass for the file.  That call chain is what ensures that *Root class
    // is started up to support extension resolution off the message class
    // (+resolveClassMethod: below) in a thread safe manner.
    [self descriptor];
  }
}

+ (instancetype)allocWithZone:(NSZone *)zone {
  // Override alloc to allocate our classes with the additional storage
  // required for the instance variables.
  GPBDescriptor *descriptor = [self descriptor];
  return NSAllocateObject(self, descriptor->storageSize_, zone);
}

+ (instancetype)alloc {
  return [self allocWithZone:nil];
}

+ (GPBDescriptor *)descriptor {
  // This is thread safe because it is called from +initialize.
  static GPBDescriptor *descriptor = NULL;
  static GPBFileDescription fileDescription = {
      .package = "internal", .prefix = "", .syntax = GPBFileSyntaxProto2};
  if (!descriptor) {
    descriptor = [GPBDescriptor
        allocDescriptorForClass:[GPBMessage class]
                    messageName:@"GPBMessage"
                fileDescription:&fileDescription
                         fields:NULL
                     fieldCount:0
                    storageSize:0
                          flags:(GPBDescriptorInitializationFlag_UsesClassRefs |
                                 GPBDescriptorInitializationFlag_Proto3OptionalKnown |
                                 GPBDescriptorInitializationFlag_ClosedEnumSupportKnown)];
  }
  return descriptor;
}

+ (instancetype)message {
  return [[[self alloc] init] autorelease];
}

- (instancetype)init {
  if ((self = [super init])) {
    messageStorage_ =
        (GPBMessage_StoragePtr)(((uint8_t *)self) + class_getInstanceSize([self class]));
    readOnlyLock_ = OS_UNFAIR_LOCK_INIT;
  }

  return self;
}

- (instancetype)initWithData:(NSData *)data error:(NSError **)errorPtr {
  return [self initWithData:data extensionRegistry:nil error:errorPtr];
}

- (instancetype)initWithData:(NSData *)data
           extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry
                       error:(NSError **)errorPtr {
  if ((self = [self init])) {
    if (![self mergeFromData:data extensionRegistry:extensionRegistry error:errorPtr]) {
      [self release];
      self = nil;
#if defined(DEBUG) && DEBUG
    } else if (!self.initialized) {
      [self release];
      self = nil;
      if (errorPtr) {
        *errorPtr = MessageError(GPBMessageErrorCodeMissingRequiredField, nil);
      }
#endif
    }
  }
  return self;
}

- (instancetype)initWithCodedInputStream:(GPBCodedInputStream *)input
                       extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry
                                   error:(NSError **)errorPtr {
  if ((self = [self init])) {
    @try {
      [self mergeFromCodedInputStream:input extensionRegistry:extensionRegistry endingTag:0];
      if (errorPtr) {
        *errorPtr = nil;
      }
    } @catch (NSException *exception) {
      [self release];
      self = nil;
      if (errorPtr) {
        *errorPtr = ErrorFromException(exception);
      }
    }
#if defined(DEBUG) && DEBUG
    if (self && !self.initialized) {
      [self release];
      self = nil;
      if (errorPtr) {
        *errorPtr = MessageError(GPBMessageErrorCodeMissingRequiredField, nil);
      }
    }
#endif
  }
  return self;
}

- (void)dealloc {
  [self internalClear:NO];
  NSCAssert(!autocreator_, @"Autocreator was not cleared before dealloc.");
  [super dealloc];
}

- (void)copyFieldsInto:(GPBMessage *)message
                  zone:(NSZone *)zone
            descriptor:(GPBDescriptor *)descriptor {
  // Copy all the storage...
  memcpy(message->messageStorage_, messageStorage_, descriptor->storageSize_);

  // Loop over the fields doing fixup...
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      id value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (value) {
        // We need to copy the array/map, but the catch is for message fields,
        // we also need to ensure all the messages as those need copying also.
        id newValue;
        if (GPBFieldDataTypeIsMessage(field)) {
          if (field.fieldType == GPBFieldTypeRepeated) {
            NSArray *existingArray = (NSArray *)value;
            NSMutableArray *newArray =
                [[NSMutableArray alloc] initWithCapacity:existingArray.count];
            newValue = newArray;
            for (GPBMessage *msg in existingArray) {
              GPBMessage *copiedMsg = [msg copyWithZone:zone];
              [newArray addObject:copiedMsg];
              [copiedMsg release];
            }
          } else {
            if (field.mapKeyDataType == GPBDataTypeString) {
              // Map is an NSDictionary.
              NSDictionary *existingDict = value;
              NSMutableDictionary *newDict =
                  [[NSMutableDictionary alloc] initWithCapacity:existingDict.count];
              newValue = newDict;
              [existingDict enumerateKeysAndObjectsUsingBlock:^(NSString *key, GPBMessage *msg,
                                                                __unused BOOL *stop) {
                GPBMessage *copiedMsg = [msg copyWithZone:zone];
                [newDict setObject:copiedMsg forKey:key];
                [copiedMsg release];
              }];
            } else {
              // Is one of the GPB*ObjectDictionary classes.  Type doesn't
              // matter, just need one to invoke the selector.
              GPBInt32ObjectDictionary *existingDict = value;
              newValue = [existingDict deepCopyWithZone:zone];
            }
          }
        } else {
          // Not messages (but is a map/array)...
          if (field.fieldType == GPBFieldTypeRepeated) {
            if (GPBFieldDataTypeIsObject(field)) {
              // NSArray
              newValue = [value mutableCopyWithZone:zone];
            } else {
              // GPB*Array
              newValue = [value copyWithZone:zone];
            }
          } else {
            if ((field.mapKeyDataType == GPBDataTypeString) && GPBFieldDataTypeIsObject(field)) {
              // NSDictionary
              newValue = [value mutableCopyWithZone:zone];
            } else {
              // Is one of the GPB*Dictionary classes.  Type doesn't matter,
              // just need one to invoke the selector.
              GPBInt32Int32Dictionary *existingDict = value;
              newValue = [existingDict copyWithZone:zone];
            }
          }
        }
        // We retain here because the memcpy picked up the pointer value and
        // the next call to SetRetainedObject... will release the current value.
        [value retain];
        GPBSetRetainedObjectIvarWithFieldPrivate(message, field, newValue);
      }
    } else if (GPBFieldDataTypeIsMessage(field)) {
      // For object types, if we have a value, copy it.  If we don't,
      // zero it to remove the pointer to something that was autocreated
      // (and the ptr just got memcpyed).
      if (GPBGetHasIvarField(self, field)) {
        GPBMessage *value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBMessage *newValue = [value copyWithZone:zone];
        // We retain here because the memcpy picked up the pointer value and
        // the next call to SetRetainedObject... will release the current value.
        [value retain];
        GPBSetRetainedObjectIvarWithFieldPrivate(message, field, newValue);
      } else {
        uint8_t *storage = (uint8_t *)message->messageStorage_;
        id *typePtr = (id *)&storage[field->description_->offset];
        *typePtr = NULL;
      }
    } else if (GPBFieldDataTypeIsObject(field) && GPBGetHasIvarField(self, field)) {
      // A set string/data value (message picked off above), copy it.
      id value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      id newValue = [value copyWithZone:zone];
      // We retain here because the memcpy picked up the pointer value and
      // the next call to SetRetainedObject... will release the current value.
      [value retain];
      GPBSetRetainedObjectIvarWithFieldPrivate(message, field, newValue);
    } else {
      // memcpy took care of the rest of the primitive fields if they were set.
    }
  }  // for (field in descriptor->fields_)
}

- (id)copyWithZone:(NSZone *)zone {
  GPBDescriptor *descriptor = [self descriptor];
  GPBMessage *result = [[descriptor.messageClass allocWithZone:zone] init];

  [self copyFieldsInto:result zone:zone descriptor:descriptor];

  @synchronized(self) {
    result->unknownFields_ = [unknownFields_ copyWithZone:zone];
    result->unknownFieldData_ = [unknownFieldData_ mutableCopyWithZone:zone];
  }

  result->extensionMap_ = CloneExtensionMap(extensionMap_, zone);
  return result;
}

- (void)clear {
  [self internalClear:YES];
}

- (void)internalClear:(BOOL)zeroStorage {
  GPBDescriptor *descriptor = [self descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      id arrayOrMap = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (arrayOrMap) {
        if (field.fieldType == GPBFieldTypeRepeated) {
          if (GPBFieldDataTypeIsObject(field)) {
            if ([arrayOrMap isKindOfClass:[GPBAutocreatedArray class]]) {
              GPBAutocreatedArray *autoArray = arrayOrMap;
              if (autoArray->_autocreator == self) {
                autoArray->_autocreator = nil;
              }
            }
          } else {
            // Type doesn't matter, it is a GPB*Array.
            GPBInt32Array *gpbArray = arrayOrMap;
            if (gpbArray->_autocreator == self) {
              gpbArray->_autocreator = nil;
            }
          }
        } else {
          if ((field.mapKeyDataType == GPBDataTypeString) && GPBFieldDataTypeIsObject(field)) {
            if ([arrayOrMap isKindOfClass:[GPBAutocreatedDictionary class]]) {
              GPBAutocreatedDictionary *autoDict = arrayOrMap;
              if (autoDict->_autocreator == self) {
                autoDict->_autocreator = nil;
              }
            }
          } else {
            // Type doesn't matter, it is a GPB*Dictionary.
            GPBInt32Int32Dictionary *gpbDict = arrayOrMap;
            if (gpbDict->_autocreator == self) {
              gpbDict->_autocreator = nil;
            }
          }
        }
        [arrayOrMap release];
      }
    } else if (GPBFieldDataTypeIsMessage(field)) {
      GPBClearAutocreatedMessageIvarWithField(self, field);
      GPBMessage *value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      [value release];
    } else if (GPBFieldDataTypeIsObject(field) && GPBGetHasIvarField(self, field)) {
      id value = GPBGetObjectIvarWithField(self, field);
      [value release];
    }
  }

  // GPBClearMessageAutocreator() expects that its caller has already been
  // removed from autocreatedExtensionMap_ so we set to nil first.
  NSArray *autocreatedValues = [autocreatedExtensionMap_ allValues];
  [autocreatedExtensionMap_ release];
  autocreatedExtensionMap_ = nil;

  // Since we're clearing all of our extensions, make sure that we clear the
  // autocreator on any that we've created so they no longer refer to us.
  for (GPBMessage *value in autocreatedValues) {
    NSCAssert(GPBWasMessageAutocreatedBy(value, self),
              @"Autocreated extension does not refer back to self.");
    GPBClearMessageAutocreator(value);
  }

  [extensionMap_ release];
  extensionMap_ = nil;
  [unknownFieldData_ release];
  unknownFieldData_ = nil;
  [unknownFields_ release];
  unknownFields_ = nil;

  // Note that clearing does not affect autocreator_. If we are being cleared
  // because of a dealloc, then autocreator_ should be nil anyway. If we are
  // being cleared because someone explicitly clears us, we don't want to
  // sever our relationship with our autocreator.

  if (zeroStorage) {
    memset(messageStorage_, 0, descriptor->storageSize_);
  }
}

- (void)clearUnknownFields {
  [unknownFieldData_ release];
  unknownFieldData_ = nil;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  [unknownFields_ release];
  unknownFields_ = nil;
#pragma clang diagnostic pop
  GPBBecomeVisibleToAutocreator(self);
}

- (BOOL)mergeUnknownFields:(GPBUnknownFields *)unknownFields
         extensionRegistry:(nullable id<GPBExtensionRegistry>)extensionRegistry
                     error:(NSError **)errorPtr {
  return [self mergeFromData:[unknownFields serializeAsData]
           extensionRegistry:extensionRegistry
                       error:errorPtr];
}

- (BOOL)isInitialized {
  GPBDescriptor *descriptor = [self descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (field.isRequired) {
      if (!GPBGetHasIvarField(self, field)) {
        return NO;
      }
    }
    if (GPBFieldDataTypeIsMessage(field)) {
      GPBFieldType fieldType = field.fieldType;
      if (fieldType == GPBFieldTypeSingle) {
        if (field.isRequired) {
          GPBMessage *message = GPBGetMessageMessageField(self, field);
          if (!message.initialized) {
            return NO;
          }
        } else {
          NSAssert(field.isOptional, @"%@: Single message field %@ not required or optional?",
                   [self class], field.name);
          if (GPBGetHasIvarField(self, field)) {
            GPBMessage *message = GPBGetMessageMessageField(self, field);
            if (!message.initialized) {
              return NO;
            }
          }
        }
      } else if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        for (GPBMessage *message in array) {
          if (!message.initialized) {
            return NO;
          }
        }
      } else {  // fieldType == GPBFieldTypeMap
        if (field.mapKeyDataType == GPBDataTypeString) {
          NSDictionary *map = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
          if (map && !GPBDictionaryIsInitializedInternalHelper(map, field)) {
            return NO;
          }
        } else {
          // Real type is GPB*ObjectDictionary, exact type doesn't matter.
          GPBInt32ObjectDictionary *map = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
          if (map && ![map isInitialized]) {
            return NO;
          }
        }
      }
    }
  }

  __block BOOL result = YES;
  [extensionMap_
      enumerateKeysAndObjectsUsingBlock:^(GPBExtensionDescriptor *extension, id obj, BOOL *stop) {
        if (GPBExtensionIsMessage(extension)) {
          if (extension.isRepeated) {
            for (GPBMessage *msg in obj) {
              if (!msg.initialized) {
                result = NO;
                *stop = YES;
                break;
              }
            }
          } else {
            GPBMessage *asMsg = obj;
            if (!asMsg.initialized) {
              result = NO;
              *stop = YES;
            }
          }
        }
      }];
  return result;
}

- (GPBDescriptor *)descriptor {
  return [[self class] descriptor];
}

- (NSData *)data {
#if defined(DEBUG) && DEBUG
  if (!self.initialized) {
    return nil;
  }
#endif
  size_t expectedSize = [self serializedSize];
  if (expectedSize > kMaximumMessageSize) {
    return nil;
  }
  NSMutableData *data = [NSMutableData dataWithLength:expectedSize];
  GPBCodedOutputStream *stream = [[GPBCodedOutputStream alloc] initWithData:data];
  @try {
    [self writeToCodedOutputStream:stream];
    [stream flush];
  } @catch (NSException *exception) {
    // This really shouldn't happen. Normally, this could mean there was a bug in the library and it
    // failed to match between computing the size and writing out the bytes. However, the more
    // common cause is while one thread was writing out the data, some other thread had a reference
    // to this message or a message used as a nested field, and that other thread mutated that
    // message, causing the pre computed serializedSize to no longer match the final size after
    // serialization. It is not safe to mutate a message while accessing it from another thread.
#if defined(DEBUG) && DEBUG
    NSLog(@"%@: Internal exception while building message data: %@", [self class], exception);
#endif
    data = nil;
  }
#if defined(DEBUG) && DEBUG
  NSAssert(!data || [stream bytesWritten] == expectedSize, @"Internal error within the library");
#endif
  [stream release];
  return data;
}

- (NSData *)delimitedData {
  size_t serializedSize = [self serializedSize];
  size_t varintSize = GPBComputeRawVarint32SizeForInteger(serializedSize);
  NSMutableData *data = [NSMutableData dataWithLength:(serializedSize + varintSize)];
  GPBCodedOutputStream *stream = [[GPBCodedOutputStream alloc] initWithData:data];
  @try {
    [self writeDelimitedToCodedOutputStream:stream];
    [stream flush];
  } @catch (NSException *exception) {
    // This really shouldn't happen. Normally, this could mean there was a bug in the library and it
    // failed to match between computing the size and writing out the bytes. However, the more
    // common cause is while one thread was writing out the data, some other thread had a reference
    // to this message or a message used as a nested field, and that other thread mutated that
    // message, causing the pre computed serializedSize to no longer match the final size after
    // serialization. It is not safe to mutate a message while accessing it from another thread.
#if defined(DEBUG) && DEBUG
    NSLog(@"%@: Internal exception while building message delimitedData: %@", [self class],
          exception);
#endif
    // If it happens, return an empty data.
    [stream release];
    return [NSData data];
  }
  [stream release];
  return data;
}

- (void)writeToOutputStream:(NSOutputStream *)output {
  GPBCodedOutputStream *stream = [[GPBCodedOutputStream alloc] initWithOutputStream:output];
  @try {
    [self writeToCodedOutputStream:stream];
    [stream flush];
    size_t bytesWritten = [stream bytesWritten];
    if (bytesWritten > kMaximumMessageSize) {
      [NSException raise:GPBMessageExceptionMessageTooLarge
                  format:@"Message would have been %zu bytes", bytesWritten];
    }
  } @finally {
    [stream release];
  }
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)output {
  GPBDescriptor *descriptor = [self descriptor];
  NSArray *fieldsArray = descriptor->fields_;
  NSUInteger fieldCount = fieldsArray.count;
  const GPBExtensionRange *extensionRanges = descriptor.extensionRanges;
  NSUInteger extensionRangesCount = descriptor.extensionRangesCount;
  NSArray *sortedExtensions =
      [[extensionMap_ allKeys] sortedArrayUsingSelector:@selector(compareByFieldNumber:)];
  for (NSUInteger i = 0, j = 0; i < fieldCount || j < extensionRangesCount;) {
    if (i == fieldCount) {
      [self writeExtensionsToCodedOutputStream:output
                                         range:extensionRanges[j++]
                              sortedExtensions:sortedExtensions];
    } else if (j == extensionRangesCount ||
               GPBFieldNumber(fieldsArray[i]) < extensionRanges[j].start) {
      [self writeField:fieldsArray[i++] toCodedOutputStream:output];
    } else {
      [self writeExtensionsToCodedOutputStream:output
                                         range:extensionRanges[j++]
                              sortedExtensions:sortedExtensions];
    }
  }
  @synchronized(self) {
    if (unknownFieldData_) {
#if defined(DEBUG) && DEBUG
      NSAssert(unknownFields_ == nil, @"Internal error both unknown states were set");
#endif
      [output writeRawData:unknownFieldData_];
    } else {
      if (descriptor.isWireFormat) {
        [unknownFields_ writeAsMessageSetTo:output];
      } else {
        [unknownFields_ writeToCodedOutputStream:output];
      }
    }
  }  // @synchronized(self)
}

- (void)writeDelimitedToOutputStream:(NSOutputStream *)output {
  GPBCodedOutputStream *codedOutput = [[GPBCodedOutputStream alloc] initWithOutputStream:output];
  @try {
    [self writeDelimitedToCodedOutputStream:codedOutput];
    [codedOutput flush];
  } @finally {
    [codedOutput release];
  }
}

- (void)writeDelimitedToCodedOutputStream:(GPBCodedOutputStream *)output {
  size_t expectedSize = [self serializedSize];
  if (expectedSize > kMaximumMessageSize) {
    [NSException raise:GPBMessageExceptionMessageTooLarge
                format:@"Message would have been %zu bytes", expectedSize];
  }
  [output writeRawVarintSizeTAs32:expectedSize];
#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
  size_t initialSize = [output bytesWritten];
#endif
  [self writeToCodedOutputStream:output];
#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
  NSAssert(([output bytesWritten] - initialSize) == expectedSize,
           @"Internal error within the library");
#endif
}

- (void)writeField:(GPBFieldDescriptor *)field toCodedOutputStream:(GPBCodedOutputStream *)output {
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeSingle) {
    BOOL has = GPBGetHasIvarField(self, field);
    if (!has) {
      return;
    }
  }
  uint32_t fieldNumber = GPBFieldNumber(field);

  switch (GPBGetFieldDataType(field)) {
      // clang-format off

//%PDDM-DEFINE FIELD_CASE(TYPE, REAL_TYPE)
//%FIELD_CASE_FULL(TYPE, REAL_TYPE, REAL_TYPE)
//%PDDM-DEFINE FIELD_CASE_FULL(TYPE, REAL_TYPE, ARRAY_TYPE)
//%    case GPBDataType##TYPE:
//%      if (fieldType == GPBFieldTypeRepeated) {
//%        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
//%        GPB##ARRAY_TYPE##Array *array =
//%            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [output write##TYPE##Array:fieldNumber values:array tag:tag];
//%      } else if (fieldType == GPBFieldTypeSingle) {
//%        [output write##TYPE:fieldNumber
//%              TYPE$S  value:GPBGetMessage##REAL_TYPE##Field(self, field)];
//%      } else {  // fieldType == GPBFieldTypeMap
//%        // Exact type here doesn't matter.
//%        GPBInt32##ARRAY_TYPE##Dictionary *dict =
//%            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [dict writeToCodedOutputStream:output asField:field];
//%      }
//%      break;
//%
//%PDDM-DEFINE FIELD_CASE2(TYPE)
//%    case GPBDataType##TYPE:
//%      if (fieldType == GPBFieldTypeRepeated) {
//%        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [output write##TYPE##Array:fieldNumber values:array];
//%      } else if (fieldType == GPBFieldTypeSingle) {
//%        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
//%        // again.
//%        [output write##TYPE:fieldNumber
//%              TYPE$S  value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
//%      } else {  // fieldType == GPBFieldTypeMap
//%        // Exact type here doesn't matter.
//%        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        GPBDataType mapKeyDataType = field.mapKeyDataType;
//%        if (mapKeyDataType == GPBDataTypeString) {
//%          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
//%        } else {
//%          [dict writeToCodedOutputStream:output asField:field];
//%        }
//%      }
//%      break;
//%
//%PDDM-EXPAND FIELD_CASE(Bool, Bool)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeBool:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBBoolArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeBoolArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeBool:fieldNumber
                    value:GPBGetMessageBoolField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32BoolDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Fixed32, UInt32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeFixed32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFixed32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFixed32:fieldNumber
                       value:GPBGetMessageUInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SFixed32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSFixed32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSFixed32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSFixed32:fieldNumber
                        value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Float, Float)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeFloat:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBFloatArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFloatArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFloat:fieldNumber
                     value:GPBGetMessageFloatField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32FloatDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Fixed64, UInt64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeFixed64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFixed64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFixed64:fieldNumber
                       value:GPBGetMessageUInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SFixed64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSFixed64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSFixed64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSFixed64:fieldNumber
                        value:GPBGetMessageInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Double, Double)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeDouble:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBDoubleArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeDoubleArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeDouble:fieldNumber
                      value:GPBGetMessageDoubleField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32DoubleDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Int32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeInt32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeInt32:fieldNumber
                     value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Int64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeInt64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeInt64:fieldNumber
                     value:GPBGetMessageInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SInt32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSInt32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSInt32:fieldNumber
                      value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SInt64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSInt64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSInt64:fieldNumber
                      value:GPBGetMessageInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(UInt32, UInt32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeUInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeUInt32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeUInt32:fieldNumber
                      value:GPBGetMessageUInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(UInt64, UInt64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeUInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeUInt64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeUInt64:fieldNumber
                      value:GPBGetMessageUInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE_FULL(Enum, Int32, Enum)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeEnum:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBEnumArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeEnumArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeEnum:fieldNumber
                    value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32EnumDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Bytes)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeBytes:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeBytesArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeBytes:fieldNumber
                     value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(String)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeString:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeStringArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeString:fieldNumber
                      value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Message)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeMessage:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeMessageArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeMessage:fieldNumber
                       value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Group)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeGroup:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeGroupArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeGroup:fieldNumber
                     value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND-END (18 expansions)

      // clang-format on
  }
}

#pragma mark - Extensions

- (id)getExtension:(GPBExtensionDescriptor *)extension {
  CheckExtension(self, extension);
  id value = [extensionMap_ objectForKey:extension];
  if (value != nil) {
    return value;
  }

  // No default for repeated.
  if (extension.isRepeated) {
    return nil;
  }
  // Non messages get their default.
  if (!GPBExtensionIsMessage(extension)) {
    return extension.defaultValue;
  }

  // Check for an autocreated value.
  os_unfair_lock_lock(&readOnlyLock_);
  value = [autocreatedExtensionMap_ objectForKey:extension];
  if (!value) {
    // Auto create the message extensions to match normal fields.
    value = CreateMessageWithAutocreatorForExtension(extension.msgClass, self, extension);

    if (autocreatedExtensionMap_ == nil) {
      autocreatedExtensionMap_ = [[NSMutableDictionary alloc] init];
    }

    // We can't simply call setExtension here because that would clear the new
    // value's autocreator.
    [autocreatedExtensionMap_ setObject:value forKey:extension];
    [value release];
  }

  os_unfair_lock_unlock(&readOnlyLock_);
  return value;
}

- (id)getExistingExtension:(GPBExtensionDescriptor *)extension {
  // This is an internal method so we don't need to call CheckExtension().
  return [extensionMap_ objectForKey:extension];
}

- (BOOL)hasExtension:(GPBExtensionDescriptor *)extension {
#if defined(DEBUG) && DEBUG
  CheckExtension(self, extension);
#endif  // DEBUG
  return nil != [extensionMap_ objectForKey:extension];
}

- (NSArray *)extensionsCurrentlySet {
  return [extensionMap_ allKeys];
}

- (void)writeExtensionsToCodedOutputStream:(GPBCodedOutputStream *)output
                                     range:(GPBExtensionRange)range
                          sortedExtensions:(NSArray *)sortedExtensions {
  uint32_t start = range.start;
  uint32_t end = range.end;
  for (GPBExtensionDescriptor *extension in sortedExtensions) {
    uint32_t fieldNumber = extension.fieldNumber;
    if (fieldNumber < start) {
      continue;
    }
    if (fieldNumber >= end) {
      break;
    }
    id value = [extensionMap_ objectForKey:extension];
    GPBWriteExtensionValueToOutputStream(extension, value, output);
  }
}

- (void)setExtension:(GPBExtensionDescriptor *)extension value:(id)value {
  if (!value) {
    [self clearExtension:extension];
    return;
  }

  CheckExtension(self, extension);

  if (extension.repeated) {
    [NSException raise:NSInvalidArgumentException
                format:@"Must call addExtension() for repeated types."];
  }

  if (extensionMap_ == nil) {
    extensionMap_ = [[NSMutableDictionary alloc] init];
  }

  // This pointless cast is for CLANG_WARN_NULLABLE_TO_NONNULL_CONVERSION.
  // Without it, the compiler complains we're passing an id nullable when
  // setObject:forKey: requires a id nonnull for the value. The check for
  // !value at the start of the method ensures it isn't nil, but the check
  // isn't smart enough to realize that.
  [extensionMap_ setObject:(id)value forKey:extension];

  GPBExtensionDescriptor *descriptor = extension;

  if (GPBExtensionIsMessage(descriptor) && !descriptor.isRepeated) {
    GPBMessage *autocreatedValue = [[autocreatedExtensionMap_ objectForKey:extension] retain];
    // Must remove from the map before calling GPBClearMessageAutocreator() so
    // that GPBClearMessageAutocreator() knows its safe to clear.
    [autocreatedExtensionMap_ removeObjectForKey:extension];
    GPBClearMessageAutocreator(autocreatedValue);
    [autocreatedValue release];
  }

  GPBBecomeVisibleToAutocreator(self);
}

- (void)addExtension:(GPBExtensionDescriptor *)extension value:(id)value {
  CheckExtension(self, extension);

  if (!extension.repeated) {
    [NSException raise:NSInvalidArgumentException
                format:@"Must call setExtension() for singular types."];
  }

  if (extensionMap_ == nil) {
    extensionMap_ = [[NSMutableDictionary alloc] init];
  }
  NSMutableArray *list = [extensionMap_ objectForKey:extension];
  if (list == nil) {
    list = [NSMutableArray array];
    [extensionMap_ setObject:list forKey:extension];
  }

  [list addObject:value];
  GPBBecomeVisibleToAutocreator(self);
}

- (void)setExtension:(GPBExtensionDescriptor *)extension index:(NSUInteger)idx value:(id)value {
  CheckExtension(self, extension);

  if (!extension.repeated) {
    [NSException raise:NSInvalidArgumentException
                format:@"Must call setExtension() for singular types."];
  }

  if (extensionMap_ == nil) {
    extensionMap_ = [[NSMutableDictionary alloc] init];
  }

  NSMutableArray *list = [extensionMap_ objectForKey:extension];

  [list replaceObjectAtIndex:idx withObject:value];
  GPBBecomeVisibleToAutocreator(self);
}

- (void)clearExtension:(GPBExtensionDescriptor *)extension {
  CheckExtension(self, extension);

  // Only become visible if there was actually a value to clear.
  if ([extensionMap_ objectForKey:extension]) {
    [extensionMap_ removeObjectForKey:extension];
    GPBBecomeVisibleToAutocreator(self);
  }
}

#pragma mark - mergeFrom

- (void)mergeFromData:(NSData *)data extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry {
  GPBCodedInputStream *input = [[GPBCodedInputStream alloc] initWithData:data];
  @try {
    [self mergeFromCodedInputStream:input extensionRegistry:extensionRegistry endingTag:0];
  } @finally {
    [input release];
  }
}

- (BOOL)mergeFromData:(NSData *)data
    extensionRegistry:(nullable id<GPBExtensionRegistry>)extensionRegistry
                error:(NSError **)errorPtr {
  GPBBecomeVisibleToAutocreator(self);
  GPBCodedInputStream *input = [[GPBCodedInputStream alloc] initWithData:data];
  @try {
    [self mergeFromCodedInputStream:input extensionRegistry:extensionRegistry endingTag:0];
    [input checkLastTagWas:0];
    if (errorPtr) {
      *errorPtr = nil;
    }
  } @catch (NSException *exception) {
    [input release];
    if (errorPtr) {
      *errorPtr = ErrorFromException(exception);
    }
    return NO;
  }
  [input release];
  return YES;
}

#pragma mark - Parse From Data Support

+ (instancetype)parseFromData:(NSData *)data error:(NSError **)errorPtr {
  return [self parseFromData:data extensionRegistry:nil error:errorPtr];
}

+ (instancetype)parseFromData:(NSData *)data
            extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry
                        error:(NSError **)errorPtr {
  return [[[self alloc] initWithData:data extensionRegistry:extensionRegistry
                               error:errorPtr] autorelease];
}

+ (instancetype)parseFromCodedInputStream:(GPBCodedInputStream *)input
                        extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry
                                    error:(NSError **)errorPtr {
  return [[[self alloc] initWithCodedInputStream:input
                               extensionRegistry:extensionRegistry
                                           error:errorPtr] autorelease];
}

#pragma mark - Parse Delimited From Data Support

+ (instancetype)parseDelimitedFromCodedInputStream:(GPBCodedInputStream *)input
                                 extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry
                                             error:(NSError **)errorPtr {
  GPBCodedInputStreamState *state = &input->state_;
  // This doesn't completely match the C++, but if the stream has nothing, just make an empty
  // message.
  if (GPBCodedInputStreamIsAtEnd(state)) {
    return [[[self alloc] init] autorelease];
  }

  // Manually extract the data and parse it. If we read a varint and push a limit, that consumes
  // some of the recursion buffer which isn't correct, it also can result in a change in error
  // codes for attempts to parse partial data; and there are projects sensitive to that, so this
  // maintains existing error flows.

  // Extract the data, but in a "no copy" mode since we will immediately parse it so this NSData
  // is transient.
  NSData *data = nil;
  @try {
    data = GPBCodedInputStreamReadRetainedBytesNoCopy(state);
  } @catch (NSException *exception) {
    if (errorPtr) {
      *errorPtr = ErrorFromException(exception);
    }
    return nil;
  }

  GPBMessage *result = [self parseFromData:data extensionRegistry:extensionRegistry error:errorPtr];
  [data release];
  if (result && errorPtr) {
    *errorPtr = nil;
  }
  return result;
}

#pragma mark - Unknown Field Support

- (GPBUnknownFieldSet *)unknownFields {
  @synchronized(self) {
    if (unknownFieldData_) {
#if defined(DEBUG) && DEBUG
      NSAssert(unknownFields_ == nil, @"Internal error both unknown states were set");
#endif
      unknownFields_ = [[GPBUnknownFieldSet alloc] init];
      MergeUnknownFieldDataIntoFieldSet(self, unknownFieldData_, nil);
      [unknownFieldData_ release];
      unknownFieldData_ = nil;
    }
    return unknownFields_;
  }  // @synchronized(self)
}

- (void)setUnknownFields:(GPBUnknownFieldSet *)unknownFields {
  if (unknownFields != unknownFields_ || unknownFieldData_ != nil) {
    // Changing sets or clearing.
    [unknownFieldData_ release];
    unknownFieldData_ = nil;
    [unknownFields_ release];
    unknownFields_ = [unknownFields copy];
    GPBBecomeVisibleToAutocreator(self);
  }
}

- (void)parseMessageSet:(GPBCodedInputStream *)input
      extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry {
  uint32_t typeId = 0;
  NSData *rawBytes = nil;
  GPBCodedInputStreamState *state = &input->state_;
  BOOL gotType = NO;
  BOOL gotBytes = NO;
  while (true) {
    uint32_t tag = GPBCodedInputStreamReadTag(state);
    if (tag == GPBWireFormatMessageSetItemEndTag || tag == 0) {
      break;
    }

    if (tag == GPBWireFormatMessageSetTypeIdTag) {
      uint32_t tmp = GPBCodedInputStreamReadUInt32(state);
      // Spec says only use the first value.
      if (!gotType) {
        gotType = YES;
        typeId = tmp;
      }
    } else if (tag == GPBWireFormatMessageSetMessageTag) {
      if (gotBytes) {
        // Skip over the payload instead of collecting it.
        [input skipField:tag];
      } else {
        rawBytes = [GPBCodedInputStreamReadRetainedBytesNoCopy(state) autorelease];
        gotBytes = YES;
      }
    } else {
      // Don't capture unknowns within the message set impl group.
      if (![input skipField:tag]) {
        break;
      }
    }
  }

  // If we get here because of end of input (tag zero) or the wrong end tag (within the skipField:),
  // this will error.
  GPBCodedInputStreamCheckLastTagWas(state, GPBWireFormatMessageSetItemEndTag);

  if (!gotType || !gotBytes) {
    // upb_Decoder_DecodeMessageSetItem does't keep this partial as an unknown field, it just drops
    // it, so do the same thing.
    return;
  }

  GPBExtensionDescriptor *extension = [extensionRegistry extensionForDescriptor:[self descriptor]
                                                                    fieldNumber:typeId];
  if (extension) {
#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
    NSAssert(extension.dataType == GPBDataTypeMessage,
             @"Internal Error: MessageSet extension must be a message field.");
    NSAssert(GPBExtensionIsWireFormat(extension->description_),
             @"Internal Error: MessageSet extension must have message_set_wire_format set.");
    NSAssert(!GPBExtensionIsRepeated(extension->description_),
             @"Internal Error: MessageSet extension can't be repeated.");
#endif
    // Look up the existing one to merge to or create a new one.
    GPBMessage *targetMessage = [self getExistingExtension:extension];
    if (!targetMessage) {
      GPBDescriptor *descriptor = [extension.msgClass descriptor];
      targetMessage = [[descriptor.messageClass alloc] init];
      [self setExtension:extension value:targetMessage];
      [targetMessage release];
    }
    GPBCodedInputStream *newInput = [[GPBCodedInputStream alloc] initWithData:rawBytes];
    @try {
      [targetMessage mergeFromCodedInputStream:newInput
                             extensionRegistry:extensionRegistry
                                     endingTag:0];
    } @finally {
      [newInput release];
    }
  } else {
    // The extension isn't in the registry, but it was well formed, so the whole group structure
    // get preserved as an unknown field.

    // rawBytes was created via a NoCopy, so it can be reusing a
    // subrange of another NSData that might go out of scope as things
    // unwind, so a copy is needed to ensure what is saved in the
    // unknown fields stays valid.
    NSData *cloned = [NSData dataWithData:rawBytes];
    AddUnknownMessageSetEntry(self, typeId, cloned);
  }
}

- (void)addUnknownMapEntry:(int32_t)fieldNum value:(NSData *)data {
  AddUnknownFieldLengthDelimited(self, fieldNum, data);
}

#pragma mark - MergeFromCodedInputStream Support

static void MergeSingleFieldFromCodedInputStream(GPBMessage *self, GPBFieldDescriptor *field,
                                                 GPBCodedInputStream *input,
                                                 id<GPBExtensionRegistry> extensionRegistry) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
#define CASE_SINGLE_POD(NAME, TYPE, FUNC_TYPE)                 \
  case GPBDataType##NAME: {                                    \
    TYPE val = GPBCodedInputStreamRead##NAME(&input->state_);  \
    GPBSet##FUNC_TYPE##IvarWithFieldPrivate(self, field, val); \
    break;                                                     \
  }
#define CASE_SINGLE_OBJECT(NAME)                                    \
  case GPBDataType##NAME: {                                         \
    id val = GPBCodedInputStreamReadRetained##NAME(&input->state_); \
    GPBSetRetainedObjectIvarWithFieldPrivate(self, field, val);     \
    break;                                                          \
  }
    CASE_SINGLE_POD(Bool, BOOL, Bool)
    CASE_SINGLE_POD(Fixed32, uint32_t, UInt32)
    CASE_SINGLE_POD(SFixed32, int32_t, Int32)
    CASE_SINGLE_POD(Float, float, Float)
    CASE_SINGLE_POD(Fixed64, uint64_t, UInt64)
    CASE_SINGLE_POD(SFixed64, int64_t, Int64)
    CASE_SINGLE_POD(Double, double, Double)
    CASE_SINGLE_POD(Int32, int32_t, Int32)
    CASE_SINGLE_POD(Int64, int64_t, Int64)
    CASE_SINGLE_POD(SInt32, int32_t, Int32)
    CASE_SINGLE_POD(SInt64, int64_t, Int64)
    CASE_SINGLE_POD(UInt32, uint32_t, UInt32)
    CASE_SINGLE_POD(UInt64, uint64_t, UInt64)
    CASE_SINGLE_OBJECT(Bytes)
    CASE_SINGLE_OBJECT(String)
#undef CASE_SINGLE_POD
#undef CASE_SINGLE_OBJECT

    case GPBDataTypeMessage: {
      if (GPBGetHasIvarField(self, field)) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has
        // check again.
        GPBMessage *message = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [input readMessage:message extensionRegistry:extensionRegistry];
      } else {
        GPBMessage *message = [[field.msgClass alloc] init];
        GPBSetRetainedObjectIvarWithFieldPrivate(self, field, message);
        [input readMessage:message extensionRegistry:extensionRegistry];
      }
      break;
    }

    case GPBDataTypeGroup: {
      if (GPBGetHasIvarField(self, field)) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has
        // check again.
        GPBMessage *message = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [input readGroup:GPBFieldNumber(field) message:message extensionRegistry:extensionRegistry];
      } else {
        GPBMessage *message = [[field.msgClass alloc] init];
        GPBSetRetainedObjectIvarWithFieldPrivate(self, field, message);
        [input readGroup:GPBFieldNumber(field) message:message extensionRegistry:extensionRegistry];
      }
      break;
    }

    case GPBDataTypeEnum: {
      int32_t val = GPBCodedInputStreamReadEnum(&input->state_);
      if (!GPBFieldIsClosedEnum(field) || [field isValidEnumValue:val]) {
        GPBSetInt32IvarWithFieldPrivate(self, field, val);
      } else {
        AddUnknownFieldVarint32(self, GPBFieldNumber(field), val);
      }
    }
  }  // switch
}

static void MergeRepeatedPackedFieldFromCodedInputStream(GPBMessage *self,
                                                         GPBFieldDescriptor *field,
                                                         GPBCodedInputStream *input) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  GPBCodedInputStreamState *state = &input->state_;
  id genericArray = GetOrCreateArrayIvarWithField(self, field);
  int32_t length = GPBCodedInputStreamReadInt32(state);
  size_t limit = GPBCodedInputStreamPushLimit(state, length);
  while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
    switch (fieldDataType) {
#define CASE_REPEATED_PACKED_POD(NAME, TYPE, ARRAY_TYPE)   \
  case GPBDataType##NAME: {                                \
    TYPE val = GPBCodedInputStreamRead##NAME(state);       \
    [(GPB##ARRAY_TYPE##Array *)genericArray addValue:val]; \
    break;                                                 \
  }
      CASE_REPEATED_PACKED_POD(Bool, BOOL, Bool)
      CASE_REPEATED_PACKED_POD(Fixed32, uint32_t, UInt32)
      CASE_REPEATED_PACKED_POD(SFixed32, int32_t, Int32)
      CASE_REPEATED_PACKED_POD(Float, float, Float)
      CASE_REPEATED_PACKED_POD(Fixed64, uint64_t, UInt64)
      CASE_REPEATED_PACKED_POD(SFixed64, int64_t, Int64)
      CASE_REPEATED_PACKED_POD(Double, double, Double)
      CASE_REPEATED_PACKED_POD(Int32, int32_t, Int32)
      CASE_REPEATED_PACKED_POD(Int64, int64_t, Int64)
      CASE_REPEATED_PACKED_POD(SInt32, int32_t, Int32)
      CASE_REPEATED_PACKED_POD(SInt64, int64_t, Int64)
      CASE_REPEATED_PACKED_POD(UInt32, uint32_t, UInt32)
      CASE_REPEATED_PACKED_POD(UInt64, uint64_t, UInt64)
#undef CASE_REPEATED_PACKED_POD

      case GPBDataTypeBytes:
      case GPBDataTypeString:
      case GPBDataTypeMessage:
      case GPBDataTypeGroup:
        NSCAssert(NO, @"Non primitive types can't be packed");
        break;

      case GPBDataTypeEnum: {
        int32_t val = GPBCodedInputStreamReadEnum(state);
        if (!GPBFieldIsClosedEnum(field) || [field isValidEnumValue:val]) {
          [(GPBEnumArray *)genericArray addRawValue:val];
        } else {
          AddUnknownFieldVarint32(self, GPBFieldNumber(field), val);
        }
        break;
      }
    }  // switch
  }  // while(BytesUntilLimit() > 0)
  GPBCodedInputStreamPopLimit(state, limit);
}

static void MergeRepeatedNotPackedFieldFromCodedInputStream(
    GPBMessage *self, GPBFieldDescriptor *field, GPBCodedInputStream *input,
    id<GPBExtensionRegistry> extensionRegistry) {
  GPBCodedInputStreamState *state = &input->state_;
  id genericArray = GetOrCreateArrayIvarWithField(self, field);
  switch (GPBGetFieldDataType(field)) {
#define CASE_REPEATED_NOT_PACKED_POD(NAME, TYPE, ARRAY_TYPE) \
  case GPBDataType##NAME: {                                  \
    TYPE val = GPBCodedInputStreamRead##NAME(state);         \
    [(GPB##ARRAY_TYPE##Array *)genericArray addValue:val];   \
    break;                                                   \
  }
#define CASE_REPEATED_NOT_PACKED_OBJECT(NAME)              \
  case GPBDataType##NAME: {                                \
    id val = GPBCodedInputStreamReadRetained##NAME(state); \
    [(NSMutableArray *)genericArray addObject:val];        \
    [val release];                                         \
    break;                                                 \
  }
    CASE_REPEATED_NOT_PACKED_POD(Bool, BOOL, Bool)
    CASE_REPEATED_NOT_PACKED_POD(Fixed32, uint32_t, UInt32)
    CASE_REPEATED_NOT_PACKED_POD(SFixed32, int32_t, Int32)
    CASE_REPEATED_NOT_PACKED_POD(Float, float, Float)
    CASE_REPEATED_NOT_PACKED_POD(Fixed64, uint64_t, UInt64)
    CASE_REPEATED_NOT_PACKED_POD(SFixed64, int64_t, Int64)
    CASE_REPEATED_NOT_PACKED_POD(Double, double, Double)
    CASE_REPEATED_NOT_PACKED_POD(Int32, int32_t, Int32)
    CASE_REPEATED_NOT_PACKED_POD(Int64, int64_t, Int64)
    CASE_REPEATED_NOT_PACKED_POD(SInt32, int32_t, Int32)
    CASE_REPEATED_NOT_PACKED_POD(SInt64, int64_t, Int64)
    CASE_REPEATED_NOT_PACKED_POD(UInt32, uint32_t, UInt32)
    CASE_REPEATED_NOT_PACKED_POD(UInt64, uint64_t, UInt64)
    CASE_REPEATED_NOT_PACKED_OBJECT(Bytes)
    CASE_REPEATED_NOT_PACKED_OBJECT(String)
#undef CASE_REPEATED_NOT_PACKED_POD
#undef CASE_NOT_PACKED_OBJECT
    case GPBDataTypeMessage: {
      GPBMessage *message = [[field.msgClass alloc] init];
      [(NSMutableArray *)genericArray addObject:message];
      // The array will now retain message, so go ahead and release it in case
      // -readMessage:extensionRegistry: throws so it won't be leaked.
      [message release];
      [input readMessage:message extensionRegistry:extensionRegistry];
      break;
    }
    case GPBDataTypeGroup: {
      GPBMessage *message = [[field.msgClass alloc] init];
      [(NSMutableArray *)genericArray addObject:message];
      // The array will now retain message, so go ahead and release it in case
      // -readGroup:extensionRegistry: throws so it won't be leaked.
      [message release];
      [input readGroup:GPBFieldNumber(field) message:message extensionRegistry:extensionRegistry];
      break;
    }
    case GPBDataTypeEnum: {
      int32_t val = GPBCodedInputStreamReadEnum(state);
      if (!GPBFieldIsClosedEnum(field) || [field isValidEnumValue:val]) {
        [(GPBEnumArray *)genericArray addRawValue:val];
      } else {
        AddUnknownFieldVarint32(self, GPBFieldNumber(field), val);
      }
      break;
    }
  }  // switch
}

- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input
                extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry
                        endingTag:(uint32_t)endingTag {
#if defined(DEBUG) && DEBUG
  NSAssert(endingTag == 0 || GPBWireFormatGetTagWireType(endingTag) == GPBWireFormatEndGroup,
           @"endingTag should have been an endGroup tag");
#endif  // DEBUG
  GPBDescriptor *descriptor = [self descriptor];
  GPBCodedInputStreamState *state = &input->state_;
  uint32_t tag = 0;
  NSUInteger startingIndex = 0;
  NSArray *fields = descriptor->fields_;
  BOOL isMessageSetWireFormat = descriptor.isWireFormat;
  NSUInteger numFields = fields.count;
  while (YES) {
    BOOL merged = NO;
    tag = GPBCodedInputStreamReadTag(state);
    if (tag == endingTag || tag == 0) {
      // If we got to the end (tag zero), when we were expecting the end group, this will
      // raise the error.
      GPBCodedInputStreamCheckLastTagWas(state, endingTag);
      return;
    }
    for (NSUInteger i = 0; i < numFields; ++i) {
      if (startingIndex >= numFields) startingIndex = 0;
      GPBFieldDescriptor *fieldDescriptor = fields[startingIndex];
      if (GPBFieldTag(fieldDescriptor) == tag) {
        GPBFieldType fieldType = fieldDescriptor.fieldType;
        if (fieldType == GPBFieldTypeSingle) {
          MergeSingleFieldFromCodedInputStream(self, fieldDescriptor, input, extensionRegistry);
          // Well formed protos will only have a single field once, advance
          // the starting index to the next field.
          startingIndex += 1;
        } else if (fieldType == GPBFieldTypeRepeated) {
          if (fieldDescriptor.isPackable) {
            MergeRepeatedPackedFieldFromCodedInputStream(self, fieldDescriptor, input);
            // Well formed protos will only have a repeated field that is
            // packed once, advance the starting index to the next field.
            startingIndex += 1;
          } else {
            MergeRepeatedNotPackedFieldFromCodedInputStream(self, fieldDescriptor, input,
                                                            extensionRegistry);
          }
        } else {  // fieldType == GPBFieldTypeMap
          // GPB*Dictionary or NSDictionary, exact type doesn't matter at this
          // point.
          id map = GetOrCreateMapIvarWithField(self, fieldDescriptor);
          [input readMapEntry:map
              extensionRegistry:extensionRegistry
                          field:fieldDescriptor
                  parentMessage:self];
        }
        merged = YES;
        break;
      } else {
        startingIndex += 1;
      }
    }  // for(i < numFields)

    if (merged) continue;  // On to the next tag

    // Primitive, repeated types can be packed or unpacked on the wire, and
    // are parsed either way.  The above loop covered tag in the preferred
    // for, so this need to check the alternate form.
    for (NSUInteger i = 0; i < numFields; ++i) {
      if (startingIndex >= numFields) startingIndex = 0;
      GPBFieldDescriptor *fieldDescriptor = fields[startingIndex];
      if ((fieldDescriptor.fieldType == GPBFieldTypeRepeated) &&
          !GPBFieldDataTypeIsObject(fieldDescriptor) &&
          (GPBFieldAlternateTag(fieldDescriptor) == tag)) {
        BOOL alternateIsPacked = !fieldDescriptor.isPackable;
        if (alternateIsPacked) {
          MergeRepeatedPackedFieldFromCodedInputStream(self, fieldDescriptor, input);
          // Well formed protos will only have a repeated field that is
          // packed once, advance the starting index to the next field.
          startingIndex += 1;
        } else {
          MergeRepeatedNotPackedFieldFromCodedInputStream(self, fieldDescriptor, input,
                                                          extensionRegistry);
        }
        merged = YES;
        break;
      } else {
        startingIndex += 1;
      }
    }

    if (merged) continue;  // On to the next tag

    if (isMessageSetWireFormat) {
      if (GPBWireFormatMessageSetItemTag == tag) {
        [self parseMessageSet:input extensionRegistry:extensionRegistry];
        continue;  // On to the next tag
      }
    } else {
      // ObjC Runtime currently doesn't track if a message supported extensions, so the check is
      // always done.
      GPBExtensionDescriptor *extension =
          [extensionRegistry extensionForDescriptor:descriptor
                                        fieldNumber:GPBWireFormatGetTagFieldNumber(tag)];
      if (extension) {
        GPBWireFormat wireType = GPBWireFormatGetTagWireType(tag);
        if (extension.wireType == wireType) {
          ExtensionMergeFromInputStream(extension, extension.packable, input, extensionRegistry,
                                        self);
          continue;  // On to the next tag
        }
        // Primitive, repeated types can be packed on unpacked on the wire, and are
        // parsed either way.
        if ([extension isRepeated] && !GPBDataTypeIsObject(extension->description_->dataType) &&
            (extension.alternateWireType == wireType)) {
          ExtensionMergeFromInputStream(extension, !extension.packable, input, extensionRegistry,
                                        self);
          continue;  // On to the next tag
        }
      }
    }

    ParseUnknownField(self, tag, input);
  }  // while(YES)
}

#pragma mark - MergeFrom Support

- (void)mergeFrom:(GPBMessage *)other {
  Class selfClass = [self class];
  Class otherClass = [other class];
  if (!([selfClass isSubclassOfClass:otherClass] || [otherClass isSubclassOfClass:selfClass])) {
    [NSException raise:NSInvalidArgumentException
                format:@"Classes must match %@ != %@", selfClass, otherClass];
  }

  // We assume something will be done and become visible.
  GPBBecomeVisibleToAutocreator(self);

  GPBDescriptor *descriptor = [[self class] descriptor];

  for (GPBFieldDescriptor *field in descriptor->fields_) {
    GPBFieldType fieldType = field.fieldType;
    if (fieldType == GPBFieldTypeSingle) {
      int32_t hasIndex = GPBFieldHasIndex(field);
      uint32_t fieldNumber = GPBFieldNumber(field);
      if (!GPBGetHasIvar(other, hasIndex, fieldNumber)) {
        // Other doesn't have the field set, on to the next.
        continue;
      }
      GPBDataType fieldDataType = GPBGetFieldDataType(field);
      switch (fieldDataType) {
        case GPBDataTypeBool:
          GPBSetBoolIvarWithFieldPrivate(self, field, GPBGetMessageBoolField(other, field));
          break;
        case GPBDataTypeSFixed32:
        case GPBDataTypeEnum:
        case GPBDataTypeInt32:
        case GPBDataTypeSInt32:
          GPBSetInt32IvarWithFieldPrivate(self, field, GPBGetMessageInt32Field(other, field));
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          GPBSetUInt32IvarWithFieldPrivate(self, field, GPBGetMessageUInt32Field(other, field));
          break;
        case GPBDataTypeSFixed64:
        case GPBDataTypeInt64:
        case GPBDataTypeSInt64:
          GPBSetInt64IvarWithFieldPrivate(self, field, GPBGetMessageInt64Field(other, field));
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          GPBSetUInt64IvarWithFieldPrivate(self, field, GPBGetMessageUInt64Field(other, field));
          break;
        case GPBDataTypeFloat:
          GPBSetFloatIvarWithFieldPrivate(self, field, GPBGetMessageFloatField(other, field));
          break;
        case GPBDataTypeDouble:
          GPBSetDoubleIvarWithFieldPrivate(self, field, GPBGetMessageDoubleField(other, field));
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeString: {
          id otherVal = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
          GPBSetObjectIvarWithFieldPrivate(self, field, otherVal);
          break;
        }
        case GPBDataTypeMessage:
        case GPBDataTypeGroup: {
          id otherVal = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
          if (GPBGetHasIvar(self, hasIndex, fieldNumber)) {
            GPBMessage *message = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
            [message mergeFrom:otherVal];
          } else {
            GPBMessage *message = [otherVal copy];
            GPBSetRetainedObjectIvarWithFieldPrivate(self, field, message);
          }
          break;
        }
      }  // switch()
    } else if (fieldType == GPBFieldTypeRepeated) {
      // In the case of a list, they need to be appended, and there is no
      // _hasIvar to worry about setting.
      id otherArray = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
      if (otherArray) {
        GPBDataType fieldDataType = field->description_->dataType;
        if (GPBDataTypeIsObject(fieldDataType)) {
          NSMutableArray *resultArray = GetOrCreateArrayIvarWithField(self, field);
          [resultArray addObjectsFromArray:otherArray];
        } else if (fieldDataType == GPBDataTypeEnum) {
          GPBEnumArray *resultArray = GetOrCreateArrayIvarWithField(self, field);
          [resultArray addRawValuesFromArray:otherArray];
        } else {
          // The array type doesn't matter, that all implement
          // -addValuesFromArray:.
          GPBInt32Array *resultArray = GetOrCreateArrayIvarWithField(self, field);
          [resultArray addValuesFromArray:otherArray];
        }
      }
    } else {  // fieldType = GPBFieldTypeMap
      // In the case of a map, they need to be merged, and there is no
      // _hasIvar to worry about setting.
      id otherDict = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
      if (otherDict) {
        GPBDataType keyDataType = field.mapKeyDataType;
        GPBDataType valueDataType = field->description_->dataType;
        if (GPBDataTypeIsObject(keyDataType) && GPBDataTypeIsObject(valueDataType)) {
          NSMutableDictionary *resultDict = GetOrCreateMapIvarWithField(self, field);
          [resultDict addEntriesFromDictionary:otherDict];
        } else if (valueDataType == GPBDataTypeEnum) {
          // The exact type doesn't matter, just need to know it is a
          // GPB*EnumDictionary.
          GPBInt32EnumDictionary *resultDict = GetOrCreateMapIvarWithField(self, field);
          [resultDict addRawEntriesFromDictionary:otherDict];
        } else {
          // The exact type doesn't matter, they all implement
          // -addEntriesFromDictionary:.
          GPBInt32Int32Dictionary *resultDict = GetOrCreateMapIvarWithField(self, field);
          [resultDict addEntriesFromDictionary:otherDict];
        }
      }
    }  // if (fieldType)..else if...else
  }  // for(fields)

  // Unknown fields.
  if (unknownFields_) {
#if defined(DEBUG) && DEBUG
    NSAssert(unknownFieldData_ == nil, @"Internal error both unknown states were set");
#endif
    @synchronized(other) {
      if (other->unknownFields_) {
#if defined(DEBUG) && DEBUG
        NSAssert(other->unknownFieldData_ == nil, @"Internal error both unknown states were set");
#endif
        [unknownFields_ mergeUnknownFields:other->unknownFields_];
      } else if (other->unknownFieldData_) {
        MergeUnknownFieldDataIntoFieldSet(self, other->unknownFieldData_, nil);
      }
    }  // @synchronized(other)
  } else {
    NSData *otherData = GPBMessageUnknownFieldsData(other);
    if (otherData) {
      if (unknownFieldData_) {
        [unknownFieldData_ appendData:otherData];
      } else {
        unknownFieldData_ = [otherData mutableCopy];
      }
    }
  }

  // Extensions

  if (other->extensionMap_.count == 0) {
    return;
  }

  if (extensionMap_ == nil) {
    extensionMap_ = CloneExtensionMap(other->extensionMap_, NSZoneFromPointer(self));
  } else {
    for (GPBExtensionDescriptor *extension in other->extensionMap_) {
      id otherValue = [other->extensionMap_ objectForKey:extension];
      id value = [extensionMap_ objectForKey:extension];
      BOOL isMessageExtension = GPBExtensionIsMessage(extension);

      if (extension.repeated) {
        NSMutableArray *list = value;
        if (list == nil) {
          list = [[NSMutableArray alloc] init];
          [extensionMap_ setObject:list forKey:extension];
          [list release];
        }
        if (isMessageExtension) {
          for (GPBMessage *otherListValue in otherValue) {
            GPBMessage *copiedValue = [otherListValue copy];
            [list addObject:copiedValue];
            [copiedValue release];
          }
        } else {
          [list addObjectsFromArray:otherValue];
        }
      } else {
        if (isMessageExtension) {
          if (value) {
            [(GPBMessage *)value mergeFrom:(GPBMessage *)otherValue];
          } else {
            GPBMessage *copiedValue = [otherValue copy];
            [extensionMap_ setObject:copiedValue forKey:extension];
            [copiedValue release];
          }
        } else {
          [extensionMap_ setObject:otherValue forKey:extension];
        }
      }

      if (isMessageExtension && !extension.isRepeated) {
        GPBMessage *autocreatedValue = [[autocreatedExtensionMap_ objectForKey:extension] retain];
        // Must remove from the map before calling GPBClearMessageAutocreator()
        // so that GPBClearMessageAutocreator() knows its safe to clear.
        [autocreatedExtensionMap_ removeObjectForKey:extension];
        GPBClearMessageAutocreator(autocreatedValue);
        [autocreatedValue release];
      }
    }
  }
}

#pragma mark - isEqual: & hash Support

- (BOOL)isEqual:(id)other {
  if (other == self) {
    return YES;
  }
  if (![other isKindOfClass:[GPBMessage class]]) {
    return NO;
  }
  GPBMessage *otherMsg = other;
  GPBDescriptor *descriptor = [[self class] descriptor];
  if ([[otherMsg class] descriptor] != descriptor) {
    return NO;
  }
  uint8_t *selfStorage = (uint8_t *)messageStorage_;
  uint8_t *otherStorage = (uint8_t *)otherMsg->messageStorage_;

  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      // In the case of a list or map, there is no _hasIvar to worry about.
      // NOTE: These are NSArray/GPB*Array or NSDictionary/GPB*Dictionary, but
      // the type doesn't really matter as the objects all support -count and
      // -isEqual:.
      NSArray *resultMapOrArray = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      NSArray *otherMapOrArray = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
      // nil and empty are equal
      if (resultMapOrArray.count != 0 || otherMapOrArray.count != 0) {
        if (![resultMapOrArray isEqual:otherMapOrArray]) {
          return NO;
        }
      }
    } else {  // Single field
      int32_t hasIndex = GPBFieldHasIndex(field);
      uint32_t fieldNum = GPBFieldNumber(field);
      BOOL selfHas = GPBGetHasIvar(self, hasIndex, fieldNum);
      BOOL otherHas = GPBGetHasIvar(other, hasIndex, fieldNum);
      if (selfHas != otherHas) {
        return NO;  // Differing has values, not equal.
      }
      if (!selfHas) {
        // Same has values, was no, nothing else to check for this field.
        continue;
      }
      // Now compare the values.
      GPBDataType fieldDataType = GPBGetFieldDataType(field);
      size_t fieldOffset = field->description_->offset;
      switch (fieldDataType) {
        case GPBDataTypeBool: {
          // Bools are stored in has_bits to avoid needing explicit space in
          // the storage structure.
          // (the field number passed to the HasIvar helper doesn't really
          // matter since the offset is never negative)
          BOOL selfValue = GPBGetHasIvar(self, (int32_t)(fieldOffset), 0);
          BOOL otherValue = GPBGetHasIvar(other, (int32_t)(fieldOffset), 0);
          if (selfValue != otherValue) {
            return NO;
          }
          break;
        }
        case GPBDataTypeSFixed32:
        case GPBDataTypeInt32:
        case GPBDataTypeSInt32:
        case GPBDataTypeEnum:
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
        case GPBDataTypeFloat: {
          GPBInternalCompileAssert(sizeof(float) == sizeof(uint32_t), float_not_32_bits);
          // These are all 32bit, signed/unsigned doesn't matter for equality.
          uint32_t *selfValPtr = (uint32_t *)&selfStorage[fieldOffset];
          uint32_t *otherValPtr = (uint32_t *)&otherStorage[fieldOffset];
          if (*selfValPtr != *otherValPtr) {
            return NO;
          }
          break;
        }
        case GPBDataTypeSFixed64:
        case GPBDataTypeInt64:
        case GPBDataTypeSInt64:
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
        case GPBDataTypeDouble: {
          GPBInternalCompileAssert(sizeof(double) == sizeof(uint64_t), double_not_64_bits);
          // These are all 64bit, signed/unsigned doesn't matter for equality.
          uint64_t *selfValPtr = (uint64_t *)&selfStorage[fieldOffset];
          uint64_t *otherValPtr = (uint64_t *)&otherStorage[fieldOffset];
          if (*selfValPtr != *otherValPtr) {
            return NO;
          }
          break;
        }
        case GPBDataTypeBytes:
        case GPBDataTypeString:
        case GPBDataTypeMessage:
        case GPBDataTypeGroup: {
          // Type doesn't matter here, they all implement -isEqual:.
          id *selfValPtr = (id *)&selfStorage[fieldOffset];
          id *otherValPtr = (id *)&otherStorage[fieldOffset];
          if (![*selfValPtr isEqual:*otherValPtr]) {
            return NO;
          }
          break;
        }
      }  // switch()
    }  // if(mapOrArray)...else
  }  // for(fields)

  // nil and empty are equal
  if (extensionMap_.count != 0 || otherMsg->extensionMap_.count != 0) {
    if (![extensionMap_ isEqual:otherMsg->extensionMap_]) {
      return NO;
    }
  }

  // Mutation while another thread is doing read only access is invalid, so the only thing we
  // need to guard against is concurrent r/o access, so we can grab the values (and retain them)
  // so we have a version to compare against safely incase the second access causes the transform
  // between internal states.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  GPBUnknownFieldSet *selfUnknownFields;
  NSData *selfUnknownFieldData;
  @synchronized(self) {
    selfUnknownFields = [unknownFields_ retain];
    selfUnknownFieldData = [unknownFieldData_ retain];
  }
  GPBUnknownFieldSet *otherUnknownFields;
  NSData *otherUnknownFieldData;
  @synchronized(otherMsg) {
    otherUnknownFields = [otherMsg->unknownFields_ retain];
    otherUnknownFieldData = [otherMsg->unknownFieldData_ retain];
  }
#pragma clang diagnostic pop
#if defined(DEBUG) && DEBUG && !defined(NS_BLOCK_ASSERTIONS)
  if (selfUnknownFields) {
    NSAssert(selfUnknownFieldData == nil, @"Internal error both unknown states were set");
  }
  if (otherUnknownFields) {
    NSAssert(otherUnknownFieldData == nil, @"Internal error both unknown states were set");
  }
#endif
  // Since a developer can set the legacy unknownFieldSet, treat nil and empty as the same.
  if (selfUnknownFields && selfUnknownFields.countOfFields == 0) {
    [selfUnknownFields release];
    selfUnknownFields = nil;
  }
  if (otherUnknownFields && otherUnknownFields.countOfFields == 0) {
    [otherUnknownFields release];
    otherUnknownFields = nil;
  }

  BOOL result = YES;

  if (selfUnknownFieldData && otherUnknownFieldData) {
    // Both had data, compare it.
    result = [selfUnknownFieldData isEqual:otherUnknownFieldData];
  } else if (selfUnknownFields && otherUnknownFields) {
    // Both had fields set, compare them.
    result = [selfUnknownFields isEqual:otherUnknownFields];
  } else {
    // At this point, we're done to one have a set/nothing, and the other having data/nothing.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    GPBUnknownFieldSet *theSet = selfUnknownFields ? selfUnknownFields : otherUnknownFields;
    NSData *theData = selfUnknownFieldData ? selfUnknownFieldData : otherUnknownFieldData;
    if (theSet) {
      if (theData) {
        GPBUnknownFieldSet *tempSet = [[GPBUnknownFieldSet alloc] init];
        MergeUnknownFieldDataIntoFieldSet(self, theData, tempSet);
        result = [tempSet isEqual:theSet];
        [tempSet release];
      } else {
        result = NO;
      }
    } else {
      // It was a data/nothing and nothing, so they equal if the other didn't have data.
      result = theData == nil;
    }
#pragma clang diagnostic pop
  }

  [selfUnknownFields release];
  [selfUnknownFieldData release];
  [otherUnknownFields release];
  [otherUnknownFieldData release];
  return result;
}

// It is very difficult to implement a generic hash for ProtoBuf messages that
// will perform well. If you need hashing on your ProtoBufs (eg you are using
// them as dictionary keys) you will probably want to implement a ProtoBuf
// message specific hash as a category on your protobuf class. Do not make it a
// category on GPBMessage as you will conflict with this hash, and will possibly
// override hash for all generated protobufs. A good implementation of hash will
// be really fast, so we would recommend only hashing protobufs that have an
// identifier field of some kind that you can easily hash. If you implement
// hash, we would strongly recommend overriding isEqual: in your category as
// well, as the default implementation of isEqual: is extremely slow, and may
// drastically affect performance in large sets.
- (NSUInteger)hash {
  GPBDescriptor *descriptor = [[self class] descriptor];
  const NSUInteger prime = 19;
  uint8_t *storage = (uint8_t *)messageStorage_;

  // Start with the descriptor and then mix it with some instance info.
  // Hopefully that will give a spread based on classes and what fields are set.
  NSUInteger result = (NSUInteger)descriptor;

  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      // Exact type doesn't matter, just check if there are any elements.
      NSArray *mapOrArray = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      NSUInteger count = mapOrArray.count;
      if (count) {
        // NSArray/NSDictionary use count, use the field number and the count.
        result = prime * result + GPBFieldNumber(field);
        result = prime * result + count;
      }
    } else if (GPBGetHasIvarField(self, field)) {
      // Just using the field number seemed simple/fast, but then a small
      // message class where all the same fields are always set (to different
      // things would end up all with the same hash, so pull in some data).
      GPBDataType fieldDataType = GPBGetFieldDataType(field);
      size_t fieldOffset = field->description_->offset;
      switch (fieldDataType) {
        case GPBDataTypeBool: {
          // Bools are stored in has_bits to avoid needing explicit space in
          // the storage structure.
          // (the field number passed to the HasIvar helper doesn't really
          // matter since the offset is never negative)
          BOOL value = GPBGetHasIvar(self, (int32_t)(fieldOffset), 0);
          result = prime * result + value;
          break;
        }
        case GPBDataTypeSFixed32:
        case GPBDataTypeInt32:
        case GPBDataTypeSInt32:
        case GPBDataTypeEnum:
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
        case GPBDataTypeFloat: {
          GPBInternalCompileAssert(sizeof(float) == sizeof(uint32_t), float_not_32_bits);
          // These are all 32bit, just mix it in.
          uint32_t *valPtr = (uint32_t *)&storage[fieldOffset];
          result = prime * result + *valPtr;
          break;
        }
        case GPBDataTypeSFixed64:
        case GPBDataTypeInt64:
        case GPBDataTypeSInt64:
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
        case GPBDataTypeDouble: {
          GPBInternalCompileAssert(sizeof(double) == sizeof(uint64_t), double_not_64_bits);
          // These are all 64bit, just mix what fits into an NSUInteger in.
          uint64_t *valPtr = (uint64_t *)&storage[fieldOffset];
          result = prime * result + (NSUInteger)(*valPtr);
          break;
        }
        case GPBDataTypeBytes:
        case GPBDataTypeString: {
          // Type doesn't matter here, they both implement -hash:.
          id *valPtr = (id *)&storage[fieldOffset];
          result = prime * result + [*valPtr hash];
          break;
        }

        case GPBDataTypeMessage:
        case GPBDataTypeGroup: {
          GPBMessage **valPtr = (GPBMessage **)&storage[fieldOffset];
          // Could call -hash on the sub message, but that could recurse pretty
          // deep; follow the lead of NSArray/NSDictionary and don't really
          // recurse for hash, instead use the field number and the descriptor
          // of the sub message.  Yes, this could suck for a bunch of messages
          // where they all only differ in the sub messages, but if you are
          // using a message with sub messages for something that needs -hash,
          // odds are you are also copying them as keys, and that deep copy
          // will also suck.
          result = prime * result + GPBFieldNumber(field);
          result = prime * result + (NSUInteger)[[*valPtr class] descriptor];
          break;
        }
      }  // switch()
    }
  }

  // Unknowns and extensions are not included.

  return result;
}

#pragma mark - Description Support

- (NSString *)description {
  NSString *textFormat = GPBTextFormatForMessage(self, @"    ");
  NSString *description =
      [NSString stringWithFormat:@"<%@ %p>: {\n%@}", [self class], self, textFormat];
  return description;
}

#if defined(DEBUG) && DEBUG

// Xcode 5.1 added support for custom quick look info.
// https://developer.apple.com/library/ios/documentation/IDEs/Conceptual/CustomClassDisplay_in_QuickLook/CH01-quick_look_for_custom_objects/CH01-quick_look_for_custom_objects.html#//apple_ref/doc/uid/TP40014001-CH2-SW1
- (id)debugQuickLookObject {
  return GPBTextFormatForMessage(self, nil);
}

#endif  // DEBUG

#pragma mark - SerializedSize

- (size_t)serializedSize {
  GPBDescriptor *descriptor = [[self class] descriptor];
  size_t result = 0;

  // Has check is done explicitly, so GPBGetObjectIvarWithFieldNoAutocreate()
  // avoids doing the has check again.

  // Fields.
  for (GPBFieldDescriptor *fieldDescriptor in descriptor->fields_) {
    GPBFieldType fieldType = fieldDescriptor.fieldType;
    GPBDataType fieldDataType = GPBGetFieldDataType(fieldDescriptor);

    // Single Fields
    if (fieldType == GPBFieldTypeSingle) {
      BOOL selfHas = GPBGetHasIvarField(self, fieldDescriptor);
      if (!selfHas) {
        continue;  // Nothing to do.
      }

      uint32_t fieldNumber = GPBFieldNumber(fieldDescriptor);

      switch (fieldDataType) {
#define CASE_SINGLE_POD(NAME, TYPE, FUNC_TYPE)                              \
  case GPBDataType##NAME: {                                                 \
    TYPE fieldVal = GPBGetMessage##FUNC_TYPE##Field(self, fieldDescriptor); \
    result += GPBCompute##NAME##Size(fieldNumber, fieldVal);                \
    break;                                                                  \
  }
#define CASE_SINGLE_OBJECT(NAME)                                                \
  case GPBDataType##NAME: {                                                     \
    id fieldVal = GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor); \
    result += GPBCompute##NAME##Size(fieldNumber, fieldVal);                    \
    break;                                                                      \
  }
        CASE_SINGLE_POD(Bool, BOOL, Bool)
        CASE_SINGLE_POD(Fixed32, uint32_t, UInt32)
        CASE_SINGLE_POD(SFixed32, int32_t, Int32)
        CASE_SINGLE_POD(Float, float, Float)
        CASE_SINGLE_POD(Fixed64, uint64_t, UInt64)
        CASE_SINGLE_POD(SFixed64, int64_t, Int64)
        CASE_SINGLE_POD(Double, double, Double)
        CASE_SINGLE_POD(Int32, int32_t, Int32)
        CASE_SINGLE_POD(Int64, int64_t, Int64)
        CASE_SINGLE_POD(SInt32, int32_t, Int32)
        CASE_SINGLE_POD(SInt64, int64_t, Int64)
        CASE_SINGLE_POD(UInt32, uint32_t, UInt32)
        CASE_SINGLE_POD(UInt64, uint64_t, UInt64)
        CASE_SINGLE_OBJECT(Bytes)
        CASE_SINGLE_OBJECT(String)
        CASE_SINGLE_OBJECT(Message)
        CASE_SINGLE_OBJECT(Group)
        CASE_SINGLE_POD(Enum, int32_t, Int32)
#undef CASE_SINGLE_POD
#undef CASE_SINGLE_OBJECT
      }

      // Repeated Fields
    } else if (fieldType == GPBFieldTypeRepeated) {
      id genericArray = GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor);
      NSUInteger count = [genericArray count];
      if (count == 0) {
        continue;  // Nothing to add.
      }
      __block size_t dataSize = 0;

      switch (fieldDataType) {
#define CASE_REPEATED_POD(NAME, TYPE, ARRAY_TYPE) CASE_REPEATED_POD_EXTRA(NAME, TYPE, ARRAY_TYPE, )
#define CASE_REPEATED_POD_EXTRA(NAME, TYPE, ARRAY_TYPE, ARRAY_ACCESSOR_NAME)           \
  case GPBDataType##NAME: {                                                            \
    GPB##ARRAY_TYPE##Array *array = genericArray;                                      \
    [array enumerate##ARRAY_ACCESSOR_NAME##                                            \
        ValuesWithBlock:^(TYPE value, __unused NSUInteger idx, __unused BOOL * stop) { \
          dataSize += GPBCompute##NAME##SizeNoTag(value);                              \
        }];                                                                            \
    break;                                                                             \
  }
#define CASE_REPEATED_OBJECT(NAME)                    \
  case GPBDataType##NAME: {                           \
    for (id value in genericArray) {                  \
      dataSize += GPBCompute##NAME##SizeNoTag(value); \
    }                                                 \
    break;                                            \
  }
        CASE_REPEATED_POD(Bool, BOOL, Bool)
        CASE_REPEATED_POD(Fixed32, uint32_t, UInt32)
        CASE_REPEATED_POD(SFixed32, int32_t, Int32)
        CASE_REPEATED_POD(Float, float, Float)
        CASE_REPEATED_POD(Fixed64, uint64_t, UInt64)
        CASE_REPEATED_POD(SFixed64, int64_t, Int64)
        CASE_REPEATED_POD(Double, double, Double)
        CASE_REPEATED_POD(Int32, int32_t, Int32)
        CASE_REPEATED_POD(Int64, int64_t, Int64)
        CASE_REPEATED_POD(SInt32, int32_t, Int32)
        CASE_REPEATED_POD(SInt64, int64_t, Int64)
        CASE_REPEATED_POD(UInt32, uint32_t, UInt32)
        CASE_REPEATED_POD(UInt64, uint64_t, UInt64)
        CASE_REPEATED_OBJECT(Bytes)
        CASE_REPEATED_OBJECT(String)
        CASE_REPEATED_OBJECT(Message)
        CASE_REPEATED_OBJECT(Group)
        CASE_REPEATED_POD_EXTRA(Enum, int32_t, Enum, Raw)
#undef CASE_REPEATED_POD
#undef CASE_REPEATED_POD_EXTRA
#undef CASE_REPEATED_OBJECT
      }  // switch
      result += dataSize;
      size_t tagSize = GPBComputeTagSize(GPBFieldNumber(fieldDescriptor));
      if (fieldDataType == GPBDataTypeGroup) {
        // Groups have both a start and an end tag.
        tagSize *= 2;
      }
      if (fieldDescriptor.isPackable) {
        result += tagSize;
        result += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
      } else {
        result += count * tagSize;
      }

      // Map<> Fields
    } else {  // fieldType == GPBFieldTypeMap
      if (GPBDataTypeIsObject(fieldDataType) &&
          (fieldDescriptor.mapKeyDataType == GPBDataTypeString)) {
        // If key type was string, then the map is an NSDictionary.
        NSDictionary *map = GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor);
        if (map) {
          result += GPBDictionaryComputeSizeInternalHelper(map, fieldDescriptor);
        }
      } else {
        // Type will be GPB*GroupDictionary, exact type doesn't matter.
        GPBInt32Int32Dictionary *map = GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor);
        result += [map computeSerializedSizeAsField:fieldDescriptor];
      }
    }
  }  // for(fields)

  // Add any unknown fields.
  @synchronized(self) {
    if (unknownFieldData_) {
#if defined(DEBUG) && DEBUG
      NSAssert(unknownFields_ == nil, @"Internal error both unknown states were set");
#endif
      result += [unknownFieldData_ length];
    } else {
      if (descriptor.wireFormat) {
        result += [unknownFields_ serializedSizeAsMessageSet];
      } else {
        result += [unknownFields_ serializedSize];
      }
    }
  }  // @synchronized(self)

  // Add any extensions.
  for (GPBExtensionDescriptor *extension in extensionMap_) {
    id value = [extensionMap_ objectForKey:extension];
    result += GPBComputeExtensionSerializedSizeIncludingTag(extension, value);
  }

  return result;
}

#pragma mark - Resolve Methods Support

typedef struct ResolveIvarAccessorMethodResult {
  IMP impToAdd;
  SEL encodingSelector;
} ResolveIvarAccessorMethodResult;

// |field| can be __unsafe_unretained because they are created at startup
// and are essentially global. No need to pay for retain/release when
// they are captured in blocks.
static void ResolveIvarGet(__unsafe_unretained GPBFieldDescriptor *field,
                           ResolveIvarAccessorMethodResult *result) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
#define CASE_GET(NAME, TYPE, TRUE_NAME)                        \
  case GPBDataType##NAME: {                                    \
    result->impToAdd = imp_implementationWithBlock(^(id obj) { \
      return GPBGetMessage##TRUE_NAME##Field(obj, field);      \
    });                                                        \
    result->encodingSelector = @selector(get##NAME);           \
    break;                                                     \
  }
#define CASE_GET_OBJECT(NAME, TYPE, TRUE_NAME)                 \
  case GPBDataType##NAME: {                                    \
    result->impToAdd = imp_implementationWithBlock(^(id obj) { \
      return GPBGetObjectIvarWithField(obj, field);            \
    });                                                        \
    result->encodingSelector = @selector(get##NAME);           \
    break;                                                     \
  }
    CASE_GET(Bool, BOOL, Bool)
    CASE_GET(Fixed32, uint32_t, UInt32)
    CASE_GET(SFixed32, int32_t, Int32)
    CASE_GET(Float, float, Float)
    CASE_GET(Fixed64, uint64_t, UInt64)
    CASE_GET(SFixed64, int64_t, Int64)
    CASE_GET(Double, double, Double)
    CASE_GET(Int32, int32_t, Int32)
    CASE_GET(Int64, int64_t, Int64)
    CASE_GET(SInt32, int32_t, Int32)
    CASE_GET(SInt64, int64_t, Int64)
    CASE_GET(UInt32, uint32_t, UInt32)
    CASE_GET(UInt64, uint64_t, UInt64)
    CASE_GET_OBJECT(Bytes, id, Object)
    CASE_GET_OBJECT(String, id, Object)
    CASE_GET_OBJECT(Message, id, Object)
    CASE_GET_OBJECT(Group, id, Object)
    CASE_GET(Enum, int32_t, Enum)
#undef CASE_GET
  }
}

// See comment about __unsafe_unretained on ResolveIvarGet.
static void ResolveIvarSet(__unsafe_unretained GPBFieldDescriptor *field,
                           ResolveIvarAccessorMethodResult *result) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
#define CASE_SET(NAME, TYPE, TRUE_NAME)                                    \
  case GPBDataType##NAME: {                                                \
    result->impToAdd = imp_implementationWithBlock(^(id obj, TYPE value) { \
      return GPBSet##TRUE_NAME##IvarWithFieldPrivate(obj, field, value);   \
    });                                                                    \
    result->encodingSelector = @selector(set##NAME:);                      \
    break;                                                                 \
  }
#define CASE_SET_COPY(NAME)                                                      \
  case GPBDataType##NAME: {                                                      \
    result->impToAdd = imp_implementationWithBlock(^(id obj, id value) {         \
      return GPBSetRetainedObjectIvarWithFieldPrivate(obj, field, [value copy]); \
    });                                                                          \
    result->encodingSelector = @selector(set##NAME:);                            \
    break;                                                                       \
  }
    CASE_SET(Bool, BOOL, Bool)
    CASE_SET(Fixed32, uint32_t, UInt32)
    CASE_SET(SFixed32, int32_t, Int32)
    CASE_SET(Float, float, Float)
    CASE_SET(Fixed64, uint64_t, UInt64)
    CASE_SET(SFixed64, int64_t, Int64)
    CASE_SET(Double, double, Double)
    CASE_SET(Int32, int32_t, Int32)
    CASE_SET(Int64, int64_t, Int64)
    CASE_SET(SInt32, int32_t, Int32)
    CASE_SET(SInt64, int64_t, Int64)
    CASE_SET(UInt32, uint32_t, UInt32)
    CASE_SET(UInt64, uint64_t, UInt64)
    CASE_SET_COPY(Bytes)
    CASE_SET_COPY(String)
    CASE_SET(Message, id, Object)
    CASE_SET(Group, id, Object)
    CASE_SET(Enum, int32_t, Enum)
#undef CASE_SET
  }
}

// Highly optimized routines for determining selector types.
// Meant to only be used by GPBMessage when resolving selectors in
// `+ (BOOL)resolveInstanceMethod:(SEL)sel`.
// These routines are intended to make negative decisions as fast as possible.
GPB_INLINE char GPBFastToUpper(char c) { return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c; }

GPB_INLINE BOOL GPBIsGetSelForField(const char *selName, GPBFieldDescriptor *descriptor) {
  // Does 'selName' == '<name>'?
  // selName and <name> have to be at least two characters long (i.e. ('a', '\0')" is the shortest
  // selector you can have).
  return (selName[0] == descriptor->description_->name[0]) &&
         (selName[1] == descriptor->description_->name[1]) &&
         (strcmp(selName + 1, descriptor->description_->name + 1) == 0);
}

GPB_INLINE BOOL GPBIsSetSelForField(const char *selName, size_t selNameLength,
                                    GPBFieldDescriptor *descriptor) {
  // Does 'selName' == 'set<Name>:'?
  // Do fastest compares up front
  const size_t kSetLength = strlen("set");
  // kSetLength is 3 and one for the colon.
  if (selNameLength <= kSetLength + 1) {
    return NO;
  }
  if (selName[kSetLength] != GPBFastToUpper(descriptor->description_->name[0])) {
    return NO;
  }

  // NB we check for "set" and the colon later in this routine because we have already checked for
  // starting with "s" and ending with ":" in `+resolveInstanceMethod:` before we get here.
  if (selName[0] != 's' || selName[1] != 'e' || selName[2] != 't') {
    return NO;
  }

  if (selName[selNameLength - 1] != ':') {
    return NO;
  }

  // Slow path.
  size_t nameLength = strlen(descriptor->description_->name);
  size_t setSelLength = nameLength + kSetLength + 1;
  if (selNameLength != setSelLength) {
    return NO;
  }
  if (strncmp(&selName[kSetLength + 1], descriptor->description_->name + 1, nameLength - 1) != 0) {
    return NO;
  }

  return YES;
}

GPB_INLINE BOOL GPBFieldHasHas(GPBFieldDescriptor *descriptor) {
  // It gets has/setHas selectors if...
  //  - not in a oneof (negative has index)
  //  - not clearing on zero
  return (descriptor->description_->hasIndex >= 0) &&
         ((descriptor->description_->flags & GPBFieldClearHasIvarOnZero) == 0);
}

GPB_INLINE BOOL GPBIsHasSelForField(const char *selName, size_t selNameLength,
                                    GPBFieldDescriptor *descriptor) {
  // Does 'selName' == 'has<Name>'?
  // Do fastest compares up front.
  const size_t kHasLength = strlen("has");
  if (selNameLength <= kHasLength) {
    return NO;
  }
  if (selName[0] != 'h' || selName[1] != 'a' || selName[2] != 's') {
    return NO;
  }
  if (selName[kHasLength] != GPBFastToUpper(descriptor->description_->name[0])) {
    return NO;
  }
  if (!GPBFieldHasHas(descriptor)) {
    return NO;
  }

  // Slow path.
  size_t nameLength = strlen(descriptor->description_->name);
  size_t setSelLength = nameLength + kHasLength;
  if (selNameLength != setSelLength) {
    return NO;
  }

  if (strncmp(&selName[kHasLength + 1], descriptor->description_->name + 1, nameLength - 1) != 0) {
    return NO;
  }
  return YES;
}

GPB_INLINE BOOL GPBIsCountSelForField(const char *selName, size_t selNameLength,
                                      GPBFieldDescriptor *descriptor) {
  // Does 'selName' == '<name>_Count'?
  // Do fastest compares up front.
  if (selName[0] != descriptor->description_->name[0]) {
    return NO;
  }
  const size_t kCountLength = strlen("_Count");
  if (selNameLength <= kCountLength) {
    return NO;
  }

  if (selName[selNameLength - kCountLength] != '_') {
    return NO;
  }

  // Slow path.
  size_t nameLength = strlen(descriptor->description_->name);
  size_t setSelLength = nameLength + kCountLength;
  if (selNameLength != setSelLength) {
    return NO;
  }
  if (strncmp(selName, descriptor->description_->name, nameLength) != 0) {
    return NO;
  }
  if (strncmp(&selName[nameLength], "_Count", kCountLength) != 0) {
    return NO;
  }
  return YES;
}

GPB_INLINE BOOL GPBIsSetHasSelForField(const char *selName, size_t selNameLength,
                                       GPBFieldDescriptor *descriptor) {
  // Does 'selName' == 'setHas<Name>:'?
  // Do fastest compares up front.
  const size_t kSetHasLength = strlen("setHas");
  // kSetHasLength is 6 and one for the colon.
  if (selNameLength <= kSetHasLength + 1) {
    return NO;
  }
  if (selName[selNameLength - 1] != ':') {
    return NO;
  }
  if (selName[kSetHasLength] != GPBFastToUpper(descriptor->description_->name[0])) {
    return NO;
  }
  if (selName[0] != 's' || selName[1] != 'e' || selName[2] != 't' || selName[3] != 'H' ||
      selName[4] != 'a' || selName[5] != 's') {
    return NO;
  }

  if (!GPBFieldHasHas(descriptor)) {
    return NO;
  }
  // Slow path.
  size_t nameLength = strlen(descriptor->description_->name);
  size_t setHasSelLength = nameLength + kSetHasLength + 1;
  if (selNameLength != setHasSelLength) {
    return NO;
  }
  if (strncmp(&selName[kSetHasLength + 1], descriptor->description_->name + 1, nameLength - 1) !=
      0) {
    return NO;
  }

  return YES;
}

GPB_INLINE BOOL GPBIsCaseOfSelForOneOf(const char *selName, size_t selNameLength,
                                       GPBOneofDescriptor *descriptor) {
  // Does 'selName' == '<name>OneOfCase'?
  // Do fastest compares up front.
  if (selName[0] != descriptor->name_[0]) {
    return NO;
  }
  const size_t kOneOfCaseLength = strlen("OneOfCase");
  if (selNameLength <= kOneOfCaseLength) {
    return NO;
  }
  if (selName[selNameLength - kOneOfCaseLength] != 'O') {
    return NO;
  }

  // Slow path.
  size_t nameLength = strlen(descriptor->name_);
  size_t setSelLength = nameLength + kOneOfCaseLength;
  if (selNameLength != setSelLength) {
    return NO;
  }
  if (strncmp(&selName[nameLength], "OneOfCase", kOneOfCaseLength) != 0) {
    return NO;
  }
  if (strncmp(selName, descriptor->name_, nameLength) != 0) {
    return NO;
  }
  return YES;
}

+ (BOOL)resolveInstanceMethod:(SEL)sel {
  const GPBDescriptor *descriptor = [self descriptor];
  if (!descriptor) {
    return [super resolveInstanceMethod:sel];
  }
  ResolveIvarAccessorMethodResult result = {NULL, NULL};

  const char *selName = sel_getName(sel);
  const size_t selNameLength = strlen(selName);
  // A setter has a leading 's' and a trailing ':' (e.g. 'setFoo:' or 'setHasFoo:').
  BOOL couldBeSetter = selName[0] == 's' && selName[selNameLength - 1] == ':';
  if (couldBeSetter) {
    // See comment about __unsafe_unretained on ResolveIvarGet.
    for (__unsafe_unretained GPBFieldDescriptor *field in descriptor->fields_) {
      BOOL isMapOrArray = GPBFieldIsMapOrArray(field);
      if (GPBIsSetSelForField(selName, selNameLength, field)) {
        if (isMapOrArray) {
          // Local for syntax so the block can directly capture it and not the
          // full lookup.
          result.impToAdd = imp_implementationWithBlock(^(id obj, id value) {
            GPBSetObjectIvarWithFieldPrivate(obj, field, value);
          });
          result.encodingSelector = @selector(setArray:);
        } else {
          ResolveIvarSet(field, &result);
        }
        break;
      } else if (!isMapOrArray && GPBIsSetHasSelForField(selName, selNameLength, field)) {
        result.impToAdd = imp_implementationWithBlock(^(id obj, BOOL value) {
          if (value) {
            [NSException raise:NSInvalidArgumentException
                        format:@"%@: %@ can only be set to NO (to clear field).", [obj class],
                               NSStringFromSelector(sel)];
          }
          GPBClearMessageField(obj, field);
        });
        result.encodingSelector = @selector(setBool:);
        break;
      }
    }
  } else {
    // See comment about __unsafe_unretained on ResolveIvarGet.
    for (__unsafe_unretained GPBFieldDescriptor *field in descriptor->fields_) {
      BOOL isMapOrArray = GPBFieldIsMapOrArray(field);
      if (GPBIsGetSelForField(selName, field)) {
        if (isMapOrArray) {
          if (field.fieldType == GPBFieldTypeRepeated) {
            result.impToAdd = imp_implementationWithBlock(^(id obj) {
              return GetArrayIvarWithField(obj, field);
            });
          } else {
            result.impToAdd = imp_implementationWithBlock(^(id obj) {
              return GetMapIvarWithField(obj, field);
            });
          }
          result.encodingSelector = @selector(getArray);
        } else {
          ResolveIvarGet(field, &result);
        }
        break;
      }
      if (!isMapOrArray) {
        if (GPBIsHasSelForField(selName, selNameLength, field)) {
          int32_t index = GPBFieldHasIndex(field);
          uint32_t fieldNum = GPBFieldNumber(field);
          result.impToAdd = imp_implementationWithBlock(^(id obj) {
            return GPBGetHasIvar(obj, index, fieldNum);
          });
          result.encodingSelector = @selector(getBool);
          break;
        } else {
          GPBOneofDescriptor *oneof = field->containingOneof_;
          if (oneof && GPBIsCaseOfSelForOneOf(selName, selNameLength, oneof)) {
            int32_t index = GPBFieldHasIndex(field);
            result.impToAdd = imp_implementationWithBlock(^(id obj) {
              return GPBGetHasOneof(obj, index);
            });
            result.encodingSelector = @selector(getEnum);
            break;
          }
        }
      } else {
        if (GPBIsCountSelForField(selName, selNameLength, field)) {
          result.impToAdd = imp_implementationWithBlock(^(id obj) {
            // Type doesn't matter, all *Array and *Dictionary types support
            // -count.
            NSArray *arrayOrMap = GPBGetObjectIvarWithFieldNoAutocreate(obj, field);
            return [arrayOrMap count];
          });
          result.encodingSelector = @selector(getArrayCount);
          break;
        }
      }
    }
  }

  if (result.impToAdd) {
    const char *encoding = GPBMessageEncodingForSelector(result.encodingSelector, YES);
    Class msgClass = descriptor.messageClass;
    BOOL methodAdded = class_addMethod(msgClass, sel, result.impToAdd, encoding);
    // class_addMethod() is documented as also failing if the method was already
    // added; so we check if the method is already there and return success so
    // the method dispatch will still happen.  Why would it already be added?
    // Two threads could cause the same method to be bound at the same time,
    // but only one will actually bind it; the other still needs to return true
    // so things will dispatch.
    if (!methodAdded) {
      methodAdded = GPBClassHasSel(msgClass, sel);
    }
    return methodAdded;
  }
  return [super resolveInstanceMethod:sel];
}

+ (BOOL)resolveClassMethod:(SEL)sel {
  // Extensions scoped to a Message and looked up via class methods.
  if (GPBResolveExtensionClassMethod(self, sel)) {
    return YES;
  }
  return [super resolveClassMethod:sel];
}

#pragma mark - NSCoding Support

+ (BOOL)supportsSecureCoding {
  return YES;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
  self = [self init];
  if (self) {
    NSData *data = [aDecoder decodeObjectOfClass:[NSData class] forKey:kGPBDataCoderKey];
    if (data.length) {
      GPBCodedInputStream *input = [[GPBCodedInputStream alloc] initWithData:data];
      @try {
        [self mergeFromCodedInputStream:input extensionRegistry:nil endingTag:0];
      } @finally {
        [input release];
      }
    }
  }
  return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {
#if defined(DEBUG) && DEBUG
  if (extensionMap_.count) {
    // Hint to go along with the docs on GPBMessage about this.
    //
    // Note: This is incomplete, in that it only checked the "root" message,
    // if a sub message in a field has extensions, the issue still exists. A
    // recursive check could be done here (like the work in
    // GPBMessageDropUnknownFieldsRecursively()), but that has the potential to
    // be expensive and could slow down serialization in DEBUG enough to cause
    // developers other problems.
    NSLog(@"Warning: writing out a GPBMessage (%@) via NSCoding and it"
          @" has %ld extensions; when read back in, those fields will be"
          @" in the unknownFields property instead.",
          [self class], (long)extensionMap_.count);
  }
#endif
  NSData *data = [self data];
  if (data.length) {
    [aCoder encodeObject:data forKey:kGPBDataCoderKey];
  }
}

#pragma mark - KVC Support

+ (BOOL)accessInstanceVariablesDirectly {
  // Make sure KVC doesn't use instance variables.
  return NO;
}

@end

#pragma mark - Messages from GPBUtilities.h but defined here for access to helpers.

// Only exists for public api, no core code should use this.
id GPBGetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field) {
#if defined(DEBUG) && DEBUG
  if (field.fieldType != GPBFieldTypeRepeated) {
    [NSException raise:NSInvalidArgumentException
                format:@"%@.%@ is not a repeated field.", [self class], field.name];
  }
#endif
  return GetOrCreateArrayIvarWithField(self, field);
}

// Only exists for public api, no core code should use this.
id GPBGetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field) {
#if defined(DEBUG) && DEBUG
  if (field.fieldType != GPBFieldTypeMap) {
    [NSException raise:NSInvalidArgumentException
                format:@"%@.%@ is not a map<> field.", [self class], field.name];
  }
#endif
  return GetOrCreateMapIvarWithField(self, field);
}

id GPBGetObjectIvarWithField(GPBMessage *self, GPBFieldDescriptor *field) {
  NSCAssert(!GPBFieldIsMapOrArray(field), @"Shouldn't get here");
  if (!GPBFieldDataTypeIsMessage(field)) {
    if (GPBGetHasIvarField(self, field)) {
      uint8_t *storage = (uint8_t *)self->messageStorage_;
      id *typePtr = (id *)&storage[field->description_->offset];
      return *typePtr;
    }
    // Not set...non messages (string/data), get their default.
    return field.defaultValue.valueMessage;
  }

  uint8_t *storage = (uint8_t *)self->messageStorage_;
  _Atomic(id) *typePtr = (_Atomic(id) *)&storage[field->description_->offset];
  id msg = atomic_load(typePtr);
  if (msg) {
    return msg;
  }

  id expected = nil;
  id autocreated = GPBCreateMessageWithAutocreator(field.msgClass, self, field);
  if (atomic_compare_exchange_strong(typePtr, &expected, autocreated)) {
    // Value was set, return it.
    return autocreated;
  }

  // Some other thread set it, release the one created and return what got set.
  GPBClearMessageAutocreator(autocreated);
  [autocreated release];
  return expected;
}

NSData *GPBMessageUnknownFieldsData(GPBMessage *self) {
  NSData *result = nil;
  @synchronized(self) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    GPBUnknownFieldSet *unknownFields = self->unknownFields_;
#pragma clang diagnostic pop
    if (unknownFields) {
#if defined(DEBUG) && DEBUG
      NSCAssert(self->unknownFieldData_ == nil, @"Internal error both unknown states were set");
#endif
      if (self.descriptor.isWireFormat) {
        NSMutableData *mutableData =
            [NSMutableData dataWithLength:unknownFields.serializedSizeAsMessageSet];
        GPBCodedOutputStream *output = [[GPBCodedOutputStream alloc] initWithData:mutableData];
        [unknownFields writeAsMessageSetTo:output];
        [output flush];
        [output release];
        result = mutableData;
      } else {
        result = [unknownFields data];
      }
    } else {
      // Internally we can borrow it without a copy since this is immediately used by callers
      // and multithreaded access with any mutation is not allow on messages.
      result = self->unknownFieldData_;
    }
  }  // @synchronized(self)
  return result;
}

#pragma clang diagnostic pop
