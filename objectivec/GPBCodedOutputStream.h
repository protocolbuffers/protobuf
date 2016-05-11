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

/// Writes out protocol message fields.
///
/// The common uses of protocol buffers shouldn't need to use this class.
/// @c GPBMessage's provide a @c -data method that will serialize the message
/// for you.
///
/// @note Subclassing of GPBCodedOutputStream is NOT supported.
@interface GPBCodedOutputStream : NSObject

/// Creates a stream to fill in the given data. Data must be sized to fit or
/// an error will be raised when out of space.
+ (instancetype)streamWithData:(NSMutableData *)data;

/// Creates a stream to write into the given @c NSOutputStream.
+ (instancetype)streamWithOutputStream:(NSOutputStream *)output;

/// Initializes a stream to fill in the given data. Data must be sized to fit
/// or an error will be raised when out of space.
- (instancetype)initWithData:(NSMutableData *)data;

/// Initializes a stream to write into the given @c NSOutputStream.
- (instancetype)initWithOutputStream:(NSOutputStream *)output;

/// Flush any buffered data out.
- (void)flush;

/// Write the raw byte out.
- (void)writeRawByte:(uint8_t)value;

/// Write the tag for the given field number and wire format.
///
/// @param fieldNumber The field number.
/// @param format      The wire format the data for the field will be in.
- (void)writeTag:(uint32_t)fieldNumber format:(GPBWireFormat)format;

/// Write a 32bit value out in little endian format.
- (void)writeRawLittleEndian32:(int32_t)value;
/// Write a 64bit value out in little endian format.
- (void)writeRawLittleEndian64:(int64_t)value;

/// Write a 32bit value out in varint format.
- (void)writeRawVarint32:(int32_t)value;
/// Write a 64bit value out in varint format.
- (void)writeRawVarint64:(int64_t)value;

/// Write a size_t out as a 32bit varint value.
///
/// @note This will truncate 64 bit values to 32.
- (void)writeRawVarintSizeTAs32:(size_t)value;

/// Writes the contents of an @c NSData out.
- (void)writeRawData:(NSData *)data;
/// Writes out the given data.
///
/// @param data   The data blob to write out.
/// @param offset The offset into the blob to start writing out.
/// @param length The number of bytes from the blob to write out.
- (void)writeRawPtr:(const void *)data
             offset:(size_t)offset
             length:(size_t)length;

//%PDDM-EXPAND _WRITE_DECLS()
// This block of code is generated, do not edit it directly.

/// Write a double for the given field number.
- (void)writeDouble:(int32_t)fieldNumber value:(double)value;
/// Write a packed array of double for the given field number.
- (void)writeDoubleArray:(int32_t)fieldNumber
                  values:(GPBDoubleArray *)values
                     tag:(uint32_t)tag;
/// Write a double without any tag.
- (void)writeDoubleNoTag:(double)value;

/// Write a float for the given field number.
- (void)writeFloat:(int32_t)fieldNumber value:(float)value;
/// Write a packed array of float for the given field number.
- (void)writeFloatArray:(int32_t)fieldNumber
                 values:(GPBFloatArray *)values
                    tag:(uint32_t)tag;
/// Write a float without any tag.
- (void)writeFloatNoTag:(float)value;

/// Write a uint64_t for the given field number.
- (void)writeUInt64:(int32_t)fieldNumber value:(uint64_t)value;
/// Write a packed array of uint64_t for the given field number.
- (void)writeUInt64Array:(int32_t)fieldNumber
                  values:(GPBUInt64Array *)values
                     tag:(uint32_t)tag;
/// Write a uint64_t without any tag.
- (void)writeUInt64NoTag:(uint64_t)value;

/// Write a int64_t for the given field number.
- (void)writeInt64:(int32_t)fieldNumber value:(int64_t)value;
/// Write a packed array of int64_t for the given field number.
- (void)writeInt64Array:(int32_t)fieldNumber
                 values:(GPBInt64Array *)values
                    tag:(uint32_t)tag;
/// Write a int64_t without any tag.
- (void)writeInt64NoTag:(int64_t)value;

/// Write a int32_t for the given field number.
- (void)writeInt32:(int32_t)fieldNumber value:(int32_t)value;
/// Write a packed array of int32_t for the given field number.
- (void)writeInt32Array:(int32_t)fieldNumber
                 values:(GPBInt32Array *)values
                    tag:(uint32_t)tag;
/// Write a int32_t without any tag.
- (void)writeInt32NoTag:(int32_t)value;

