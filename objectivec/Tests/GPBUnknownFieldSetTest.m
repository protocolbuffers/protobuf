// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"
#import "GPBUnknownFieldSet.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUnknownField_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

@interface GPBUnknownFieldSet (GPBUnknownFieldSetTest)
- (void)getTags:(int32_t*)tags;
@end

@interface UnknownFieldSetTest : GPBTestCase {
 @private
  TestAllTypes* allFields_;
  NSData* allFieldsData_;

  // An empty message that has been parsed from allFieldsData.  So, it has
  // unknown fields of every type.
  TestEmptyMessage* emptyMessage_;
  GPBUnknownFieldSet* unknownFields_;
}

@end

@implementation UnknownFieldSetTest

- (void)setUp {
  allFields_ = [self allSetRepeatedCount:kGPBDefaultRepeatCount];
  allFieldsData_ = [allFields_ data];
  emptyMessage_ = [TestEmptyMessage parseFromData:allFieldsData_ error:NULL];
  unknownFields_ = emptyMessage_.unknownFields;
}

- (void)testInvalidFieldNumber {
  GPBUnknownFieldSet* set = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* field = [[[GPBUnknownField alloc] initWithNumber:0] autorelease];
  XCTAssertThrowsSpecificNamed([set addField:field], NSException, NSInvalidArgumentException);
}

- (void)testEqualityAndHash {
  // Empty

  GPBUnknownFieldSet* set1 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  XCTAssertTrue([set1 isEqual:set1]);
  XCTAssertFalse([set1 isEqual:@"foo"]);
  GPBUnknownFieldSet* set2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  XCTAssertEqualObjects(set1, set2);
  XCTAssertEqual([set1 hash], [set2 hash]);

  // Varint

  GPBUnknownField* field1 = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field1 addVarint:1];
  [set1 addField:field1];
  XCTAssertNotEqualObjects(set1, set2);
  GPBUnknownField* field2 = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field2 addVarint:1];
  [set2 addField:field2];
  XCTAssertEqualObjects(set1, set2);
  XCTAssertEqual([set1 hash], [set2 hash]);

  // Fixed32

  field1 = [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  [field1 addFixed32:2];
  [set1 addField:field1];
  XCTAssertNotEqualObjects(set1, set2);
  field2 = [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  [field2 addFixed32:2];
  [set2 addField:field2];
  XCTAssertEqualObjects(set1, set2);
  XCTAssertEqual([set1 hash], [set2 hash]);

  // Fixed64

  field1 = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field1 addFixed64:3];
  [set1 addField:field1];
  XCTAssertNotEqualObjects(set1, set2);
  field2 = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field2 addFixed64:3];
  [set2 addField:field2];
  XCTAssertEqualObjects(set1, set2);
  XCTAssertEqual([set1 hash], [set2 hash]);

  // LengthDelimited

  field1 = [[[GPBUnknownField alloc] initWithNumber:4] autorelease];
  [field1 addLengthDelimited:DataFromCStr("foo")];
  [set1 addField:field1];
  XCTAssertNotEqualObjects(set1, set2);
  field2 = [[[GPBUnknownField alloc] initWithNumber:4] autorelease];
  [field2 addLengthDelimited:DataFromCStr("foo")];
  [set2 addField:field2];
  XCTAssertEqualObjects(set1, set2);
  XCTAssertEqual([set1 hash], [set2 hash]);

  // Group

  GPBUnknownFieldSet* group1 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup1 = [[[GPBUnknownField alloc] initWithNumber:10] autorelease];
  [fieldGroup1 addVarint:1];
  [group1 addField:fieldGroup1];
  GPBUnknownFieldSet* group2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup2 = [[[GPBUnknownField alloc] initWithNumber:10] autorelease];
  [fieldGroup2 addVarint:1];
  [group2 addField:fieldGroup2];

  field1 = [[[GPBUnknownField alloc] initWithNumber:5] autorelease];
  [field1 addGroup:group1];
  [set1 addField:field1];
  XCTAssertNotEqualObjects(set1, set2);
  field2 = [[[GPBUnknownField alloc] initWithNumber:5] autorelease];
  [field2 addGroup:group2];
  [set2 addField:field2];
  XCTAssertEqualObjects(set1, set2);
  XCTAssertEqual([set1 hash], [set2 hash]);

  // Exercise description for completeness.
  XCTAssertTrue(set1.description.length > 10);
}

