// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBExtensionInternals.h"

#import <objc/runtime.h>

#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

GPB_INLINE size_t DataTypeSize(GPBDataType dataType) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
  switch (dataType) {
    case GPBDataTypeBool:
      return 1;
    case GPBDataTypeFixed32:
    case GPBDataTypeSFixed32:
    case GPBDataTypeFloat:
      return 4;
    case GPBDataTypeFixed64:
    case GPBDataTypeSFixed64:
    case GPBDataTypeDouble:
      return 8;
    default:
      return 0;
  }
#pragma clang diagnostic pop
}

static size_t ComputePBSerializedSizeNoTagOfObject(GPBDataType dataType, id object) {
#define FIELD_CASE(TYPE, ACCESSOR) \
  case GPBDataType##TYPE:          \
    return GPBCompute##TYPE##SizeNoTag([(NSNumber *)object ACCESSOR]);
#define FIELD_CASE2(TYPE) \
  case GPBDataType##TYPE: \
    return GPBCompute##TYPE##SizeNoTag(object);
  switch (dataType) {
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
    FIELD_CASE2(Bytes)
    FIELD_CASE2(String)
    FIELD_CASE2(Message)
    FIELD_CASE2(Group)
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static size_t ComputeSerializedSizeIncludingTagOfObject(GPBExtensionDescription *description,
                                                        id object) {
#define FIELD_CASE(TYPE, ACCESSOR) \
  case GPBDataType##TYPE:          \
    return GPBCompute##TYPE##Size(description->fieldNumber, [(NSNumber *)object ACCESSOR]);
#define FIELD_CASE2(TYPE) \
  case GPBDataType##TYPE: \
    return GPBCompute##TYPE##Size(description->fieldNumber, object);
  switch (description->dataType) {
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
    FIELD_CASE2(Bytes)
    FIELD_CASE2(String)
    FIELD_CASE2(Group)
    case GPBDataTypeMessage:
      if (GPBExtensionIsWireFormat(description)) {
        return GPBComputeMessageSetExtensionSize(description->fieldNumber, object);
      } else {
        return GPBComputeMessageSize(description->fieldNumber, object);
      }
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static size_t ComputeSerializedSizeIncludingTagOfArray(GPBExtensionDescription *description,
                                                       NSArray *values) {
  if (GPBExtensionIsPacked(description)) {
    size_t size = 0;
    size_t typeSize = DataTypeSize(description->dataType);
    if (typeSize != 0) {
      size = values.count * typeSize;
    } else {
      for (id value in values) {
        size += ComputePBSerializedSizeNoTagOfObject(description->dataType, value);
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

static void WriteObjectIncludingTagToCodedOutputStream(id object,
                                                       GPBExtensionDescription *description,
                                                       GPBCodedOutputStream *output) {
#define FIELD_CASE(TYPE, ACCESSOR)                                                     \
  case GPBDataType##TYPE:                                                              \
    [output write##TYPE:description->fieldNumber value:[(NSNumber *)object ACCESSOR]]; \
    return;
#define FIELD_CASE2(TYPE)                                       \
  case GPBDataType##TYPE:                                       \
    [output write##TYPE:description->fieldNumber value:object]; \
    return;
  switch (description->dataType) {
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
    FIELD_CASE2(Bytes)
    FIELD_CASE2(String)
    FIELD_CASE2(Group)
    case GPBDataTypeMessage:
      if (GPBExtensionIsWireFormat(description)) {
        [output writeMessageSetExtension:description->fieldNumber value:object];
      } else {
        [output writeMessage:description->fieldNumber value:object];
      }
      return;
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static void WriteObjectNoTagToCodedOutputStream(id object, GPBExtensionDescription *description,
                                                GPBCodedOutputStream *output) {
#define FIELD_CASE(TYPE, ACCESSOR)                             \
  case GPBDataType##TYPE:                                      \
    [output write##TYPE##NoTag:[(NSNumber *)object ACCESSOR]]; \
    return;
#define FIELD_CASE2(TYPE)               \
  case GPBDataType##TYPE:               \
    [output write##TYPE##NoTag:object]; \
    return;
  switch (description->dataType) {
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
    FIELD_CASE2(Bytes)
    FIELD_CASE2(String)
    FIELD_CASE2(Message)
    case GPBDataTypeGroup:
      [output writeGroupNoTag:description->fieldNumber value:object];
      return;
  }
#undef FIELD_CASE
#undef FIELD_CASE2
}

static void WriteArrayIncludingTagsToCodedOutputStream(NSArray *values,
                                                       GPBExtensionDescription *description,
                                                       GPBCodedOutputStream *output) {
  if (GPBExtensionIsPacked(description)) {
    [output writeTag:description->fieldNumber format:GPBWireFormatLengthDelimited];
    size_t dataSize = 0;
    size_t typeSize = DataTypeSize(description->dataType);
    if (typeSize != 0) {
      dataSize = values.count * typeSize;
    } else {
      for (id value in values) {
        dataSize += ComputePBSerializedSizeNoTagOfObject(description->dataType, value);
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

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

void GPBWriteExtensionValueToOutputStream(GPBExtensionDescriptor *extension, id value,
                                          GPBCodedOutputStream *output) {
  GPBExtensionDescription *description = extension->description_;
  if (GPBExtensionIsRepeated(description)) {
    WriteArrayIncludingTagsToCodedOutputStream(value, description, output);
  } else {
    WriteObjectIncludingTagToCodedOutputStream(value, description, output);
  }
}

size_t GPBComputeExtensionSerializedSizeIncludingTag(GPBExtensionDescriptor *extension, id value) {
  GPBExtensionDescription *description = extension->description_;
  if (GPBExtensionIsRepeated(description)) {
    return ComputeSerializedSizeIncludingTagOfArray(description, value);
  } else {
    return ComputeSerializedSizeIncludingTagOfObject(description, value);
  }
}

#pragma clang diagnostic pop
