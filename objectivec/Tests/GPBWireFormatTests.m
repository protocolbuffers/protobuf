// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBCodedInputStream.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBTestUtilities.h"
#import "GPBUnknownField.h"
#import "GPBUnknownField_PackagePrivate.h"
#import "GPBUnknownFields.h"
#import "GPBWireFormat.h"
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
const int kUnknownTypeId2 = 1550056;

- (void)testSerializeMessageSet {
  // Set up a MSetMessage with two known messages and an unknown one.
  MSetMessage* message_set = [MSetMessage message];
  [[message_set getExtension:[MSetMessageExtension1 messageSetExtension]] setI:123];
  [[message_set getExtension:[MSetMessageExtension2 messageSetExtension]] setStr:@"foo"];

  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  GPBUnknownFields* group = [ufs addGroupWithFieldNumber:GPBWireFormatMessageSetItem];
  [group addFieldNumber:GPBWireFormatMessageSetTypeId varint:kUnknownTypeId2];
  [group addFieldNumber:GPBWireFormatMessageSetMessage lengthDelimited:DataFromCStr("baz")];
  XCTAssertTrue([message_set mergeUnknownFields:ufs
                              extensionRegistry:[MSetUnittestMsetRoot extensionRegistry]
                                          error:NULL]);

  NSData* data = [message_set data];

  // Parse back using MSetRawMessageSet and check the contents.
  MSetRawMessageSet* raw = [MSetRawMessageSet parseFromData:data error:NULL];

  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] initFromMessage:raw] autorelease];
  XCTAssertTrue(ufs2.empty);

  XCTAssertEqual(raw.itemArray.count, (NSUInteger)3);
  XCTAssertEqual((uint32_t)[raw.itemArray[0] typeId],
                 [MSetMessageExtension1 messageSetExtension].fieldNumber);
  XCTAssertEqual((uint32_t)[raw.itemArray[1] typeId],
                 [MSetMessageExtension2 messageSetExtension].fieldNumber);
  XCTAssertEqual([raw.itemArray[2] typeId], kUnknownTypeId2);

  MSetMessageExtension1* message1 =
      [MSetMessageExtension1 parseFromData:[((MSetRawMessageSet_Item*)raw.itemArray[0]) message]
                                     error:NULL];
  XCTAssertEqual(message1.i, 123);

  MSetMessageExtension2* message2 =
      [MSetMessageExtension2 parseFromData:[((MSetRawMessageSet_Item*)raw.itemArray[1]) message]
                                     error:NULL];
  XCTAssertEqualObjects(message2.str, @"foo");

  XCTAssertEqualObjects([raw.itemArray[2] message], DataFromCStr("baz"));
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
    item.message = DataFromCStr("bar");
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

  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] initFromMessage:messageSet] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  GPBUnknownFields* group = [ufs firstGroup:GPBWireFormatMessageSetItem];
  XCTAssertNotNil(group);
  XCTAssertEqual(group.count, (NSUInteger)2);
  uint64_t varint = 0;
  XCTAssertTrue([group getFirst:GPBWireFormatMessageSetTypeId varint:&varint]);
  XCTAssertEqual(varint, kUnknownTypeId);
  XCTAssertEqualObjects([group firstLengthDelimited:GPBWireFormatMessageSetMessage],
                        DataFromCStr("bar"));
}

- (void)testParseMessageSet_FirstValueSticks {
  MSetRawBreakableMessageSet* raw = [MSetRawBreakableMessageSet message];

  {
    MSetRawBreakableMessageSet_Item* item = [MSetRawBreakableMessageSet_Item message];

    [item.typeIdArray addValue:[MSetMessageExtension1 messageSetExtension].fieldNumber];
    MSetMessageExtension1* message1 = [MSetMessageExtension1 message];
    message1.i = 123;
    NSData* itemData = [message1 data];
    [item.messageArray addObject:itemData];

    [item.typeIdArray addValue:[MSetMessageExtension2 messageSetExtension].fieldNumber];
    MSetMessageExtension2* message2 = [MSetMessageExtension2 message];
    message2.str = @"foo";
    itemData = [message2 data];
    [item.messageArray addObject:itemData];

    [raw.itemArray addObject:item];
  }

  NSData* data = [raw data];

  // Parse as a MSetMessage and check the contents.
  NSError* err = nil;
  MSetMessage* messageSet = [MSetMessage parseFromData:data
                                     extensionRegistry:[MSetUnittestMsetRoot extensionRegistry]
                                                 error:&err];
  XCTAssertNotNil(messageSet);
  XCTAssertNil(err);
  XCTAssertTrue([messageSet hasExtension:[MSetMessageExtension1 messageSetExtension]]);
  XCTAssertEqual([[messageSet getExtension:[MSetMessageExtension1 messageSetExtension]] i], 123);
  XCTAssertFalse([messageSet hasExtension:[MSetMessageExtension2 messageSetExtension]]);
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] initFromMessage:messageSet] autorelease];
  XCTAssertTrue(ufs.empty);
}

- (void)testParseMessageSet_PartialValuesDropped {
  MSetRawBreakableMessageSet* raw = [MSetRawBreakableMessageSet message];

  {
    MSetRawBreakableMessageSet_Item* item = [MSetRawBreakableMessageSet_Item message];
    [item.typeIdArray addValue:[MSetMessageExtension1 messageSetExtension].fieldNumber];
    // No payload.
    [raw.itemArray addObject:item];
  }

  {
    MSetRawBreakableMessageSet_Item* item = [MSetRawBreakableMessageSet_Item message];
    // No type ID.
    MSetMessageExtension2* message = [MSetMessageExtension2 message];
    message.str = @"foo";
    NSData* itemData = [message data];
    [item.messageArray addObject:itemData];
    [raw.itemArray addObject:item];
  }

  {
    MSetRawBreakableMessageSet_Item* item = [MSetRawBreakableMessageSet_Item message];
    // Neither type ID nor payload.
    [raw.itemArray addObject:item];
  }

  NSData* data = [raw data];

  // Parse as a MSetMessage and check the contents.
  NSError* err = nil;
  MSetMessage* messageSet = [MSetMessage parseFromData:data
                                     extensionRegistry:[MSetUnittestMsetRoot extensionRegistry]
                                                 error:&err];
  XCTAssertNotNil(messageSet);
  XCTAssertNil(err);
  XCTAssertEqual([messageSet extensionsCurrentlySet].count,
                 (NSUInteger)0);  // None because they were all partial and dropped.
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] initFromMessage:messageSet] autorelease];
  XCTAssertTrue(ufs.empty);
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