// Constructs a protocol buffer which contains fields with all the same
// numbers as allFieldsData except that each field is some other wire
// type.
- (NSData*)getBizarroData {
  GPBUnknownFieldSet* bizarroFields = [[[GPBUnknownFieldSet alloc] init] autorelease];
  NSUInteger count = [unknownFields_ countOfFields];
  int32_t* tags = malloc(count * sizeof(int32_t));
  if (!tags) {
    XCTFail(@"Failed to make scratch buffer for testing");
    return [NSData data];
  }
  @try {
    [unknownFields_ getTags:tags];
    for (NSUInteger i = 0; i < count; ++i) {
      int32_t tag = tags[i];
      GPBUnknownField* field = [unknownFields_ getField:tag];
      if (field.varintList.count == 0) {
        // Original field is not a varint, so use a varint.
        GPBUnknownField* varintField = [[[GPBUnknownField alloc] initWithNumber:tag] autorelease];
        [varintField addVarint:1];
        [bizarroFields addField:varintField];
      } else {
        // Original field *is* a varint, so use something else.
        GPBUnknownField* fixed32Field = [[[GPBUnknownField alloc] initWithNumber:tag] autorelease];
        [fixed32Field addFixed32:1];
        [bizarroFields addField:fixed32Field];
      }
    }
  } @finally {
    free(tags);
  }

  return [bizarroFields data];
}

- (void)testSerialize {
  // Check that serializing the UnknownFieldSet produces the original data
  // again.
  NSData* data = [emptyMessage_ data];
  XCTAssertEqualObjects(allFieldsData_, data);
}

- (void)testCopyFrom {
  TestEmptyMessage* message = [TestEmptyMessage message];
  [message mergeFrom:emptyMessage_];

  XCTAssertEqualObjects(emptyMessage_.data, message.data);
}

- (void)testMergeFrom {
  GPBUnknownFieldSet* set1 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* field = [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  [field addVarint:2];
  [set1 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field addVarint:4];
  [set1 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:4] autorelease];
  [field addFixed32:6];
  [set1 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:5] autorelease];
  [field addFixed64:20];
  [set1 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:10] autorelease];
  [field addLengthDelimited:DataFromCStr("data1")];
  [set1 addField:field];

  GPBUnknownFieldSet* group1 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup1 = [[[GPBUnknownField alloc] initWithNumber:200] autorelease];
  [fieldGroup1 addVarint:100];
  [group1 addField:fieldGroup1];

  field = [[[GPBUnknownField alloc] initWithNumber:11] autorelease];
  [field addGroup:group1];
  [set1 addField:field];

  GPBUnknownFieldSet* set2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  field = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field addVarint:1];
  [set2 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field addVarint:3];
  [set2 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:4] autorelease];
  [field addFixed32:7];
  [set2 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:5] autorelease];
  [field addFixed64:30];
  [set2 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:10] autorelease];
  [field addLengthDelimited:DataFromCStr("data2")];
  [set2 addField:field];

  GPBUnknownFieldSet* group2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup2 = [[[GPBUnknownField alloc] initWithNumber:201] autorelease];
  [fieldGroup2 addVarint:99];
  [group2 addField:fieldGroup2];

  field = [[[GPBUnknownField alloc] initWithNumber:11] autorelease];
  [field addGroup:group2];
  [set2 addField:field];

  GPBUnknownFieldSet* set3 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  field = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field addVarint:1];
  [set3 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  [field addVarint:2];
  [set3 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field addVarint:4];
  [set3 addField:field];
  [field addVarint:3];
  [set3 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:4] autorelease];
  [field addFixed32:6];
  [field addFixed32:7];
  [set3 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:5] autorelease];
  [field addFixed64:20];
  [field addFixed64:30];
  [set3 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:10] autorelease];
  [field addLengthDelimited:DataFromCStr("data1")];
  [field addLengthDelimited:DataFromCStr("data2")];
  [set3 addField:field];

  GPBUnknownFieldSet* group3a = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup3a1 = [[[GPBUnknownField alloc] initWithNumber:200] autorelease];
  [fieldGroup3a1 addVarint:100];
  [group3a addField:fieldGroup3a1];
  GPBUnknownFieldSet* group3b = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup3b2 = [[[GPBUnknownField alloc] initWithNumber:201] autorelease];
  [fieldGroup3b2 addVarint:99];
  [group3b addField:fieldGroup3b2];

  field = [[[GPBUnknownField alloc] initWithNumber:11] autorelease];
  [field addGroup:group1];
  [field addGroup:group3b];
  [set3 addField:field];

  TestEmptyMessage* source1 = [TestEmptyMessage message];
  [source1 setUnknownFields:set1];
  TestEmptyMessage* source2 = [TestEmptyMessage message];
  [source2 setUnknownFields:set2];
  TestEmptyMessage* source3 = [TestEmptyMessage message];
  [source3 setUnknownFields:set3];

  TestEmptyMessage* destination1 = [TestEmptyMessage message];
  [destination1 mergeFrom:source1];
  [destination1 mergeFrom:source2];

  TestEmptyMessage* destination2 = [TestEmptyMessage message];
  [destination2 mergeFrom:source3];

  XCTAssertEqualObjects(destination1.unknownFields, destination2.unknownFields);
  XCTAssertEqualObjects(destination1.unknownFields, source3.unknownFields);
  XCTAssertEqualObjects(destination2.unknownFields, source3.unknownFields);

  XCTAssertEqualObjects(destination1.data, destination2.data);
  XCTAssertEqualObjects(destination1.data, source3.data);
  XCTAssertEqualObjects(destination2.data, source3.data);
}

