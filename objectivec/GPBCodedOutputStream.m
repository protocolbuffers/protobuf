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

#import "GPBCodedOutputStream_PackagePrivate.h"

#import <mach/vm_param.h>

#import "GPBArray.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

// Structure for containing state of a GPBCodedInputStream. Brought out into
// a struct so that we can inline several common functions instead of dealing
// with overhead of ObjC dispatch.
typedef struct GPBOutputBufferState {
  uint8_t *bytes;
  size_t size;
  size_t position;
  NSOutputStream *output;
} GPBOutputBufferState;

@implementation GPBCodedOutputStream {
  GPBOutputBufferState state_;
  NSMutableData *buffer_;
}

static const int32_t LITTLE_ENDIAN_32_SIZE = sizeof(uint32_t);
static const int32_t LITTLE_ENDIAN_64_SIZE = sizeof(uint64_t);

// Internal helper that writes the current buffer to the output. The
// buffer position is reset to its initial value when this returns.
static void GPBRefreshBuffer(GPBOutputBufferState *state) {
  if (state->output == nil) {
    // We're writing to a single buffer.
    [NSException raise:@"OutOfSpace" format:@""];
  }
  if (state->position != 0) {
    NSInteger written =
        [state->output write:state->bytes maxLength:state->position];
    if (written != (NSInteger)state->position) {
      [NSException raise:@"WriteFailed" format:@""];
    }
    state->position = 0;
  }
}

static void GPBWriteRawByte(GPBOutputBufferState *state, uint8_t value) {
  if (state->position == state->size) {
    GPBRefreshBuffer(state);
  }
  state->bytes[state->position++] = value;
}

static void GPBWriteRawVarint32(GPBOutputBufferState *state, int32_t value) {
  while (YES) {
    if ((value & ~0x7F) == 0) {
      uint8_t val = (uint8_t)value;
      GPBWriteRawByte(state, val);
      return;
    } else {
      GPBWriteRawByte(state, (value & 0x7F) | 0x80);
      value = GPBLogicalRightShift32(value, 7);
    }
  }
}

static void GPBWriteRawVarint64(GPBOutputBufferState *state, int64_t value) {
  while (YES) {
    if ((value & ~0x7FL) == 0) {
      uint8_t val = (uint8_t)value;
      GPBWriteRawByte(state, val);
      return;
    } else {
      GPBWriteRawByte(state, ((int32_t)value & 0x7F) | 0x80);
      value = GPBLogicalRightShift64(value, 7);
    }
  }
}

static void GPBWriteInt32NoTag(GPBOutputBufferState *state, int32_t value) {
  if (value >= 0) {
    GPBWriteRawVarint32(state, value);
  } else {
    // Must sign-extend
    GPBWriteRawVarint64(state, value);
  }
}

static void GPBWriteUInt32(GPBOutputBufferState *state, int32_t fieldNumber,
                           uint32_t value) {
  GPBWriteTagWithFormat(state, fieldNumber, GPBWireFormatVarint);
  GPBWriteRawVarint32(state, value);
}

static void GPBWriteTagWithFormat(GPBOutputBufferState *state,
                                  uint32_t fieldNumber, GPBWireFormat format) {
  GPBWriteRawVarint32(state, GPBWireFormatMakeTag(fieldNumber, format));
}

static void GPBWriteRawLittleEndian32(GPBOutputBufferState *state,
                                      int32_t value) {
  GPBWriteRawByte(state, (value)&0xFF);
  GPBWriteRawByte(state, (value >> 8) & 0xFF);
  GPBWriteRawByte(state, (value >> 16) & 0xFF);
  GPBWriteRawByte(state, (value >> 24) & 0xFF);
}

static void GPBWriteRawLittleEndian64(GPBOutputBufferState *state,
                                      int64_t value) {
  GPBWriteRawByte(state, (int32_t)(value)&0xFF);
  GPBWriteRawByte(state, (int32_t)(value >> 8) & 0xFF);
  GPBWriteRawByte(state, (int32_t)(value >> 16) & 0xFF);
  GPBWriteRawByte(state, (int32_t)(value >> 24) & 0xFF);
  GPBWriteRawByte(state, (int32_t)(value >> 32) & 0xFF);
  GPBWriteRawByte(state, (int32_t)(value >> 40) & 0xFF);
  GPBWriteRawByte(state, (int32_t)(value >> 48) & 0xFF);
  GPBWriteRawByte(state, (int32_t)(value >> 56) & 0xFF);
}

