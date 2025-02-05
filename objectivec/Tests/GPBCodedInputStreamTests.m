// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBCodedInputStream.h"
#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream.h"
#import "GPBTestUtilities.h"
#import "GPBUnknownField.h"
#import "GPBUnknownFields.h"
#import "GPBUtilities.h"
#import "GPBUtilities_PackagePrivate.h"
#import "GPBWireFormat.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

@interface CodedInputStreamTests : GPBTestCase
@end

@implementation CodedInputStreamTests

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

- (void)testDecodeZigZag {
  [self assertReadZigZag32:bytes(0x0) value:0];
  [self assertReadZigZag32:bytes(0x1) value:-1];
  [self assertReadZigZag32:bytes(0x2) value:1];
  [self assertReadZigZag32:bytes(0x3) value:-2];

  [self assertReadZigZag32:bytes(0xFE, 0xFF, 0xFF, 0xFF, 0x07) value:(int32_t)0x3FFFFFFF];
  [self assertReadZigZag32:bytes(0xFF, 0xFF, 0xFF, 0xFF, 0x07) value:(int32_t)0xC0000000];
  [self assertReadZigZag32:bytes(0xFE, 0xFF, 0xFF, 0xFF, 0x0F) value:(int32_t)0x7FFFFFFF];
  [self assertReadZigZag32:bytes(0xFF, 0xFF, 0xFF, 0xFF, 0x0F) value:(int32_t)0x80000000];

  [self assertReadZigZag64:bytes(0x0) value:0];
  [self assertReadZigZag64:bytes(0x1) value:-1];
  [self assertReadZigZag64:bytes(0x2) value:1];
  [self assertReadZigZag64:bytes(0x3) value:-2];

  [self assertReadZigZag64:bytes(0xFE, 0xFF, 0xFF, 0xFF, 0x07) value:(int32_t)0x3FFFFFFF];
  [self assertReadZigZag64:bytes(0xFF, 0xFF, 0xFF, 0xFF, 0x07) value:(int32_t)0xC0000000];
  [self assertReadZigZag64:bytes(0xFE, 0xFF, 0xFF, 0xFF, 0x0F) value:(int32_t)0x7FFFFFFF];
  [self assertReadZigZag64:bytes(0xFF, 0xFF, 0xFF, 0xFF, 0x0F) value:(int32_t)0x80000000];

  [self assertReadZigZag64:bytes(0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01)
                     value:0x7FFFFFFFFFFFFFFFL];
  [self assertReadZigZag64:bytes(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01)
                     value:0x8000000000000000L];
}

- (void)assertReadVarint:(NSData*)data value:(int64_t)value {
  if (value <= INT32_MAX && value >= INT32_MIN) {
    {
      GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
      XCTAssertEqual((int32_t)value, [input readInt32]);
    }
    {
      GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
      XCTAssertEqual((int32_t)value, [input readEnum]);
    }
  }
  if (value <= UINT32_MAX && value >= 0) {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual((uint32_t)value, [input readUInt32]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value, [input readInt64]);
  }
  if (value >= 0) {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual((uint64_t)value, [input readUInt64]);
  }
}

- (void)assertReadLittleEndian32:(NSData*)data value:(int32_t)value {
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value, [input readSFixed32]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(GPBConvertInt32ToFloat(value), [input readFloat]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual((uint32_t)value, [input readFixed32]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value, [input readSFixed32]);
  }
}

- (void)assertReadLittleEndian64:(NSData*)data value:(int64_t)value {
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value, [input readSFixed64]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(GPBConvertInt64ToDouble(value), [input readDouble]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual((uint64_t)value, [input readFixed64]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value, [input readSFixed64]);
  }
}

- (void)assertReadZigZag32:(NSData*)data value:(int64_t)value {
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  XCTAssertEqual((int32_t)value, [input readSInt32]);
}