/// Write a uint32_t for the given field number.
- (void)writeUInt32:(int32_t)fieldNumber value:(uint32_t)value;
/// Write a packed array of uint32_t for the given field number.
- (void)writeUInt32Array:(int32_t)fieldNumber
                  values:(GPBUInt32Array *)values
                     tag:(uint32_t)tag;
/// Write a uint32_t without any tag.
- (void)writeUInt32NoTag:(uint32_t)value;

/// Write a uint64_t for the given field number.
- (void)writeFixed64:(int32_t)fieldNumber value:(uint64_t)value;
/// Write a packed array of uint64_t for the given field number.
- (void)writeFixed64Array:(int32_t)fieldNumber
                   values:(GPBUInt64Array *)values
                      tag:(uint32_t)tag;
/// Write a uint64_t without any tag.
- (void)writeFixed64NoTag:(uint64_t)value;

/// Write a uint32_t for the given field number.
- (void)writeFixed32:(int32_t)fieldNumber value:(uint32_t)value;
/// Write a packed array of uint32_t for the given field number.
- (void)writeFixed32Array:(int32_t)fieldNumber
                   values:(GPBUInt32Array *)values
                      tag:(uint32_t)tag;
/// Write a uint32_t without any tag.
- (void)writeFixed32NoTag:(uint32_t)value;

/// Write a int32_t for the given field number.
- (void)writeSInt32:(int32_t)fieldNumber value:(int32_t)value;
/// Write a packed array of int32_t for the given field number.
- (void)writeSInt32Array:(int32_t)fieldNumber
                  values:(GPBInt32Array *)values
                     tag:(uint32_t)tag;
/// Write a int32_t without any tag.
- (void)writeSInt32NoTag:(int32_t)value;

/// Write a int64_t for the given field number.
- (void)writeSInt64:(int32_t)fieldNumber value:(int64_t)value;
/// Write a packed array of int64_t for the given field number.
- (void)writeSInt64Array:(int32_t)fieldNumber
                  values:(GPBInt64Array *)values
                     tag:(uint32_t)tag;
/// Write a int64_t without any tag.
- (void)writeSInt64NoTag:(int64_t)value;

/// Write a int64_t for the given field number.
- (void)writeSFixed64:(int32_t)fieldNumber value:(int64_t)value;
/// Write a packed array of int64_t for the given field number.
- (void)writeSFixed64Array:(int32_t)fieldNumber
                    values:(GPBInt64Array *)values
                       tag:(uint32_t)tag;
/// Write a int64_t without any tag.
- (void)writeSFixed64NoTag:(int64_t)value;

/// Write a int32_t for the given field number.
- (void)writeSFixed32:(int32_t)fieldNumber value:(int32_t)value;
/// Write a packed array of int32_t for the given field number.
- (void)writeSFixed32Array:(int32_t)fieldNumber
                    values:(GPBInt32Array *)values
                       tag:(uint32_t)tag;
/// Write a int32_t without any tag.
- (void)writeSFixed32NoTag:(int32_t)value;

/// Write a BOOL for the given field number.
- (void)writeBool:(int32_t)fieldNumber value:(BOOL)value;
/// Write a packed array of BOOL for the given field number.
- (void)writeBoolArray:(int32_t)fieldNumber
                values:(GPBBoolArray *)values
                   tag:(uint32_t)tag;
/// Write a BOOL without any tag.
- (void)writeBoolNoTag:(BOOL)value;

/// Write a int32_t for the given field number.
- (void)writeEnum:(int32_t)fieldNumber value:(int32_t)value;
/// Write a packed array of int32_t for the given field number.
- (void)writeEnumArray:(int32_t)fieldNumber
                values:(GPBEnumArray *)values
                   tag:(uint32_t)tag;
/// Write a int32_t without any tag.
- (void)writeEnumNoTag:(int32_t)value;

/// Write a NSString for the given field number.
- (void)writeString:(int32_t)fieldNumber value:(NSString *)value;
/// Write an array of NSString for the given field number.
- (void)writeStringArray:(int32_t)fieldNumber values:(NSArray<NSString*> *)values;
/// Write a NSString without any tag.
- (void)writeStringNoTag:(NSString *)value;

/// Write a GPBMessage for the given field number.
- (void)writeMessage:(int32_t)fieldNumber value:(GPBMessage *)value;
/// Write an array of GPBMessage for the given field number.
- (void)writeMessageArray:(int32_t)fieldNumber values:(NSArray<GPBMessage*> *)values;
/// Write a GPBMessage without any tag.
- (void)writeMessageNoTag:(GPBMessage *)value;