- (void)dealloc {
  [self flush];
  [state_.output close];
  [state_.output release];
  [buffer_ release];

  [super dealloc];
}

- (instancetype)initWithOutputStream:(NSOutputStream *)output {
  NSMutableData *data = [NSMutableData dataWithLength:PAGE_SIZE];
  return [self initWithOutputStream:output data:data];
}

- (instancetype)initWithData:(NSMutableData *)data {
  return [self initWithOutputStream:nil data:data];
}

// This initializer isn't exposed, but it is the designated initializer.
// Setting OutputStream and NSData is to control the buffering behavior/size
// of the work, but that is more obvious via the bufferSize: version.
- (instancetype)initWithOutputStream:(NSOutputStream *)output
                                data:(NSMutableData *)data {
  if ((self = [super init])) {
    buffer_ = [data retain];
    [output open];
    state_.bytes = [data mutableBytes];
    state_.size = [data length];
    state_.output = [output retain];
  }
  return self;
}

+ (instancetype)streamWithOutputStream:(NSOutputStream *)output {
  NSMutableData *data = [NSMutableData dataWithLength:PAGE_SIZE];
  return [[[self alloc] initWithOutputStream:output
                                        data:data] autorelease];
}

+ (instancetype)streamWithData:(NSMutableData *)data {
  return [[[self alloc] initWithData:data] autorelease];
}

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

- (void)writeDoubleNoTag:(double)value {
  GPBWriteRawLittleEndian64(&state_, GPBConvertDoubleToInt64(value));
}

- (void)writeDouble:(int32_t)fieldNumber value:(double)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatFixed64);
  GPBWriteRawLittleEndian64(&state_, GPBConvertDoubleToInt64(value));
}

- (void)writeFloatNoTag:(float)value {
  GPBWriteRawLittleEndian32(&state_, GPBConvertFloatToInt32(value));
}

- (void)writeFloat:(int32_t)fieldNumber value:(float)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatFixed32);
  GPBWriteRawLittleEndian32(&state_, GPBConvertFloatToInt32(value));
}

- (void)writeUInt64NoTag:(uint64_t)value {
  GPBWriteRawVarint64(&state_, value);
}

- (void)writeUInt64:(int32_t)fieldNumber value:(uint64_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatVarint);
  GPBWriteRawVarint64(&state_, value);
}

- (void)writeInt64NoTag:(int64_t)value {
  GPBWriteRawVarint64(&state_, value);
}

- (void)writeInt64:(int32_t)fieldNumber value:(int64_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatVarint);
  GPBWriteRawVarint64(&state_, value);
}

- (void)writeInt32NoTag:(int32_t)value {
  GPBWriteInt32NoTag(&state_, value);
}

- (void)writeInt32:(int32_t)fieldNumber value:(int32_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatVarint);
  GPBWriteInt32NoTag(&state_, value);
}

- (void)writeFixed64NoTag:(uint64_t)value {
  GPBWriteRawLittleEndian64(&state_, value);
}

- (void)writeFixed64:(int32_t)fieldNumber value:(uint64_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatFixed64);
  GPBWriteRawLittleEndian64(&state_, value);
}

- (void)writeFixed32NoTag:(uint32_t)value {
  GPBWriteRawLittleEndian32(&state_, value);
}

- (void)writeFixed32:(int32_t)fieldNumber value:(uint32_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatFixed32);
  GPBWriteRawLittleEndian32(&state_, value);
}

- (void)writeBoolNoTag:(BOOL)value {
  GPBWriteRawByte(&state_, (value ? 1 : 0));
}

- (void)writeBool:(int32_t)fieldNumber value:(BOOL)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatVarint);
  GPBWriteRawByte(&state_, (value ? 1 : 0));
}

