// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

#import "GPBCodedInputStream.h"
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

@interface GPBCodedOutputStream (InternalMethods)
// Declared in the .m file, expose for testing.
- (instancetype)initWithOutputStream:(NSOutputStream*)output data:(NSMutableData*)data;
@end

@interface GPBCodedOutputStream (Helper)
+ (instancetype)streamWithOutputStream:(NSOutputStream*)output bufferSize:(size_t)bufferSize;
@end

@implementation GPBCodedOutputStream (Helper)
+ (instancetype)streamWithOutputStream:(NSOutputStream*)output bufferSize:(size_t)bufferSize {
  NSMutableData* data = [NSMutableData dataWithLength:bufferSize];
  return [[[self alloc] initWithOutputStream:output data:data] autorelease];
}
@end

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
  GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];
  [output writeRawLittleEndian32:(int32_t)value];
  XCTAssertEqual(output.bytesWritten, data.length);
  [output flush];
  XCTAssertEqual(output.bytesWritten, data.length);

  NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  XCTAssertEqualObjects(data, actual);

  // Try different block sizes.
  for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
    rawOutput = [NSOutputStream outputStreamToMemory];
    output = [GPBCodedOutputStream streamWithOutputStream:rawOutput bufferSize:blockSize];
    [output writeRawLittleEndian32:(int32_t)value];
    XCTAssertEqual(output.bytesWritten, data.length);
    [output flush];
    XCTAssertEqual(output.bytesWritten, data.length);

    actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);
  }
}

- (void)assertWriteLittleEndian64:(NSData*)data value:(int64_t)value {
  NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];
  [output writeRawLittleEndian64:value];
  XCTAssertEqual(output.bytesWritten, data.length);
  [output flush];
  XCTAssertEqual(output.bytesWritten, data.length);

  NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  XCTAssertEqualObjects(data, actual);

  // Try different block sizes.
  for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
    rawOutput = [NSOutputStream outputStreamToMemory];
    output = [GPBCodedOutputStream streamWithOutputStream:rawOutput bufferSize:blockSize];
    [output writeRawLittleEndian64:value];
    XCTAssertEqual(output.bytesWritten, data.length);
    [output flush];
    XCTAssertEqual(output.bytesWritten, data.length);

    actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);
  }
}

- (void)assertWriteVarint:(NSData*)data value:(int64_t)value {
  // Only do 32-bit write if the value fits in 32 bits.
  if (GPBLogicalRightShift64(value, 32) == 0) {
    NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
    GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];
    [output writeRawVarint32:(int32_t)value];
    XCTAssertEqual(output.bytesWritten, data.length);
    [output flush];
    XCTAssertEqual(output.bytesWritten, data.length);

    NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);

    // Also try computing size.
    XCTAssertEqual(GPBComputeRawVarint32Size((int32_t)value), (size_t)data.length);
  }

  {
    NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
    GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];
    [output writeRawVarint64:value];
    XCTAssertEqual(output.bytesWritten, data.length);
    [output flush];
    XCTAssertEqual(output.bytesWritten, data.length);

    NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual);

    // Also try computing size.
    XCTAssertEqual(GPBComputeRawVarint64Size(value), (size_t)data.length);
  }

  // Try different block sizes.
  for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
    // Only do 32-bit write if the value fits in 32 bits.
    if (GPBLogicalRightShift64(value, 32) == 0) {
      NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
      GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                                                       bufferSize:blockSize];

      [output writeRawVarint32:(int32_t)value];
      XCTAssertEqual(output.bytesWritten, data.length);
      [output flush];
      XCTAssertEqual(output.bytesWritten, data.length);

      NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
      XCTAssertEqualObjects(data, actual);
    }

    {
      NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
      GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                                                       bufferSize:blockSize];

      [output writeRawVarint64:value];
      XCTAssertEqual(output.bytesWritten, data.length);
      [output flush];
      XCTAssertEqual(output.bytesWritten, data.length);

      NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
      XCTAssertEqualObjects(data, actual);
    }
  }
}

