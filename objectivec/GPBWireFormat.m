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

GPBWireFormat GPBWireFormatForType(GPBType type, BOOL isPacked) {
  if (isPacked) {
    return GPBWireFormatLengthDelimited;
  }

  static const GPBWireFormat format[GPBTypeCount] = {
      GPBWireFormatVarint,           // GPBTypeBool
      GPBWireFormatFixed32,          // GPBTypeFixed32
      GPBWireFormatFixed32,          // GPBTypeSFixed32
      GPBWireFormatFixed32,          // GPBTypeFloat
      GPBWireFormatFixed64,          // GPBTypeFixed64
      GPBWireFormatFixed64,          // GPBTypeSFixed64
      GPBWireFormatFixed64,          // GPBTypeDouble
      GPBWireFormatVarint,           // GPBTypeInt32
      GPBWireFormatVarint,           // GPBTypeInt64
      GPBWireFormatVarint,           // GPBTypeSInt32
      GPBWireFormatVarint,           // GPBTypeSInt64
      GPBWireFormatVarint,           // GPBTypeUInt32
      GPBWireFormatVarint,           // GPBTypeUInt64
      GPBWireFormatLengthDelimited,  // GPBTypeBytes
      GPBWireFormatLengthDelimited,  // GPBTypeString
      GPBWireFormatLengthDelimited,  // GPBTypeMessage
      GPBWireFormatStartGroup,       // GPBTypeGroup
      GPBWireFormatVarint            // GPBTypeEnum
  };
  return format[type];
}
