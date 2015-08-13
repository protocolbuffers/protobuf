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

#import <Foundation/Foundation.h>

#import "GPBRuntimeTypes.h"
#import "GPBWireFormat.h"

@class GPBBoolArray;
@class GPBDoubleArray;
@class GPBEnumArray;
@class GPBFloatArray;
@class GPBMessage;
@class GPBInt32Array;
@class GPBInt64Array;
@class GPBUInt32Array;
@class GPBUInt64Array;
@class GPBUnknownFieldSet;

NS_ASSUME_NONNULL_BEGIN

@interface GPBCodedOutputStream : NSObject

// Creates a new stream to write into data.  Data must be sized to fit or it
// will error when it runs out of space.
+ (instancetype)streamWithData:(NSMutableData *)data;
+ (instancetype)streamWithOutputStream:(NSOutputStream *)output;
+ (instancetype)streamWithOutputStream:(NSOutputStream *)output
                            bufferSize:(size_t)bufferSize;

- (instancetype)initWithData:(NSMutableData *)data;
- (instancetype)initWithOutputStream:(NSOutputStream *)output;
- (instancetype)initWithOutputStream:(NSOutputStream *)output
                          bufferSize:(size_t)bufferSize;

- (void)flush;

- (void)writeRawByte:(uint8_t)value;

- (void)writeTag:(uint32_t)fieldNumber format:(GPBWireFormat)format;

- (void)writeRawLittleEndian32:(int32_t)value;
- (void)writeRawLittleEndian64:(int64_t)value;

- (void)writeRawVarint32:(int32_t)value;
- (void)writeRawVarint64:(int64_t)value;

// Note that this will truncate 64 bit values to 32.
- (void)writeRawVarintSizeTAs32:(size_t)value;

- (void)writeRawData:(NSData *)data;
- (void)writeRawPtr:(const void *)data
             offset:(size_t)offset
             length:(size_t)length;

//%PDDM-EXPAND _WRITE_DECLS()
// This block of code is generated, do not edit it directly.

- (void)writeDouble:(int32_t)fieldNumber value:(double)value;
- (void)writeDoubleArray:(int32_t)fieldNumber
                  values:(GPBDoubleArray *)values
                     tag:(uint32_t)tag;
- (void)writeDoubleNoTag:(double)value;

- (void)writeFloat:(int32_t)fieldNumber value:(float)value;
- (void)writeFloatArray:(int32_t)fieldNumber
                 values:(GPBFloatArray *)values
                    tag:(uint32_t)tag;
- (void)writeFloatNoTag:(float)value;

- (void)writeUInt64:(int32_t)fieldNumber value:(uint64_t)value;
- (void)writeUInt64Array:(int32_t)fieldNumber
                  values:(GPBUInt64Array *)values
                     tag:(uint32_t)tag;
- (void)writeUInt64NoTag:(uint64_t)value;

- (void)writeInt64:(int32_t)fieldNumber value:(int64_t)value;
- (void)writeInt64Array:(int32_t)fieldNumber
                 values:(GPBInt64Array *)values
                    tag:(uint32_t)tag;
- (void)writeInt64NoTag:(int64_t)value;

- (void)writeInt32:(int32_t)fieldNumber value:(int32_t)value;
- (void)writeInt32Array:(int32_t)fieldNumber
                 values:(GPBInt32Array *)values
                    tag:(uint32_t)tag;
- (void)writeInt32NoTag:(int32_t)value;

- (void)writeUInt32:(int32_t)fieldNumber value:(uint32_t)value;
- (void)writeUInt32Array:(int32_t)fieldNumber
                  values:(GPBUInt32Array *)values
                     tag:(uint32_t)tag;
- (void)writeUInt32NoTag:(uint32_t)value;

- (void)writeFixed64:(int32_t)fieldNumber value:(uint64_t)value;
- (void)writeFixed64Array:(int32_t)fieldNumber
                   values:(GPBUInt64Array *)values
                      tag:(uint32_t)tag;
- (void)writeFixed64NoTag:(uint64_t)value;

- (void)writeFixed32:(int32_t)fieldNumber value:(uint32_t)value;
- (void)writeFixed32Array:(int32_t)fieldNumber
                   values:(GPBUInt32Array *)values
                      tag:(uint32_t)tag;
- (void)writeFixed32NoTag:(uint32_t)value;

- (void)writeSInt32:(int32_t)fieldNumber value:(int32_t)value;
- (void)writeSInt32Array:(int32_t)fieldNumber
                  values:(GPBInt32Array *)values
                     tag:(uint32_t)tag;