- (void)writeStringNoTag:(const NSString *)value {
  size_t length = [value lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
  GPBWriteRawVarint32(&state_, (int32_t)length);
  if (length == 0) {
    return;
  }

  const char *quickString =
      CFStringGetCStringPtr((CFStringRef)value, kCFStringEncodingUTF8);

  // Fast path: Most strings are short, if the buffer already has space,
  // add to it directly.
  NSUInteger bufferBytesLeft = state_.size - state_.position;
  if (bufferBytesLeft >= length) {
    NSUInteger usedBufferLength = 0;
    BOOL result;
    if (quickString != NULL) {
      memcpy(state_.bytes + state_.position, quickString, length);
      usedBufferLength = length;
      result = YES;
    } else {
      result = [value getBytes:state_.bytes + state_.position
                     maxLength:bufferBytesLeft
                    usedLength:&usedBufferLength
                      encoding:NSUTF8StringEncoding
                       options:0
                         range:NSMakeRange(0, [value length])
                remainingRange:NULL];
    }
    if (result) {
      NSAssert2((usedBufferLength == length),
                @"Our UTF8 calc was wrong? %tu vs %zd", usedBufferLength,
                length);
      state_.position += usedBufferLength;
      return;
    }
  } else if (quickString != NULL) {
    [self writeRawPtr:quickString offset:0 length:length];
  } else {
    // Slow path: just get it as data and write it out.
    NSData *utf8Data = [value dataUsingEncoding:NSUTF8StringEncoding];
    NSAssert2(([utf8Data length] == length),
              @"Strings UTF8 length was wrong? %tu vs %zd", [utf8Data length],
              length);
    [self writeRawData:utf8Data];
  }
}

- (void)writeString:(int32_t)fieldNumber value:(NSString *)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatLengthDelimited);
  [self writeStringNoTag:value];
}

- (void)writeGroupNoTag:(int32_t)fieldNumber value:(GPBMessage *)value {
  [value writeToCodedOutputStream:self];
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatEndGroup);
}

- (void)writeGroup:(int32_t)fieldNumber value:(GPBMessage *)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatStartGroup);
  [self writeGroupNoTag:fieldNumber value:value];
}

- (void)writeUnknownGroupNoTag:(int32_t)fieldNumber
                         value:(const GPBUnknownFieldSet *)value {
  [value writeToCodedOutputStream:self];
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatEndGroup);
}

- (void)writeUnknownGroup:(int32_t)fieldNumber
                    value:(GPBUnknownFieldSet *)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatStartGroup);
  [self writeUnknownGroupNoTag:fieldNumber value:value];
}

- (void)writeMessageNoTag:(GPBMessage *)value {
  GPBWriteRawVarint32(&state_, (int32_t)[value serializedSize]);
  [value writeToCodedOutputStream:self];
}

- (void)writeMessage:(int32_t)fieldNumber value:(GPBMessage *)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatLengthDelimited);
  [self writeMessageNoTag:value];
}

- (void)writeBytesNoTag:(NSData *)value {
  GPBWriteRawVarint32(&state_, (int32_t)[value length]);
  [self writeRawData:value];
}

- (void)writeBytes:(int32_t)fieldNumber value:(NSData *)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatLengthDelimited);
  [self writeBytesNoTag:value];
}

- (void)writeUInt32NoTag:(uint32_t)value {
  GPBWriteRawVarint32(&state_, value);
}

- (void)writeUInt32:(int32_t)fieldNumber value:(uint32_t)value {
  GPBWriteUInt32(&state_, fieldNumber, value);
}

- (void)writeEnumNoTag:(int32_t)value {
  GPBWriteRawVarint32(&state_, value);
}

- (void)writeEnum:(int32_t)fieldNumber value:(int32_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatVarint);
  GPBWriteRawVarint32(&state_, value);
}

- (void)writeSFixed32NoTag:(int32_t)value {
  GPBWriteRawLittleEndian32(&state_, value);
}

- (void)writeSFixed32:(int32_t)fieldNumber value:(int32_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatFixed32);
  GPBWriteRawLittleEndian32(&state_, value);
}

- (void)writeSFixed64NoTag:(int64_t)value {
  GPBWriteRawLittleEndian64(&state_, value);
}

