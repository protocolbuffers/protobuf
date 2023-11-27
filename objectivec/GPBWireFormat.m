// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBWireFormat.h"

#import "GPBUtilities_PackagePrivate.h"

enum {
  GPBWireFormatTagTypeBits = 3,
  GPBWireFormatTagTypeMask = 7 /* = (1 << GPBWireFormatTagTypeBits) - 1 */,
};

uint32_t GPBWireFormatMakeTag(uint32_t fieldNumber, GPBWireFormat wireType) {
  return (fieldNumber << GPBWireFormatTagTypeBits) | wireType;
}

GPBWireFormat GPBWireFormatGetTagWireType(uint32_t tag) {
  return (GPBWireFormat)(tag & GPBWireFormatTagTypeMask);
}

uint32_t GPBWireFormatGetTagFieldNumber(uint32_t tag) {
  return GPBLogicalRightShift32(tag, GPBWireFormatTagTypeBits);
}

BOOL GPBWireFormatIsValidTag(uint32_t tag) {
  uint32_t formatBits = (tag & GPBWireFormatTagTypeMask);
  // The valid GPBWireFormat* values are 0-5, anything else is not a valid tag.
  BOOL result = (formatBits <= 5);
  return result;
}

GPBWireFormat GPBWireFormatForType(GPBDataType type, BOOL isPacked) {
  if (isPacked) {
    return GPBWireFormatLengthDelimited;
  }

  static const GPBWireFormat format[GPBDataType_Count] = {
      GPBWireFormatVarint,           // GPBDataTypeBool
      GPBWireFormatFixed32,          // GPBDataTypeFixed32
      GPBWireFormatFixed32,          // GPBDataTypeSFixed32
      GPBWireFormatFixed32,          // GPBDataTypeFloat
      GPBWireFormatFixed64,          // GPBDataTypeFixed64
      GPBWireFormatFixed64,          // GPBDataTypeSFixed64
      GPBWireFormatFixed64,          // GPBDataTypeDouble
      GPBWireFormatVarint,           // GPBDataTypeInt32
      GPBWireFormatVarint,           // GPBDataTypeInt64
      GPBWireFormatVarint,           // GPBDataTypeSInt32
      GPBWireFormatVarint,           // GPBDataTypeSInt64
      GPBWireFormatVarint,           // GPBDataTypeUInt32
      GPBWireFormatVarint,           // GPBDataTypeUInt64
      GPBWireFormatLengthDelimited,  // GPBDataTypeBytes
      GPBWireFormatLengthDelimited,  // GPBDataTypeString
      GPBWireFormatLengthDelimited,  // GPBDataTypeMessage
      GPBWireFormatStartGroup,       // GPBDataTypeGroup
      GPBWireFormatVarint            // GPBDataTypeEnum
  };
  return format[type];
}