- (void)assertReadZigZag64:(NSData*)data value:(int64_t)value {
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  XCTAssertEqual(value, [input readSInt64]);
}

- (void)assertReadVarintFailure:(NSData*)data {
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertThrows([input readInt32]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    XCTAssertThrows([input readInt64]);
  }
}

- (void)testBytes {
  NSData* data = bytes(0xa2, 0x74);
  XCTAssertEqual(data.length, (NSUInteger)2);
  XCTAssertEqual(((uint8_t*)data.bytes)[0], (uint8_t)0xa2);
  XCTAssertEqual(((uint8_t*)data.bytes)[1], (uint8_t)0x74);
}

- (void)testReadBool {
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:bytes(0x00)];
    XCTAssertEqual(NO, [input readBool]);
  }
  {
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:bytes(0x01)];
    XCTAssertEqual(YES, [input readBool]);
  }
}

- (void)testReadVarint {
  [self assertReadVarint:bytes(0x00) value:0];
  [self assertReadVarint:bytes(0x01) value:1];
  [self assertReadVarint:bytes(0x7f) value:127];
  // 14882
  [self assertReadVarint:bytes(0xa2, 0x74) value:(0x22 << 0) | (0x74 << 7)];
  // 1904930
  [self assertReadVarint:bytes(0xa2, 0xa2, 0x74) value:(0x22 << 0) | (0x22 << 7) | (0x74 << 14)];
  // 243831074
  [self assertReadVarint:bytes(0xa2, 0xa2, 0xa2, 0x74)
                   value:(0x22 << 0) | (0x22 << 7) | (0x22 << 14) | (0x74 << 21)];
  // 2961488830
  [self assertReadVarint:bytes(0xbe, 0xf7, 0x92, 0x84, 0x0b)
                   value:(0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x0bLL << 28)];

  // 64-bit
  // 7256456126
  [self assertReadVarint:bytes(0xbe, 0xf7, 0x92, 0x84, 0x1b)
                   value:(0x3e << 0) | (0x77 << 7) | (0x12 << 14) | (0x04 << 21) | (0x1bLL << 28)];
  // 41256202580718336
  [self assertReadVarint:bytes(0x80, 0xe6, 0xeb, 0x9c, 0xc3, 0xc9, 0xa4, 0x49)
                   value:(0x00 << 0) | (0x66 << 7) | (0x6b << 14) | (0x1c << 21) | (0x43LL << 28) |
                         (0x49LL << 35) | (0x24LL << 42) | (0x49LL << 49)];
  // 11964378330978735131
  [self assertReadVarint:bytes(0x9b, 0xa8, 0xf9, 0xc2, 0xbb, 0xd6, 0x80, 0x85, 0xa6, 0x01)
                   value:(0x1b << 0) | (0x28 << 7) | (0x79 << 14) | (0x42 << 21) | (0x3bLL << 28) |
                         (0x56LL << 35) | (0x00LL << 42) | (0x05LL << 49) | (0x26LL << 56) |
                         (0x01ULL << 63)];

  // Failures
  [self assertReadVarintFailure:bytes(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
                                      0x00)];
  [self assertReadVarintFailure:bytes(0x80)];
}

- (void)testReadVarint32FromVarint64 {
  {
    // Turn on lower 31 bits of the upper half on a 64 bit varint.
    NSData* data = bytes(0x80, 0x80, 0x80, 0x80, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E);

    int32_t value32 = 0x0;
    GPBCodedInputStream* input32 = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value32, [input32 readInt32]);

    int64_t value64 = INT64_MAX & 0xFFFFFFFF00000000;
    GPBCodedInputStream* input64 = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value64, [input64 readInt64]);
  }
  {
    // Turn on lower 31 bits and lower 31 bits on upper half on a 64 bit varint.
    NSData* data = bytes(0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E);

    int32_t value32 = INT32_MAX;
    GPBCodedInputStream* input32 = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value32, [input32 readInt32]);

    int64_t value64 = INT64_MAX & 0xFFFFFFFF7FFFFFFF;
    GPBCodedInputStream* input64 = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value64, [input64 readInt64]);
  }
  {
    // Turn on bits 32 and 64 bit on a 64 bit varint.
    NSData* data = bytes(0x80, 0x80, 0x80, 0x80, 0x88, 0x80, 0x80, 0x80, 0x80, 0x01);

    int32_t value32 = INT32_MIN;
    GPBCodedInputStream* input32 = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value32, [input32 readInt32]);

    int64_t value64 = INT64_MIN | (0x01LL << 31);
    GPBCodedInputStream* input64 = [GPBCodedInputStream streamWithData:data];
    XCTAssertEqual(value64, [input64 readInt64]);
  }
}