- (void)writeSFixed64:(int32_t)fieldNumber value:(int64_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatFixed64);
  GPBWriteRawLittleEndian64(&state_, value);
}

- (void)writeSInt32NoTag:(int32_t)value {
  GPBWriteRawVarint32(&state_, GPBEncodeZigZag32(value));
}

- (void)writeSInt32:(int32_t)fieldNumber value:(int32_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatVarint);
  GPBWriteRawVarint32(&state_, GPBEncodeZigZag32(value));
}

- (void)writeSInt64NoTag:(int64_t)value {
  GPBWriteRawVarint64(&state_, GPBEncodeZigZag64(value));
}

- (void)writeSInt64:(int32_t)fieldNumber value:(int64_t)value {
  GPBWriteTagWithFormat(&state_, fieldNumber, GPBWireFormatVarint);
  GPBWriteRawVarint64(&state_, GPBEncodeZigZag64(value));
}

//%PDDM-DEFINE WRITE_PACKABLE_DEFNS(NAME, ARRAY_TYPE, TYPE, ACCESSOR_NAME)
//%- (void)write##NAME##Array:(int32_t)fieldNumber
//%       NAME$S     values:(GPB##ARRAY_TYPE##Array *)values
//%       NAME$S        tag:(uint32_t)tag {
//%  if (tag != 0) {
//%    if (values.count == 0) return;
//%    __block size_t dataSize = 0;
//%    [values enumerate##ACCESSOR_NAME##ValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%#pragma unused(idx, stop)
//%      dataSize += GPBCompute##NAME##SizeNoTag(value);
//%    }];
//%    GPBWriteRawVarint32(&state_, tag);
//%    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
//%    [values enumerate##ACCESSOR_NAME##ValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%#pragma unused(idx, stop)
//%      [self write##NAME##NoTag:value];
//%    }];
//%  } else {
//%    [values enumerate##ACCESSOR_NAME##ValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%#pragma unused(idx, stop)
//%      [self write##NAME:fieldNumber value:value];
//%    }];
//%  }
//%}
//%
//%PDDM-DEFINE WRITE_UNPACKABLE_DEFNS(NAME, TYPE)
//%- (void)write##NAME##Array:(int32_t)fieldNumber values:(NSArray *)values {
//%  for (TYPE *value in values) {
//%    [self write##NAME:fieldNumber value:value];
//%  }
//%}
//%
//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Double, Double, double, )
// This block of code is generated, do not edit it directly.

- (void)writeDoubleArray:(int32_t)fieldNumber
                  values:(GPBDoubleArray *)values
                     tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(double value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeDoubleSizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(double value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeDoubleNoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(double value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeDouble:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Float, Float, float, )
// This block of code is generated, do not edit it directly.

- (void)writeFloatArray:(int32_t)fieldNumber
                 values:(GPBFloatArray *)values
                    tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(float value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeFloatSizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(float value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeFloatNoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(float value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeFloat:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(UInt64, UInt64, uint64_t, )
// This block of code is generated, do not edit it directly.

- (void)writeUInt64Array:(int32_t)fieldNumber
                  values:(GPBUInt64Array *)values
                     tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeUInt64SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeUInt64NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeUInt64:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Int64, Int64, int64_t, )
// This block of code is generated, do not edit it directly.

- (void)writeInt64Array:(int32_t)fieldNumber
                 values:(GPBInt64Array *)values
                    tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeInt64SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeInt64NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeInt64:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Int32, Int32, int32_t, )
// This block of code is generated, do not edit it directly.

- (void)writeInt32Array:(int32_t)fieldNumber
                 values:(GPBInt32Array *)values
                    tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeInt32SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeInt32NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeInt32:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(UInt32, UInt32, uint32_t, )
// This block of code is generated, do not edit it directly.

- (void)writeUInt32Array:(int32_t)fieldNumber
                  values:(GPBUInt32Array *)values
                     tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeUInt32SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeUInt32NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeUInt32:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Fixed64, UInt64, uint64_t, )
// This block of code is generated, do not edit it directly.

- (void)writeFixed64Array:(int32_t)fieldNumber
                   values:(GPBUInt64Array *)values
                      tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeFixed64SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeFixed64NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeFixed64:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Fixed32, UInt32, uint32_t, )
// This block of code is generated, do not edit it directly.

- (void)writeFixed32Array:(int32_t)fieldNumber
                   values:(GPBUInt32Array *)values
                      tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeFixed32SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeFixed32NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeFixed32:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(SInt32, Int32, int32_t, )
// This block of code is generated, do not edit it directly.

- (void)writeSInt32Array:(int32_t)fieldNumber
                  values:(GPBInt32Array *)values
                     tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeSInt32SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSInt32NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSInt32:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(SInt64, Int64, int64_t, )
// This block of code is generated, do not edit it directly.

- (void)writeSInt64Array:(int32_t)fieldNumber
                  values:(GPBInt64Array *)values
                     tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeSInt64SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSInt64NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSInt64:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(SFixed64, Int64, int64_t, )
// This block of code is generated, do not edit it directly.

- (void)writeSFixed64Array:(int32_t)fieldNumber
                    values:(GPBInt64Array *)values
                       tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeSFixed64SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSFixed64NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSFixed64:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(SFixed32, Int32, int32_t, )
// This block of code is generated, do not edit it directly.

- (void)writeSFixed32Array:(int32_t)fieldNumber
                    values:(GPBInt32Array *)values
                       tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeSFixed32SizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSFixed32NoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeSFixed32:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Bool, Bool, BOOL, )
