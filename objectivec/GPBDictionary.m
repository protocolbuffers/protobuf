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

#import "GPBDictionary_PackagePrivate.h"

#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

// ------------------------------ NOTE ------------------------------
// At the moment, this is all using NSNumbers in NSDictionaries under
// the hood, but it is all hidden so we can come back and optimize
// with direct CFDictionary usage later.  The reason that wasn't
// done yet is needing to support 32bit iOS builds.  Otherwise
// it would be pretty simple to store all this data in CFDictionaries
// directly.
// ------------------------------------------------------------------

// Used to include code only visible to specific versions of the static
// analyzer. Useful for wrapping code that only exists to silence the analyzer.
// Determine the values you want to use for BEGIN_APPLE_BUILD_VERSION,
// END_APPLE_BUILD_VERSION using:
//   xcrun clang -dM -E -x c /dev/null | grep __apple_build_version__
// Example usage:
//  #if GPB_STATIC_ANALYZER_ONLY(5621, 5623) ... #endif
#define GPB_STATIC_ANALYZER_ONLY(BEGIN_APPLE_BUILD_VERSION, END_APPLE_BUILD_VERSION) \
    (defined(__clang_analyzer__) && \
     (__apple_build_version__ >= BEGIN_APPLE_BUILD_VERSION && \
      __apple_build_version__ <= END_APPLE_BUILD_VERSION))

enum {
  kMapKeyFieldNumber = 1,
  kMapValueFieldNumber = 2,
};

static BOOL DictDefault_IsValidValue(int32_t value) {
  // Anything but the bad value marker is allowed.
  return (value != kGPBUnrecognizedEnumeratorValue);
}

//%PDDM-DEFINE SERIALIZE_SUPPORT_2_TYPE(VALUE_NAME, VALUE_TYPE, GPBDATATYPE_NAME1, GPBDATATYPE_NAME2)
//%static size_t ComputeDict##VALUE_NAME##FieldSize(VALUE_TYPE value, uint32_t fieldNum, GPBDataType dataType) {
//%  if (dataType == GPBDataType##GPBDATATYPE_NAME1) {
//%    return GPBCompute##GPBDATATYPE_NAME1##Size(fieldNum, value);
//%  } else if (dataType == GPBDataType##GPBDATATYPE_NAME2) {
//%    return GPBCompute##GPBDATATYPE_NAME2##Size(fieldNum, value);
//%  } else {
//%    NSCAssert(NO, @"Unexpected type %d", dataType);
//%    return 0;
//%  }
//%}
//%
//%static void WriteDict##VALUE_NAME##Field(GPBCodedOutputStream *stream, VALUE_TYPE value, uint32_t fieldNum, GPBDataType dataType) {
//%  if (dataType == GPBDataType##GPBDATATYPE_NAME1) {
//%    [stream write##GPBDATATYPE_NAME1##:fieldNum value:value];
//%  } else if (dataType == GPBDataType##GPBDATATYPE_NAME2) {
//%    [stream write##GPBDATATYPE_NAME2##:fieldNum value:value];
//%  } else {
//%    NSCAssert(NO, @"Unexpected type %d", dataType);
//%  }
//%}
//%
//%PDDM-DEFINE SERIALIZE_SUPPORT_3_TYPE(VALUE_NAME, VALUE_TYPE, GPBDATATYPE_NAME1, GPBDATATYPE_NAME2, GPBDATATYPE_NAME3)
//%static size_t ComputeDict##VALUE_NAME##FieldSize(VALUE_TYPE value, uint32_t fieldNum, GPBDataType dataType) {
//%  if (dataType == GPBDataType##GPBDATATYPE_NAME1) {
//%    return GPBCompute##GPBDATATYPE_NAME1##Size(fieldNum, value);
//%  } else if (dataType == GPBDataType##GPBDATATYPE_NAME2) {
//%    return GPBCompute##GPBDATATYPE_NAME2##Size(fieldNum, value);
//%  } else if (dataType == GPBDataType##GPBDATATYPE_NAME3) {
//%    return GPBCompute##GPBDATATYPE_NAME3##Size(fieldNum, value);
//%  } else {
//%    NSCAssert(NO, @"Unexpected type %d", dataType);
//%    return 0;
//%  }
//%}
//%
//%static void WriteDict##VALUE_NAME##Field(GPBCodedOutputStream *stream, VALUE_TYPE value, uint32_t fieldNum, GPBDataType dataType) {
//%  if (dataType == GPBDataType##GPBDATATYPE_NAME1) {
//%    [stream write##GPBDATATYPE_NAME1##:fieldNum value:value];
//%  } else if (dataType == GPBDataType##GPBDATATYPE_NAME2) {
//%    [stream write##GPBDATATYPE_NAME2##:fieldNum value:value];
//%  } else if (dataType == GPBDataType##GPBDATATYPE_NAME3) {
//%    [stream write##GPBDATATYPE_NAME3##:fieldNum value:value];
//%  } else {
//%    NSCAssert(NO, @"Unexpected type %d", dataType);
//%  }
//%}
//%
//%PDDM-DEFINE SIMPLE_SERIALIZE_SUPPORT(VALUE_NAME, VALUE_TYPE, VisP)
//%static size_t ComputeDict##VALUE_NAME##FieldSize(VALUE_TYPE VisP##value, uint32_t fieldNum, GPBDataType dataType) {
//%  NSCAssert(dataType == GPBDataType##VALUE_NAME, @"bad type: %d", dataType);
//%  #pragma unused(dataType)  // For when asserts are off in release.
//%  return GPBCompute##VALUE_NAME##Size(fieldNum, value);
//%}
//%
//%static void WriteDict##VALUE_NAME##Field(GPBCodedOutputStream *stream, VALUE_TYPE VisP##value, uint32_t fieldNum, GPBDataType dataType) {
//%  NSCAssert(dataType == GPBDataType##VALUE_NAME, @"bad type: %d", dataType);
//%  #pragma unused(dataType)  // For when asserts are off in release.
//%  [stream write##VALUE_NAME##:fieldNum value:value];
//%}
//%
//%PDDM-DEFINE SERIALIZE_SUPPORT_HELPERS()
//%SERIALIZE_SUPPORT_3_TYPE(Int32, int32_t, Int32, SInt32, SFixed32)
//%SERIALIZE_SUPPORT_2_TYPE(UInt32, uint32_t, UInt32, Fixed32)
//%SERIALIZE_SUPPORT_3_TYPE(Int64, int64_t, Int64, SInt64, SFixed64)
//%SERIALIZE_SUPPORT_2_TYPE(UInt64, uint64_t, UInt64, Fixed64)
//%SIMPLE_SERIALIZE_SUPPORT(Bool, BOOL, )
//%SIMPLE_SERIALIZE_SUPPORT(Enum, int32_t, )
//%SIMPLE_SERIALIZE_SUPPORT(Float, float, )
//%SIMPLE_SERIALIZE_SUPPORT(Double, double, )
//%SIMPLE_SERIALIZE_SUPPORT(String, NSString, *)
//%SERIALIZE_SUPPORT_3_TYPE(Object, id, Message, String, Bytes)
//%PDDM-EXPAND SERIALIZE_SUPPORT_HELPERS()
// This block of code is generated, do not edit it directly.

static size_t ComputeDictInt32FieldSize(int32_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeInt32) {
    return GPBComputeInt32Size(fieldNum, value);
  } else if (dataType == GPBDataTypeSInt32) {
    return GPBComputeSInt32Size(fieldNum, value);
  } else if (dataType == GPBDataTypeSFixed32) {
    return GPBComputeSFixed32Size(fieldNum, value);
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
    return 0;
  }
}

static void WriteDictInt32Field(GPBCodedOutputStream *stream, int32_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeInt32) {
    [stream writeInt32:fieldNum value:value];
  } else if (dataType == GPBDataTypeSInt32) {
    [stream writeSInt32:fieldNum value:value];
  } else if (dataType == GPBDataTypeSFixed32) {
    [stream writeSFixed32:fieldNum value:value];
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
  }
}

static size_t ComputeDictUInt32FieldSize(uint32_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeUInt32) {
    return GPBComputeUInt32Size(fieldNum, value);
  } else if (dataType == GPBDataTypeFixed32) {
    return GPBComputeFixed32Size(fieldNum, value);
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
    return 0;
  }
}

static void WriteDictUInt32Field(GPBCodedOutputStream *stream, uint32_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeUInt32) {
    [stream writeUInt32:fieldNum value:value];
  } else if (dataType == GPBDataTypeFixed32) {
    [stream writeFixed32:fieldNum value:value];
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
  }
}

static size_t ComputeDictInt64FieldSize(int64_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeInt64) {
    return GPBComputeInt64Size(fieldNum, value);
  } else if (dataType == GPBDataTypeSInt64) {
    return GPBComputeSInt64Size(fieldNum, value);
  } else if (dataType == GPBDataTypeSFixed64) {
    return GPBComputeSFixed64Size(fieldNum, value);
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
    return 0;
  }
}

static void WriteDictInt64Field(GPBCodedOutputStream *stream, int64_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeInt64) {
    [stream writeInt64:fieldNum value:value];
  } else if (dataType == GPBDataTypeSInt64) {
    [stream writeSInt64:fieldNum value:value];
  } else if (dataType == GPBDataTypeSFixed64) {
    [stream writeSFixed64:fieldNum value:value];
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
  }
}

static size_t ComputeDictUInt64FieldSize(uint64_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeUInt64) {
    return GPBComputeUInt64Size(fieldNum, value);
  } else if (dataType == GPBDataTypeFixed64) {
    return GPBComputeFixed64Size(fieldNum, value);
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
    return 0;
  }
}

static void WriteDictUInt64Field(GPBCodedOutputStream *stream, uint64_t value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeUInt64) {
    [stream writeUInt64:fieldNum value:value];
  } else if (dataType == GPBDataTypeFixed64) {
    [stream writeFixed64:fieldNum value:value];
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
  }
}

static size_t ComputeDictBoolFieldSize(BOOL value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeBool, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  return GPBComputeBoolSize(fieldNum, value);
}

static void WriteDictBoolField(GPBCodedOutputStream *stream, BOOL value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeBool, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  [stream writeBool:fieldNum value:value];
}

static size_t ComputeDictEnumFieldSize(int32_t value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeEnum, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  return GPBComputeEnumSize(fieldNum, value);
}

static void WriteDictEnumField(GPBCodedOutputStream *stream, int32_t value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeEnum, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  [stream writeEnum:fieldNum value:value];
}

static size_t ComputeDictFloatFieldSize(float value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeFloat, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  return GPBComputeFloatSize(fieldNum, value);
}

static void WriteDictFloatField(GPBCodedOutputStream *stream, float value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeFloat, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  [stream writeFloat:fieldNum value:value];
}

static size_t ComputeDictDoubleFieldSize(double value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeDouble, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  return GPBComputeDoubleSize(fieldNum, value);
}

static void WriteDictDoubleField(GPBCodedOutputStream *stream, double value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeDouble, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  [stream writeDouble:fieldNum value:value];
}

static size_t ComputeDictStringFieldSize(NSString *value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeString, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  return GPBComputeStringSize(fieldNum, value);
}

static void WriteDictStringField(GPBCodedOutputStream *stream, NSString *value, uint32_t fieldNum, GPBDataType dataType) {
  NSCAssert(dataType == GPBDataTypeString, @"bad type: %d", dataType);
  #pragma unused(dataType)  // For when asserts are off in release.
  [stream writeString:fieldNum value:value];
}

static size_t ComputeDictObjectFieldSize(id value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeMessage) {
    return GPBComputeMessageSize(fieldNum, value);
  } else if (dataType == GPBDataTypeString) {
    return GPBComputeStringSize(fieldNum, value);
  } else if (dataType == GPBDataTypeBytes) {
    return GPBComputeBytesSize(fieldNum, value);
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
    return 0;
  }
}

static void WriteDictObjectField(GPBCodedOutputStream *stream, id value, uint32_t fieldNum, GPBDataType dataType) {
  if (dataType == GPBDataTypeMessage) {
    [stream writeMessage:fieldNum value:value];
  } else if (dataType == GPBDataTypeString) {
    [stream writeString:fieldNum value:value];
  } else if (dataType == GPBDataTypeBytes) {
    [stream writeBytes:fieldNum value:value];
  } else {
    NSCAssert(NO, @"Unexpected type %d", dataType);
  }
}

//%PDDM-EXPAND-END SERIALIZE_SUPPORT_HELPERS()