- (void)testReadLittleEndian {
  [self assertReadLittleEndian32:bytes(0x78, 0x56, 0x34, 0x12) value:0x12345678];
  [self assertReadLittleEndian32:bytes(0xf0, 0xde, 0xbc, 0x9a) value:0x9abcdef0];

  [self assertReadLittleEndian64:bytes(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12)
                           value:0x123456789abcdef0LL];
  [self assertReadLittleEndian64:bytes(0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a)
                           value:0x9abcdef012345678LL];
}

- (void)testReadWholeMessage {
  TestAllTypes* message = [self allSetRepeatedCount:kGPBDefaultRepeatCount];

  NSData* rawBytes = message.data;
  XCTAssertEqual(message.serializedSize, (size_t)rawBytes.length);

  TestAllTypes* message2 = [TestAllTypes parseFromData:rawBytes extensionRegistry:nil error:NULL];
  [self assertAllFieldsSet:message2 repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testSkipWholeMessage {
  TestAllTypes* message = [self allSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* rawBytes = message.data;

  TestEmptyMessage* empty = [TestEmptyMessage parseFromData:rawBytes error:NULL];
  XCTAssertNotNil(empty);
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] initFromMessage:empty] autorelease];
  NSMutableArray<NSNumber*>* fieldNumbers = [NSMutableArray arrayWithCapacity:ufs.count];
  for (GPBUnknownField* field in ufs) {
    GPBWireFormat wireFormat;
    switch (field.type) {
      case GPBUnknownFieldTypeFixed32:
        wireFormat = GPBWireFormatFixed32;
        break;
      case GPBUnknownFieldTypeFixed64:
        wireFormat = GPBWireFormatFixed64;
        break;
      case GPBUnknownFieldTypeVarint:
        wireFormat = GPBWireFormatVarint;
        break;
      case GPBUnknownFieldTypeLengthDelimited:
        wireFormat = GPBWireFormatLengthDelimited;
        break;
      case GPBUnknownFieldTypeGroup:
        wireFormat = GPBWireFormatStartGroup;
        break;
    }
    uint32_t tag = GPBWireFormatMakeTag(field.number, wireFormat);
    [fieldNumbers addObject:@(tag)];
  }

  // Check the tags compared to what's in the UnknownFields to confirm the stream is
  // skipping as expected (this covers the tags within a group also).
  GPBCodedInputStream* input2 = [GPBCodedInputStream streamWithData:rawBytes];

  NSUInteger idx = 0;
  while (YES) {
    int32_t tag = [input2 readTag];
    if (tag == 0) {
      XCTAssertEqual(idx, fieldNumbers.count);
      break;
    }
    XCTAssertEqual(tag, [fieldNumbers[idx] intValue]);
    [input2 skipField:tag];
    ++idx;
  }
}

- (void)testLimit {
  TestAllTypes* message = [self allSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* rawBytes = message.data;
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:rawBytes];
  XCTAssertEqual([input bytesUntilLimit], rawBytes.length);
  [input pushLimit:8];
  XCTAssertEqual([input bytesUntilLimit], 8u);
  [input popLimit:3];
  XCTAssertEqual([input bytesUntilLimit], 3u);
  [input readTag];
  XCTAssertEqual([input position], 1u);
  XCTAssertEqual([input bytesUntilLimit], 2u);
}