// This block of code is generated, do not edit it directly.

- (void)writeBoolArray:(int32_t)fieldNumber
                values:(GPBBoolArray *)values
                   tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateValuesWithBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeBoolSizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateValuesWithBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeBoolNoTag:value];
    }];
  } else {
    [values enumerateValuesWithBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeBool:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_PACKABLE_DEFNS(Enum, Enum, int32_t, Raw)
// This block of code is generated, do not edit it directly.

- (void)writeEnumArray:(int32_t)fieldNumber
                values:(GPBEnumArray *)values
                   tag:(uint32_t)tag {
  if (tag != 0) {
    if (values.count == 0) return;
    __block size_t dataSize = 0;
    [values enumerateRawValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      dataSize += GPBComputeEnumSizeNoTag(value);
    }];
    GPBWriteRawVarint32(&state_, tag);
    GPBWriteRawVarint32(&state_, (int32_t)dataSize);
    [values enumerateRawValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeEnumNoTag:value];
    }];
  } else {
    [values enumerateRawValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
#pragma unused(idx, stop)
      [self writeEnum:fieldNumber value:value];
    }];
  }
}

//%PDDM-EXPAND WRITE_UNPACKABLE_DEFNS(String, NSString)
// This block of code is generated, do not edit it directly.

- (void)writeStringArray:(int32_t)fieldNumber values:(NSArray *)values {
  for (NSString *value in values) {
    [self writeString:fieldNumber value:value];
  }
}

//%PDDM-EXPAND WRITE_UNPACKABLE_DEFNS(Message, GPBMessage)
// This block of code is generated, do not edit it directly.

- (void)writeMessageArray:(int32_t)fieldNumber values:(NSArray *)values {
  for (GPBMessage *value in values) {
    [self writeMessage:fieldNumber value:value];
  }
}

//%PDDM-EXPAND WRITE_UNPACKABLE_DEFNS(Bytes, NSData)
// This block of code is generated, do not edit it directly.

- (void)writeBytesArray:(int32_t)fieldNumber values:(NSArray *)values {
  for (NSData *value in values) {
    [self writeBytes:fieldNumber value:value];
  }
}

//%PDDM-EXPAND WRITE_UNPACKABLE_DEFNS(Group, GPBMessage)
// This block of code is generated, do not edit it directly.

- (void)writeGroupArray:(int32_t)fieldNumber values:(NSArray *)values {
  for (GPBMessage *value in values) {
    [self writeGroup:fieldNumber value:value];
  }
}

//%PDDM-EXPAND WRITE_UNPACKABLE_DEFNS(UnknownGroup, GPBUnknownFieldSet)
// This block of code is generated, do not edit it directly.

