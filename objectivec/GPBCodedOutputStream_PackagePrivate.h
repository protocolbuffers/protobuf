// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
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

#import "GPBCodedOutputStream.h"

NS_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

size_t GPBComputeDoubleSize(int32_t fieldNumber, double value) __attribute__((const));
size_t GPBComputeFloatSize(int32_t fieldNumber, float value) __attribute__((const));
size_t GPBComputeUInt64Size(int32_t fieldNumber, uint64_t value) __attribute__((const));
size_t GPBComputeInt64Size(int32_t fieldNumber, int64_t value) __attribute__((const));
size_t GPBComputeInt32Size(int32_t fieldNumber, int32_t value) __attribute__((const));
size_t GPBComputeFixed64Size(int32_t fieldNumber, uint64_t value) __attribute__((const));
size_t GPBComputeFixed32Size(int32_t fieldNumber, uint32_t value) __attribute__((const));
size_t GPBComputeBoolSize(int32_t fieldNumber, BOOL value) __attribute__((const));
size_t GPBComputeStringSize(int32_t fieldNumber, NSString *value) __attribute__((const));
size_t GPBComputeGroupSize(int32_t fieldNumber, GPBMessage *value) __attribute__((const));
size_t GPBComputeUnknownGroupSize(int32_t fieldNumber, GPBUnknownFieldSet *value)
    __attribute__((const));
size_t GPBComputeMessageSize(int32_t fieldNumber, GPBMessage *value) __attribute__((const));
size_t GPBComputeBytesSize(int32_t fieldNumber, NSData *value) __attribute__((const));
size_t GPBComputeUInt32Size(int32_t fieldNumber, uint32_t value) __attribute__((const));
size_t GPBComputeSFixed32Size(int32_t fieldNumber, int32_t value) __attribute__((const));
size_t GPBComputeSFixed64Size(int32_t fieldNumber, int64_t value) __attribute__((const));
size_t GPBComputeSInt32Size(int32_t fieldNumber, int32_t value) __attribute__((const));
size_t GPBComputeSInt64Size(int32_t fieldNumber, int64_t value) __attribute__((const));
size_t GPBComputeTagSize(int32_t fieldNumber) __attribute__((const));
size_t GPBComputeWireFormatTagSize(int field_number, GPBDataType dataType) __attribute__((const));

size_t GPBComputeDoubleSizeNoTag(double value) __attribute__((const));
size_t GPBComputeFloatSizeNoTag(float value) __attribute__((const));
size_t GPBComputeUInt64SizeNoTag(uint64_t value) __attribute__((const));
size_t GPBComputeInt64SizeNoTag(int64_t value) __attribute__((const));
size_t GPBComputeInt32SizeNoTag(int32_t value) __attribute__((const));
size_t GPBComputeFixed64SizeNoTag(uint64_t value) __attribute__((const));
size_t GPBComputeFixed32SizeNoTag(uint32_t value) __attribute__((const));
size_t GPBComputeBoolSizeNoTag(BOOL value) __attribute__((const));
size_t GPBComputeStringSizeNoTag(NSString *value) __attribute__((const));
size_t GPBComputeGroupSizeNoTag(GPBMessage *value) __attribute__((const));
size_t GPBComputeUnknownGroupSizeNoTag(GPBUnknownFieldSet *value) __attribute__((const));
size_t GPBComputeMessageSizeNoTag(GPBMessage *value) __attribute__((const));
size_t GPBComputeBytesSizeNoTag(NSData *value) __attribute__((const));
size_t GPBComputeUInt32SizeNoTag(int32_t value) __attribute__((const));
size_t GPBComputeEnumSizeNoTag(int32_t value) __attribute__((const));
size_t GPBComputeSFixed32SizeNoTag(int32_t value) __attribute__((const));
size_t GPBComputeSFixed64SizeNoTag(int64_t value) __attribute__((const));
size_t GPBComputeSInt32SizeNoTag(int32_t value) __attribute__((const));
size_t GPBComputeSInt64SizeNoTag(int64_t value) __attribute__((const));

// Note that this will calculate the size of 64 bit values truncated to 32.
size_t GPBComputeSizeTSizeAsInt32NoTag(size_t value) __attribute__((const));

size_t GPBComputeRawVarint32Size(int32_t value) __attribute__((const));
size_t GPBComputeRawVarint64Size(int64_t value) __attribute__((const));

// Note that this will calculate the size of 64 bit values truncated to 32.
size_t GPBComputeRawVarint32SizeForInteger(NSInteger value) __attribute__((const));

// Compute the number of bytes that would be needed to encode a
// MessageSet extension to the stream.  For historical reasons,
// the wire format differs from normal fields.
size_t GPBComputeMessageSetExtensionSize(int32_t fieldNumber, GPBMessage *value)
    __attribute__((const));

// Compute the number of bytes that would be needed to encode an
// unparsed MessageSet extension field to the stream.  For
// historical reasons, the wire format differs from normal fields.
size_t GPBComputeRawMessageSetExtensionSize(int32_t fieldNumber, NSData *value)
    __attribute__((const));

size_t GPBComputeEnumSize(int32_t fieldNumber, int32_t value) __attribute__((const));

CF_EXTERN_C_END

NS_ASSUME_NONNULL_END