size_t GPBDictionaryComputeSizeInternalHelper(NSDictionary *dict, GPBFieldDescriptor *field) {
  GPBDataType mapValueType = GPBGetFieldDataType(field);
  __block size_t result = 0;
  [dict enumerateKeysAndObjectsUsingBlock:^(NSString *key, id obj, BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = GPBComputeStringSize(kMapKeyFieldNumber, key);
    msgSize += ComputeDictObjectFieldSize(obj, kMapValueFieldNumber, mapValueType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * dict.count;
  return result;
}

void GPBDictionaryWriteToStreamInternalHelper(GPBCodedOutputStream *outputStream,
                                              NSDictionary *dict,
                                              GPBFieldDescriptor *field) {
  NSCAssert(field.mapKeyDataType == GPBDataTypeString, @"Unexpected key type");
  GPBDataType mapValueType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [dict enumerateKeysAndObjectsUsingBlock:^(NSString *key, id obj, BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = GPBComputeStringSize(kMapKeyFieldNumber, key);
    msgSize += ComputeDictObjectFieldSize(obj, kMapValueFieldNumber, mapValueType);

    // Write the size and fields.
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    [outputStream writeString:kMapKeyFieldNumber value:key];
    WriteDictObjectField(outputStream, obj, kMapValueFieldNumber, mapValueType);
  }];
}

BOOL GPBDictionaryIsInitializedInternalHelper(NSDictionary *dict, GPBFieldDescriptor *field) {
  NSCAssert(field.mapKeyDataType == GPBDataTypeString, @"Unexpected key type");
  NSCAssert(GPBGetFieldDataType(field) == GPBDataTypeMessage, @"Unexpected value type");
  #pragma unused(field)  // For when asserts are off in release.
  for (GPBMessage *msg in [dict objectEnumerator]) {
    if (!msg.initialized) {
      return NO;
    }
  }
  return YES;
}

// Note: if the type is an object, it the retain pass back to the caller.
static void ReadValue(GPBCodedInputStream *stream,
                      GPBGenericValue *valueToFill,
                      GPBDataType type,
                      GPBExtensionRegistry *registry,
                      GPBFieldDescriptor *field) {
  switch (type) {
    case GPBDataTypeBool:
      valueToFill->valueBool = GPBCodedInputStreamReadBool(&stream->state_);
      break;
    case GPBDataTypeFixed32:
      valueToFill->valueUInt32 = GPBCodedInputStreamReadFixed32(&stream->state_);
      break;
    case GPBDataTypeSFixed32:
      valueToFill->valueInt32 = GPBCodedInputStreamReadSFixed32(&stream->state_);
      break;
    case GPBDataTypeFloat:
      valueToFill->valueFloat = GPBCodedInputStreamReadFloat(&stream->state_);
      break;
    case GPBDataTypeFixed64:
      valueToFill->valueUInt64 = GPBCodedInputStreamReadFixed64(&stream->state_);
      break;
    case GPBDataTypeSFixed64:
      valueToFill->valueInt64 = GPBCodedInputStreamReadSFixed64(&stream->state_);
      break;
    case GPBDataTypeDouble:
      valueToFill->valueDouble = GPBCodedInputStreamReadDouble(&stream->state_);
      break;
    case GPBDataTypeInt32:
      valueToFill->valueInt32 = GPBCodedInputStreamReadInt32(&stream->state_);
      break;
    case GPBDataTypeInt64:
      valueToFill->valueInt64 = GPBCodedInputStreamReadInt32(&stream->state_);
      break;
    case GPBDataTypeSInt32:
      valueToFill->valueInt32 = GPBCodedInputStreamReadSInt32(&stream->state_);
      break;
    case GPBDataTypeSInt64:
      valueToFill->valueInt64 = GPBCodedInputStreamReadSInt64(&stream->state_);
      break;
    case GPBDataTypeUInt32:
      valueToFill->valueUInt32 = GPBCodedInputStreamReadUInt32(&stream->state_);
      break;
    case GPBDataTypeUInt64:
      valueToFill->valueUInt64 = GPBCodedInputStreamReadUInt64(&stream->state_);
      break;
    case GPBDataTypeBytes:
      [valueToFill->valueData release];
      valueToFill->valueData = GPBCodedInputStreamReadRetainedBytes(&stream->state_);
      break;
    case GPBDataTypeString:
      [valueToFill->valueString release];
      valueToFill->valueString = GPBCodedInputStreamReadRetainedString(&stream->state_);
      break;
    case GPBDataTypeMessage: {
      GPBMessage *message = [[field.msgClass alloc] init];
      [stream readMessage:message extensionRegistry:registry];
      [valueToFill->valueMessage release];
      valueToFill->valueMessage = message;
      break;
    }
    case GPBDataTypeGroup:
      NSCAssert(NO, @"Can't happen");
      break;
    case GPBDataTypeEnum:
      valueToFill->valueEnum = GPBCodedInputStreamReadEnum(&stream->state_);
      break;
  }
}

void GPBDictionaryReadEntry(id mapDictionary,
                            GPBCodedInputStream *stream,
                            GPBExtensionRegistry *registry,
                            GPBFieldDescriptor *field,
                            GPBMessage *parentMessage) {
  GPBDataType keyDataType = field.mapKeyDataType;
  GPBDataType valueDataType = GPBGetFieldDataType(field);

  GPBGenericValue key;
  GPBGenericValue value;
  // Zero them (but pick up any enum default for proto2).
  key.valueString = value.valueString = nil;
  if (valueDataType == GPBDataTypeEnum) {
    value = field.defaultValue;
  }

  GPBCodedInputStreamState *state = &stream->state_;
  uint32_t keyTag =
      GPBWireFormatMakeTag(kMapKeyFieldNumber, GPBWireFormatForType(keyDataType, NO));
  uint32_t valueTag =
      GPBWireFormatMakeTag(kMapValueFieldNumber, GPBWireFormatForType(valueDataType, NO));

  BOOL hitError = NO;
  while (YES) {
    uint32_t tag = GPBCodedInputStreamReadTag(state);
    if (tag == keyTag) {
      ReadValue(stream, &key, keyDataType, registry, field);
    } else if (tag == valueTag) {
      ReadValue(stream, &value, valueDataType, registry, field);
    } else if (tag == 0) {
      // zero signals EOF / limit reached
      break;
    } else {  // Unknown
      if (![stream skipField:tag]){
        hitError = YES;
        break;
      }
    }
  }

  if (!hitError) {
    // Handle the special defaults and/or missing key/value.
    if ((keyDataType == GPBDataTypeString) && (key.valueString == nil)) {
      key.valueString = [@"" retain];
    }
    if (GPBDataTypeIsObject(valueDataType) && value.valueString == nil) {
      switch (valueDataType) {
        case GPBDataTypeString:
          value.valueString = [@"" retain];
          break;
        case GPBDataTypeBytes:
          value.valueData = [GPBEmptyNSData() retain];
          break;
#if defined(__clang_analyzer__)
        case GPBDataTypeGroup:
          // Maps can't really have Groups as the value type, but this case is needed
          // so the analyzer won't report the posibility of send nil in for the value
          // in the NSMutableDictionary case below.
#endif
        case GPBDataTypeMessage: {
          value.valueMessage = [[field.msgClass alloc] init];
          break;
        }
        default:
          // Nothing
          break;
      }
    }

    if ((keyDataType == GPBDataTypeString) && GPBDataTypeIsObject(valueDataType)) {
#if GPB_STATIC_ANALYZER_ONLY(6020053, 7000181)
     // Limited to Xcode 6.4 - 7.2, are known to fail here. The upper end can
     // be raised as needed for new Xcodes.
     //
     // This is only needed on a "shallow" analyze; on a "deep" analyze, the
     // existing code path gets this correct. In shallow, the analyzer decides
     // GPBDataTypeIsObject(valueDataType) is both false and true on a single
     // path through this function, allowing nil to be used for the
     // setObject:forKey:.
     if (value.valueString == nil) {
       value.valueString = [@"" retain];
     }
#endif
      // mapDictionary is an NSMutableDictionary
      [(NSMutableDictionary *)mapDictionary setObject:value.valueString
                                               forKey:key.valueString];
    } else {
      if (valueDataType == GPBDataTypeEnum) {
        if (GPBHasPreservingUnknownEnumSemantics([parentMessage descriptor].file.syntax) ||
            [field isValidEnumValue:value.valueEnum]) {
          [mapDictionary setGPBGenericValue:&value forGPBGenericValueKey:&key];
        } else {
          NSData *data = [mapDictionary serializedDataForUnknownValue:value.valueEnum
                                                               forKey:&key
                                                          keyDataType:keyDataType];
          [parentMessage addUnknownMapEntry:GPBFieldNumber(field) value:data];
        }
      } else {
        [mapDictionary setGPBGenericValue:&value forGPBGenericValueKey:&key];
      }
    }
  }

  if (GPBDataTypeIsObject(keyDataType)) {
    [key.valueString release];
  }
  if (GPBDataTypeIsObject(valueDataType)) {
    [value.valueString release];
  }
}

//
// Macros for the common basic cases.
//

//%PDDM-DEFINE DICTIONARY_IMPL_FOR_POD_KEY(KEY_NAME, KEY_TYPE)
//%DICTIONARY_POD_IMPL_FOR_KEY(KEY_NAME, KEY_TYPE, , POD)
//%DICTIONARY_POD_KEY_TO_OBJECT_IMPL(KEY_NAME, KEY_TYPE, Object, id)

//%PDDM-DEFINE DICTIONARY_POD_IMPL_FOR_KEY(KEY_NAME, KEY_TYPE, KisP, KHELPER)
//%DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, UInt32, uint32_t, KHELPER)
//%DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, Int32, int32_t, KHELPER)
//%DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, UInt64, uint64_t, KHELPER)
//%DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, Int64, int64_t, KHELPER)
//%DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, Bool, BOOL, KHELPER)
//%DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, Float, float, KHELPER)
//%DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, Double, double, KHELPER)
//%DICTIONARY_KEY_TO_ENUM_IMPL(KEY_NAME, KEY_TYPE, KisP, Enum, int32_t, KHELPER)

//%PDDM-DEFINE DICTIONARY_KEY_TO_POD_IMPL(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER)
//%DICTIONARY_COMMON_IMPL(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, POD, value)

//%PDDM-DEFINE DICTIONARY_POD_KEY_TO_OBJECT_IMPL(KEY_NAME, KEY_TYPE, VALUE_NAME, VALUE_TYPE)
//%DICTIONARY_COMMON_IMPL(KEY_NAME, KEY_TYPE, , VALUE_NAME, VALUE_TYPE, POD, OBJECT, object)

//%PDDM-DEFINE DICTIONARY_COMMON_IMPL(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER, VNAME)
//%#pragma mark - KEY_NAME -> VALUE_NAME
//%
//%@implementation GPB##KEY_NAME##VALUE_NAME##Dictionary {
//% @package
//%  NSMutableDictionary *_dictionary;
//%}
//%
//%+ (instancetype)dictionary {
//%  return [[[self alloc] initWith##VNAME$u##s:NULL forKeys:NULL count:0] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWith##VNAME$u##:(VALUE_TYPE)##VNAME
//%                      ##VNAME$S##  forKey:(KEY_TYPE##KisP$S##KisP)key {
//%  // Cast is needed so the compiler knows what class we are invoking initWith##VNAME$u##s:forKeys:count:
//%  // on to get the type correct.
//%  return [[(GPB##KEY_NAME##VALUE_NAME##Dictionary*)[self alloc] initWith##VNAME$u##s:&##VNAME
//%               KEY_NAME$S VALUE_NAME$S                        ##VNAME$S##  forKeys:&key
//%               KEY_NAME$S VALUE_NAME$S                        ##VNAME$S##    count:1] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWith##VNAME$u##s:(const VALUE_TYPE [])##VNAME##s
//%                      ##VNAME$S##  forKeys:(const KEY_TYPE##KisP$S##KisP [])keys
//%                      ##VNAME$S##    count:(NSUInteger)count {
//%  // Cast is needed so the compiler knows what class we are invoking initWith##VNAME$u##s:forKeys:count:
//%  // on to get the type correct.
//%  return [[(GPB##KEY_NAME##VALUE_NAME##Dictionary*)[self alloc] initWith##VNAME$u##s:##VNAME##s
//%               KEY_NAME$S VALUE_NAME$S                               forKeys:keys
//%               KEY_NAME$S VALUE_NAME$S                                 count:count] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary {
//%  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
//%  // on to get the type correct.
//%  return [[(GPB##KEY_NAME##VALUE_NAME##Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
//%  return [[[self alloc] initWithCapacity:numItems] autorelease];
//%}
//%
//%- (instancetype)init {
//%  return [self initWith##VNAME$u##s:NULL forKeys:NULL count:0];
//%}
//%
//%- (instancetype)initWith##VNAME$u##s:(const VALUE_TYPE [])##VNAME##s
//%                ##VNAME$S##  forKeys:(const KEY_TYPE##KisP$S##KisP [])keys
//%                ##VNAME$S##    count:(NSUInteger)count {
//%  self = [super init];
//%  if (self) {
//%    _dictionary = [[NSMutableDictionary alloc] init];
//%    if (count && VNAME##s && keys) {
//%      for (NSUInteger i = 0; i < count; ++i) {
//%DICTIONARY_VALIDATE_VALUE_##VHELPER(VNAME##s[i], ______)##DICTIONARY_VALIDATE_KEY_##KHELPER(keys[i], ______)        [_dictionary setObject:WRAPPED##VHELPER(VNAME##s[i]) forKey:WRAPPED##KHELPER(keys[i])];
//%      }
//%    }
//%  }
//%  return self;
//%}
//%
//%- (instancetype)initWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary {
//%  self = [self initWith##VNAME$u##s:NULL forKeys:NULL count:0];
//%  if (self) {
//%    if (dictionary) {
//%      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
//%    }
//%  }
//%  return self;
//%}
//%
//%- (instancetype)initWithCapacity:(NSUInteger)numItems {
//%  #pragma unused(numItems)
//%  return [self initWith##VNAME$u##s:NULL forKeys:NULL count:0];
//%}
//%
//%DICTIONARY_IMMUTABLE_CORE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER, VNAME, )
//%
//%VALUE_FOR_KEY_##VHELPER(KEY_TYPE##KisP$S##KisP, VALUE_NAME, VALUE_TYPE, KHELPER)
//%
//%DICTIONARY_MUTABLE_CORE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER, VNAME, )
//%
//%@end
//%

//%PDDM-DEFINE DICTIONARY_KEY_TO_ENUM_IMPL(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER)
//%DICTIONARY_KEY_TO_ENUM_IMPL2(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, POD)
//%PDDM-DEFINE DICTIONARY_KEY_TO_ENUM_IMPL2(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER)
//%#pragma mark - KEY_NAME -> VALUE_NAME
//%
//%@implementation GPB##KEY_NAME##VALUE_NAME##Dictionary {
//% @package
//%  NSMutableDictionary *_dictionary;
//%  GPBEnumValidationFunc _validationFunc;
//%}
//%
//%@synthesize validationFunc = _validationFunc;
//%
//%+ (instancetype)dictionary {
//%  return [[[self alloc] initWithValidationFunction:NULL
//%                                         rawValues:NULL
//%                                           forKeys:NULL
//%                                             count:0] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func {
//%  return [[[self alloc] initWithValidationFunction:func
//%                                         rawValues:NULL
//%                                           forKeys:NULL
//%                                             count:0] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
//%                                        rawValue:(VALUE_TYPE)rawValue
//%                                          forKey:(KEY_TYPE##KisP$S##KisP)key {
//%  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
//%  // on to get the type correct.
//%  return [[(GPB##KEY_NAME##VALUE_NAME##Dictionary*)[self alloc] initWithValidationFunction:func
//%               KEY_NAME$S VALUE_NAME$S                                         rawValues:&rawValue
//%               KEY_NAME$S VALUE_NAME$S                                           forKeys:&key
//%               KEY_NAME$S VALUE_NAME$S                                             count:1] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
//%                                       rawValues:(const VALUE_TYPE [])rawValues
//%                                         forKeys:(const KEY_TYPE##KisP$S##KisP [])keys
//%                                           count:(NSUInteger)count {
//%  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
//%  // on to get the type correct.
//%  return [[(GPB##KEY_NAME##VALUE_NAME##Dictionary*)[self alloc] initWithValidationFunction:func
//%               KEY_NAME$S VALUE_NAME$S                                         rawValues:rawValues
//%               KEY_NAME$S VALUE_NAME$S                                           forKeys:keys
//%               KEY_NAME$S VALUE_NAME$S                                             count:count] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary {
//%  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
//%  // on to get the type correct.
//%  return [[(GPB##KEY_NAME##VALUE_NAME##Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
//%                                        capacity:(NSUInteger)numItems {
//%  return [[[self alloc] initWithValidationFunction:func capacity:numItems] autorelease];
//%}
//%
//%- (instancetype)init {
//%  return [self initWithValidationFunction:NULL rawValues:NULL forKeys:NULL count:0];
//%}
//%
//%- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
//%  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
//%}
//%
//%- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
//%                                 rawValues:(const VALUE_TYPE [])rawValues
//%                                   forKeys:(const KEY_TYPE##KisP$S##KisP [])keys
//%                                     count:(NSUInteger)count {
//%  self = [super init];
//%  if (self) {
//%    _dictionary = [[NSMutableDictionary alloc] init];
//%    _validationFunc = (func != NULL ? func : DictDefault_IsValidValue);
//%    if (count && rawValues && keys) {
//%      for (NSUInteger i = 0; i < count; ++i) {
//%DICTIONARY_VALIDATE_KEY_##KHELPER(keys[i], ______)        [_dictionary setObject:WRAPPED##VHELPER(rawValues[i]) forKey:WRAPPED##KHELPER(keys[i])];
//%      }
//%    }
//%  }
//%  return self;
//%}
//%
//%- (instancetype)initWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary {
//%  self = [self initWithValidationFunction:dictionary.validationFunc
//%                                rawValues:NULL
//%                                  forKeys:NULL
//%                                    count:0];
//%  if (self) {
//%    if (dictionary) {
//%      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
//%    }
//%  }
//%  return self;
//%}
//%
//%- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
//%                                  capacity:(NSUInteger)numItems {
//%  #pragma unused(numItems)
//%  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
//%}
//%
//%DICTIONARY_IMMUTABLE_CORE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER, value, Raw)
//%
//%- (BOOL)valueForKey:(KEY_TYPE##KisP$S##KisP)key value:(VALUE_TYPE *)value {
//%  NSNumber *wrapped = [_dictionary objectForKey:WRAPPED##KHELPER(key)];
//%  if (wrapped && value) {
//%    VALUE_TYPE result = UNWRAP##VALUE_NAME(wrapped);
//%    if (!_validationFunc(result)) {
//%      result = kGPBUnrecognizedEnumeratorValue;
//%    }
//%    *value = result;
//%  }
//%  return (wrapped != NULL);
//%}
//%
//%- (BOOL)valueForKey:(KEY_TYPE##KisP$S##KisP)key rawValue:(VALUE_TYPE *)rawValue {
//%  NSNumber *wrapped = [_dictionary objectForKey:WRAPPED##KHELPER(key)];
//%  if (wrapped && rawValue) {
//%    *rawValue = UNWRAP##VALUE_NAME(wrapped);
//%  }
//%  return (wrapped != NULL);
//%}
//%
//%- (void)enumerateKeysAndValuesUsingBlock:
//%    (void (^)(KEY_TYPE KisP##key, VALUE_TYPE value, BOOL *stop))block {
//%  GPBEnumValidationFunc func = _validationFunc;
//%  [_dictionary enumerateKeysAndObjectsUsingBlock:^(ENUM_TYPE##KHELPER(KEY_TYPE)##aKey,
//%                                                   ENUM_TYPE##VHELPER(VALUE_TYPE)##aValue,
//%                                                   BOOL *stop) {
//%      VALUE_TYPE unwrapped = UNWRAP##VALUE_NAME(aValue);
//%      if (!func(unwrapped)) {
//%        unwrapped = kGPBUnrecognizedEnumeratorValue;
//%      }
//%      block(UNWRAP##KEY_NAME(aKey), unwrapped, stop);
//%  }];
//%}
//%
//%DICTIONARY_MUTABLE_CORE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER, value, Raw)
//%
//%- (void)setValue:(VALUE_TYPE)value forKey:(KEY_TYPE##KisP$S##KisP)key {
//%DICTIONARY_VALIDATE_KEY_##KHELPER(key, )  if (!_validationFunc(value)) {
//%    [NSException raise:NSInvalidArgumentException
//%                format:@"GPB##KEY_NAME##VALUE_NAME##Dictionary: Attempt to set an unknown enum value (%d)",
//%                       value];
//%  }
//%
//%  [_dictionary setObject:WRAPPED##VHELPER(value) forKey:WRAPPED##KHELPER(key)];
//%  if (_autocreator) {
//%    GPBAutocreatedDictionaryModified(_autocreator, self);
//%  }
//%}
//%
//%@end
//%

//%PDDM-DEFINE DICTIONARY_IMMUTABLE_CORE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER, VNAME, ACCESSOR_NAME)
//%- (void)dealloc {
//%  NSAssert(!_autocreator,
//%           @"%@: Autocreator must be cleared before release, autocreator: %@",
//%           [self class], _autocreator);
//%  [_dictionary release];
//%  [super dealloc];
//%}
//%
//%- (instancetype)copyWithZone:(NSZone *)zone {
//%  return [[GPB##KEY_NAME##VALUE_NAME##Dictionary allocWithZone:zone] initWithDictionary:self];
//%}
//%
//%- (BOOL)isEqual:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)other {
//%  if (self == other) {
//%    return YES;
//%  }
//%  if (![other isKindOfClass:[GPB##KEY_NAME##VALUE_NAME##Dictionary class]]) {
//%    return NO;
//%  }
//%  return [_dictionary isEqual:other->_dictionary];
//%}
//%
//%- (NSUInteger)hash {
//%  return _dictionary.count;
//%}
//%
//%- (NSString *)description {
//%  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
//%}
//%
//%- (NSUInteger)count {
//%  return _dictionary.count;
//%}
//%
//%- (void)enumerateKeysAnd##ACCESSOR_NAME##VNAME$u##sUsingBlock:
//%    (void (^)(KEY_TYPE KisP##key, VALUE_TYPE VNAME, BOOL *stop))block {
//%  [_dictionary enumerateKeysAndObjectsUsingBlock:^(ENUM_TYPE##KHELPER(KEY_TYPE)##aKey,
//%                                                   ENUM_TYPE##VHELPER(VALUE_TYPE)##a##VNAME$u,
//%                                                   BOOL *stop) {
//%      block(UNWRAP##KEY_NAME(aKey), UNWRAP##VALUE_NAME(a##VNAME$u), stop);
//%  }];
//%}
//%
//%EXTRA_METHODS_##VHELPER(KEY_NAME, VALUE_NAME)- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
//%  NSUInteger count = _dictionary.count;
//%  if (count == 0) {
//%    return 0;
//%  }
//%
//%  GPBDataType valueDataType = GPBGetFieldDataType(field);
//%  GPBDataType keyDataType = field.mapKeyDataType;
//%  __block size_t result = 0;
//%  [_dictionary enumerateKeysAndObjectsUsingBlock:^(ENUM_TYPE##KHELPER(KEY_TYPE)##aKey,
//%                                                   ENUM_TYPE##VHELPER(VALUE_TYPE)##a##VNAME$u##,
//%                                                   BOOL *stop) {
//%    #pragma unused(stop)
//%    size_t msgSize = ComputeDict##KEY_NAME##FieldSize(UNWRAP##KEY_NAME(aKey), kMapKeyFieldNumber, keyDataType);
//%    msgSize += ComputeDict##VALUE_NAME##FieldSize(UNWRAP##VALUE_NAME(a##VNAME$u), kMapValueFieldNumber, valueDataType);
//%    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
//%  }];
//%  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
//%  result += tagSize * count;
//%  return result;
//%}
//%
//%- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
//%                         asField:(GPBFieldDescriptor *)field {
//%  GPBDataType valueDataType = GPBGetFieldDataType(field);
//%  GPBDataType keyDataType = field.mapKeyDataType;
//%  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
//%  [_dictionary enumerateKeysAndObjectsUsingBlock:^(ENUM_TYPE##KHELPER(KEY_TYPE)##aKey,
//%                                                   ENUM_TYPE##VHELPER(VALUE_TYPE)##a##VNAME$u,
//%                                                   BOOL *stop) {
//%    #pragma unused(stop)
//%    // Write the tag.
//%    [outputStream writeInt32NoTag:tag];
//%    // Write the size of the message.
//%    size_t msgSize = ComputeDict##KEY_NAME##FieldSize(UNWRAP##KEY_NAME(aKey), kMapKeyFieldNumber, keyDataType);
//%    msgSize += ComputeDict##VALUE_NAME##FieldSize(UNWRAP##VALUE_NAME(a##VNAME$u), kMapValueFieldNumber, valueDataType);
//%    [outputStream writeInt32NoTag:(int32_t)msgSize];
//%    // Write the fields.
//%    WriteDict##KEY_NAME##Field(outputStream, UNWRAP##KEY_NAME(aKey), kMapKeyFieldNumber, keyDataType);
//%    WriteDict##VALUE_NAME##Field(outputStream, UNWRAP##VALUE_NAME(a##VNAME$u), kMapValueFieldNumber, valueDataType);
//%  }];
//%}
//%
//%SERIAL_DATA_FOR_ENTRY_##VHELPER(KEY_NAME, VALUE_NAME)- (void)setGPBGenericValue:(GPBGenericValue *)value
//%     forGPBGenericValueKey:(GPBGenericValue *)key {
//%  [_dictionary setObject:WRAPPED##VHELPER(value->##GPBVALUE_##VHELPER(VALUE_NAME)##) forKey:WRAPPED##KHELPER(key->value##KEY_NAME)];
//%}
//%
//%- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
//%  [self enumerateKeysAnd##ACCESSOR_NAME##VNAME$u##sUsingBlock:^(KEY_TYPE KisP##key, VALUE_TYPE VNAME, BOOL *stop) {
//%      #pragma unused(stop)
//%      block(TEXT_FORMAT_OBJ##KEY_NAME(key), TEXT_FORMAT_OBJ##VALUE_NAME(VNAME));
//%  }];
//%}
//%PDDM-DEFINE DICTIONARY_MUTABLE_CORE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, KHELPER, VHELPER, VNAME, ACCESSOR_NAME)
//%- (void)add##ACCESSOR_NAME##EntriesFromDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)otherDictionary {
//%  if (otherDictionary) {
//%    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
//%    if (_autocreator) {
//%      GPBAutocreatedDictionaryModified(_autocreator, self);
//%    }
//%  }
//%}
//%
//%- (void)set##ACCESSOR_NAME##VNAME$u##:(VALUE_TYPE)VNAME forKey:(KEY_TYPE##KisP$S##KisP)key {
//%DICTIONARY_VALIDATE_VALUE_##VHELPER(VNAME, )##DICTIONARY_VALIDATE_KEY_##KHELPER(key, )  [_dictionary setObject:WRAPPED##VHELPER(VNAME) forKey:WRAPPED##KHELPER(key)];
//%  if (_autocreator) {
//%    GPBAutocreatedDictionaryModified(_autocreator, self);
//%  }
//%}
//%
//%- (void)remove##VNAME$u##ForKey:(KEY_TYPE##KisP$S##KisP)aKey {
//%  [_dictionary removeObjectForKey:WRAPPED##KHELPER(aKey)];
//%}
//%
//%- (void)removeAll {
//%  [_dictionary removeAllObjects];
//%}

//
// Custom Generation for Bool keys
//

//%PDDM-DEFINE DICTIONARY_BOOL_KEY_TO_POD_IMPL(VALUE_NAME, VALUE_TYPE)
//%DICTIONARY_BOOL_KEY_TO_VALUE_IMPL(VALUE_NAME, VALUE_TYPE, POD, value)
//%PDDM-DEFINE DICTIONARY_BOOL_KEY_TO_OBJECT_IMPL(VALUE_NAME, VALUE_TYPE)
//%DICTIONARY_BOOL_KEY_TO_VALUE_IMPL(VALUE_NAME, VALUE_TYPE, OBJECT, object)

//%PDDM-DEFINE DICTIONARY_BOOL_KEY_TO_VALUE_IMPL(VALUE_NAME, VALUE_TYPE, HELPER, VNAME)
//%#pragma mark - Bool -> VALUE_NAME
//%
//%@implementation GPBBool##VALUE_NAME##Dictionary {
//% @package
//%  VALUE_TYPE _values[2];
//%BOOL_DICT_HAS_STORAGE_##HELPER()}
//%
//%+ (instancetype)dictionary {
//%  return [[[self alloc] initWith##VNAME$u##s:NULL forKeys:NULL count:0] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWith##VNAME$u##:(VALUE_TYPE)VNAME
//%                      ##VNAME$S##  forKey:(BOOL)key {
//%  // Cast is needed so the compiler knows what class we are invoking initWith##VNAME$u##s:forKeys:count:
//%  // on to get the type correct.
//%  return [[(GPBBool##VALUE_NAME##Dictionary*)[self alloc] initWith##VNAME$u##s:&##VNAME
//%                    VALUE_NAME$S                        ##VNAME$S##  forKeys:&key
//%                    VALUE_NAME$S                        ##VNAME$S##    count:1] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWith##VNAME$u##s:(const VALUE_TYPE [])##VNAME##s
//%                      ##VNAME$S##  forKeys:(const BOOL [])keys
//%                      ##VNAME$S##    count:(NSUInteger)count {
//%  // Cast is needed so the compiler knows what class we are invoking initWith##VNAME$u##s:forKeys:count:
//%  // on to get the type correct.
//%  return [[(GPBBool##VALUE_NAME##Dictionary*)[self alloc] initWith##VNAME$u##s:##VNAME##s
//%                    VALUE_NAME$S                        ##VNAME$S##  forKeys:keys
//%                    VALUE_NAME$S                        ##VNAME$S##    count:count] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithDictionary:(GPBBool##VALUE_NAME##Dictionary *)dictionary {
//%  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
//%  // on to get the type correct.
//%  return [[(GPBBool##VALUE_NAME##Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
//%}
//%
//%+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
//%  return [[[self alloc] initWithCapacity:numItems] autorelease];
//%}
//%
//%- (instancetype)init {
//%  return [self initWith##VNAME$u##s:NULL forKeys:NULL count:0];
//%}
//%
//%BOOL_DICT_INITS_##HELPER(VALUE_NAME, VALUE_TYPE)
//%
//%- (instancetype)initWithCapacity:(NSUInteger)numItems {
//%  #pragma unused(numItems)
//%  return [self initWith##VNAME$u##s:NULL forKeys:NULL count:0];
//%}
//%
//%BOOL_DICT_DEALLOC##HELPER()
//%
//%- (instancetype)copyWithZone:(NSZone *)zone {
//%  return [[GPBBool##VALUE_NAME##Dictionary allocWithZone:zone] initWithDictionary:self];
//%}
//%
//%- (BOOL)isEqual:(GPBBool##VALUE_NAME##Dictionary *)other {
//%  if (self == other) {
//%    return YES;
//%  }
//%  if (![other isKindOfClass:[GPBBool##VALUE_NAME##Dictionary class]]) {
//%    return NO;
//%  }
//%  if ((BOOL_DICT_W_HAS##HELPER(0, ) != BOOL_DICT_W_HAS##HELPER(0, other->)) ||
//%      (BOOL_DICT_W_HAS##HELPER(1, ) != BOOL_DICT_W_HAS##HELPER(1, other->))) {
//%    return NO;
//%  }
//%  if ((BOOL_DICT_W_HAS##HELPER(0, ) && (NEQ_##HELPER(_values[0], other->_values[0]))) ||
//%      (BOOL_DICT_W_HAS##HELPER(1, ) && (NEQ_##HELPER(_values[1], other->_values[1])))) {
//%    return NO;
//%  }
//%  return YES;
//%}
//%
//%- (NSUInteger)hash {
//%  return (BOOL_DICT_W_HAS##HELPER(0, ) ? 1 : 0) + (BOOL_DICT_W_HAS##HELPER(1, ) ? 1 : 0);
//%}
//%
//%- (NSString *)description {
//%  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
//%  if (BOOL_DICT_W_HAS##HELPER(0, )) {
//%    [result appendFormat:@"NO: STR_FORMAT_##HELPER(VALUE_NAME)", _values[0]];
//%  }
//%  if (BOOL_DICT_W_HAS##HELPER(1, )) {
//%    [result appendFormat:@"YES: STR_FORMAT_##HELPER(VALUE_NAME)", _values[1]];
//%  }
//%  [result appendString:@" }"];
//%  return result;
//%}
//%
//%- (NSUInteger)count {
//%  return (BOOL_DICT_W_HAS##HELPER(0, ) ? 1 : 0) + (BOOL_DICT_W_HAS##HELPER(1, ) ? 1 : 0);
//%}
//%
//%BOOL_VALUE_FOR_KEY_##HELPER(VALUE_TYPE)
//%
//%BOOL_SET_GPBVALUE_FOR_KEY_##HELPER(VALUE_NAME, VALUE_TYPE, VisP)
//%
//%- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
//%  if (BOOL_DICT_HAS##HELPER(0, )) {
//%    block(@"false", TEXT_FORMAT_OBJ##VALUE_NAME(_values[0]));
//%  }
//%  if (BOOL_DICT_W_HAS##HELPER(1, )) {
//%    block(@"true", TEXT_FORMAT_OBJ##VALUE_NAME(_values[1]));
//%  }
//%}
//%
//%- (void)enumerateKeysAnd##VNAME$u##sUsingBlock:
//%    (void (^)(BOOL key, VALUE_TYPE VNAME, BOOL *stop))block {
//%  BOOL stop = NO;
//%  if (BOOL_DICT_HAS##HELPER(0, )) {
//%    block(NO, _values[0], &stop);
//%  }
//%  if (!stop && BOOL_DICT_W_HAS##HELPER(1, )) {
//%    block(YES, _values[1], &stop);
//%  }
//%}
//%
//%BOOL_EXTRA_METHODS_##HELPER(Bool, VALUE_NAME)- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
//%  GPBDataType valueDataType = GPBGetFieldDataType(field);
//%  NSUInteger count = 0;
//%  size_t result = 0;
//%  for (int i = 0; i < 2; ++i) {
//%    if (BOOL_DICT_HAS##HELPER(i, )) {
//%      ++count;
//%      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
//%      msgSize += ComputeDict##VALUE_NAME##FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
//%      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
//%    }
//%  }
//%  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
//%  result += tagSize * count;
//%  return result;
//%}
//%
//%- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
//%                         asField:(GPBFieldDescriptor *)field {
//%  GPBDataType valueDataType = GPBGetFieldDataType(field);
//%  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
//%  for (int i = 0; i < 2; ++i) {
//%    if (BOOL_DICT_HAS##HELPER(i, )) {
//%      // Write the tag.
//%      [outputStream writeInt32NoTag:tag];
//%      // Write the size of the message.
//%      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
//%      msgSize += ComputeDict##VALUE_NAME##FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
//%      [outputStream writeInt32NoTag:(int32_t)msgSize];
//%      // Write the fields.
//%      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
//%      WriteDict##VALUE_NAME##Field(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
//%    }
//%  }
//%}
//%
//%BOOL_DICT_MUTATIONS_##HELPER(VALUE_NAME, VALUE_TYPE)
//%
//%@end
//%


//
// Helpers for PODs
//

//%PDDM-DEFINE VALUE_FOR_KEY_POD(KEY_TYPE, VALUE_NAME, VALUE_TYPE, KHELPER)
//%- (BOOL)valueForKey:(KEY_TYPE)key value:(VALUE_TYPE *)value {
//%  NSNumber *wrapped = [_dictionary objectForKey:WRAPPED##KHELPER(key)];
//%  if (wrapped && value) {
//%    *value = UNWRAP##VALUE_NAME(wrapped);
//%  }
//%  return (wrapped != NULL);
//%}
//%PDDM-DEFINE WRAPPEDPOD(VALUE)
//%@(VALUE)
//%PDDM-DEFINE UNWRAPUInt32(VALUE)
//%[VALUE unsignedIntValue]
//%PDDM-DEFINE UNWRAPInt32(VALUE)
//%[VALUE intValue]
//%PDDM-DEFINE UNWRAPUInt64(VALUE)
//%[VALUE unsignedLongLongValue]
//%PDDM-DEFINE UNWRAPInt64(VALUE)
//%[VALUE longLongValue]
//%PDDM-DEFINE UNWRAPBool(VALUE)
//%[VALUE boolValue]
//%PDDM-DEFINE UNWRAPFloat(VALUE)
//%[VALUE floatValue]
//%PDDM-DEFINE UNWRAPDouble(VALUE)
//%[VALUE doubleValue]
//%PDDM-DEFINE UNWRAPEnum(VALUE)
//%[VALUE intValue]
//%PDDM-DEFINE TEXT_FORMAT_OBJUInt32(VALUE)
//%[NSString stringWithFormat:@"%u", VALUE]
//%PDDM-DEFINE TEXT_FORMAT_OBJInt32(VALUE)
//%[NSString stringWithFormat:@"%d", VALUE]
//%PDDM-DEFINE TEXT_FORMAT_OBJUInt64(VALUE)
//%[NSString stringWithFormat:@"%llu", VALUE]
//%PDDM-DEFINE TEXT_FORMAT_OBJInt64(VALUE)
//%[NSString stringWithFormat:@"%lld", VALUE]
//%PDDM-DEFINE TEXT_FORMAT_OBJBool(VALUE)
//%(VALUE ? @"true" : @"false")
//%PDDM-DEFINE TEXT_FORMAT_OBJFloat(VALUE)
//%[NSString stringWithFormat:@"%.*g", FLT_DIG, VALUE]
//%PDDM-DEFINE TEXT_FORMAT_OBJDouble(VALUE)
//%[NSString stringWithFormat:@"%.*lg", DBL_DIG, VALUE]
//%PDDM-DEFINE TEXT_FORMAT_OBJEnum(VALUE)
//%@(VALUE)
//%PDDM-DEFINE ENUM_TYPEPOD(TYPE)
//%NSNumber *
//%PDDM-DEFINE NEQ_POD(VAL1, VAL2)
//%VAL1 != VAL2
//%PDDM-DEFINE EXTRA_METHODS_POD(KEY_NAME, VALUE_NAME)
// Empty
//%PDDM-DEFINE BOOL_EXTRA_METHODS_POD(KEY_NAME, VALUE_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD(KEY_NAME, VALUE_NAME)
//%SERIAL_DATA_FOR_ENTRY_POD_##VALUE_NAME(KEY_NAME)
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_UInt32(KEY_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_Int32(KEY_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_UInt64(KEY_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_Int64(KEY_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_Bool(KEY_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_Float(KEY_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_Double(KEY_NAME)
// Empty
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_POD_Enum(KEY_NAME)
//%- (NSData *)serializedDataForUnknownValue:(int32_t)value
//%                                   forKey:(GPBGenericValue *)key
//%                              keyDataType:(GPBDataType)keyDataType {
//%  size_t msgSize = ComputeDict##KEY_NAME##FieldSize(key->value##KEY_NAME, kMapKeyFieldNumber, keyDataType);
//%  msgSize += ComputeDictEnumFieldSize(value, kMapValueFieldNumber, GPBDataTypeEnum);
//%  NSMutableData *data = [NSMutableData dataWithLength:msgSize];
//%  GPBCodedOutputStream *outputStream = [[GPBCodedOutputStream alloc] initWithData:data];
//%  WriteDict##KEY_NAME##Field(outputStream, key->value##KEY_NAME, kMapKeyFieldNumber, keyDataType);
//%  WriteDictEnumField(outputStream, value, kMapValueFieldNumber, GPBDataTypeEnum);
//%  [outputStream release];
//%  return data;
//%}
//%
//%PDDM-DEFINE GPBVALUE_POD(VALUE_NAME)
//%value##VALUE_NAME
//%PDDM-DEFINE DICTIONARY_VALIDATE_VALUE_POD(VALUE_NAME, EXTRA_INDENT)
// Empty
//%PDDM-DEFINE DICTIONARY_VALIDATE_KEY_POD(KEY_NAME, EXTRA_INDENT)
// Empty

//%PDDM-DEFINE BOOL_DICT_HAS_STORAGE_POD()
//%  BOOL _valueSet[2];
//%
//%PDDM-DEFINE BOOL_DICT_INITS_POD(VALUE_NAME, VALUE_TYPE)
//%- (instancetype)initWithValues:(const VALUE_TYPE [])values
//%                       forKeys:(const BOOL [])keys
//%                         count:(NSUInteger)count {
//%  self = [super init];
//%  if (self) {
//%    for (NSUInteger i = 0; i < count; ++i) {
//%      int idx = keys[i] ? 1 : 0;
//%      _values[idx] = values[i];
//%      _valueSet[idx] = YES;
//%    }
//%  }
//%  return self;
//%}
//%
//%- (instancetype)initWithDictionary:(GPBBool##VALUE_NAME##Dictionary *)dictionary {
//%  self = [self initWithValues:NULL forKeys:NULL count:0];
//%  if (self) {
//%    if (dictionary) {
//%      for (int i = 0; i < 2; ++i) {
//%        if (dictionary->_valueSet[i]) {
//%          _values[i] = dictionary->_values[i];
//%          _valueSet[i] = YES;
//%        }
//%      }
//%    }
//%  }
//%  return self;
//%}
//%PDDM-DEFINE BOOL_DICT_DEALLOCPOD()
//%#if !defined(NS_BLOCK_ASSERTIONS)
//%- (void)dealloc {
//%  NSAssert(!_autocreator,
//%           @"%@: Autocreator must be cleared before release, autocreator: %@",
//%           [self class], _autocreator);
//%  [super dealloc];
//%}
//%#endif  // !defined(NS_BLOCK_ASSERTIONS)
//%PDDM-DEFINE BOOL_DICT_W_HASPOD(IDX, REF)
//%BOOL_DICT_HASPOD(IDX, REF)
//%PDDM-DEFINE BOOL_DICT_HASPOD(IDX, REF)
//%REF##_valueSet[IDX]
//%PDDM-DEFINE BOOL_VALUE_FOR_KEY_POD(VALUE_TYPE)
//%- (BOOL)valueForKey:(BOOL)key value:(VALUE_TYPE *)value {
//%  int idx = (key ? 1 : 0);
//%  if (_valueSet[idx]) {
//%    if (value) {
//%      *value = _values[idx];
//%    }
//%    return YES;
//%  }
//%  return NO;
//%}
//%PDDM-DEFINE BOOL_SET_GPBVALUE_FOR_KEY_POD(VALUE_NAME, VALUE_TYPE, VisP)
//%- (void)setGPBGenericValue:(GPBGenericValue *)value
//%     forGPBGenericValueKey:(GPBGenericValue *)key {
//%  int idx = (key->valueBool ? 1 : 0);
//%  _values[idx] = value->value##VALUE_NAME;
//%  _valueSet[idx] = YES;
//%}
//%PDDM-DEFINE BOOL_DICT_MUTATIONS_POD(VALUE_NAME, VALUE_TYPE)
//%- (void)addEntriesFromDictionary:(GPBBool##VALUE_NAME##Dictionary *)otherDictionary {
//%  if (otherDictionary) {
//%    for (int i = 0; i < 2; ++i) {
//%      if (otherDictionary->_valueSet[i]) {
//%        _valueSet[i] = YES;
//%        _values[i] = otherDictionary->_values[i];
//%      }
//%    }
//%    if (_autocreator) {
//%      GPBAutocreatedDictionaryModified(_autocreator, self);
//%    }
//%  }
//%}
//%
//%- (void)setValue:(VALUE_TYPE)value forKey:(BOOL)key {
//%  int idx = (key ? 1 : 0);
//%  _values[idx] = value;
//%  _valueSet[idx] = YES;
//%  if (_autocreator) {
//%    GPBAutocreatedDictionaryModified(_autocreator, self);
//%  }
//%}
//%
//%- (void)removeValueForKey:(BOOL)aKey {
//%  _valueSet[aKey ? 1 : 0] = NO;
//%}
//%
//%- (void)removeAll {
//%  _valueSet[0] = NO;
//%  _valueSet[1] = NO;
//%}
//%PDDM-DEFINE STR_FORMAT_POD(VALUE_NAME)
//%STR_FORMAT_##VALUE_NAME()
//%PDDM-DEFINE STR_FORMAT_UInt32()
//%%u
//%PDDM-DEFINE STR_FORMAT_Int32()
//%%d
//%PDDM-DEFINE STR_FORMAT_UInt64()
//%%llu
//%PDDM-DEFINE STR_FORMAT_Int64()
//%%lld
//%PDDM-DEFINE STR_FORMAT_Bool()
//%%d
//%PDDM-DEFINE STR_FORMAT_Float()
//%%f
//%PDDM-DEFINE STR_FORMAT_Double()
//%%lf

//
// Helpers for Objects
//

//%PDDM-DEFINE VALUE_FOR_KEY_OBJECT(KEY_TYPE, VALUE_NAME, VALUE_TYPE, KHELPER)
//%- (VALUE_TYPE)objectForKey:(KEY_TYPE)key {
//%  VALUE_TYPE result = [_dictionary objectForKey:WRAPPED##KHELPER(key)];
//%  return result;
//%}
//%PDDM-DEFINE WRAPPEDOBJECT(VALUE)
//%VALUE
//%PDDM-DEFINE UNWRAPString(VALUE)
//%VALUE
//%PDDM-DEFINE UNWRAPObject(VALUE)
//%VALUE
//%PDDM-DEFINE TEXT_FORMAT_OBJString(VALUE)
//%VALUE
//%PDDM-DEFINE TEXT_FORMAT_OBJObject(VALUE)
//%VALUE
//%PDDM-DEFINE ENUM_TYPEOBJECT(TYPE)
//%ENUM_TYPEOBJECT_##TYPE()
//%PDDM-DEFINE ENUM_TYPEOBJECT_NSString()
//%NSString *
//%PDDM-DEFINE ENUM_TYPEOBJECT_id()
//%id ##
//%PDDM-DEFINE NEQ_OBJECT(VAL1, VAL2)
//%![VAL1 isEqual:VAL2]
//%PDDM-DEFINE EXTRA_METHODS_OBJECT(KEY_NAME, VALUE_NAME)
//%- (BOOL)isInitialized {
//%  for (GPBMessage *msg in [_dictionary objectEnumerator]) {
//%    if (!msg.initialized) {
//%      return NO;
//%    }
//%  }
//%  return YES;
//%}
//%
//%- (instancetype)deepCopyWithZone:(NSZone *)zone {
//%  GPB##KEY_NAME##VALUE_NAME##Dictionary *newDict =
//%      [[GPB##KEY_NAME##VALUE_NAME##Dictionary alloc] init];
//%  [_dictionary enumerateKeysAndObjectsUsingBlock:^(id aKey,
//%                                                   GPBMessage *msg,
//%                                                   BOOL *stop) {
//%    #pragma unused(stop)
//%    GPBMessage *copiedMsg = [msg copyWithZone:zone];
//%    [newDict->_dictionary setObject:copiedMsg forKey:aKey];
//%    [copiedMsg release];
//%  }];
//%  return newDict;
//%}
//%
//%
//%PDDM-DEFINE BOOL_EXTRA_METHODS_OBJECT(KEY_NAME, VALUE_NAME)
//%- (BOOL)isInitialized {
//%  if (_values[0] && ![_values[0] isInitialized]) {
//%    return NO;
//%  }
//%  if (_values[1] && ![_values[1] isInitialized]) {
//%    return NO;
//%  }
//%  return YES;
//%}
//%
//%- (instancetype)deepCopyWithZone:(NSZone *)zone {
//%  GPB##KEY_NAME##VALUE_NAME##Dictionary *newDict =
//%      [[GPB##KEY_NAME##VALUE_NAME##Dictionary alloc] init];
//%  for (int i = 0; i < 2; ++i) {
//%    if (_values[i] != nil) {
//%      newDict->_values[i] = [_values[i] copyWithZone:zone];
//%    }
//%  }
//%  return newDict;
//%}
//%
//%
//%PDDM-DEFINE SERIAL_DATA_FOR_ENTRY_OBJECT(KEY_NAME, VALUE_NAME)
// Empty
//%PDDM-DEFINE GPBVALUE_OBJECT(VALUE_NAME)
//%valueString
//%PDDM-DEFINE DICTIONARY_VALIDATE_VALUE_OBJECT(VALUE_NAME, EXTRA_INDENT)
//%##EXTRA_INDENT$S##  if (!##VALUE_NAME) {
//%##EXTRA_INDENT$S##    [NSException raise:NSInvalidArgumentException
//%##EXTRA_INDENT$S##                format:@"Attempting to add nil object to a Dictionary"];
//%##EXTRA_INDENT$S##  }
//%
//%PDDM-DEFINE DICTIONARY_VALIDATE_KEY_OBJECT(KEY_NAME, EXTRA_INDENT)
//%##EXTRA_INDENT$S##  if (!##KEY_NAME) {
//%##EXTRA_INDENT$S##    [NSException raise:NSInvalidArgumentException
//%##EXTRA_INDENT$S##                format:@"Attempting to add nil key to a Dictionary"];
//%##EXTRA_INDENT$S##  }
//%

//%PDDM-DEFINE BOOL_DICT_HAS_STORAGE_OBJECT()
// Empty
//%PDDM-DEFINE BOOL_DICT_INITS_OBJECT(VALUE_NAME, VALUE_TYPE)
//%- (instancetype)initWithObjects:(const VALUE_TYPE [])objects
//%                        forKeys:(const BOOL [])keys
//%                          count:(NSUInteger)count {
//%  self = [super init];
//%  if (self) {
//%    for (NSUInteger i = 0; i < count; ++i) {
//%      if (!objects[i]) {
//%        [NSException raise:NSInvalidArgumentException
//%                    format:@"Attempting to add nil object to a Dictionary"];
//%      }
//%      int idx = keys[i] ? 1 : 0;
//%      [_values[idx] release];
//%      _values[idx] = (VALUE_TYPE)[objects[i] retain];
//%    }
//%  }
//%  return self;
//%}
//%
//%- (instancetype)initWithDictionary:(GPBBool##VALUE_NAME##Dictionary *)dictionary {
//%  self = [self initWithObjects:NULL forKeys:NULL count:0];
//%  if (self) {
//%    if (dictionary) {
//%      _values[0] = [dictionary->_values[0] retain];
//%      _values[1] = [dictionary->_values[1] retain];
//%    }
//%  }
//%  return self;
//%}
//%PDDM-DEFINE BOOL_DICT_DEALLOCOBJECT()
//%- (void)dealloc {
//%  NSAssert(!_autocreator,
//%           @"%@: Autocreator must be cleared before release, autocreator: %@",
//%           [self class], _autocreator);
//%  [_values[0] release];
//%  [_values[1] release];
//%  [super dealloc];
//%}
//%PDDM-DEFINE BOOL_DICT_W_HASOBJECT(IDX, REF)
//%(BOOL_DICT_HASOBJECT(IDX, REF))
//%PDDM-DEFINE BOOL_DICT_HASOBJECT(IDX, REF)
//%REF##_values[IDX] != nil
//%PDDM-DEFINE BOOL_VALUE_FOR_KEY_OBJECT(VALUE_TYPE)
//%- (VALUE_TYPE)objectForKey:(BOOL)key {
//%  return _values[key ? 1 : 0];
//%}
//%PDDM-DEFINE BOOL_SET_GPBVALUE_FOR_KEY_OBJECT(VALUE_NAME, VALUE_TYPE, VisP)
//%- (void)setGPBGenericValue:(GPBGenericValue *)value
//%     forGPBGenericValueKey:(GPBGenericValue *)key {
//%  int idx = (key->valueBool ? 1 : 0);
//%  [_values[idx] release];
//%  _values[idx] = [value->valueString retain];
//%}

//%PDDM-DEFINE BOOL_DICT_MUTATIONS_OBJECT(VALUE_NAME, VALUE_TYPE)
//%- (void)addEntriesFromDictionary:(GPBBool##VALUE_NAME##Dictionary *)otherDictionary {
//%  if (otherDictionary) {
//%    for (int i = 0; i < 2; ++i) {
//%      if (otherDictionary->_values[i] != nil) {
//%        [_values[i] release];
//%        _values[i] = [otherDictionary->_values[i] retain];
//%      }
//%    }
//%    if (_autocreator) {
//%      GPBAutocreatedDictionaryModified(_autocreator, self);
//%    }
//%  }
//%}
//%
//%- (void)setObject:(VALUE_TYPE)object forKey:(BOOL)key {
//%  if (!object) {
//%    [NSException raise:NSInvalidArgumentException
//%                format:@"Attempting to add nil object to a Dictionary"];
//%  }
//%  int idx = (key ? 1 : 0);
//%  [_values[idx] release];
//%  _values[idx] = [object retain];
//%  if (_autocreator) {
//%    GPBAutocreatedDictionaryModified(_autocreator, self);
//%  }
//%}
//%
//%- (void)removeObjectForKey:(BOOL)aKey {
//%  int idx = (aKey ? 1 : 0);
//%  [_values[idx] release];
//%  _values[idx] = nil;
//%}
//%
//%- (void)removeAll {
//%  for (int i = 0; i < 2; ++i) {
//%    [_values[i] release];
//%    _values[i] = nil;
//%  }
//%}
//%PDDM-DEFINE STR_FORMAT_OBJECT(VALUE_NAME)
//%%@


//%PDDM-EXPAND DICTIONARY_IMPL_FOR_POD_KEY(UInt32, uint32_t)
// This block of code is generated, do not edit it directly.

#pragma mark - UInt32 -> UInt32

@implementation GPBUInt32UInt32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32UInt32Dictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32UInt32Dictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32UInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32UInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32UInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32UInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32UInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32UInt32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, uint32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue unsignedIntValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt32Field(outputStream, [aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt32) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint32_t key, uint32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], [NSString stringWithFormat:@"%u", value]);
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(uint32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedIntValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt32UInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint32_t)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt32 -> Int32

@implementation GPBUInt32Int32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32Int32Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32Int32Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32Int32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32Int32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32Int32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32Int32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32Int32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32Int32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt32Field(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt32) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint32_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], [NSString stringWithFormat:@"%d", value]);
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt32Int32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int32_t)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt32 -> UInt64

@implementation GPBUInt32UInt64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32UInt64Dictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32UInt64Dictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32UInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32UInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32UInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32UInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32UInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32UInt64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, uint64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue unsignedLongLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt64Field(outputStream, [aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt64) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint32_t key, uint64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], [NSString stringWithFormat:@"%llu", value]);
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(uint64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedLongLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt32UInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint64_t)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt32 -> Int64

@implementation GPBUInt32Int64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32Int64Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32Int64Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32Int64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32Int64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32Int64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32Int64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32Int64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32Int64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, int64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue longLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt64Field(outputStream, [aValue longLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt64) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint32_t key, int64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], [NSString stringWithFormat:@"%lld", value]);
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(int64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped longLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt32Int64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int64_t)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt32 -> Bool

@implementation GPBUInt32BoolDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32BoolDictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32BoolDictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32BoolDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32BoolDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32BoolDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32BoolDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32BoolDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32BoolDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, BOOL value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue boolValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictBoolField(outputStream, [aValue boolValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueBool) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint32_t key, BOOL value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], (value ? @"true" : @"false"));
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(BOOL *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped boolValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt32BoolDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(BOOL)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt32 -> Float

@implementation GPBUInt32FloatDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32FloatDictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32FloatDictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32FloatDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32FloatDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32FloatDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32FloatDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32FloatDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32FloatDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, float value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue floatValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictFloatField(outputStream, [aValue floatValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueFloat) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint32_t key, float value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], [NSString stringWithFormat:@"%.*g", FLT_DIG, value]);
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(float *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped floatValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt32FloatDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(float)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt32 -> Double

@implementation GPBUInt32DoubleDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32DoubleDictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32DoubleDictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32DoubleDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32DoubleDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32DoubleDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32DoubleDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32DoubleDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32DoubleDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, double value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue doubleValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictDoubleField(outputStream, [aValue doubleValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueDouble) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint32_t key, double value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], [NSString stringWithFormat:@"%.*lg", DBL_DIG, value]);
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(double *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped doubleValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt32DoubleDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(double)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt32 -> Enum

@implementation GPBUInt32EnumDictionary {
 @package
  NSMutableDictionary *_dictionary;
  GPBEnumValidationFunc _validationFunc;
}

@synthesize validationFunc = _validationFunc;

+ (instancetype)dictionary {
  return [[[self alloc] initWithValidationFunction:NULL
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func {
  return [[[self alloc] initWithValidationFunction:func
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                   rawValues:&rawValue
                                                                     forKeys:&key
                                                                       count:1] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])rawValues
                                         forKeys:(const uint32_t [])keys
                                           count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                   rawValues:rawValues
                                                                     forKeys:keys
                                                                       count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32EnumDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32EnumDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems {
  return [[[self alloc] initWithValidationFunction:func capacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValidationFunction:NULL rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])rawValues
                                   forKeys:(const uint32_t [])keys
                                     count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    _validationFunc = (func != NULL ? func : DictDefault_IsValidValue);
    if (count && rawValues && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(rawValues[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32EnumDictionary *)dictionary {
  self = [self initWithValidationFunction:dictionary.validationFunc
                                rawValues:NULL
                                  forKeys:NULL
                                    count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32EnumDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32EnumDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32EnumDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(uint32_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictEnumField(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType {
  size_t msgSize = ComputeDictUInt32FieldSize(key->valueUInt32, kMapKeyFieldNumber, keyDataType);
  msgSize += ComputeDictEnumFieldSize(value, kMapValueFieldNumber, GPBDataTypeEnum);
  NSMutableData *data = [NSMutableData dataWithLength:msgSize];
  GPBCodedOutputStream *outputStream = [[GPBCodedOutputStream alloc] initWithData:data];
  WriteDictUInt32Field(outputStream, key->valueUInt32, kMapKeyFieldNumber, keyDataType);
  WriteDictEnumField(outputStream, value, kMapValueFieldNumber, GPBDataTypeEnum);
  [outputStream release];
  return data;
}
- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueEnum) forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndRawValuesUsingBlock:^(uint32_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], @(value));
  }];
}

- (BOOL)valueForKey:(uint32_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    int32_t result = [wrapped intValue];
    if (!_validationFunc(result)) {
      result = kGPBUnrecognizedEnumeratorValue;
    }
    *value = result;
  }
  return (wrapped != NULL);
}

- (BOOL)valueForKey:(uint32_t)key rawValue:(int32_t *)rawValue {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && rawValue) {
    *rawValue = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, int32_t value, BOOL *stop))block {
  GPBEnumValidationFunc func = _validationFunc;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      int32_t unwrapped = [aValue intValue];
      if (!func(unwrapped)) {
        unwrapped = kGPBUnrecognizedEnumeratorValue;
      }
      block([aKey unsignedIntValue], unwrapped, stop);
  }];
}

- (void)addRawEntriesFromDictionary:(GPBUInt32EnumDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setRawValue:(int32_t)value forKey:(uint32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

- (void)setValue:(int32_t)value forKey:(uint32_t)key {
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"GPBUInt32EnumDictionary: Attempt to set an unknown enum value (%d)",
                       value];
  }

  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

@end

#pragma mark - UInt32 -> Object

@implementation GPBUInt32ObjectDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithObjects:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithObject:(id)object
                              forKey:(uint32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32ObjectDictionary*)[self alloc] initWithObjects:&object
                                                            forKeys:&key
                                                              count:1] autorelease];
}

+ (instancetype)dictionaryWithObjects:(const id [])objects
                              forKeys:(const uint32_t [])keys
                                count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt32ObjectDictionary*)[self alloc] initWithObjects:objects
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt32ObjectDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt32ObjectDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (instancetype)initWithObjects:(const id [])objects
                        forKeys:(const uint32_t [])keys
                          count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && objects && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!objects[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil object to a Dictionary"];
        }
        [_dictionary setObject:objects[i] forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt32ObjectDictionary *)dictionary {
  self = [self initWithObjects:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32ObjectDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt32ObjectDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32ObjectDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(uint32_t key, id object, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
      block([aKey unsignedIntValue], aObject, stop);
  }];
}

- (BOOL)isInitialized {
  for (GPBMessage *msg in [_dictionary objectEnumerator]) {
    if (!msg.initialized) {
      return NO;
    }
  }
  return YES;
}

- (instancetype)deepCopyWithZone:(NSZone *)zone {
  GPBUInt32ObjectDictionary *newDict =
      [[GPBUInt32ObjectDictionary alloc] init];
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(id aKey,
                                                   GPBMessage *msg,
                                                   BOOL *stop) {
    #pragma unused(stop)
    GPBMessage *copiedMsg = [msg copyWithZone:zone];
    [newDict->_dictionary setObject:copiedMsg forKey:aKey];
    [copiedMsg release];
  }];
  return newDict;
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt32FieldSize([aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt32Field(outputStream, [aKey unsignedIntValue], kMapKeyFieldNumber, keyDataType);
    WriteDictObjectField(outputStream, aObject, kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:value->valueString forKey:@(key->valueUInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndObjectsUsingBlock:^(uint32_t key, id object, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%u", key], object);
  }];
}

- (id)objectForKey:(uint32_t)key {
  id result = [_dictionary objectForKey:@(key)];
  return result;
}

- (void)addEntriesFromDictionary:(GPBUInt32ObjectDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setObject:(id)object forKey:(uint32_t)key {
  if (!object) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil object to a Dictionary"];
  }
  [_dictionary setObject:object forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeObjectForKey:(uint32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

//%PDDM-EXPAND DICTIONARY_IMPL_FOR_POD_KEY(Int32, int32_t)
// This block of code is generated, do not edit it directly.

#pragma mark - Int32 -> UInt32

@implementation GPBInt32UInt32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32UInt32Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32UInt32Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32UInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32UInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32UInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32UInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32UInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32UInt32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, uint32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue unsignedIntValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt32Field(outputStream, [aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt32) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int32_t key, uint32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], [NSString stringWithFormat:@"%u", value]);
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(uint32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedIntValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt32UInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint32_t)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int32 -> Int32

@implementation GPBInt32Int32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32Int32Dictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32Int32Dictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32Int32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32Int32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32Int32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32Int32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32Int32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32Int32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt32Field(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt32) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int32_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], [NSString stringWithFormat:@"%d", value]);
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt32Int32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int32_t)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int32 -> UInt64

@implementation GPBInt32UInt64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32UInt64Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32UInt64Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32UInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32UInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32UInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32UInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32UInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32UInt64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, uint64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue unsignedLongLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt64Field(outputStream, [aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt64) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int32_t key, uint64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], [NSString stringWithFormat:@"%llu", value]);
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(uint64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedLongLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt32UInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint64_t)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int32 -> Int64

@implementation GPBInt32Int64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32Int64Dictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32Int64Dictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32Int64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32Int64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32Int64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32Int64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32Int64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32Int64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, int64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue longLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt64Field(outputStream, [aValue longLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt64) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int32_t key, int64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], [NSString stringWithFormat:@"%lld", value]);
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(int64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped longLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt32Int64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int64_t)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int32 -> Bool

