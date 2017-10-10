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

// Constructs a protocol buffer which contains fields with all the same
// numbers as allFieldsData except that each field is some other wire
// type.
- (NSData*)getBizarroData {
  GPBUnknownFieldSet* bizarroFields =
      [[[GPBUnknownFieldSet alloc] init] autorelease];
  NSUInteger count = [unknownFields_ countOfFields];
  int32_t tags[count];
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

  GPBUnknownFieldSet* set2 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  field = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field addVarint:1];
  [set2 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field addVarint:3];
  [set2 addField:field];

  GPBUnknownFieldSet* set3 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  field = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field addVarint:1];
  [set3 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field addVarint:4];
  [set3 addField:field];

  GPBUnknownFieldSet* set4 = [[[GPBUnknownFieldSet alloc] init] autorelease];
  field = [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  [field addVarint:2];
  [set4 addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field addVarint:3];
  [set4 addField:field];

  TestEmptyMessage* source1 = [TestEmptyMessage message];
  [source1 setUnknownFields:set1];
  TestEmptyMessage* source2 = [TestEmptyMessage message];
  [source2 setUnknownFields:set2];
  TestEmptyMessage* source3 = [TestEmptyMessage message];
  [source3 setUnknownFields:set3];
  TestEmptyMessage* source4 = [TestEmptyMessage message];
  [source4 setUnknownFields:set4];

  TestEmptyMessage* destination1 = [TestEmptyMessage message];
  [destination1 mergeFrom:source1];
  [destination1 mergeFrom:source2];

  TestEmptyMessage* destination2 = [TestEmptyMessage message];
  [destination2 mergeFrom:source3];
  [destination2 mergeFrom:source4];

  XCTAssertEqualObjects(destination1.data, destination2.data);
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

- (void)testMergingFields {
  GPBUnknownField* field1 = [[[GPBUnknownField alloc] initWithNumber:1] autorelease];
  [field1 addVarint:1];
  [field1 addFixed32:2];
  [field1 addFixed64:3];
  [field1 addLengthDelimited:[NSData dataWithBytes:"hello" length:5]];
  [field1 addGroup:[[unknownFields_ copy] autorelease]];
  GPBUnknownField* field2 = [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  [field2 mergeFromField:field1];
  XCTAssertEqualObjects(field1, field2);
}

@end
