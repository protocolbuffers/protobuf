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

#import "GPBTestUtilities.h"

#import "GPBCodedOutputStream.h"
#import "GPBCodedInputStream.h"
#import "GPBUtilities_PackagePrivate.h"
#import "google/protobuf/Unittest.pbobjc.h"

@interface CodedOutputStreamTests : GPBTestCase
@end

@implementation CodedOutputStreamTests

- (NSData*)bytes_with_sentinel:(int32_t)unused, ... {
  va_list list;
  va_start(list, unused);

  NSMutableData* values = [NSMutableData dataWithCapacity:0];
  int32_t i;

  while ((i = va_arg(list, int32_t)) != 256) {
    NSAssert(i >= 0 && i < 256, @"");
    uint8_t u = (uint8_t)i;
    [values appendBytes:&u length:1];
  }

  va_end(list);

  return values;
}

#define bytes(...) [self bytes_with_sentinel:0, __VA_ARGS__, 256]

- (void)assertWriteLittleEndian32:(NSData*)data value:(int32_t)value {
  NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream* output =
      [GPBCodedOutputStream streamWithOutputStream:rawOutput];
  [output writeRawLittleEndian32:(int32_t)value];
  [output flush];

  NSData* actual =
      [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  XCTAssertEqualObjects(data, actual);

  // Try different block sizes.
  for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
    rawOutput = [NSOutputStream outputStreamToMemory];
    output = [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                               bufferSize:blockSize];
    [output writeRawLittleEndian32:(int32_t)value];
    [output flush];

    actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);
  }
}

- (void)assertWriteLittleEndian64:(NSData*)data value:(int64_t)value {
  NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream* output =
      [GPBCodedOutputStream streamWithOutputStream:rawOutput];
  [output writeRawLittleEndian64:value];
  [output flush];

  NSData* actual =
      [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  XCTAssertEqualObjects(data, actual);

  // Try different block sizes.
  for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
    rawOutput = [NSOutputStream outputStreamToMemory];
    output = [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                               bufferSize:blockSize];
    [output writeRawLittleEndian64:value];
    [output flush];

    actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);
  }
}

- (void)assertWriteVarint:(NSData*)data value:(int64_t)value {
  // Only do 32-bit write if the value fits in 32 bits.
  if (GPBLogicalRightShift64(value, 32) == 0) {
    NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
    GPBCodedOutputStream* output =
        [GPBCodedOutputStream streamWithOutputStream:rawOutput];
    [output writeRawVarint32:(int32_t)value];
    [output flush];

    NSData* actual =
        [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);

    // Also try computing size.
    XCTAssertEqual(GPBComputeRawVarint32Size((int32_t)value),
                   (size_t)data.length);
  }

  {
    NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
    GPBCodedOutputStream* output =
        [GPBCodedOutputStream streamWithOutputStream:rawOutput];
    [output writeRawVarint64:value];
    [output flush];

    NSData* actual =
        [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);

    // Also try computing size.
    XCTAssertEqual(GPBComputeRawVarint64Size(value), (size_t)data.length);
  }

  // Try different block sizes.
  for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
    // Only do 32-bit write if the value fits in 32 bits.
    if (GPBLogicalRightShift64(value, 32) == 0) {
      NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
      GPBCodedOutputStream* output =
          [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                            bufferSize:blockSize];

      [output writeRawVarint32:(int32_t)value];
      [output flush];

      NSData* actual =
          [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
      XCTAssertEqualObjects(data, actual);
    }

    {
      NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
      GPBCodedOutputStream* output =
          [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                            bufferSize:blockSize];

      [output writeRawVarint64:value];
      [output flush];

      NSData* actual =
          [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
      XCTAssertEqualObjects(data, actual);
    }
  }
}

- (void)testWriteVarint1 {
  [self assertWriteVarint:bytes(0x00) value:0];
}

- (void)testWriteVarint2 {
  [self assertWriteVarint:bytes(0x01) value:1];
}

- (void)testWriteVarint3 {
  [self assertWriteVarint:bytes(0x7f) value:127];
}

- (void)testWriteVarint4 {
  // 14882
  [self assertWriteVarint:bytes(0xa2, 0x74) value:(0x22 << 0) | (0x74 << 7)];
}

- (void)testWriteVarint5 {
  // 2961488830
  [self assertWriteVarint:bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b)
                    value:(0x3e << 0) | (0x77 << 7) | (0x12 << 14) |
                          (0x04 << 21) | (0x0bLL << 28)];
}

- (void)testWriteVarint6 {
  // 64-bit
  // 7256456126
  [self assertWriteVarint:bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b)
                    value:(0x3e << 0) | (0x77 << 7) | (0x12 << 14) |
                          (0x04 << 21) | (0x1bLL << 28)];
}

- (void)testWriteVarint7 {
  // 41256202580718336
  [self assertWriteVarint:bytes(0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49)
                    value:(0x00 << 0) | (0x66 << 7) | (0x6b << 14) |
                          (0x1c << 21) | (0x43LL << 28) | (0x49LL << 35) |
                          (0x24LL << 42) | (0x49LL << 49)];
}

- (void)testWriteVarint8 {
  // 11964378330978735131
  [self assertWriteVarint:bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85,
                                0xa6, 0x01)
                    value:(0x1b << 0) | (0x28 << 7) | (0x79 << 14) |
                          (0x42 << 21) | (0x3bLL << 28) | (0x56LL << 35) |
                          (0x00LL << 42) | (0x05LL << 49) | (0x26LL << 56) |
                          (0x01LL << 63)];
}