@implementation GPBInt32BoolDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32BoolDictionary*)[self alloc] initWithValues:&value
                                                        forKeys:&key
                                                          count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32BoolDictionary*)[self alloc] initWithValues:values
                                                        forKeys:keys
                                                          count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32BoolDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32BoolDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32BoolDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32BoolDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32BoolDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32BoolDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, BOOL value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue boolValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictBoolField(outputStream, [aValue boolValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueBool) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int32_t key, BOOL value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], (value ? @"true" : @"false"));
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(BOOL *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped boolValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt32BoolDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(BOOL)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int32 -> Float

@implementation GPBInt32FloatDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32FloatDictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32FloatDictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32FloatDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32FloatDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32FloatDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32FloatDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32FloatDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32FloatDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, float value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue floatValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictFloatField(outputStream, [aValue floatValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueFloat) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int32_t key, float value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], [NSString stringWithFormat:@"%.*g", FLT_DIG, value]);
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(float *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped floatValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt32FloatDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(float)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int32 -> Double

@implementation GPBInt32DoubleDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32DoubleDictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32DoubleDictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32DoubleDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32DoubleDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32DoubleDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32DoubleDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32DoubleDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32DoubleDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, double value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue doubleValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictDoubleField(outputStream, [aValue doubleValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueDouble) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int32_t key, double value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], [NSString stringWithFormat:@"%.*lg", DBL_DIG, value]);
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(double *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped doubleValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt32DoubleDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(double)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int32 -> Enum

@implementation GPBInt32EnumDictionary {
 @package
  NSMutableDictionary *_dictionary;
  GPBEnumValidationFunc _validationFunc;
}

@synthesize validationFunc = _validationFunc;

+ (instancetype)dictionary {
  return [[[self alloc] initWithValidationFunction:NULL
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func {
  return [[[self alloc] initWithValidationFunction:func
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                  rawValues:&rawValue
                                                                    forKeys:&key
                                                                      count:1] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])rawValues
                                         forKeys:(const int32_t [])keys
                                           count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                  rawValues:rawValues
                                                                    forKeys:keys
                                                                      count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32EnumDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32EnumDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems {
  return [[[self alloc] initWithValidationFunction:func capacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValidationFunction:NULL rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])rawValues
                                   forKeys:(const int32_t [])keys
                                     count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    _validationFunc = (func != NULL ? func : DictDefault_IsValidValue);
    if (count && rawValues && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(rawValues[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32EnumDictionary *)dictionary {
  self = [self initWithValidationFunction:dictionary.validationFunc
                                rawValues:NULL
                                  forKeys:NULL
                                    count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32EnumDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32EnumDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32EnumDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(int32_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey intValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictEnumField(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType {
  size_t msgSize = ComputeDictInt32FieldSize(key->valueInt32, kMapKeyFieldNumber, keyDataType);
  msgSize += ComputeDictEnumFieldSize(value, kMapValueFieldNumber, GPBDataTypeEnum);
  NSMutableData *data = [NSMutableData dataWithLength:msgSize];
  GPBCodedOutputStream *outputStream = [[GPBCodedOutputStream alloc] initWithData:data];
  WriteDictInt32Field(outputStream, key->valueInt32, kMapKeyFieldNumber, keyDataType);
  WriteDictEnumField(outputStream, value, kMapValueFieldNumber, GPBDataTypeEnum);
  [outputStream release];
  return data;
}
- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueEnum) forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndRawValuesUsingBlock:^(int32_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], @(value));
  }];
}

- (BOOL)valueForKey:(int32_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    int32_t result = [wrapped intValue];
    if (!_validationFunc(result)) {
      result = kGPBUnrecognizedEnumeratorValue;
    }
    *value = result;
  }
  return (wrapped != NULL);
}

- (BOOL)valueForKey:(int32_t)key rawValue:(int32_t *)rawValue {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && rawValue) {
    *rawValue = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, int32_t value, BOOL *stop))block {
  GPBEnumValidationFunc func = _validationFunc;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      int32_t unwrapped = [aValue intValue];
      if (!func(unwrapped)) {
        unwrapped = kGPBUnrecognizedEnumeratorValue;
      }
      block([aKey intValue], unwrapped, stop);
  }];
}

- (void)addRawEntriesFromDictionary:(GPBInt32EnumDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setRawValue:(int32_t)value forKey:(int32_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

- (void)setValue:(int32_t)value forKey:(int32_t)key {
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"GPBInt32EnumDictionary: Attempt to set an unknown enum value (%d)",
                       value];
  }

  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

@end

#pragma mark - Int32 -> Object

@implementation GPBInt32ObjectDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithObjects:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithObject:(id)object
                              forKey:(int32_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32ObjectDictionary*)[self alloc] initWithObjects:&object
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithObjects:(const id [])objects
                              forKeys:(const int32_t [])keys
                                count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt32ObjectDictionary*)[self alloc] initWithObjects:objects
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt32ObjectDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt32ObjectDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (instancetype)initWithObjects:(const id [])objects
                        forKeys:(const int32_t [])keys
                          count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && objects && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!objects[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil object to a Dictionary"];
        }
        [_dictionary setObject:objects[i] forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt32ObjectDictionary *)dictionary {
  self = [self initWithObjects:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32ObjectDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt32ObjectDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32ObjectDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(int32_t key, id object, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
      block([aKey intValue], aObject, stop);
  }];
}

- (BOOL)isInitialized {
  for (GPBMessage *msg in [_dictionary objectEnumerator]) {
    if (!msg.initialized) {
      return NO;
    }
  }
  return YES;
}

- (instancetype)deepCopyWithZone:(NSZone *)zone {
  GPBInt32ObjectDictionary *newDict =
      [[GPBInt32ObjectDictionary alloc] init];
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(id aKey,
                                                   GPBMessage *msg,
                                                   BOOL *stop) {
    #pragma unused(stop)
    GPBMessage *copiedMsg = [msg copyWithZone:zone];
    [newDict->_dictionary setObject:copiedMsg forKey:aKey];
    [copiedMsg release];
  }];
  return newDict;
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt32FieldSize([aKey intValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt32Field(outputStream, [aKey intValue], kMapKeyFieldNumber, keyDataType);
    WriteDictObjectField(outputStream, aObject, kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:value->valueString forKey:@(key->valueInt32)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndObjectsUsingBlock:^(int32_t key, id object, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%d", key], object);
  }];
}

- (id)objectForKey:(int32_t)key {
  id result = [_dictionary objectForKey:@(key)];
  return result;
}

- (void)addEntriesFromDictionary:(GPBInt32ObjectDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setObject:(id)object forKey:(int32_t)key {
  if (!object) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil object to a Dictionary"];
  }
  [_dictionary setObject:object forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeObjectForKey:(int32_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

//%PDDM-EXPAND DICTIONARY_IMPL_FOR_POD_KEY(UInt64, uint64_t)
// This block of code is generated, do not edit it directly.

#pragma mark - UInt64 -> UInt32

@implementation GPBUInt64UInt32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64UInt32Dictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64UInt32Dictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64UInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64UInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64UInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64UInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64UInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64UInt32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, uint32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue unsignedIntValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt32Field(outputStream, [aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt32) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint64_t key, uint32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], [NSString stringWithFormat:@"%u", value]);
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(uint32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedIntValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt64UInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint32_t)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt64 -> Int32

@implementation GPBUInt64Int32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64Int32Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64Int32Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64Int32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64Int32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64Int32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64Int32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64Int32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64Int32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt32Field(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt32) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint64_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], [NSString stringWithFormat:@"%d", value]);
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt64Int32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int32_t)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt64 -> UInt64

@implementation GPBUInt64UInt64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64UInt64Dictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64UInt64Dictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64UInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64UInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64UInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64UInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64UInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64UInt64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, uint64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue unsignedLongLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt64Field(outputStream, [aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt64) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint64_t key, uint64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], [NSString stringWithFormat:@"%llu", value]);
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(uint64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedLongLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt64UInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint64_t)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt64 -> Int64

@implementation GPBUInt64Int64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64Int64Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64Int64Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64Int64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64Int64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64Int64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64Int64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64Int64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64Int64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, int64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue longLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt64Field(outputStream, [aValue longLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt64) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint64_t key, int64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], [NSString stringWithFormat:@"%lld", value]);
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(int64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped longLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt64Int64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int64_t)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt64 -> Bool