- (void)writeSInt32NoTag:(int32_t)value;

- (void)writeSInt64:(int32_t)fieldNumber value:(int64_t)value;
- (void)writeSInt64Array:(int32_t)fieldNumber
                  values:(GPBInt64Array *)values
                     tag:(uint32_t)tag;
- (void)writeSInt64NoTag:(int64_t)value;

- (void)writeSFixed64:(int32_t)fieldNumber value:(int64_t)value;
- (void)writeSFixed64Array:(int32_t)fieldNumber
                    values:(GPBInt64Array *)values
                       tag:(uint32_t)tag;
- (void)writeSFixed64NoTag:(int64_t)value;

- (void)writeSFixed32:(int32_t)fieldNumber value:(int32_t)value;
- (void)writeSFixed32Array:(int32_t)fieldNumber
                    values:(GPBInt32Array *)values
                       tag:(uint32_t)tag;
- (void)writeSFixed32NoTag:(int32_t)value;

- (void)writeBool:(int32_t)fieldNumber value:(BOOL)value;
- (void)writeBoolArray:(int32_t)fieldNumber
                values:(GPBBoolArray *)values
                   tag:(uint32_t)tag;
- (void)writeBoolNoTag:(BOOL)value;

- (void)writeEnum:(int32_t)fieldNumber value:(int32_t)value;
- (void)writeEnumArray:(int32_t)fieldNumber
                values:(GPBEnumArray *)values
                   tag:(uint32_t)tag;
- (void)writeEnumNoTag:(int32_t)value;

- (void)writeString:(int32_t)fieldNumber value:(NSString *)value;
- (void)writeStringArray:(int32_t)fieldNumber values:(NSArray *)values;
- (void)writeStringNoTag:(NSString *)value;

- (void)writeMessage:(int32_t)fieldNumber value:(GPBMessage *)value;
- (void)writeMessageArray:(int32_t)fieldNumber values:(NSArray *)values;
- (void)writeMessageNoTag:(GPBMessage *)value;

- (void)writeBytes:(int32_t)fieldNumber value:(NSData *)value;
- (void)writeBytesArray:(int32_t)fieldNumber values:(NSArray *)values;
- (void)writeBytesNoTag:(NSData *)value;

- (void)writeGroup:(int32_t)fieldNumber
             value:(GPBMessage *)value;
- (void)writeGroupArray:(int32_t)fieldNumber values:(NSArray *)values;
- (void)writeGroupNoTag:(int32_t)fieldNumber
                  value:(GPBMessage *)value;

- (void)writeUnknownGroup:(int32_t)fieldNumber
                    value:(GPBUnknownFieldSet *)value;
- (void)writeUnknownGroupArray:(int32_t)fieldNumber values:(NSArray *)values;
- (void)writeUnknownGroupNoTag:(int32_t)fieldNumber
                         value:(GPBUnknownFieldSet *)value;

//%PDDM-EXPAND-END _WRITE_DECLS()

// Write a MessageSet extension field to the stream.  For historical reasons,
// the wire format differs from normal fields.
- (void)writeMessageSetExtension:(int32_t)fieldNumber value:(GPBMessage *)value;

// Write an unparsed MessageSet extension field to the stream.  For
// historical reasons, the wire format differs from normal fields.
- (void)writeRawMessageSetExtension:(int32_t)fieldNumber value:(NSData *)value;

@end

CF_EXTERN_C_BEGIN

size_t GPBComputeDoubleSize(int32_t fieldNumber, double value)
    __attribute__((const));
size_t GPBComputeFloatSize(int32_t fieldNumber, float value)
    __attribute__((const));
size_t GPBComputeUInt64Size(int32_t fieldNumber, uint64_t value)
    __attribute__((const));
size_t GPBComputeInt64Size(int32_t fieldNumber, int64_t value)
    __attribute__((const));
size_t GPBComputeInt32Size(int32_t fieldNumber, int32_t value)
    __attribute__((const));
size_t GPBComputeFixed64Size(int32_t fieldNumber, uint64_t value)
    __attribute__((const));
size_t GPBComputeFixed32Size(int32_t fieldNumber, uint32_t value)
    __attribute__((const));
size_t GPBComputeBoolSize(int32_t fieldNumber, BOOL value)
    __attribute__((const));
size_t GPBComputeStringSize(int32_t fieldNumber, NSString *value)
    __attribute__((const));
size_t GPBComputeGroupSize(int32_t fieldNumber, GPBMessage *value)
    __attribute__((const));
size_t GPBComputeUnknownGroupSize(int32_t fieldNumber,
                                  GPBUnknownFieldSet *value)
    __attribute__((const));