- (void)testReadHugeBlob {
  // Allocate and initialize a 1MB blob.
  NSMutableData* blob = [NSMutableData dataWithLength:1 << 20];
  for (NSUInteger i = 0; i < blob.length; i++) {
    ((uint8_t*)blob.mutableBytes)[i] = (uint8_t)i;
  }

  // Make a message containing it.
  TestAllTypes* message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:kGPBDefaultRepeatCount];
  [message setOptionalBytes:blob];

  // Serialize and parse it.  Make sure to parse from an InputStream, not
  // directly from a ByteString, so that CodedInputStream uses buffered
  // reading.
  NSData* messageData = message.data;
  XCTAssertNotNil(messageData);
  GPBCodedInputStream* stream = [GPBCodedInputStream streamWithData:messageData];
  TestAllTypes* message2 = [TestAllTypes parseFromCodedInputStream:stream
                                                 extensionRegistry:nil
                                                             error:NULL];

  XCTAssertEqualObjects(message.optionalBytes, message2.optionalBytes);

  // Make sure all the other fields were parsed correctly.
  TestAllTypes* message3 = [[message2 copy] autorelease];
  TestAllTypes* types = [self allSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* data = [types optionalBytes];
  [message3 setOptionalBytes:data];

  [self assertAllFieldsSet:message3 repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testReadMaliciouslyLargeBlob {
  NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];

  int32_t tag = GPBWireFormatMakeTag(1, GPBWireFormatLengthDelimited);
  [output writeRawVarint32:tag];
  [output writeRawVarint32:0x7FFFFFFF];
  uint8_t bytes[32] = {0};
  [output writeRawData:[NSData dataWithBytes:bytes length:32]];
  [output flush];

  NSData* data = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  GPBCodedInputStream* input =
      [GPBCodedInputStream streamWithData:[NSMutableData dataWithData:data]];
  XCTAssertEqual(tag, [input readTag]);

  XCTAssertThrows([input readBytes]);
}

- (void)testReadEmptyString {
  NSData* data = bytes(0x00);
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  XCTAssertEqualObjects(@"", [input readString]);
}

- (void)testInvalidGroupEndTagThrows {
  NSData* data = bytes(0x0B, 0x1A, 0x02, 0x4B, 0x50, 0x14);
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  XCTAssertThrowsSpecificNamed([input skipMessage], NSException, GPBCodedInputStreamException,
                               @"should throw a GPBCodedInputStreamException exception ");
}

- (void)testBytesOver2GB {
  NSData* data = bytes(0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x01, 0x02, 0x03);  // don't need all the bytes
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  @try {
    __unused NSData* result = [input readBytes];
    XCTFail(@"Should have thrown");
  } @catch (NSException* anException) {
    // Ensure the correct error within the exception.
    XCTAssertTrue([anException isKindOfClass:[NSException class]]);
    XCTAssertEqualObjects(anException.name, GPBCodedInputStreamException);
    NSDictionary* userInfo = anException.userInfo;
    XCTAssertNotNil(userInfo);
    NSError* err = userInfo[GPBCodedInputStreamUnderlyingErrorKey];
    XCTAssertNotNil(err);
    XCTAssertEqualObjects(err.domain, GPBCodedInputStreamErrorDomain);
    XCTAssertEqual(err.code, GPBCodedInputStreamErrorInvalidSize);
  }
}

// Verifies fix for b/10315336.
// Note: Now that there isn't a custom string class under the hood, this test
// isn't as critical, but it does cover bad input and if a custom class is added
// again, it will help validate that class' handing of bad utf8.
- (void)testReadMalformedString {
  NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];

  int32_t tag =
      GPBWireFormatMakeTag(TestAllTypes_FieldNumber_DefaultString, GPBWireFormatLengthDelimited);
  [output writeRawVarint32:tag];
  [output writeRawVarint32:5];
  // Create an invalid utf-8 byte array.
  uint8_t bytes[] = {0xc2, 0xf2, 0x0, 0x0, 0x0};
  [output writeRawData:[NSData dataWithBytes:bytes length:sizeof(bytes)]];
  [output flush];

  NSData* data = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  NSError* error = nil;
  TestAllTypes* message = [TestAllTypes parseFromCodedInputStream:input
                                                extensionRegistry:nil
                                                            error:&error];
  XCTAssertNotNil(error);
  XCTAssertNil(message);
}