@implementation GPBUInt64BoolDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64BoolDictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64BoolDictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64BoolDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64BoolDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64BoolDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64BoolDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64BoolDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64BoolDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, BOOL value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue boolValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictBoolField(outputStream, [aValue boolValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueBool) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint64_t key, BOOL value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], (value ? @"true" : @"false"));
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(BOOL *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped boolValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt64BoolDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(BOOL)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt64 -> Float

@implementation GPBUInt64FloatDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64FloatDictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64FloatDictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64FloatDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64FloatDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64FloatDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64FloatDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64FloatDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64FloatDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, float value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue floatValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictFloatField(outputStream, [aValue floatValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueFloat) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint64_t key, float value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], [NSString stringWithFormat:@"%.*g", FLT_DIG, value]);
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(float *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped floatValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt64FloatDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(float)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt64 -> Double

@implementation GPBUInt64DoubleDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64DoubleDictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64DoubleDictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64DoubleDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64DoubleDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64DoubleDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64DoubleDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64DoubleDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64DoubleDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, double value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue doubleValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictDoubleField(outputStream, [aValue doubleValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueDouble) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(uint64_t key, double value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], [NSString stringWithFormat:@"%.*lg", DBL_DIG, value]);
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(double *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped doubleValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBUInt64DoubleDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(double)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - UInt64 -> Enum

@implementation GPBUInt64EnumDictionary {
 @package
  NSMutableDictionary *_dictionary;
  GPBEnumValidationFunc _validationFunc;
}

@synthesize validationFunc = _validationFunc;

+ (instancetype)dictionary {
  return [[[self alloc] initWithValidationFunction:NULL
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func {
  return [[[self alloc] initWithValidationFunction:func
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                   rawValues:&rawValue
                                                                     forKeys:&key
                                                                       count:1] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])rawValues
                                         forKeys:(const uint64_t [])keys
                                           count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                   rawValues:rawValues
                                                                     forKeys:keys
                                                                       count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64EnumDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64EnumDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems {
  return [[[self alloc] initWithValidationFunction:func capacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValidationFunction:NULL rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])rawValues
                                   forKeys:(const uint64_t [])keys
                                     count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    _validationFunc = (func != NULL ? func : DictDefault_IsValidValue);
    if (count && rawValues && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(rawValues[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64EnumDictionary *)dictionary {
  self = [self initWithValidationFunction:dictionary.validationFunc
                                rawValues:NULL
                                  forKeys:NULL
                                    count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64EnumDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64EnumDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64EnumDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(uint64_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictEnumField(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType {
  size_t msgSize = ComputeDictUInt64FieldSize(key->valueUInt64, kMapKeyFieldNumber, keyDataType);
  msgSize += ComputeDictEnumFieldSize(value, kMapValueFieldNumber, GPBDataTypeEnum);
  NSMutableData *data = [NSMutableData dataWithLength:msgSize];
  GPBCodedOutputStream *outputStream = [[GPBCodedOutputStream alloc] initWithData:data];
  WriteDictUInt64Field(outputStream, key->valueUInt64, kMapKeyFieldNumber, keyDataType);
  WriteDictEnumField(outputStream, value, kMapValueFieldNumber, GPBDataTypeEnum);
  [outputStream release];
  return data;
}
- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueEnum) forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndRawValuesUsingBlock:^(uint64_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], @(value));
  }];
}

- (BOOL)valueForKey:(uint64_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    int32_t result = [wrapped intValue];
    if (!_validationFunc(result)) {
      result = kGPBUnrecognizedEnumeratorValue;
    }
    *value = result;
  }
  return (wrapped != NULL);
}

- (BOOL)valueForKey:(uint64_t)key rawValue:(int32_t *)rawValue {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && rawValue) {
    *rawValue = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, int32_t value, BOOL *stop))block {
  GPBEnumValidationFunc func = _validationFunc;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      int32_t unwrapped = [aValue intValue];
      if (!func(unwrapped)) {
        unwrapped = kGPBUnrecognizedEnumeratorValue;
      }
      block([aKey unsignedLongLongValue], unwrapped, stop);
  }];
}

- (void)addRawEntriesFromDictionary:(GPBUInt64EnumDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setRawValue:(int32_t)value forKey:(uint64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

- (void)setValue:(int32_t)value forKey:(uint64_t)key {
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"GPBUInt64EnumDictionary: Attempt to set an unknown enum value (%d)",
                       value];
  }

  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

@end

#pragma mark - UInt64 -> Object

@implementation GPBUInt64ObjectDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithObjects:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithObject:(id)object
                              forKey:(uint64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64ObjectDictionary*)[self alloc] initWithObjects:&object
                                                            forKeys:&key
                                                              count:1] autorelease];
}

+ (instancetype)dictionaryWithObjects:(const id [])objects
                              forKeys:(const uint64_t [])keys
                                count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBUInt64ObjectDictionary*)[self alloc] initWithObjects:objects
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBUInt64ObjectDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBUInt64ObjectDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (instancetype)initWithObjects:(const id [])objects
                        forKeys:(const uint64_t [])keys
                          count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && objects && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!objects[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil object to a Dictionary"];
        }
        [_dictionary setObject:objects[i] forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBUInt64ObjectDictionary *)dictionary {
  self = [self initWithObjects:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64ObjectDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBUInt64ObjectDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64ObjectDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(uint64_t key, id object, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
      block([aKey unsignedLongLongValue], aObject, stop);
  }];
}