size_t GPBComputeMessageSize(int32_t fieldNumber, GPBMessage *value)
    __attribute__((const));
size_t GPBComputeBytesSize(int32_t fieldNumber, NSData *value)
    __attribute__((const));
size_t GPBComputeUInt32Size(int32_t fieldNumber, uint32_t value)
    __attribute__((const));
size_t GPBComputeSFixed32Size(int32_t fieldNumber, int32_t value)
    __attribute__((const));
size_t GPBComputeSFixed64Size(int32_t fieldNumber, int64_t value)
    __attribute__((const));
size_t GPBComputeSInt32Size(int32_t fieldNumber, int32_t value)
    __attribute__((const));
size_t GPBComputeSInt64Size(int32_t fieldNumber, int64_t value)
    __attribute__((const));
size_t GPBComputeTagSize(int32_t fieldNumber) __attribute__((const));
size_t GPBComputeWireFormatTagSize(int field_number, GPBDataType dataType)
    __attribute__((const));

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
size_t GPBComputeUnknownGroupSizeNoTag(GPBUnknownFieldSet *value)
    __attribute__((const));
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
size_t GPBComputeRawVarint32SizeForInteger(NSInteger value)
    __attribute__((const));

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

size_t GPBComputeEnumSize(int32_t fieldNumber, int32_t value)
    __attribute__((const));

CF_EXTERN_C_END

NS_ASSUME_NONNULL_END

// Write methods for types that can be in packed arrays.
//%PDDM-DEFINE _WRITE_PACKABLE_DECLS(NAME, ARRAY_TYPE, TYPE)
//%- (void)write##NAME:(int32_t)fieldNumber value:(TYPE)value;
//%- (void)write##NAME##Array:(int32_t)fieldNumber
//%       NAME$S     values:(GPB##ARRAY_TYPE##Array *)values
//%       NAME$S        tag:(uint32_t)tag;
//%- (void)write##NAME##NoTag:(TYPE)value;
//%
// Write methods for types that aren't in packed arrays.
//%PDDM-DEFINE _WRITE_UNPACKABLE_DECLS(NAME, TYPE)
//%- (void)write##NAME:(int32_t)fieldNumber value:(TYPE)value;
//%- (void)write##NAME##Array:(int32_t)fieldNumber values:(NSArray *)values;
//%- (void)write##NAME##NoTag:(TYPE)value;
//%
// Special write methods for Groups.
//%PDDM-DEFINE _WRITE_GROUP_DECLS(NAME, TYPE)
//%- (void)write##NAME:(int32_t)fieldNumber
//%       NAME$S value:(TYPE)value;
//%- (void)write##NAME##Array:(int32_t)fieldNumber values:(NSArray *)values;
//%- (void)write##NAME##NoTag:(int32_t)fieldNumber
//%            NAME$S value:(TYPE)value;
//%

// One macro to hide it all up above.
//%PDDM-DEFINE _WRITE_DECLS()
//%_WRITE_PACKABLE_DECLS(Double, Double, double)
//%_WRITE_PACKABLE_DECLS(Float, Float, float)
//%_WRITE_PACKABLE_DECLS(UInt64, UInt64, uint64_t)
//%_WRITE_PACKABLE_DECLS(Int64, Int64, int64_t)
//%_WRITE_PACKABLE_DECLS(Int32, Int32, int32_t)
//%_WRITE_PACKABLE_DECLS(UInt32, UInt32, uint32_t)
//%_WRITE_PACKABLE_DECLS(Fixed64, UInt64, uint64_t)
//%_WRITE_PACKABLE_DECLS(Fixed32, UInt32, uint32_t)
//%_WRITE_PACKABLE_DECLS(SInt32, Int32, int32_t)
//%_WRITE_PACKABLE_DECLS(SInt64, Int64, int64_t)
//%_WRITE_PACKABLE_DECLS(SFixed64, Int64, int64_t)
//%_WRITE_PACKABLE_DECLS(SFixed32, Int32, int32_t)
//%_WRITE_PACKABLE_DECLS(Bool, Bool, BOOL)
//%_WRITE_PACKABLE_DECLS(Enum, Enum, int32_t)
//%_WRITE_UNPACKABLE_DECLS(String, NSString *)
//%_WRITE_UNPACKABLE_DECLS(Message, GPBMessage *)
//%_WRITE_UNPACKABLE_DECLS(Bytes, NSData *)
//%_WRITE_GROUP_DECLS(Group, GPBMessage *)
//%_WRITE_GROUP_DECLS(UnknownGroup, GPBUnknownFieldSet *)