- (void)assertWriteStringNoTag:(NSData*)data
                         value:(NSString*)value
                       context:(NSString*)contextMessage {
  NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];
  [output writeStringNoTag:value];
  XCTAssertEqual(output.bytesWritten, data.length);
  [output flush];
  XCTAssertEqual(output.bytesWritten, data.length);

  NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  XCTAssertEqualObjects(data, actual, @"%@", contextMessage);

  // Try different block sizes.
  for (int blockSize = 1; blockSize <= 16; blockSize *= 2) {
    rawOutput = [NSOutputStream outputStreamToMemory];
    output = [GPBCodedOutputStream streamWithOutputStream:rawOutput bufferSize:blockSize];
    [output writeStringNoTag:value];
    XCTAssertEqual(output.bytesWritten, data.length);
    [output flush];
    XCTAssertEqual(output.bytesWritten, data.length);

    actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(data, actual, @"%@", contextMessage);
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
  // The sign/nosign distinction is done here because normally varints are
  // around 64bit values, but for some cases a 32bit value is forced with
  // with the sign bit (tags, uint32, etc.)

  // 1887747006 (no sign bit)
  [self assertWriteVarint:bytes(0xbe, 0xf7, 0x92, 0x84, 0x07)
                    value:(0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x07LL << 28)];
  // 2961488830 (sign bit)
  [self assertWriteVarint:bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b)
                    value:(0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x0bLL << 28)];
}

- (void)testWriteVarint6 {
  // 64-bit
  // 7256456126
  [self assertWriteVarint:bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b)
                    value:(0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x1bLL << 28)];
}

- (void)testWriteVarint7 {
  // 41256202580718336
  [self assertWriteVarint:bytes(0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49)
                    value:(0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) | (0x43LL << 28) |
                          (0x49LL << 35) | (0x24LL << 42) | (0x49LL << 49)];
}

- (void)testWriteVarint8 {
  // 11964378330978735131
  [self assertWriteVarint:bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01)
                    value:(0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) | (0x3bLL << 28) |
                          (0x56LL << 35) | (0x00LL << 42) | (0x05LL << 49) | (0x26LL << 56) |
                          (0x01ULL << 63)];
}

- (void)testWriteLittleEndian {
  [self assertWriteLittleEndian32:bytes(0x78, 0x56, 0x34, 0x12) value:0x12345678];
  [self assertWriteLittleEndian32:bytes(0xf0, 0xde, 0xbc, 0x9a) value:0x9abcdef0];

  [self assertWriteLittleEndian64:bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12)
                            value:0x123456789abcdef0LL];
  [self assertWriteLittleEndian64:bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a)
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
  XCTAssertEqual(0x000000007FFFFFFEULL, GPBEncodeZigZag64(0x000000003FFFFFFFLL));
  XCTAssertEqual(0x000000007FFFFFFFULL, GPBEncodeZigZag64(0xFFFFFFFFC0000000LL));
  XCTAssertEqual(0x00000000FFFFFFFEULL, GPBEncodeZigZag64(0x000000007FFFFFFFLL));
  XCTAssertEqual(0x00000000FFFFFFFFULL, GPBEncodeZigZag64(0xFFFFFFFF80000000LL));
  XCTAssertEqual(0xFFFFFFFFFFFFFFFEULL, GPBEncodeZigZag64(0x7FFFFFFFFFFFFFFFLL));
  XCTAssertEqual(0xFFFFFFFFFFFFFFFFULL, GPBEncodeZigZag64(0x8000000000000000LL));

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

  XCTAssertEqual(856912304801416ULL, GPBEncodeZigZag64(GPBDecodeZigZag64(856912304801416LL)));
  XCTAssertEqual(-75123905439571256ULL, GPBEncodeZigZag64(GPBDecodeZigZag64(-75123905439571256LL)));
}