- (void)writeUnknownGroupArray:(int32_t)fieldNumber values:(NSArray *)values {
  for (GPBUnknownFieldSet *value in values) {
    [self writeUnknownGroup:fieldNumber value:value];
  }
}

//%PDDM-EXPAND-END (19 expansions)

- (void)writeMessageSetExtension:(int32_t)fieldNumber
                           value:(GPBMessage *)value {
  GPBWriteTagWithFormat(&state_, GPBWireFormatMessageSetItem,
                        GPBWireFormatStartGroup);
  GPBWriteUInt32(&state_, GPBWireFormatMessageSetTypeId, fieldNumber);
  [self writeMessage:GPBWireFormatMessageSetMessage value:value];
  GPBWriteTagWithFormat(&state_, GPBWireFormatMessageSetItem,
                        GPBWireFormatEndGroup);
}

- (void)writeRawMessageSetExtension:(int32_t)fieldNumber value:(NSData *)value {
  GPBWriteTagWithFormat(&state_, GPBWireFormatMessageSetItem,
                        GPBWireFormatStartGroup);
  GPBWriteUInt32(&state_, GPBWireFormatMessageSetTypeId, fieldNumber);
  [self writeBytes:GPBWireFormatMessageSetMessage value:value];
  GPBWriteTagWithFormat(&state_, GPBWireFormatMessageSetItem,
                        GPBWireFormatEndGroup);
}

- (void)flush {
  if (state_.output != nil) {
    GPBRefreshBuffer(&state_);
  }
}

- (void)writeRawByte:(uint8_t)value {
  GPBWriteRawByte(&state_, value);
}

- (void)writeRawData:(const NSData *)data {
  [self writeRawPtr:[data bytes] offset:0 length:[data length]];
}

- (void)writeRawPtr:(const void *)value
             offset:(size_t)offset
             length:(size_t)length {
  if (value == nil || length == 0) {
    return;
  }

  NSUInteger bufferLength = state_.size;
  NSUInteger bufferBytesLeft = bufferLength - state_.position;
  if (bufferBytesLeft >= length) {
    // We have room in the current buffer.
    memcpy(state_.bytes + state_.position, ((uint8_t *)value) + offset, length);
    state_.position += length;
  } else {
    // Write extends past current buffer.  Fill the rest of this buffer and
    // flush.
    size_t bytesWritten = bufferBytesLeft;
    memcpy(state_.bytes + state_.position, ((uint8_t *)value) + offset,
           bytesWritten);
    offset += bytesWritten;
    length -= bytesWritten;
    state_.position = bufferLength;
    GPBRefreshBuffer(&state_);
    bufferLength = state_.size;

    // Now deal with the rest.
    // Since we have an output stream, this is our buffer
    // and buffer offset == 0
    if (length <= bufferLength) {
      // Fits in new buffer.
      memcpy(state_.bytes, ((uint8_t *)value) + offset, length);
      state_.position = length;
    } else {
      // Write is very big.  Let's do it all at once.
      [state_.output write:((uint8_t *)value) + offset maxLength:length];
    }
  }
}

- (void)writeTag:(uint32_t)fieldNumber format:(GPBWireFormat)format {
  GPBWriteTagWithFormat(&state_, fieldNumber, format);
}

- (void)writeRawVarint32:(int32_t)value {
  GPBWriteRawVarint32(&state_, value);
}

- (void)writeRawVarintSizeTAs32:(size_t)value {
  // Note the truncation.
  GPBWriteRawVarint32(&state_, (int32_t)value);
}

- (void)writeRawVarint64:(int64_t)value {
  GPBWriteRawVarint64(&state_, value);
}

- (void)writeRawLittleEndian32:(int32_t)value {
  GPBWriteRawLittleEndian32(&state_, value);
}

- (void)writeRawLittleEndian64:(int64_t)value {
  GPBWriteRawLittleEndian64(&state_, value);
}

#pragma clang diagnostic pop

@end

size_t GPBComputeDoubleSizeNoTag(Float64 value) {
#pragma unused(value)
  return LITTLE_ENDIAN_64_SIZE;
}

size_t GPBComputeFloatSizeNoTag(Float32 value) {
#pragma unused(value)
  return LITTLE_ENDIAN_32_SIZE;
}