- (void)testClearMessage {
  TestEmptyMessage* message = [TestEmptyMessage message];
  [message mergeFrom:emptyMessage_];
  [message clear];
  XCTAssertEqual(message.serializedSize, (size_t)0);
}

- (void)testParseKnownAndUnknown {
  // Test mixing known and unknown fields when parsing.
  GPBUnknownFieldSet* fields = [[unknownFields_ copy] autorelease];
  GPBUnknownField* field = [[[GPBUnknownField alloc] initWithNumber:123456] autorelease];
  [field addVarint:654321];
  [fields addField:field];

  NSData* data = fields.data;
  TestAllTypes* destination = [TestAllTypes parseFromData:data error:NULL];

  [self assertAllFieldsSet:destination repeatedCount:kGPBDefaultRepeatCount];
  XCTAssertEqual(destination.unknownFields.countOfFields, (NSUInteger)1);

  GPBUnknownField* field2 = [destination.unknownFields getField:123456];
  XCTAssertEqual(field2.varintList.count, (NSUInteger)1);
  XCTAssertEqual(654321ULL, [field2.varintList valueAtIndex:0]);
}

- (void)testWrongTypeTreatedAsUnknown {
  // Test that fields of the wrong wire type are treated like unknown fields
  // when parsing.

  NSData* bizarroData = [self getBizarroData];
  TestAllTypes* allTypesMessage = [TestAllTypes parseFromData:bizarroData error:NULL];
  TestEmptyMessage* emptyMessage = [TestEmptyMessage parseFromData:bizarroData error:NULL];

  // All fields should have been interpreted as unknown, so the debug strings
  // should be the same.
  XCTAssertEqualObjects(emptyMessage.data, allTypesMessage.data);
}

- (void)testUnknownExtensions {
  // Make sure fields are properly parsed to the UnknownFieldSet even when
  // they are declared as extension numbers.

  TestEmptyMessageWithExtensions* message =
      [TestEmptyMessageWithExtensions parseFromData:allFieldsData_ error:NULL];

  XCTAssertEqual(unknownFields_.countOfFields, message.unknownFields.countOfFields);
  XCTAssertEqualObjects(allFieldsData_, message.data);

  // Just confirm as known extensions, they don't go into unknown data and end up in the
  // extensions dictionary.
  TestAllExtensions* allExtensionsMessage =
      [TestAllExtensions parseFromData:allFieldsData_
                     extensionRegistry:[UnittestRoot extensionRegistry]
                                 error:NULL];
  XCTAssertEqual(allExtensionsMessage.unknownFields.countOfFields, (NSUInteger)0);
  XCTAssertEqualObjects([allExtensionsMessage data], allFieldsData_);
}

- (void)testWrongExtensionTypeTreatedAsUnknown {
  // Test that fields of the wrong wire type are treated like unknown fields
  // when parsing extensions.

  NSData* bizarroData = [self getBizarroData];
  TestAllExtensions* allExtensionsMessage =
      [TestAllExtensions parseFromData:bizarroData
                     extensionRegistry:[UnittestRoot extensionRegistry]
                                 error:NULL];
  TestEmptyMessage* emptyMessage = [TestEmptyMessage parseFromData:bizarroData error:NULL];

  // All fields should have been interpreted as unknown, so the debug strings
  // should be the same.
  XCTAssertEqualObjects(emptyMessage.data, allExtensionsMessage.data);
}