- (void)testWriteWholeMessage {
  // Not kGPBDefaultRepeatCount because we are comparing to a golden master file
  // that was generated with 2.
  TestAllTypes* message = [self allSetRepeatedCount:2];

  NSData* rawBytes = message.data;
  NSData* goldenData = [self getDataFileNamed:@"golden_message" dataToWrite:rawBytes];
  XCTAssertEqualObjects(rawBytes, goldenData);

  // Try different block sizes.
  for (int blockSize = 1; blockSize < 256; blockSize *= 2) {
    NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
    GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput
                                                                     bufferSize:blockSize];
    [message writeToCodedOutputStream:output];
    [output flush];

    NSData* actual = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    XCTAssertEqualObjects(rawBytes, actual);
  }

  // Not kGPBDefaultRepeatCount because we are comparing to a golden master file
  // that was generated with 2.
  TestAllExtensions* extensions = [self allExtensionsSetRepeatedCount:2];
  rawBytes = extensions.data;
  goldenData = [self getDataFileNamed:@"golden_packed_fields_message" dataToWrite:rawBytes];
  XCTAssertEqualObjects(rawBytes, goldenData);
}

- (void)testCFStringGetCStringPtrAndStringsWithNullChars {
  // This test exists to verify that CFStrings with embedded NULLs still expose
  // their raw buffer if they are backed by UTF8 storage. If this fails, the
  // quick/direct access paths in GPBCodedOutputStream that depend on
  // CFStringGetCStringPtr need to be re-evalutated (maybe just removed).
  // And yes, we do get NULLs in strings from some servers.

  char zeroTest[] = "\0Test\0String";
  // Note: there is a \0 at the end of this since it is a c-string.
  NSString* asNSString = [[NSString alloc] initWithBytes:zeroTest
                                                  length:sizeof(zeroTest)
                                                encoding:NSUTF8StringEncoding];
  const char* cString = CFStringGetCStringPtr((CFStringRef)asNSString, kCFStringEncodingUTF8);
  XCTAssertTrue(cString != NULL);
  // Again, if the above assert fails, then it means NSString no longer exposes
  // the raw utf8 storage of a string created from utf8 input, so the code using
  // CFStringGetCStringPtr in GPBCodedOutputStream will still work (it will take
  // a different code path); but the optimizations for when
  // CFStringGetCStringPtr does work could possibly go away.

  XCTAssertEqual(sizeof(zeroTest), [asNSString lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
  XCTAssertTrue(0 == memcmp(cString, zeroTest, sizeof(zeroTest)));
  [asNSString release];
}

- (void)testWriteStringsWithZeroChar {
  // Unicode allows `\0` as a character, and NSString is a class cluster, so
  // there are a few different classes that could end up behind a given string.
  // Historically, we've seen differences based on constant strings in code and
  // strings built via the NSString apis. So this round trips them to ensure
  // they are acting as expected.

  NSArray<NSString*>* strs = @[
    @"\0at start",
    @"in\0middle",
    @"at end\0",
  ];
  int i = 0;
  for (NSString* str in strs) {
    NSData* asUTF8 = [str dataUsingEncoding:NSUTF8StringEncoding];
    NSMutableData* expected = [NSMutableData data];
    uint8_t lengthByte = (uint8_t)asUTF8.length;
    [expected appendBytes:&lengthByte length:1];
    [expected appendData:asUTF8];

    NSString* context = [NSString stringWithFormat:@"Loop %d - Literal", i];
    [self assertWriteStringNoTag:expected value:str context:context];

    // Force a new string to be built which gets a different class from the
    // NSString class cluster than the literal did.
    NSString* str2 = [NSString stringWithFormat:@"%@", str];
    context = [NSString stringWithFormat:@"Loop %d - Built", i];
    [self assertWriteStringNoTag:expected value:str2 context:context];

    ++i;
  }
}

- (void)testThatItThrowsWhenWriteRawPtrFails {
  NSOutputStream* output = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream* codedOutput =
      [GPBCodedOutputStream streamWithOutputStream:output bufferSize:0];  // Skip buffering.
  [output close];  // Close the output stream to force failure on write.
  const char* cString = "raw";
  XCTAssertThrowsSpecificNamed([codedOutput writeRawPtr:cString offset:0 length:strlen(cString)],
                               NSException, GPBCodedOutputStreamException_WriteFailed);
}

@end