- (BOOL)isInitialized {
  for (GPBMessage *msg in [_dictionary objectEnumerator]) {
    if (!msg.initialized) {
      return NO;
    }
  }
  return YES;
}

- (instancetype)deepCopyWithZone:(NSZone *)zone {
  GPBUInt64ObjectDictionary *newDict =
      [[GPBUInt64ObjectDictionary alloc] init];
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(id aKey,
                                                   GPBMessage *msg,
                                                   BOOL *stop) {
    #pragma unused(stop)
    GPBMessage *copiedMsg = [msg copyWithZone:zone];
    [newDict->_dictionary setObject:copiedMsg forKey:aKey];
    [copiedMsg release];
  }];
  return newDict;
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictUInt64FieldSize([aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictUInt64Field(outputStream, [aKey unsignedLongLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictObjectField(outputStream, aObject, kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:value->valueString forKey:@(key->valueUInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndObjectsUsingBlock:^(uint64_t key, id object, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%llu", key], object);
  }];
}

- (id)objectForKey:(uint64_t)key {
  id result = [_dictionary objectForKey:@(key)];
  return result;
}

- (void)addEntriesFromDictionary:(GPBUInt64ObjectDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setObject:(id)object forKey:(uint64_t)key {
  if (!object) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil object to a Dictionary"];
  }
  [_dictionary setObject:object forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeObjectForKey:(uint64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

//%PDDM-EXPAND DICTIONARY_IMPL_FOR_POD_KEY(Int64, int64_t)
// This block of code is generated, do not edit it directly.

#pragma mark - Int64 -> UInt32

@implementation GPBInt64UInt32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64UInt32Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64UInt32Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64UInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64UInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64UInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64UInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64UInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64UInt32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, uint32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue unsignedIntValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt32Field(outputStream, [aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt32) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int64_t key, uint32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], [NSString stringWithFormat:@"%u", value]);
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(uint32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedIntValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt64UInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint32_t)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int64 -> Int32

@implementation GPBInt64Int32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64Int32Dictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64Int32Dictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64Int32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64Int32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64Int32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64Int32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64Int32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64Int32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt32Field(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt32) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int64_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], [NSString stringWithFormat:@"%d", value]);
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt64Int32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int32_t)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int64 -> UInt64

@implementation GPBInt64UInt64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64UInt64Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64UInt64Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64UInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64UInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64UInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64UInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64UInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64UInt64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, uint64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue unsignedLongLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictUInt64Field(outputStream, [aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt64) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int64_t key, uint64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], [NSString stringWithFormat:@"%llu", value]);
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(uint64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped unsignedLongLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt64UInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint64_t)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int64 -> Int64

@implementation GPBInt64Int64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64Int64Dictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64Int64Dictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64Int64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64Int64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64Int64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64Int64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64Int64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64Int64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, int64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue longLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictInt64Field(outputStream, [aValue longLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt64) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int64_t key, int64_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], [NSString stringWithFormat:@"%lld", value]);
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(int64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped longLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt64Int64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int64_t)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int64 -> Bool

@implementation GPBInt64BoolDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64BoolDictionary*)[self alloc] initWithValues:&value
                                                        forKeys:&key
                                                          count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64BoolDictionary*)[self alloc] initWithValues:values
                                                        forKeys:keys
                                                          count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64BoolDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64BoolDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64BoolDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64BoolDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64BoolDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64BoolDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, BOOL value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue boolValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictBoolField(outputStream, [aValue boolValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueBool) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int64_t key, BOOL value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], (value ? @"true" : @"false"));
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(BOOL *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped boolValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt64BoolDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(BOOL)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int64 -> Float

@implementation GPBInt64FloatDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64FloatDictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64FloatDictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64FloatDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64FloatDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64FloatDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64FloatDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64FloatDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64FloatDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, float value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue floatValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictFloatField(outputStream, [aValue floatValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueFloat) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int64_t key, float value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], [NSString stringWithFormat:@"%.*g", FLT_DIG, value]);
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(float *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped floatValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt64FloatDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(float)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int64 -> Double

@implementation GPBInt64DoubleDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64DoubleDictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64DoubleDictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64DoubleDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64DoubleDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(values[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64DoubleDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64DoubleDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64DoubleDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64DoubleDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, double value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue doubleValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictDoubleField(outputStream, [aValue doubleValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueDouble) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(int64_t key, double value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], [NSString stringWithFormat:@"%.*lg", DBL_DIG, value]);
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(double *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    *value = [wrapped doubleValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBInt64DoubleDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(double)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - Int64 -> Enum

@implementation GPBInt64EnumDictionary {
 @package
  NSMutableDictionary *_dictionary;
  GPBEnumValidationFunc _validationFunc;
}

@synthesize validationFunc = _validationFunc;

+ (instancetype)dictionary {
  return [[[self alloc] initWithValidationFunction:NULL
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func {
  return [[[self alloc] initWithValidationFunction:func
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                  rawValues:&rawValue
                                                                    forKeys:&key
                                                                      count:1] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])rawValues
                                         forKeys:(const int64_t [])keys
                                           count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64EnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                  rawValues:rawValues
                                                                    forKeys:keys
                                                                      count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64EnumDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64EnumDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems {
  return [[[self alloc] initWithValidationFunction:func capacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValidationFunction:NULL rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])rawValues
                                   forKeys:(const int64_t [])keys
                                     count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    _validationFunc = (func != NULL ? func : DictDefault_IsValidValue);
    if (count && rawValues && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        [_dictionary setObject:@(rawValues[i]) forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64EnumDictionary *)dictionary {
  self = [self initWithValidationFunction:dictionary.validationFunc
                                rawValues:NULL
                                  forKeys:NULL
                                    count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64EnumDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64EnumDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64EnumDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(int64_t key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block([aKey longLongValue], [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictEnumField(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType {
  size_t msgSize = ComputeDictInt64FieldSize(key->valueInt64, kMapKeyFieldNumber, keyDataType);
  msgSize += ComputeDictEnumFieldSize(value, kMapValueFieldNumber, GPBDataTypeEnum);
  NSMutableData *data = [NSMutableData dataWithLength:msgSize];
  GPBCodedOutputStream *outputStream = [[GPBCodedOutputStream alloc] initWithData:data];
  WriteDictInt64Field(outputStream, key->valueInt64, kMapKeyFieldNumber, keyDataType);
  WriteDictEnumField(outputStream, value, kMapValueFieldNumber, GPBDataTypeEnum);
  [outputStream release];
  return data;
}
- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueEnum) forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndRawValuesUsingBlock:^(int64_t key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], @(value));
  }];
}

- (BOOL)valueForKey:(int64_t)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && value) {
    int32_t result = [wrapped intValue];
    if (!_validationFunc(result)) {
      result = kGPBUnrecognizedEnumeratorValue;
    }
    *value = result;
  }
  return (wrapped != NULL);
}

- (BOOL)valueForKey:(int64_t)key rawValue:(int32_t *)rawValue {
  NSNumber *wrapped = [_dictionary objectForKey:@(key)];
  if (wrapped && rawValue) {
    *rawValue = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, int32_t value, BOOL *stop))block {
  GPBEnumValidationFunc func = _validationFunc;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      int32_t unwrapped = [aValue intValue];
      if (!func(unwrapped)) {
        unwrapped = kGPBUnrecognizedEnumeratorValue;
      }
      block([aKey longLongValue], unwrapped, stop);
  }];
}

- (void)addRawEntriesFromDictionary:(GPBInt64EnumDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setRawValue:(int32_t)value forKey:(int64_t)key {
  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

- (void)setValue:(int32_t)value forKey:(int64_t)key {
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"GPBInt64EnumDictionary: Attempt to set an unknown enum value (%d)",
                       value];
  }

  [_dictionary setObject:@(value) forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

@end

#pragma mark - Int64 -> Object

@implementation GPBInt64ObjectDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithObjects:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithObject:(id)object
                              forKey:(int64_t)key {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64ObjectDictionary*)[self alloc] initWithObjects:&object
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithObjects:(const id [])objects
                              forKeys:(const int64_t [])keys
                                count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBInt64ObjectDictionary*)[self alloc] initWithObjects:objects
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBInt64ObjectDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBInt64ObjectDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (instancetype)initWithObjects:(const id [])objects
                        forKeys:(const int64_t [])keys
                          count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && objects && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!objects[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil object to a Dictionary"];
        }
        [_dictionary setObject:objects[i] forKey:@(keys[i])];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBInt64ObjectDictionary *)dictionary {
  self = [self initWithObjects:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64ObjectDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBInt64ObjectDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64ObjectDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(int64_t key, id object, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
      block([aKey longLongValue], aObject, stop);
  }];
}

- (BOOL)isInitialized {
  for (GPBMessage *msg in [_dictionary objectEnumerator]) {
    if (!msg.initialized) {
      return NO;
    }
  }
  return YES;
}

- (instancetype)deepCopyWithZone:(NSZone *)zone {
  GPBInt64ObjectDictionary *newDict =
      [[GPBInt64ObjectDictionary alloc] init];
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(id aKey,
                                                   GPBMessage *msg,
                                                   BOOL *stop) {
    #pragma unused(stop)
    GPBMessage *copiedMsg = [msg copyWithZone:zone];
    [newDict->_dictionary setObject:copiedMsg forKey:aKey];
    [copiedMsg release];
  }];
  return newDict;
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSNumber *aKey,
                                                   id aObject,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictInt64FieldSize([aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictObjectFieldSize(aObject, kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictInt64Field(outputStream, [aKey longLongValue], kMapKeyFieldNumber, keyDataType);
    WriteDictObjectField(outputStream, aObject, kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:value->valueString forKey:@(key->valueInt64)];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndObjectsUsingBlock:^(int64_t key, id object, BOOL *stop) {
      #pragma unused(stop)
      block([NSString stringWithFormat:@"%lld", key], object);
  }];
}

- (id)objectForKey:(int64_t)key {
  id result = [_dictionary objectForKey:@(key)];
  return result;
}

- (void)addEntriesFromDictionary:(GPBInt64ObjectDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setObject:(id)object forKey:(int64_t)key {
  if (!object) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil object to a Dictionary"];
  }
  [_dictionary setObject:object forKey:@(key)];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeObjectForKey:(int64_t)aKey {
  [_dictionary removeObjectForKey:@(aKey)];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

//%PDDM-EXPAND DICTIONARY_POD_IMPL_FOR_KEY(String, NSString, *, OBJECT)
// This block of code is generated, do not edit it directly.

#pragma mark - String -> UInt32

@implementation GPBStringUInt32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringUInt32Dictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const NSString * [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringUInt32Dictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringUInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBStringUInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const NSString * [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(values[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringUInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringUInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringUInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringUInt32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, uint32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue unsignedIntValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt32FieldSize([aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictUInt32Field(outputStream, [aValue unsignedIntValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt32) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(NSString *key, uint32_t value, BOOL *stop) {
      #pragma unused(stop)
      block(key, [NSString stringWithFormat:@"%u", value]);
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(uint32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    *value = [wrapped unsignedIntValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBStringUInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint32_t)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - String -> Int32

@implementation GPBStringInt32Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringInt32Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const NSString * [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringInt32Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBStringInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const NSString * [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(values[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringInt32Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt32FieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictInt32Field(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt32) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(NSString *key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block(key, [NSString stringWithFormat:@"%d", value]);
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    *value = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBStringInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int32_t)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - String -> UInt64

@implementation GPBStringUInt64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringUInt64Dictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const NSString * [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringUInt64Dictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringUInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBStringUInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const NSString * [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(values[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringUInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringUInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringUInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringUInt64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, uint64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue unsignedLongLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictUInt64FieldSize([aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictUInt64Field(outputStream, [aValue unsignedLongLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueUInt64) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(NSString *key, uint64_t value, BOOL *stop) {
      #pragma unused(stop)
      block(key, [NSString stringWithFormat:@"%llu", value]);
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(uint64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    *value = [wrapped unsignedLongLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBStringUInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint64_t)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - String -> Int64

@implementation GPBStringInt64Dictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringInt64Dictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const NSString * [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringInt64Dictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBStringInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const NSString * [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(values[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringInt64Dictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, int64_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue longLongValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictInt64FieldSize([aValue longLongValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictInt64Field(outputStream, [aValue longLongValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueInt64) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(NSString *key, int64_t value, BOOL *stop) {
      #pragma unused(stop)
      block(key, [NSString stringWithFormat:@"%lld", value]);
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(int64_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    *value = [wrapped longLongValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBStringInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int64_t)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - String -> Bool

@implementation GPBStringBoolDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringBoolDictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const NSString * [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringBoolDictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringBoolDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBStringBoolDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const NSString * [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(values[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringBoolDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringBoolDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringBoolDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringBoolDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, BOOL value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue boolValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictBoolFieldSize([aValue boolValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictBoolField(outputStream, [aValue boolValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueBool) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(NSString *key, BOOL value, BOOL *stop) {
      #pragma unused(stop)
      block(key, (value ? @"true" : @"false"));
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(BOOL *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    *value = [wrapped boolValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBStringBoolDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(BOOL)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - String -> Float

@implementation GPBStringFloatDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringFloatDictionary*)[self alloc] initWithValues:&value
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const NSString * [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringFloatDictionary*)[self alloc] initWithValues:values
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringFloatDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBStringFloatDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const NSString * [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(values[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringFloatDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringFloatDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringFloatDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringFloatDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, float value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue floatValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictFloatFieldSize([aValue floatValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictFloatField(outputStream, [aValue floatValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueFloat) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(NSString *key, float value, BOOL *stop) {
      #pragma unused(stop)
      block(key, [NSString stringWithFormat:@"%.*g", FLT_DIG, value]);
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(float *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    *value = [wrapped floatValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBStringFloatDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(float)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - String -> Double

@implementation GPBStringDoubleDictionary {
 @package
  NSMutableDictionary *_dictionary;
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringDoubleDictionary*)[self alloc] initWithValues:&value
                                                           forKeys:&key
                                                             count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const NSString * [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringDoubleDictionary*)[self alloc] initWithValues:values
                                                           forKeys:keys
                                                             count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringDoubleDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBStringDoubleDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const NSString * [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    if (count && values && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(values[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringDoubleDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringDoubleDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringDoubleDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringDoubleDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, double value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue doubleValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictDoubleFieldSize([aValue doubleValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictDoubleField(outputStream, [aValue doubleValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueDouble) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndValuesUsingBlock:^(NSString *key, double value, BOOL *stop) {
      #pragma unused(stop)
      block(key, [NSString stringWithFormat:@"%.*lg", DBL_DIG, value]);
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(double *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    *value = [wrapped doubleValue];
  }
  return (wrapped != NULL);
}

- (void)addEntriesFromDictionary:(GPBStringDoubleDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(double)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

@end

#pragma mark - String -> Enum

@implementation GPBStringEnumDictionary {
 @package
  NSMutableDictionary *_dictionary;
  GPBEnumValidationFunc _validationFunc;
}

@synthesize validationFunc = _validationFunc;

+ (instancetype)dictionary {
  return [[[self alloc] initWithValidationFunction:NULL
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func {
  return [[[self alloc] initWithValidationFunction:func
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(NSString *)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringEnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                   rawValues:&rawValue
                                                                     forKeys:&key
                                                                       count:1] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])rawValues
                                         forKeys:(const NSString * [])keys
                                           count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringEnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                   rawValues:rawValues
                                                                     forKeys:keys
                                                                       count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBStringEnumDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBStringEnumDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems {
  return [[[self alloc] initWithValidationFunction:func capacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValidationFunction:NULL rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])rawValues
                                   forKeys:(const NSString * [])keys
                                     count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] init];
    _validationFunc = (func != NULL ? func : DictDefault_IsValidValue);
    if (count && rawValues && keys) {
      for (NSUInteger i = 0; i < count; ++i) {
        if (!keys[i]) {
          [NSException raise:NSInvalidArgumentException
                      format:@"Attempting to add nil key to a Dictionary"];
        }
        [_dictionary setObject:@(rawValues[i]) forKey:keys[i]];
      }
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBStringEnumDictionary *)dictionary {
  self = [self initWithValidationFunction:dictionary.validationFunc
                                rawValues:NULL
                                  forKeys:NULL
                                    count:0];
  if (self) {
    if (dictionary) {
      [_dictionary addEntriesFromDictionary:dictionary->_dictionary];
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBStringEnumDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBStringEnumDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBStringEnumDictionary class]]) {
    return NO;
  }
  return [_dictionary isEqual:other->_dictionary];
}

- (NSUInteger)hash {
  return _dictionary.count;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"<%@ %p> { %@ }", [self class], self, _dictionary];
}

- (NSUInteger)count {
  return _dictionary.count;
}

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(NSString *key, int32_t value, BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      block(aKey, [aValue intValue], stop);
  }];
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  NSUInteger count = _dictionary.count;
  if (count == 0) {
    return 0;
  }

  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  __block size_t result = 0;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
  }];
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  GPBDataType keyDataType = field.mapKeyDataType;
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
    #pragma unused(stop)
    // Write the tag.
    [outputStream writeInt32NoTag:tag];
    // Write the size of the message.
    size_t msgSize = ComputeDictStringFieldSize(aKey, kMapKeyFieldNumber, keyDataType);
    msgSize += ComputeDictEnumFieldSize([aValue intValue], kMapValueFieldNumber, valueDataType);
    [outputStream writeInt32NoTag:(int32_t)msgSize];
    // Write the fields.
    WriteDictStringField(outputStream, aKey, kMapKeyFieldNumber, keyDataType);
    WriteDictEnumField(outputStream, [aValue intValue], kMapValueFieldNumber, valueDataType);
  }];
}

- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType {
  size_t msgSize = ComputeDictStringFieldSize(key->valueString, kMapKeyFieldNumber, keyDataType);
  msgSize += ComputeDictEnumFieldSize(value, kMapValueFieldNumber, GPBDataTypeEnum);
  NSMutableData *data = [NSMutableData dataWithLength:msgSize];
  GPBCodedOutputStream *outputStream = [[GPBCodedOutputStream alloc] initWithData:data];
  WriteDictStringField(outputStream, key->valueString, kMapKeyFieldNumber, keyDataType);
  WriteDictEnumField(outputStream, value, kMapValueFieldNumber, GPBDataTypeEnum);
  [outputStream release];
  return data;
}
- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  [_dictionary setObject:@(value->valueEnum) forKey:key->valueString];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  [self enumerateKeysAndRawValuesUsingBlock:^(NSString *key, int32_t value, BOOL *stop) {
      #pragma unused(stop)
      block(key, @(value));
  }];
}

- (BOOL)valueForKey:(NSString *)key value:(int32_t *)value {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && value) {
    int32_t result = [wrapped intValue];
    if (!_validationFunc(result)) {
      result = kGPBUnrecognizedEnumeratorValue;
    }
    *value = result;
  }
  return (wrapped != NULL);
}

- (BOOL)valueForKey:(NSString *)key rawValue:(int32_t *)rawValue {
  NSNumber *wrapped = [_dictionary objectForKey:key];
  if (wrapped && rawValue) {
    *rawValue = [wrapped intValue];
  }
  return (wrapped != NULL);
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, int32_t value, BOOL *stop))block {
  GPBEnumValidationFunc func = _validationFunc;
  [_dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *aKey,
                                                   NSNumber *aValue,
                                                   BOOL *stop) {
      int32_t unwrapped = [aValue intValue];
      if (!func(unwrapped)) {
        unwrapped = kGPBUnrecognizedEnumeratorValue;
      }
      block(aKey, unwrapped, stop);
  }];
}

- (void)addRawEntriesFromDictionary:(GPBStringEnumDictionary *)otherDictionary {
  if (otherDictionary) {
    [_dictionary addEntriesFromDictionary:otherDictionary->_dictionary];
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setRawValue:(int32_t)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(NSString *)aKey {
  [_dictionary removeObjectForKey:aKey];
}

- (void)removeAll {
  [_dictionary removeAllObjects];
}

- (void)setValue:(int32_t)value forKey:(NSString *)key {
  if (!key) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil key to a Dictionary"];
  }
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"GPBStringEnumDictionary: Attempt to set an unknown enum value (%d)",
                       value];
  }

  [_dictionary setObject:@(value) forKey:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

@end

//%PDDM-EXPAND-END (5 expansions)


//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_POD_IMPL(UInt32, uint32_t)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> UInt32

@implementation GPBBoolUInt32Dictionary {
 @package
  uint32_t _values[2];
  BOOL _valueSet[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolUInt32Dictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolUInt32Dictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolUInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolUInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = values[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolUInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolUInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolUInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolUInt32Dictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %u", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %u", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(uint32_t *)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      *value = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueUInt32;
  _valueSet[idx] = YES;
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", [NSString stringWithFormat:@"%u", _values[0]]);
  }
  if (_valueSet[1]) {
    block(@"true", [NSString stringWithFormat:@"%u", _values[1]]);
  }
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, uint32_t value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictUInt32FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictUInt32FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictUInt32Field(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolUInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint32_t)value forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_POD_IMPL(Int32, int32_t)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Int32

@implementation GPBBoolInt32Dictionary {
 @package
  int32_t _values[2];
  BOOL _valueSet[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolInt32Dictionary*)[self alloc] initWithValues:&value
                                                        forKeys:&key
                                                          count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolInt32Dictionary*)[self alloc] initWithValues:values
                                                        forKeys:keys
                                                          count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolInt32Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolInt32Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = values[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolInt32Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolInt32Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolInt32Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolInt32Dictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %d", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %d", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(int32_t *)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      *value = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueInt32;
  _valueSet[idx] = YES;
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", [NSString stringWithFormat:@"%d", _values[0]]);
  }
  if (_valueSet[1]) {
    block(@"true", [NSString stringWithFormat:@"%d", _values[1]]);
  }
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, int32_t value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictInt32FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictInt32FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictInt32Field(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolInt32Dictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int32_t)value forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_POD_IMPL(UInt64, uint64_t)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> UInt64

@implementation GPBBoolUInt64Dictionary {
 @package
  uint64_t _values[2];
  BOOL _valueSet[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolUInt64Dictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolUInt64Dictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolUInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolUInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = values[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolUInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolUInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolUInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolUInt64Dictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %llu", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %llu", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(uint64_t *)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      *value = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueUInt64;
  _valueSet[idx] = YES;
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", [NSString stringWithFormat:@"%llu", _values[0]]);
  }
  if (_valueSet[1]) {
    block(@"true", [NSString stringWithFormat:@"%llu", _values[1]]);
  }
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, uint64_t value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictUInt64FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictUInt64FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictUInt64Field(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolUInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(uint64_t)value forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_POD_IMPL(Int64, int64_t)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Int64

@implementation GPBBoolInt64Dictionary {
 @package
  int64_t _values[2];
  BOOL _valueSet[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolInt64Dictionary*)[self alloc] initWithValues:&value
                                                        forKeys:&key
                                                          count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolInt64Dictionary*)[self alloc] initWithValues:values
                                                        forKeys:keys
                                                          count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolInt64Dictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolInt64Dictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = values[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolInt64Dictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolInt64Dictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolInt64Dictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolInt64Dictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %lld", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %lld", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(int64_t *)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      *value = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueInt64;
  _valueSet[idx] = YES;
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", [NSString stringWithFormat:@"%lld", _values[0]]);
  }
  if (_valueSet[1]) {
    block(@"true", [NSString stringWithFormat:@"%lld", _values[1]]);
  }
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, int64_t value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictInt64FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictInt64FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictInt64Field(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolInt64Dictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int64_t)value forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_POD_IMPL(Bool, BOOL)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Bool

@implementation GPBBoolBoolDictionary {
 @package
  BOOL _values[2];
  BOOL _valueSet[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolBoolDictionary*)[self alloc] initWithValues:&value
                                                       forKeys:&key
                                                         count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolBoolDictionary*)[self alloc] initWithValues:values
                                                       forKeys:keys
                                                         count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolBoolDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolBoolDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = values[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolBoolDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolBoolDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolBoolDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolBoolDictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %d", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %d", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(BOOL *)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      *value = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueBool;
  _valueSet[idx] = YES;
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", (_values[0] ? @"true" : @"false"));
  }
  if (_valueSet[1]) {
    block(@"true", (_values[1] ? @"true" : @"false"));
  }
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, BOOL value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictBoolFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictBoolFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictBoolField(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolBoolDictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(BOOL)value forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_POD_IMPL(Float, float)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Float

@implementation GPBBoolFloatDictionary {
 @package
  float _values[2];
  BOOL _valueSet[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolFloatDictionary*)[self alloc] initWithValues:&value
                                                        forKeys:&key
                                                          count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolFloatDictionary*)[self alloc] initWithValues:values
                                                        forKeys:keys
                                                          count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolFloatDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolFloatDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = values[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolFloatDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolFloatDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolFloatDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolFloatDictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %f", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %f", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(float *)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      *value = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueFloat;
  _valueSet[idx] = YES;
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", [NSString stringWithFormat:@"%.*g", FLT_DIG, _values[0]]);
  }
  if (_valueSet[1]) {
    block(@"true", [NSString stringWithFormat:@"%.*g", FLT_DIG, _values[1]]);
  }
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, float value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictFloatFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictFloatFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictFloatField(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolFloatDictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(float)value forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_POD_IMPL(Double, double)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Double

@implementation GPBBoolDoubleDictionary {
 @package
  double _values[2];
  BOOL _valueSet[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithValues:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolDoubleDictionary*)[self alloc] initWithValues:&value
                                                         forKeys:&key
                                                           count:1] autorelease];
}

+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolDoubleDictionary*)[self alloc] initWithValues:values
                                                         forKeys:keys
                                                           count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolDoubleDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolDoubleDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = values[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolDoubleDictionary *)dictionary {
  self = [self initWithValues:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolDoubleDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolDoubleDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolDoubleDictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %lf", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %lf", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(double *)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      *value = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueDouble;
  _valueSet[idx] = YES;
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", [NSString stringWithFormat:@"%.*lg", DBL_DIG, _values[0]]);
  }
  if (_valueSet[1]) {
    block(@"true", [NSString stringWithFormat:@"%.*lg", DBL_DIG, _values[1]]);
  }
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, double value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictDoubleFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictDoubleFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictDoubleField(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolDoubleDictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(double)value forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

//%PDDM-EXPAND DICTIONARY_BOOL_KEY_TO_OBJECT_IMPL(Object, id)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Object

@implementation GPBBoolObjectDictionary {
 @package
  id _values[2];
}

+ (instancetype)dictionary {
  return [[[self alloc] initWithObjects:NULL forKeys:NULL count:0] autorelease];
}

+ (instancetype)dictionaryWithObject:(id)object
                              forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolObjectDictionary*)[self alloc] initWithObjects:&object
                                                          forKeys:&key
                                                            count:1] autorelease];
}

+ (instancetype)dictionaryWithObjects:(const id [])objects
                              forKeys:(const BOOL [])keys
                                count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithObjects:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolObjectDictionary*)[self alloc] initWithObjects:objects
                                                          forKeys:keys
                                                            count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolObjectDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithDictionary:
  // on to get the type correct.
  return [[(GPBBoolObjectDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems {
  return [[[self alloc] initWithCapacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (instancetype)initWithObjects:(const id [])objects
                        forKeys:(const BOOL [])keys
                          count:(NSUInteger)count {
  self = [super init];
  if (self) {
    for (NSUInteger i = 0; i < count; ++i) {
      if (!objects[i]) {
        [NSException raise:NSInvalidArgumentException
                    format:@"Attempting to add nil object to a Dictionary"];
      }
      int idx = keys[i] ? 1 : 0;
      [_values[idx] release];
      _values[idx] = (id)[objects[i] retain];
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolObjectDictionary *)dictionary {
  self = [self initWithObjects:NULL forKeys:NULL count:0];
  if (self) {
    if (dictionary) {
      _values[0] = [dictionary->_values[0] retain];
      _values[1] = [dictionary->_values[1] retain];
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)numItems {
  #pragma unused(numItems)
  return [self initWithObjects:NULL forKeys:NULL count:0];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_values[0] release];
  [_values[1] release];
  [super dealloc];
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolObjectDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolObjectDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolObjectDictionary class]]) {
    return NO;
  }
  if (((_values[0] != nil) != (other->_values[0] != nil)) ||
      ((_values[1] != nil) != (other->_values[1] != nil))) {
    return NO;
  }
  if (((_values[0] != nil) && (![_values[0] isEqual:other->_values[0]])) ||
      ((_values[1] != nil) && (![_values[1] isEqual:other->_values[1]]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return ((_values[0] != nil) ? 1 : 0) + ((_values[1] != nil) ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if ((_values[0] != nil)) {
    [result appendFormat:@"NO: %@", _values[0]];
  }
  if ((_values[1] != nil)) {
    [result appendFormat:@"YES: %@", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return ((_values[0] != nil) ? 1 : 0) + ((_values[1] != nil) ? 1 : 0);
}

- (id)objectForKey:(BOOL)key {
  return _values[key ? 1 : 0];
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  [_values[idx] release];
  _values[idx] = [value->valueString retain];
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_values[0] != nil) {
    block(@"false", _values[0]);
  }
  if ((_values[1] != nil)) {
    block(@"true", _values[1]);
  }
}

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(BOOL key, id object, BOOL *stop))block {
  BOOL stop = NO;
  if (_values[0] != nil) {
    block(NO, _values[0], &stop);
  }
  if (!stop && (_values[1] != nil)) {
    block(YES, _values[1], &stop);
  }
}

- (BOOL)isInitialized {
  if (_values[0] && ![_values[0] isInitialized]) {
    return NO;
  }
  if (_values[1] && ![_values[1] isInitialized]) {
    return NO;
  }
  return YES;
}

- (instancetype)deepCopyWithZone:(NSZone *)zone {
  GPBBoolObjectDictionary *newDict =
      [[GPBBoolObjectDictionary alloc] init];
  for (int i = 0; i < 2; ++i) {
    if (_values[i] != nil) {
      newDict->_values[i] = [_values[i] copyWithZone:zone];
    }
  }
  return newDict;
}

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_values[i] != nil) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictObjectFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_values[i] != nil) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictObjectFieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictObjectField(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)addEntriesFromDictionary:(GPBBoolObjectDictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_values[i] != nil) {
        [_values[i] release];
        _values[i] = [otherDictionary->_values[i] retain];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setObject:(id)object forKey:(BOOL)key {
  if (!object) {
    [NSException raise:NSInvalidArgumentException
                format:@"Attempting to add nil object to a Dictionary"];
  }
  int idx = (key ? 1 : 0);
  [_values[idx] release];
  _values[idx] = [object retain];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeObjectForKey:(BOOL)aKey {
  int idx = (aKey ? 1 : 0);
  [_values[idx] release];
  _values[idx] = nil;
}

- (void)removeAll {
  for (int i = 0; i < 2; ++i) {
    [_values[i] release];
    _values[i] = nil;
  }
}

@end

//%PDDM-EXPAND-END (8 expansions)

#pragma mark - Bool -> Enum

@implementation GPBBoolEnumDictionary {
 @package
  GPBEnumValidationFunc _validationFunc;
  int32_t _values[2];
  BOOL _valueSet[2];
}

@synthesize validationFunc = _validationFunc;

+ (instancetype)dictionary {
  return [[[self alloc] initWithValidationFunction:NULL
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func {
  return [[[self alloc] initWithValidationFunction:func
                                         rawValues:NULL
                                           forKeys:NULL
                                             count:0] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                       rawValue:(int32_t)rawValue
                                          forKey:(BOOL)key {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolEnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                 rawValues:&rawValue
                                                                   forKeys:&key
                                                                     count:1] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])values
                                         forKeys:(const BOOL [])keys
                                           count:(NSUInteger)count {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolEnumDictionary*)[self alloc] initWithValidationFunction:func
                                                                 rawValues:values
                                                                   forKeys:keys
                                                                     count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(GPBBoolEnumDictionary *)dictionary {
  // Cast is needed so the compiler knows what class we are invoking initWithValues:forKeys:count:
  // on to get the type correct.
  return [[(GPBBoolEnumDictionary*)[self alloc] initWithDictionary:dictionary] autorelease];
}

+ (instancetype)dictionaryWithValidationFunction:(GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems {
  return [[[self alloc] initWithValidationFunction:func capacity:numItems] autorelease];
}

- (instancetype)init {
  return [self initWithValidationFunction:NULL rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                rawValues:(const int32_t [])rawValues
                                   forKeys:(const BOOL [])keys
                                     count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _validationFunc = (func != NULL ? func : DictDefault_IsValidValue);
    for (NSUInteger i = 0; i < count; ++i) {
      int idx = keys[i] ? 1 : 0;
      _values[idx] = rawValues[i];
      _valueSet[idx] = YES;
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(GPBBoolEnumDictionary *)dictionary {
  self = [self initWithValidationFunction:dictionary.validationFunc
                                rawValues:NULL
                                  forKeys:NULL
                                    count:0];
  if (self) {
    if (dictionary) {
      for (int i = 0; i < 2; ++i) {
        if (dictionary->_valueSet[i]) {
          _values[i] = dictionary->_values[i];
          _valueSet[i] = YES;
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems {
#pragma unused(numItems)
  return [self initWithValidationFunction:func rawValues:NULL forKeys:NULL count:0];
}

#if !defined(NS_BLOCK_ASSERTIONS)
- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [super dealloc];
}
#endif  // !defined(NS_BLOCK_ASSERTIONS)

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolEnumDictionary allocWithZone:zone] initWithDictionary:self];
}

- (BOOL)isEqual:(GPBBoolEnumDictionary *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolEnumDictionary class]]) {
    return NO;
  }
  if ((_valueSet[0] != other->_valueSet[0]) ||
      (_valueSet[1] != other->_valueSet[1])) {
    return NO;
  }
  if ((_valueSet[0] && (_values[0] != other->_values[0])) ||
      (_valueSet[1] && (_values[1] != other->_values[1]))) {
    return NO;
  }
  return YES;
}

- (NSUInteger)hash {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> {", [self class], self];
  if (_valueSet[0]) {
    [result appendFormat:@"NO: %d", _values[0]];
  }
  if (_valueSet[1]) {
    [result appendFormat:@"YES: %d", _values[1]];
  }
  [result appendString:@" }"];
  return result;
}

- (NSUInteger)count {
  return (_valueSet[0] ? 1 : 0) + (_valueSet[1] ? 1 : 0);
}

- (BOOL)valueForKey:(BOOL)key value:(int32_t*)value {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (value) {
      int32_t result = _values[idx];
      if (!_validationFunc(result)) {
        result = kGPBUnrecognizedEnumeratorValue;
      }
      *value = result;
    }
    return YES;
  }
  return NO;
}

- (BOOL)valueForKey:(BOOL)key rawValue:(int32_t*)rawValue {
  int idx = (key ? 1 : 0);
  if (_valueSet[idx]) {
    if (rawValue) {
      *rawValue = _values[idx];
    }
    return YES;
  }
  return NO;
}

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, int32_t value, BOOL *stop))block {
  BOOL stop = NO;
  if (_valueSet[0]) {
    block(NO, _values[0], &stop);
  }
  if (!stop && _valueSet[1]) {
    block(YES, _values[1], &stop);
  }
}

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(BOOL key, int32_t rawValue, BOOL *stop))block {
  BOOL stop = NO;
  GPBEnumValidationFunc func = _validationFunc;
  int32_t validatedValue;
  if (_valueSet[0]) {
    validatedValue = _values[0];
    if (!func(validatedValue)) {
      validatedValue = kGPBUnrecognizedEnumeratorValue;
    }
    block(NO, validatedValue, &stop);
  }
  if (!stop && _valueSet[1]) {
    validatedValue = _values[1];
    if (!func(validatedValue)) {
      validatedValue = kGPBUnrecognizedEnumeratorValue;
    }
    block(YES, validatedValue, &stop);
  }
}

//%PDDM-EXPAND SERIAL_DATA_FOR_ENTRY_POD_Enum(Bool)
// This block of code is generated, do not edit it directly.

- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType {
  size_t msgSize = ComputeDictBoolFieldSize(key->valueBool, kMapKeyFieldNumber, keyDataType);
  msgSize += ComputeDictEnumFieldSize(value, kMapValueFieldNumber, GPBDataTypeEnum);
  NSMutableData *data = [NSMutableData dataWithLength:msgSize];
  GPBCodedOutputStream *outputStream = [[GPBCodedOutputStream alloc] initWithData:data];
  WriteDictBoolField(outputStream, key->valueBool, kMapKeyFieldNumber, keyDataType);
  WriteDictEnumField(outputStream, value, kMapValueFieldNumber, GPBDataTypeEnum);
  [outputStream release];
  return data;
}

//%PDDM-EXPAND-END SERIAL_DATA_FOR_ENTRY_POD_Enum(Bool)

- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  NSUInteger count = 0;
  size_t result = 0;
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      ++count;
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictInt32FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      result += GPBComputeRawVarint32SizeForInteger(msgSize) + msgSize;
    }
  }
  size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field), GPBDataTypeMessage);
  result += tagSize * count;
  return result;
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field {
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  uint32_t tag = GPBWireFormatMakeTag(GPBFieldNumber(field), GPBWireFormatLengthDelimited);
  for (int i = 0; i < 2; ++i) {
    if (_valueSet[i]) {
      // Write the tag.
      [outputStream writeInt32NoTag:tag];
      // Write the size of the message.
      size_t msgSize = ComputeDictBoolFieldSize((i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      msgSize += ComputeDictInt32FieldSize(_values[i], kMapValueFieldNumber, valueDataType);
      [outputStream writeInt32NoTag:(int32_t)msgSize];
      // Write the fields.
      WriteDictBoolField(outputStream, (i == 1), kMapKeyFieldNumber, GPBDataTypeBool);
      WriteDictInt32Field(outputStream, _values[i], kMapValueFieldNumber, valueDataType);
    }
  }
}

- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block {
  if (_valueSet[0]) {
    block(@"false", @(_values[0]));
  }
  if (_valueSet[1]) {
    block(@"true", @(_values[1]));
  }
}

- (void)setGPBGenericValue:(GPBGenericValue *)value
     forGPBGenericValueKey:(GPBGenericValue *)key {
  int idx = (key->valueBool ? 1 : 0);
  _values[idx] = value->valueInt32;
  _valueSet[idx] = YES;
}

- (void)addRawEntriesFromDictionary:(GPBBoolEnumDictionary *)otherDictionary {
  if (otherDictionary) {
    for (int i = 0; i < 2; ++i) {
      if (otherDictionary->_valueSet[i]) {
        _valueSet[i] = YES;
        _values[i] = otherDictionary->_values[i];
      }
    }
    if (_autocreator) {
      GPBAutocreatedDictionaryModified(_autocreator, self);
    }
  }
}

- (void)setValue:(int32_t)value forKey:(BOOL)key {
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"GPBBoolEnumDictionary: Attempt to set an unknown enum value (%d)",
     value];
  }
  int idx = (key ? 1 : 0);
  _values[idx] = value;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)setRawValue:(int32_t)rawValue forKey:(BOOL)key {
  int idx = (key ? 1 : 0);
  _values[idx] = rawValue;
  _valueSet[idx] = YES;
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeValueForKey:(BOOL)aKey {
  _valueSet[aKey ? 1 : 0] = NO;
}

- (void)removeAll {
  _valueSet[0] = NO;
  _valueSet[1] = NO;
}

@end

#pragma mark - NSDictionary Subclass

@implementation GPBAutocreatedDictionary {
  NSMutableDictionary *_dictionary;
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_dictionary release];
  [super dealloc];
}

#pragma mark Required NSDictionary overrides

- (instancetype)initWithObjects:(const id [])objects
                        forKeys:(const id<NSCopying> [])keys
                          count:(NSUInteger)count {
  self = [super init];
  if (self) {
    _dictionary = [[NSMutableDictionary alloc] initWithObjects:objects
                                                       forKeys:keys
                                                         count:count];
  }
  return self;
}

- (NSUInteger)count {
  return [_dictionary count];
}

- (id)objectForKey:(id)aKey {
  return [_dictionary objectForKey:aKey];
}

- (NSEnumerator *)keyEnumerator {
  if (_dictionary == nil) {
    _dictionary = [[NSMutableDictionary alloc] init];
  }
  return [_dictionary keyEnumerator];
}

#pragma mark Required NSMutableDictionary overrides

// Only need to call GPBAutocreatedDictionaryModified() when adding things
// since we only autocreate empty dictionaries.

- (void)setObject:(id)anObject forKey:(id<NSCopying>)aKey {
  if (_dictionary == nil) {
    _dictionary = [[NSMutableDictionary alloc] init];
  }
  [_dictionary setObject:anObject forKey:aKey];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)removeObjectForKey:(id)aKey {
  [_dictionary removeObjectForKey:aKey];
}

#pragma mark Extra things hooked

- (id)copyWithZone:(NSZone *)zone {
  if (_dictionary == nil) {
    _dictionary = [[NSMutableDictionary alloc] init];
  }
  return [_dictionary copyWithZone:zone];
}

- (id)mutableCopyWithZone:(NSZone *)zone {
  if (_dictionary == nil) {
    _dictionary = [[NSMutableDictionary alloc] init];
  }
  return [_dictionary mutableCopyWithZone:zone];
}

- (id)objectForKeyedSubscript:(id)key {
  return [_dictionary objectForKeyedSubscript:key];
}

- (void)setObject:(id)obj forKeyedSubscript:(id<NSCopying>)key {
  if (_dictionary == nil) {
    _dictionary = [[NSMutableDictionary alloc] init];
  }
  [_dictionary setObject:obj forKeyedSubscript:key];
  if (_autocreator) {
    GPBAutocreatedDictionaryModified(_autocreator, self);
  }
}

- (void)enumerateKeysAndObjectsUsingBlock:(void (^)(id key,
                                                    id obj,
                                                    BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsUsingBlock:block];
}

- (void)enumerateKeysAndObjectsWithOptions:(NSEnumerationOptions)opts
                                usingBlock:(void (^)(id key,
                                                     id obj,
                                                     BOOL *stop))block {
  [_dictionary enumerateKeysAndObjectsWithOptions:opts usingBlock:block];
}

@end