size_t GPBComputeUInt64SizeNoTag(uint64_t value) {
  return GPBComputeRawVarint64Size(value);
}

size_t GPBComputeInt64SizeNoTag(int64_t value) {
  return GPBComputeRawVarint64Size(value);
}

size_t GPBComputeInt32SizeNoTag(int32_t value) {
  if (value >= 0) {
    return GPBComputeRawVarint32Size(value);
  } else {
    // Must sign-extend.
    return 10;
  }
}

size_t GPBComputeSizeTSizeAsInt32NoTag(size_t value) {
  return GPBComputeInt32SizeNoTag((int32_t)value);
}

size_t GPBComputeFixed64SizeNoTag(uint64_t value) {
#pragma unused(value)
  return LITTLE_ENDIAN_64_SIZE;
}

size_t GPBComputeFixed32SizeNoTag(uint32_t value) {
#pragma unused(value)
  return LITTLE_ENDIAN_32_SIZE;
}

size_t GPBComputeBoolSizeNoTag(BOOL value) {
#pragma unused(value)
  return 1;
}

size_t GPBComputeStringSizeNoTag(NSString *value) {
  NSUInteger length = [value lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
  return GPBComputeRawVarint32SizeForInteger(length) + length;
}

size_t GPBComputeGroupSizeNoTag(GPBMessage *value) {
  return [value serializedSize];
}

size_t GPBComputeUnknownGroupSizeNoTag(GPBUnknownFieldSet *value) {
  return value.serializedSize;
}

size_t GPBComputeMessageSizeNoTag(GPBMessage *value) {
  size_t size = [value serializedSize];
  return GPBComputeRawVarint32SizeForInteger(size) + size;
}

size_t GPBComputeBytesSizeNoTag(NSData *value) {
  NSUInteger valueLength = [value length];
  return GPBComputeRawVarint32SizeForInteger(valueLength) + valueLength;
}

size_t GPBComputeUInt32SizeNoTag(int32_t value) {
  return GPBComputeRawVarint32Size(value);
}

size_t GPBComputeEnumSizeNoTag(int32_t value) {
  return GPBComputeRawVarint32Size(value);
}

size_t GPBComputeSFixed32SizeNoTag(int32_t value) {
#pragma unused(value)
  return LITTLE_ENDIAN_32_SIZE;
}

size_t GPBComputeSFixed64SizeNoTag(int64_t value) {
#pragma unused(value)
  return LITTLE_ENDIAN_64_SIZE;
}

size_t GPBComputeSInt32SizeNoTag(int32_t value) {
  return GPBComputeRawVarint32Size(GPBEncodeZigZag32(value));
}

size_t GPBComputeSInt64SizeNoTag(int64_t value) {
  return GPBComputeRawVarint64Size(GPBEncodeZigZag64(value));
}

size_t GPBComputeDoubleSize(int32_t fieldNumber, double value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeDoubleSizeNoTag(value);
}

size_t GPBComputeFloatSize(int32_t fieldNumber, float value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeFloatSizeNoTag(value);
}

size_t GPBComputeUInt64Size(int32_t fieldNumber, uint64_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeUInt64SizeNoTag(value);
}

size_t GPBComputeInt64Size(int32_t fieldNumber, int64_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeInt64SizeNoTag(value);
}

size_t GPBComputeInt32Size(int32_t fieldNumber, int32_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeInt32SizeNoTag(value);
}

size_t GPBComputeFixed64Size(int32_t fieldNumber, uint64_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeFixed64SizeNoTag(value);
}

size_t GPBComputeFixed32Size(int32_t fieldNumber, uint32_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeFixed32SizeNoTag(value);
}

size_t GPBComputeBoolSize(int32_t fieldNumber, BOOL value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeBoolSizeNoTag(value);
}

size_t GPBComputeStringSize(int32_t fieldNumber, NSString *value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeStringSizeNoTag(value);
}

size_t GPBComputeGroupSize(int32_t fieldNumber, GPBMessage *value) {
  return GPBComputeTagSize(fieldNumber) * 2 + GPBComputeGroupSizeNoTag(value);
}

size_t GPBComputeUnknownGroupSize(int32_t fieldNumber,
                                  GPBUnknownFieldSet *value) {
  return GPBComputeTagSize(fieldNumber) * 2 +
         GPBComputeUnknownGroupSizeNoTag(value);
}

size_t GPBComputeMessageSize(int32_t fieldNumber, GPBMessage *value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeMessageSizeNoTag(value);
}

size_t GPBComputeBytesSize(int32_t fieldNumber, NSData *value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeBytesSizeNoTag(value);
}

size_t GPBComputeUInt32Size(int32_t fieldNumber, uint32_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeUInt32SizeNoTag(value);
}

size_t GPBComputeEnumSize(int32_t fieldNumber, int32_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeEnumSizeNoTag(value);
}

size_t GPBComputeSFixed32Size(int32_t fieldNumber, int32_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeSFixed32SizeNoTag(value);
}

size_t GPBComputeSFixed64Size(int32_t fieldNumber, int64_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeSFixed64SizeNoTag(value);
}

size_t GPBComputeSInt32Size(int32_t fieldNumber, int32_t value) {
  return GPBComputeTagSize(fieldNumber) + GPBComputeSInt32SizeNoTag(value);
}

size_t GPBComputeSInt64Size(int32_t fieldNumber, int64_t value) {
  return GPBComputeTagSize(fieldNumber) +
         GPBComputeRawVarint64Size(GPBEncodeZigZag64(value));
}

size_t GPBComputeMessageSetExtensionSize(int32_t fieldNumber,
                                         GPBMessage *value) {
  return GPBComputeTagSize(GPBWireFormatMessageSetItem) * 2 +
         GPBComputeUInt32Size(GPBWireFormatMessageSetTypeId, fieldNumber) +
         GPBComputeMessageSize(GPBWireFormatMessageSetMessage, value);
}

size_t GPBComputeRawMessageSetExtensionSize(int32_t fieldNumber,
                                            NSData *value) {
  return GPBComputeTagSize(GPBWireFormatMessageSetItem) * 2 +
         GPBComputeUInt32Size(GPBWireFormatMessageSetTypeId, fieldNumber) +
         GPBComputeBytesSize(GPBWireFormatMessageSetMessage, value);
}

size_t GPBComputeTagSize(int32_t fieldNumber) {
  return GPBComputeRawVarint32Size(
      GPBWireFormatMakeTag(fieldNumber, GPBWireFormatVarint));
}

size_t GPBComputeWireFormatTagSize(int field_number, GPBDataType dataType) {
  size_t result = GPBComputeTagSize(field_number);
  if (dataType == GPBDataTypeGroup) {
    // Groups have both a start and an end tag.
    return result * 2;
  } else {
    return result;
  }
}

size_t GPBComputeRawVarint32Size(int32_t value) {
  // value is treated as unsigned, so it won't be sign-extended if negative.
  if ((value & (0xffffffff << 7)) == 0) return 1;
  if ((value & (0xffffffff << 14)) == 0) return 2;
  if ((value & (0xffffffff << 21)) == 0) return 3;
  if ((value & (0xffffffff << 28)) == 0) return 4;
  return 5;
}

size_t GPBComputeRawVarint32SizeForInteger(NSInteger value) {
  // Note the truncation.
  return GPBComputeRawVarint32Size((int32_t)value);
}

size_t GPBComputeRawVarint64Size(int64_t value) {
  if ((value & (0xffffffffffffffffL << 7)) == 0) return 1;
  if ((value & (0xffffffffffffffffL << 14)) == 0) return 2;
  if ((value & (0xffffffffffffffffL << 21)) == 0) return 3;
  if ((value & (0xffffffffffffffffL << 28)) == 0) return 4;
  if ((value & (0xffffffffffffffffL << 35)) == 0) return 5;
  if ((value & (0xffffffffffffffffL << 42)) == 0) return 6;
  if ((value & (0xffffffffffffffffL << 49)) == 0) return 7;
  if ((value & (0xffffffffffffffffL << 56)) == 0) return 8;
  if ((value & (0xffffffffffffffffL << 63)) == 0) return 9;
  return 10;
}