- (void)testLargeVarint {
  GPBUnknownFieldSet* fields = [[unknownFields_ copy] autorelease];
  GPBUnknownField* field = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field addVarint:0x7FFFFFFFFFFFFFFFL];
  [fields addField:field];

  NSData* data = [fields data];

  GPBUnknownFieldSet* parsed = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBCodedInputStream* input = [[[GPBCodedInputStream alloc] initWithData:data] autorelease];
  [parsed mergeFromCodedInputStream:input];
  GPBUnknownField* field2 = [parsed getField:1];
  XCTAssertEqual(field2.varintList.count, (NSUInteger)1);
  XCTAssertEqual(0x7FFFFFFFFFFFFFFFULL, [field2.varintList valueAtIndex:0]);
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

- (void)testParsingNestingGroupData {
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

  TestEmptyMessage* m = [TestEmptyMessage parseFromData:DataFromBytes(35, 36)
                                                  error:NULL];  // empty group
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  GPBUnknownField* field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  GPBUnknownFieldSet* group = field.groupList[0];
  XCTAssertEqual(group.countOfFields, (NSUInteger)0);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 8, 1, 36) error:NULL];  // varint
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  field = [group getField:1];
  XCTAssertEqual(field.varintList.count, (NSUInteger)1);
  XCTAssertEqual([field.varintList valueAtIndex:0], 1);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 21, 0x78, 0x56, 0x34, 0x12, 36)
                                error:NULL];  // fixed32
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  field = [group getField:2];
  XCTAssertEqual(field.fixed32List.count, (NSUInteger)1);
  XCTAssertEqual([field.fixed32List valueAtIndex:0], 0x12345678);

  m = [TestEmptyMessage
      parseFromData:DataFromBytes(35, 25, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                                  36)
              error:NULL];  // fixed64
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  field = [group getField:3];
  XCTAssertEqual(field.fixed64List.count, (NSUInteger)1);
  XCTAssertEqual([field.fixed64List valueAtIndex:0], 0x123456789abcdef0LL);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 50, 0, 36)
                                error:NULL];  // length delimited, length 0
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  field = [group getField:6];
  XCTAssertEqual(field.lengthDelimitedList.count, (NSUInteger)1);
  XCTAssertEqualObjects(field.lengthDelimitedList[0], [NSData data]);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 50, 1, 42, 36)
                                error:NULL];  // length delimited, length 1, byte 42
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  field = [group getField:6];
  XCTAssertEqual(field.lengthDelimitedList.count, (NSUInteger)1);
  XCTAssertEqualObjects(field.lengthDelimitedList[0], DataFromBytes(42));

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 43, 44, 36) error:NULL];  // Sub group
  field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  XCTAssertEqual(group.countOfFields, (NSUInteger)1);
  field = [group getField:5];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  XCTAssertEqual(group.countOfFields, (NSUInteger)0);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 8, 1, 43, 8, 2, 44, 36)
                                error:NULL];  // varint and sub group with varint
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  field = [m.unknownFields getField:4];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  XCTAssertEqual(group.countOfFields, (NSUInteger)2);
  field = [group getField:1];
  XCTAssertEqual(field.varintList.count, (NSUInteger)1);
  XCTAssertEqual([field.varintList valueAtIndex:0], 1);
  field = [group getField:5];
  XCTAssertEqual(field.groupList.count, (NSUInteger)1);
  group = field.groupList[0];
  field = [group getField:1];
  XCTAssertEqual(field.varintList.count, (NSUInteger)1);
  XCTAssertEqual([field.varintList valueAtIndex:0], 2);

  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 0, 36)
                                         error:NULL]);  // Invalid field number
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 15, 36)
                                         error:NULL]);  // Invalid wire type
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 21, 0x78, 0x56, 0x34)
                                         error:NULL]);  // truncated fixed32
  XCTAssertNil([TestEmptyMessage
      parseFromData:DataFromBytes(35, 25, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56,
                                  0x34)
              error:NULL]);  // truncated fixed64

  // Mising end group
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 8, 1) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 43) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 43, 8, 1) error:NULL]);

  // Wrong end group
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 44) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 8, 1, 44) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 43, 36) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 43, 8, 1, 36) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 43, 44, 44) error:NULL]);
  XCTAssertNil([TestEmptyMessage parseFromData:DataFromBytes(35, 43, 8, 1, 44, 44) error:NULL]);

  // This is the same limit as within GPBCodedInputStream.
  const NSUInteger kDefaultRecursionLimit = 100;
  // That depth parses.
  NSData* testData = DataForGroupsOfDepth(kDefaultRecursionLimit);
  m = [TestEmptyMessage parseFromData:testData error:NULL];
  XCTAssertEqual(m.unknownFields.countOfFields, (NSUInteger)1);
  field = [m.unknownFields getField:4];
  for (NSUInteger i = 0; i < kDefaultRecursionLimit; ++i) {
    XCTAssertEqual(field.varintList.count, (NSUInteger)0);
    XCTAssertEqual(field.fixed32List.count, (NSUInteger)0);
    XCTAssertEqual(field.fixed64List.count, (NSUInteger)0);
    XCTAssertEqual(field.lengthDelimitedList.count, (NSUInteger)0);
    XCTAssertEqual(field.groupList.count, (NSUInteger)1);
    group = field.groupList[0];
    XCTAssertEqual(group.countOfFields, (NSUInteger)1);
    field = [group getField:(i < (kDefaultRecursionLimit - 1) ? 4 : 1)];
  }
  // field is of the inner most group
  XCTAssertEqual(field.varintList.count, (NSUInteger)1);
  XCTAssertEqual([field.varintList valueAtIndex:0], (NSUInteger)1);
  XCTAssertEqual(field.fixed32List.count, (NSUInteger)0);
  XCTAssertEqual(field.fixed64List.count, (NSUInteger)0);
  XCTAssertEqual(field.lengthDelimitedList.count, (NSUInteger)0);
  XCTAssertEqual(field.groupList.count, (NSUInteger)0);
  // One more level deep fails.
  testData = DataForGroupsOfDepth(kDefaultRecursionLimit + 1);
  XCTAssertNil([TestEmptyMessage parseFromData:testData error:NULL]);
}

