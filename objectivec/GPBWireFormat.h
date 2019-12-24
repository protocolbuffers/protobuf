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

#import "GPBRuntimeTypes.h"

CF_EXTERN_C_BEGIN

NS_ASSUME_NONNULL_BEGIN

typedef enum {
  GPBWireFormatVarint = 0,
  GPBWireFormatFixed64 = 1,
  GPBWireFormatLengthDelimited = 2,
  GPBWireFormatStartGroup = 3,
  GPBWireFormatEndGroup = 4,
  GPBWireFormatFixed32 = 5,
} GPBWireFormat;

enum {
  GPBWireFormatMessageSetItem = 1,
  GPBWireFormatMessageSetTypeId = 2,
  GPBWireFormatMessageSetMessage = 3
};

uint32_t GPBWireFormatMakeTag(uint32_t fieldNumber, GPBWireFormat wireType)
    __attribute__((const));
GPBWireFormat GPBWireFormatGetTagWireType(uint32_t tag) __attribute__((const));
uint32_t GPBWireFormatGetTagFieldNumber(uint32_t tag) __attribute__((const));
BOOL GPBWireFormatIsValidTag(uint32_t tag) __attribute__((const));

GPBWireFormat GPBWireFormatForType(GPBDataType dataType, BOOL isPacked)
    __attribute__((const));

#define GPBWireFormatMessageSetItemTag \
  (GPBWireFormatMakeTag(GPBWireFormatMessageSetItem, GPBWireFormatStartGroup))
#define GPBWireFormatMessageSetItemEndTag \
  (GPBWireFormatMakeTag(GPBWireFormatMessageSetItem, GPBWireFormatEndGroup))
#define GPBWireFormatMessageSetTypeIdTag \
  (GPBWireFormatMakeTag(GPBWireFormatMessageSetTypeId, GPBWireFormatVarint))
#define GPBWireFormatMessageSetMessageTag               \
  (GPBWireFormatMakeTag(GPBWireFormatMessageSetMessage, \
                        GPBWireFormatLengthDelimited))

NS_ASSUME_NONNULL_END

CF_EXTERN_C_END
