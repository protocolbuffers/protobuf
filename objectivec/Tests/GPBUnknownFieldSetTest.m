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

#import "GPBUnknownField_PackagePrivate.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "google/protobuf/Unittest.pbobjc.h"

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
  GPBUnknownFieldSet *set = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* field = [[[GPBUnknownField alloc] initWithNumber:0] autorelease];
  XCTAssertThrowsSpecificNamed([set addField:field], NSException, NSInvalidArgumentException);
}

- (void)testEqualityAndHash {
  // Empty

  GPBUnknownFieldSet *set1 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  XCTAssertTrue([set1 isEqual:set1]);
  XCTAssertFalse([set1 isEqual:@"foo"]);
  GPBUnknownFieldSet *set2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
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

  GPBUnknownFieldSet *group1 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup1 = [[[GPBUnknownField alloc] initWithNumber:10] autorelease];
  [fieldGroup1 addVarint:1];
  [group1 addField:fieldGroup1];
  GPBUnknownFieldSet *group2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
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
  GPBUnknownFieldSet* bizarroFields =
      [[[GPBUnknownFieldSet alloc] init] autorelease];
  NSUInteger count = [unknownFields_ countOfFields];
  int32_t *tags = malloc(count * sizeof(int32_t));
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
        GPBUnknownField* varintField =
            [[[GPBUnknownField alloc] initWithNumber:tag] autorelease];
        [varintField addVarint:1];
        [bizarroFields addField:varintField];
      } else {
        // Original field *is* a varint, so use something else.
        GPBUnknownField* fixed32Field =
            [[[GPBUnknownField alloc] initWithNumber:tag] autorelease];
        [fixed32Field addFixed32:1];
        [bizarroFields addField:fixed32Field];
      }
    }
  }
  @finally {
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

  GPBUnknownFieldSet *group1 = [[[GPBUnknownFieldSet alloc] init] autorelease];
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

  GPBUnknownFieldSet *group2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
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

  GPBUnknownFieldSet *group3a = [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField* fieldGroup3a1 = [[[GPBUnknownField alloc] initWithNumber:200] autorelease];
  [fieldGroup3a1 addVarint:100];
  [group3a addField:fieldGroup3a1];
  GPBUnknownFieldSet *group3b = [[[GPBUnknownFieldSet alloc] init] autorelease];
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

  XCTAssertEqualObjects(destination1.data, destination2.data);
  XCTAssertEqualObjects(destination1.data, source3.data);
  XCTAssertEqualObjects(destination2.data, source3.data);
}

- (void)testClearMessage {
  TestEmptyMessage *message = [TestEmptyMessage message];
  [message mergeFrom:emptyMessage_];
  [message clear];
  XCTAssertEqual(message.serializedSize, (size_t)0);
}

- (void)testParseKnownAndUnknown {
  // Test mixing known and unknown fields when parsing.
  GPBUnknownFieldSet *fields = [[unknownFields_ copy] autorelease];
  GPBUnknownField *field =
    [[[GPBUnknownField alloc] initWithNumber:123456] autorelease];
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
  TestAllTypes* allTypesMessage =
      [TestAllTypes parseFromData:bizarroData error:NULL];
  TestEmptyMessage* emptyMessage =
      [TestEmptyMessage parseFromData:bizarroData error:NULL];

  // All fields should have been interpreted as unknown, so the debug strings
  // should be the same.
  XCTAssertEqualObjects(emptyMessage.data, allTypesMessage.data);
}

- (void)testUnknownExtensions {
  // Make sure fields are properly parsed to the UnknownFieldSet even when
  // they are declared as extension numbers.

  TestEmptyMessageWithExtensions* message =
      [TestEmptyMessageWithExtensions parseFromData:allFieldsData_ error:NULL];

  XCTAssertEqual(unknownFields_.countOfFields,
                 message.unknownFields.countOfFields);
  XCTAssertEqualObjects(allFieldsData_, message.data);
}

- (void)testWrongExtensionTypeTreatedAsUnknown {
  // Test that fields of the wrong wire type are treated like unknown fields
  // when parsing extensions.

  NSData* bizarroData = [self getBizarroData];
  TestAllExtensions* allExtensionsMessage =
      [TestAllExtensions parseFromData:bizarroData error:NULL];
  TestEmptyMessage* emptyMessage =
      [TestEmptyMessage parseFromData:bizarroData error:NULL];

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
  [parsed mergeFromData:data];
  GPBUnknownField* field2 = [parsed getField:1];
  XCTAssertEqual(field2.varintList.count, (NSUInteger)1);
  XCTAssertEqual(0x7FFFFFFFFFFFFFFFULL, [field2.varintList valueAtIndex:0]);
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

  GPBUnknownFieldSet *group = [[[GPBUnknownFieldSet alloc] init] autorelease];
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