#pragma mark - Field tests
// Some tests directly on fields since the dictionary in FieldSet can gate
// testing some of these.

- (void)testFieldEqualityAndHash {
  GPBUnknownField* field1 = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  XCTAssertTrue([field1 isEqual:field1]);
  XCTAssertFalse([field1 isEqual:@"foo"]);
  GPBUnknownField* field2 = [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  XCTAssertNotEqualObjects(field1, field2);

  field2 = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Varint

  [field1 addVarint:10];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addVarint:10];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);
  [field1 addVarint:11];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addVarint:11];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Fixed32

  [field1 addFixed32:20];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addFixed32:20];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);
  [field1 addFixed32:21];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addFixed32:21];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Fixed64

  [field1 addFixed64:30];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addFixed64:30];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);
  [field1 addFixed64:31];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addFixed64:31];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);

  // LengthDelimited

  [field1 addLengthDelimited:DataFromCStr("foo")];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addLengthDelimited:DataFromCStr("foo")];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);
  [field1 addLengthDelimited:DataFromCStr("bar")];
  XCTAssertNotEqualObjects(field1, field2);
  [field2 addLengthDelimited:DataFromCStr("bar")];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Group

  GPBUnknownFieldSet* group = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup = [[[GPBUnknownField alloc] initWithNumber:100] autorelease];
  [fieldGroup addVarint:100];
  [group addField:fieldGroup];
  [field1 addGroup:group];
  XCTAssertNotEqualObjects(field1, field2);
  group = [[[GPBUnknownFieldSet alloc] init] autorelease];
  fieldGroup = [[[GPBUnknownField alloc] initWithNumber:100] autorelease];
  [fieldGroup addVarint:100];
  [group addField:fieldGroup];
  [field2 addGroup:group];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);

  group = [[[GPBUnknownFieldSet alloc] init] autorelease];
  fieldGroup = [[[GPBUnknownField alloc] initWithNumber:101] autorelease];
  [fieldGroup addVarint:101];
  [group addField:fieldGroup];
  [field1 addGroup:group];
  XCTAssertNotEqualObjects(field1, field2);
  group = [[[GPBUnknownFieldSet alloc] init] autorelease];
  fieldGroup = [[[GPBUnknownField alloc] initWithNumber:101] autorelease];
  [fieldGroup addVarint:101];
  [group addField:fieldGroup];
  [field2 addGroup:group];
  XCTAssertEqualObjects(field1, field2);
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Exercise description for completeness.
  XCTAssertTrue(field1.description.length > 10);
}

- (void)testMergingFields {
  GPBUnknownField* field1 = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field1 addVarint:1];
  [field1 addFixed32:2];
  [field1 addFixed64:3];
  [field1 addLengthDelimited:[NSData dataWithBytes:"hello" length:5]];
  [field1 addGroup:[[unknownFields_ copy] autorelease]];
  GPBUnknownField* field2 = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field2 mergeFromField:field1];
}

@end

#pragma clang diagnostic pop