- (void)testBOMWithinStrings {
  // We've seen servers that end up with BOMs within strings (not always at the
  // start, and sometimes in multiple places), make sure they always parse
  // correctly. (Again, this is inpart in case a custom string class is ever
  // used again.)
  const char* strs[] = {
      "\xEF\xBB\xBF String with BOM",
      "String with \xEF\xBB\xBF in middle",
      "String with end bom \xEF\xBB\xBF",
      "\xEF\xBB\xBF\xe2\x99\xa1",  // BOM White Heart
      "\xEF\xBB\xBF\xEF\xBB\xBF String with Two BOM",
  };
  for (size_t i = 0; i < GPBARRAYSIZE(strs); ++i) {
    NSOutputStream* rawOutput = [NSOutputStream outputStreamToMemory];
    GPBCodedOutputStream* output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];

    int32_t tag =
        GPBWireFormatMakeTag(TestAllTypes_FieldNumber_DefaultString, GPBWireFormatLengthDelimited);
    [output writeRawVarint32:tag];
    size_t length = strlen(strs[i]);
    [output writeRawVarint32:(int32_t)length];
    [output writeRawData:[NSData dataWithBytes:strs[i] length:length]];
    [output flush];

    NSData* data = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
    GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
    TestAllTypes* message = [TestAllTypes parseFromCodedInputStream:input
                                                  extensionRegistry:nil
                                                              error:NULL];
    XCTAssertNotNil(message, @"Loop %zd", i);
    // Ensure the string is there. NSString can consume the BOM in some
    // cases, so don't actually check the string for exact equality.
    XCTAssertTrue(message.defaultString.length > 0, @"Loop %zd", i);
  }
}

- (void)assertReadByteToEndGroupFails:(NSData*)data {
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  uint32_t tag = [input readTag];
  XCTAssertThrows(GPBCodedInputStreamReadRetainedBytesToEndGroupNoCopy(
      &input->state_, GPBWireFormatGetTagFieldNumber(tag)));
}

- (void)assertReadByteToEndGroup:(NSData*)data value:(NSData*)value {
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  uint32_t tag = [input readTag];
  NSData* readValue = GPBCodedInputStreamReadRetainedBytesToEndGroupNoCopy(
      &input->state_, GPBWireFormatGetTagFieldNumber(tag));
  XCTAssertNotNil(readValue);
  XCTAssertEqualObjects(readValue, value);
  [readValue release];
}

static NSData* DataForGroupsOfDepth(NSUInteger depth) {
  NSMutableData* data = [NSMutableData dataWithCapacity:0];

  uint32_t byte = 35;  // 35 = 0b100011 -> field 4/start group
  for (NSUInteger i = 0; i < depth; ++i) {
    [data appendBytes:&byte length:1];
  }

  byte = 8;  // 8 = 0b1000, -> field 1/varint
  [data appendBytes:&byte length:1];
  byte = 1;  // 1 -> varint value of 1
  [data appendBytes:&byte length:1];

  byte = 36;  // 36 = 0b100100 -> field 4/end group
  for (NSUInteger i = 0; i < depth; ++i) {
    [data appendBytes:&byte length:1];
  }
  return data;
}