- (void)testWriteLittleEndian {
  [self assertWriteLittleEndian32:bytes(0x78, 0x56, 0x34, 0x12)
                            value:0x12345678];
  [self assertWriteLittleEndian32:bytes(0xf0, 0xde, 0xbc, 0x9a)
                            value:0x9abcdef0];

  [self assertWriteLittleEndian64:bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56,
                                        0x34, 0x12)
                            value:0x123456789abcdef0LL];
  [self assertWriteLittleEndian64:bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde,
                                        0xbc, 0x9a)
                            value:0x9abcdef012345678LL];
}

- (void)testEncodeZigZag {
  XCTAssertEqual(0U, GPBEncodeZigZag32(0));
  XCTAssertEqual(1U, GPBEncodeZigZag32(-1));
  XCTAssertEqual(2U, GPBEncodeZigZag32(1));
  XCTAssertEqual(3U, GPBEncodeZigZag32(-2));
  XCTAssertEqual(0x7FFFFFFEU, GPBEncodeZigZag32(0x3FFFFFFF));
  XCTAssertEqual(0x7FFFFFFFU, GPBEncodeZigZag32(0xC0000000));
  XCTAssertEqual(0xFFFFFFFEU, GPBEncodeZigZag32(0x7FFFFFFF));
  XCTAssertEqual(0xFFFFFFFFU, GPBEncodeZigZag32(0x80000000));

  XCTAssertEqual(0ULL, GPBEncodeZigZag64(0));
  XCTAssertEqual(1ULL, GPBEncodeZigZag64(-1));
  XCTAssertEqual(2ULL, GPBEncodeZigZag64(1));
  XCTAssertEqual(3ULL, GPBEncodeZigZag64(-2));
  XCTAssertEqual(0x000000007FFFFFFEULL,
                 GPBEncodeZigZag64(0x000000003FFFFFFFLL));
  XCTAssertEqual(0x000000007FFFFFFFULL,
                 GPBEncodeZigZag64(0xFFFFFFFFC0000000LL));
  XCTAssertEqual(0x00000000FFFFFFFEULL,
                 GPBEncodeZigZag64(0x000000007FFFFFFFLL));
  XCTAssertEqual(0x00000000FFFFFFFFULL,
                 GPBEncodeZigZag64(0xFFFFFFFF80000000LL));
  XCTAssertEqual(0xFFFFFFFFFFFFFFFEULL,
                 GPBEncodeZigZag64(0x7FFFFFFFFFFFFFFFLL));
  XCTAssertEqual(0xFFFFFFFFFFFFFFFFULL,
                 GPBEncodeZigZag64(0x8000000000000000LL));

  // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
  // were chosen semi-randomly via keyboard bashing.
  XCTAssertEqual(0U, GPBEncodeZigZag32(GPBDecodeZigZag32(0)));
  XCTAssertEqual(1U, GPBEncodeZigZag32(GPBDecodeZigZag32(1)));
  XCTAssertEqual(-1U, GPBEncodeZigZag32(GPBDecodeZigZag32(-1)));
  XCTAssertEqual(14927U, GPBEncodeZigZag32(GPBDecodeZigZag32(14927)));
  XCTAssertEqual(-3612U, GPBEncodeZigZag32(GPBDecodeZigZag32(-3612)));

  XCTAssertEqual(0ULL, GPBEncodeZigZag64(GPBDecodeZigZag64(0)));
  XCTAssertEqual(1ULL, GPBEncodeZigZag64(GPBDecodeZigZag64(1)));
  XCTAssertEqual(-1ULL, GPBEncodeZigZag64(GPBDecodeZigZag64(-1)));
  XCTAssertEqual(14927ULL, GPBEncodeZigZag64(GPBDecodeZigZag64(14927)));
  XCTAssertEqual(-3612ULL, GPBEncodeZigZag64(GPBDecodeZigZag64(-3612)));

  XCTAssertEqual(856912304801416ULL,
                 GPBEncodeZigZag64(GPBDecodeZigZag64(856912304801416LL)));
  XCTAssertEqual(-75123905439571256ULL,
                 GPBEncodeZigZag64(GPBDecodeZigZag64(-75123905439571256LL)));
}

- (void)testWriteWholeMessage {
  // Not kGPBDefaultRepeatCount because we are comparing to a golden master file
  // that was generated with 2.
  TestAllTypes* message = [self allSetRepeatedCount:2];

  NSData* rawBytes = message.data;
  NSData* goldenData =
      [self getDataFileNamed:@"golden_message" dataToWrite:rawBytes];
  XCTAssertEqualObjects(rawBytes, goldenData);

  // Try different block sizes.
  for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
    NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
    GPBCodedOutputStream* output =
        [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                          bufferSize:blockSize];
    [message writeToCodedOutputStream:output];
    [output flush];

    NSData* actual =
        [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(rawBytes, actual);
  }

  // Not kGPBDefaultRepeatCount because we are comparing to a golden master file
  // that was generated with 2.
  TestAllExtensions* extensions = [self allExtensionsSetRepeatedCount:2];
  rawBytes = extensions.data;
  goldenData = [self getDataFileNamed:@"golden_packed_fields_message"
                          dataToWrite:rawBytes];
  XCTAssertEqualObjects(rawBytes, goldenData);
}

@end
