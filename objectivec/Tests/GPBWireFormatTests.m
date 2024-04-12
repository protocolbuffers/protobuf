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

#import "GPBCodedInputStream.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUnknownField_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestMset.pbobjc.h"

@interface WireFormatTests : GPBTestCase
@end

@implementation WireFormatTests

- (void)testSerialization {
  TestAllTypes* message = [self allSetRepeatedCount:kGPBDefaultRepeatCount];

  NSData* rawBytes = message.data;
  [self assertFieldsInOrder:rawBytes];
  XCTAssertEqual(message.serializedSize, (size_t)rawBytes.length);

  TestAllTypes* message2 = [TestAllTypes parseFromData:rawBytes error:NULL];

  [self assertAllFieldsSet:message2 repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testSerializationPacked {
  TestPackedTypes* message = [self packedSetRepeatedCount:kGPBDefaultRepeatCount];

  NSData* rawBytes = message.data;
  [self assertFieldsInOrder:rawBytes];
  XCTAssertEqual(message.serializedSize, (size_t)rawBytes.length);

  TestPackedTypes* message2 = [TestPackedTypes parseFromData:rawBytes error:NULL];

  [self assertPackedFieldsSet:message2 repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testSerializeExtensions {
  // TestAllTypes and TestAllExtensions should have compatible wire formats,
  // so if we serealize a TestAllExtensions then parse it as TestAllTypes
  // it should work.

  TestAllExtensions* message = [self allExtensionsSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* rawBytes = message.data;
  [self assertFieldsInOrder:rawBytes];
  XCTAssertEqual(message.serializedSize, (size_t)rawBytes.length);

  TestAllTypes* message2 = [TestAllTypes parseFromData:rawBytes error:NULL];

  [self assertAllFieldsSet:message2 repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testSerializePackedExtensions {
  // TestPackedTypes and TestPackedExtensions should have compatible wire
  // formats; check that they serialize to the same string.
  TestPackedExtensions* message = [self packedExtensionsSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* rawBytes = message.data;
  [self assertFieldsInOrder:rawBytes];

  TestPackedTypes* message2 = [self packedSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* rawBytes2 = message2.data;

  XCTAssertEqualObjects(rawBytes, rawBytes2);
}

- (void)testParseExtensions {
  // TestAllTypes and TestAllExtensions should have compatible wire formats,
  // so if we serialize a TestAllTypes then parse it as TestAllExtensions
  // it should work.

  TestAllTypes* message = [self allSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* rawBytes = message.data;
  [self assertFieldsInOrder:rawBytes];

  GPBExtensionRegistry* registry = [self extensionRegistry];

  TestAllExtensions* message2 = [TestAllExtensions parseFromData:rawBytes
                                               extensionRegistry:registry
                                                           error:NULL];

  [self assertAllExtensionsSet:message2 repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testExtensionsSerializedSize {
  size_t allSet = [self allSetRepeatedCount:kGPBDefaultRepeatCount].serializedSize;
  size_t extensionSet = [self allExtensionsSetRepeatedCount:kGPBDefaultRepeatCount].serializedSize;
  XCTAssertEqual(allSet, extensionSet);
}

- (void)testParsePackedExtensions {
  // Ensure that packed extensions can be properly parsed.
  TestPackedExtensions* message = [self packedExtensionsSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* rawBytes = message.data;
  [self assertFieldsInOrder:rawBytes];

  GPBExtensionRegistry* registry = [self extensionRegistry];

  TestPackedExtensions* message2 = [TestPackedExtensions parseFromData:rawBytes
                                                     extensionRegistry:registry
                                                                 error:NULL];

  [self assertPackedExtensionsSet:message2 repeatedCount:kGPBDefaultRepeatCount];
}

const int kUnknownTypeId = 1550055;

- (void)testSerializeMessageSet {
  // Set up a MSetMessage with two known messages and an unknown one.
  MSetMessage* message_set = [MSetMessage message];
  [[message_set getExtension:[MSetMessageExtension1 messageSetExtension]] setI:123];
  [[message_set getExtension:[MSetMessageExtension2 messageSetExtension]] setStr:@"foo"];
  GPBUnknownField* unknownField =
      [[[GPBUnknownField alloc] initWithNumber:kUnknownTypeId] autorelease];
  [unknownField addLengthDelimited:[NSData dataWithBytes:"bar" length:3]];
  GPBUnknownFieldSet* unknownFieldSet = [[[GPBUnknownFieldSet alloc] init] autorelease];
  [unknownFieldSet addField:unknownField];
  [message_set setUnknownFields:unknownFieldSet];

  NSData* data = [message_set data];

  // Parse back using MSetRawMessageSet and check the contents.
  MSetRawMessageSet* raw = [MSetRawMessageSet parseFromData:data error:NULL];

  XCTAssertEqual([raw.unknownFields countOfFields], (NSUInteger)0);

  XCTAssertEqual(raw.itemArray.count, (NSUInteger)3);
  XCTAssertEqual((uint32_t)[raw.itemArray[0] typeId],
                 [MSetMessageExtension1 messageSetExtension].fieldNumber);
  XCTAssertEqual((uint32_t)[raw.itemArray[1] typeId],
                 [MSetMessageExtension2 messageSetExtension].fieldNumber);
  XCTAssertEqual([raw.itemArray[2] typeId], kUnknownTypeId);

  MSetMessageExtension1* message1 =
      [MSetMessageExtension1 parseFromData:[((MSetRawMessageSet_Item*)raw.itemArray[0]) message]
                                     error:NULL];
  XCTAssertEqual(message1.i, 123);

  MSetMessageExtension2* message2 =
      [MSetMessageExtension2 parseFromData:[((MSetRawMessageSet_Item*)raw.itemArray[1]) message]
                                     error:NULL];
  XCTAssertEqualObjects(message2.str, @"foo");

  XCTAssertEqualObjects([raw.itemArray[2] message], [NSData dataWithBytes:"bar" length:3]);
}

- (void)testParseMessageSet {
  // Set up a MSetRawMessageSet with two known messages and an unknown one.
  MSetRawMessageSet* raw = [MSetRawMessageSet message];

  {
    MSetRawMessageSet_Item* item = [MSetRawMessageSet_Item message];
    item.typeId = [MSetMessageExtension1 messageSetExtension].fieldNumber;
    MSetMessageExtension1* message = [MSetMessageExtension1 message];
    message.i = 123;
    item.message = [message data];
    [raw.itemArray addObject:item];
  }

  {
    MSetRawMessageSet_Item* item = [MSetRawMessageSet_Item message];
    item.typeId = [MSetMessageExtension2 messageSetExtension].fieldNumber;
    MSetMessageExtension2* message = [MSetMessageExtension2 message];
    message.str = @"foo";
    item.message = [message data];
    [raw.itemArray addObject:item];
  }

  {
    MSetRawMessageSet_Item* item = [MSetRawMessageSet_Item message];
    item.typeId = kUnknownTypeId;
    item.message = [NSData dataWithBytes:"bar" length:3];
    [raw.itemArray addObject:item];
  }

  NSData* data = [raw data];

  // Parse as a MSetMessage and check the contents.
  MSetMessage* messageSet = [MSetMessage parseFromData:data
                                     extensionRegistry:[MSetUnittestMsetRoot extensionRegistry]
                                                 error:NULL];

  XCTAssertEqual([[messageSet getExtension:[MSetMessageExtension1 messageSetExtension]] i], 123);
  XCTAssertEqualObjects([[messageSet getExtension:[MSetMessageExtension2 messageSetExtension]] str],
                        @"foo");

  XCTAssertEqual([messageSet.unknownFields countOfFields], (NSUInteger)1);
  GPBUnknownField* unknownField = [messageSet.unknownFields getField:kUnknownTypeId];
  XCTAssertNotNil(unknownField);
  XCTAssertEqual(unknownField.lengthDelimitedList.count, (NSUInteger)1);
  XCTAssertEqualObjects(unknownField.lengthDelimitedList[0], [NSData dataWithBytes:"bar" length:3]);
}

- (void)assertFieldsInOrder:(NSData*)data {
  GPBCodedInputStream* input = [GPBCodedInputStream streamWithData:data];
  int32_t previousTag = 0;

  while (YES) {
    int32_t tag = [input readTag];
    if (tag == 0) {
      break;
    }

    XCTAssertGreaterThan(tag, previousTag);
    [input skipField:tag];
  }
}

@end