- (void)testBytesToEndGroup {
  // 35 = 0b100011 -> field 4/start group
  // 36 = 0b100100 -> field 4/end group
  // 43 = 0b101011 -> field 5/end group
  // 44 = 0b101100 -> field 5/end group
  // 8 = 0b1000, 1 -> field 1/varint, value of 1
  // 21 = 0b10101, 0x78, 0x56, 0x34, 0x12 -> field 2/fixed32, value of 0x12345678
  // 25 = 0b11001, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12 -> field 3/fixed64,
  //                                                                 value of 0x123456789abcdef0LL
  // 50 = 0b110010, 0x0 -> field 6/length delimited, length 0
  // 50 = 0b110010, 0x1, 42 -> field 6/length delimited, length 1, byte 42
  // 0 -> field 0 which is invalid/varint
  // 15 = 0b1111 -> field 1, wire type 7 which is invalid

  [self assertReadByteToEndGroup:bytes(35, 36) value:bytes(36)];              // empty group
  [self assertReadByteToEndGroup:bytes(35, 8, 1, 36) value:bytes(8, 1, 36)];  // varint
  [self assertReadByteToEndGroup:bytes(35, 21, 0x78, 0x56, 0x34, 0x12, 36)    // fixed32
                           value:bytes(21, 0x78, 0x56, 0x34, 0x12, 36)];
  [self assertReadByteToEndGroup:bytes(35, 25, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                                       36)  // fixed64
                           value:bytes(25, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 36)];
  [self assertReadByteToEndGroup:bytes(35, 50, 0, 36)
                           value:bytes(50, 0, 36)];  // length delimited, length 0
  [self assertReadByteToEndGroup:bytes(35, 50, 1, 42, 36)
                           value:bytes(50, 1, 42, 36)];  // length delimited, length 1, byte 42
  [self assertReadByteToEndGroup:bytes(35, 43, 44, 36) value:bytes(43, 44, 36)];  // Sub group
  [self assertReadByteToEndGroup:bytes(35, 8, 1, 43, 8, 1, 44,
                                       36)  // varint and sub group with varint
                           value:bytes(8, 1, 43, 8, 1, 44, 36)];

  [self assertReadByteToEndGroupFails:bytes(35, 0, 36)];                 // Invalid field number
  [self assertReadByteToEndGroupFails:bytes(35, 15, 36)];                // Invalid wire type
  [self assertReadByteToEndGroupFails:bytes(35, 21, 0x78, 0x56, 0x34)];  // truncated fixed32
  [self assertReadByteToEndGroupFails:bytes(35, 25, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56,
                                            0x34)];  // truncated fixed64

  // Missing end group
  [self assertReadByteToEndGroupFails:bytes(35)];
  [self assertReadByteToEndGroupFails:bytes(35, 8, 1)];
  [self assertReadByteToEndGroupFails:bytes(35, 43)];
  [self assertReadByteToEndGroupFails:bytes(35, 43, 8, 1)];

  // Wrong end group
  [self assertReadByteToEndGroupFails:bytes(35, 44)];
  [self assertReadByteToEndGroupFails:bytes(35, 8, 1, 44)];
  [self assertReadByteToEndGroupFails:bytes(35, 43, 36)];
  [self assertReadByteToEndGroupFails:bytes(35, 43, 8, 1, 36)];
  [self assertReadByteToEndGroupFails:bytes(35, 43, 44, 44)];
  [self assertReadByteToEndGroupFails:bytes(35, 43, 8, 1, 44, 44)];

  // This is the same limit as within GPBCodedInputStream.
  const NSUInteger kDefaultRecursionLimit = 100;
  // That depth parses.
  NSData* testData = DataForGroupsOfDepth(kDefaultRecursionLimit);
  [self assertReadByteToEndGroup:testData
                           value:[testData subdataWithRange:NSMakeRange(1, testData.length - 1)]];
  // One more level deep fails.
  testData = DataForGroupsOfDepth(kDefaultRecursionLimit + 1);
  [self assertReadByteToEndGroupFails:testData];
}

@end