/// Write a NSData for the given field number.
- (void)writeBytes:(int32_t)fieldNumber value:(NSData *)value;
/// Write an array of NSData for the given field number.
- (void)writeBytesArray:(int32_t)fieldNumber values:(NSArray<NSData*> *)values;
/// Write a NSData without any tag.
- (void)writeBytesNoTag:(NSData *)value;

/// Write a GPBMessage for the given field number.
- (void)writeGroup:(int32_t)fieldNumber
             value:(GPBMessage *)value;
/// Write an array of GPBMessage for the given field number.
- (void)writeGroupArray:(int32_t)fieldNumber values:(NSArray<GPBMessage*> *)values;
/// Write a GPBMessage without any tag (but does write the endGroup tag).
- (void)writeGroupNoTag:(int32_t)fieldNumber
                  value:(GPBMessage *)value;

/// Write a GPBUnknownFieldSet for the given field number.
- (void)writeUnknownGroup:(int32_t)fieldNumber
                    value:(GPBUnknownFieldSet *)value;
/// Write an array of GPBUnknownFieldSet for the given field number.
- (void)writeUnknownGroupArray:(int32_t)fieldNumber values:(NSArray<GPBUnknownFieldSet*> *)values;
/// Write a GPBUnknownFieldSet without any tag (but does write the endGroup tag).
- (void)writeUnknownGroupNoTag:(int32_t)fieldNumber
                         value:(GPBUnknownFieldSet *)value;

//%PDDM-EXPAND-END _WRITE_DECLS()

/// Write a MessageSet extension field to the stream. For historical reasons,
/// the wire format differs from normal fields.
- (void)writeMessageSetExtension:(int32_t)fieldNumber value:(GPBMessage *)value;

/// Write an unparsed MessageSet extension field to the stream. For
/// historical reasons, the wire format differs from normal fields.
- (void)writeRawMessageSetExtension:(int32_t)fieldNumber value:(NSData *)value;

@end

NS_ASSUME_NONNULL_END

// Write methods for types that can be in packed arrays.
//%PDDM-DEFINE _WRITE_PACKABLE_DECLS(NAME, ARRAY_TYPE, TYPE)
//%/// Write a TYPE for the given field number.
//%- (void)write##NAME:(int32_t)fieldNumber value:(TYPE)value;
//%/// Write a packed array of TYPE for the given field number.
//%- (void)write##NAME##Array:(int32_t)fieldNumber
//%       NAME$S     values:(GPB##ARRAY_TYPE##Array *)values
//%       NAME$S        tag:(uint32_t)tag;
//%/// Write a TYPE without any tag.
//%- (void)write##NAME##NoTag:(TYPE)value;
//%
// Write methods for types that aren't in packed arrays.
//%PDDM-DEFINE _WRITE_UNPACKABLE_DECLS(NAME, TYPE)
//%/// Write a TYPE for the given field number.
//%- (void)write##NAME:(int32_t)fieldNumber value:(TYPE *)value;
//%/// Write an array of TYPE for the given field number.
//%- (void)write##NAME##Array:(int32_t)fieldNumber values:(NSArray<##TYPE##*> *)values;
//%/// Write a TYPE without any tag.
//%- (void)write##NAME##NoTag:(TYPE *)value;
//%
// Special write methods for Groups.
//%PDDM-DEFINE _WRITE_GROUP_DECLS(NAME, TYPE)
//%/// Write a TYPE for the given field number.
//%- (void)write##NAME:(int32_t)fieldNumber
//%       NAME$S value:(TYPE *)value;
//%/// Write an array of TYPE for the given field number.
//%- (void)write##NAME##Array:(int32_t)fieldNumber values:(NSArray<##TYPE##*> *)values;
//%/// Write a TYPE without any tag (but does write the endGroup tag).
//%- (void)write##NAME##NoTag:(int32_t)fieldNumber
//%            NAME$S value:(TYPE *)value;
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
//%_WRITE_UNPACKABLE_DECLS(String, NSString)
//%_WRITE_UNPACKABLE_DECLS(Message, GPBMessage)
//%_WRITE_UNPACKABLE_DECLS(Bytes, NSData)
//%_WRITE_GROUP_DECLS(Group, GPBMessage)
//%_WRITE_GROUP_DECLS(UnknownGroup, GPBUnknownFieldSet)
