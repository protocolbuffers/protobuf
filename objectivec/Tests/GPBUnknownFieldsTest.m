// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"
#import "GPBUnknownField.h"
#import "GPBUnknownFields.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

@interface UnknownFieldsTest : GPBTestCase
@end

@implementation UnknownFieldsTest

- (void)testEmptyAndClear {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  XCTAssertTrue(ufs.empty);

  [ufs addFieldNumber:1 varint:1];
  XCTAssertFalse(ufs.empty);
  [ufs clear];
  XCTAssertTrue(ufs.empty);

  [ufs addFieldNumber:1 fixed32:1];
  XCTAssertFalse(ufs.empty);
  [ufs clear];
  XCTAssertTrue(ufs.empty);

  [ufs addFieldNumber:1 fixed64:1];
  XCTAssertFalse(ufs.empty);
  [ufs clear];
  XCTAssertTrue(ufs.empty);

  [ufs addFieldNumber:1 lengthDelimited:[NSData data]];
  XCTAssertFalse(ufs.empty);
  [ufs clear];
  XCTAssertTrue(ufs.empty);

  GPBUnknownFields* group = [ufs addGroupWithFieldNumber:1];
  XCTAssertNotNil(group);
  XCTAssertFalse(ufs.empty);
}

- (void)testEqualityAndHash {
  // Empty

  GPBUnknownFields* ufs1 = [[[GPBUnknownFields alloc] init] autorelease];
  XCTAssertTrue([ufs1 isEqual:ufs1]);
  XCTAssertFalse([ufs1 isEqual:@"foo"]);
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);

  // Varint

  [ufs1 addFieldNumber:1 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  [ufs2 addFieldNumber:1 varint:1];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);

  // Fixed32

  [ufs1 addFieldNumber:2 fixed32:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  [ufs2 addFieldNumber:2 fixed32:1];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);

  // Fixed64

  [ufs1 addFieldNumber:3 fixed64:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  [ufs2 addFieldNumber:3 fixed64:1];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);

  // LengthDelimited

  [ufs1 addFieldNumber:4 lengthDelimited:DataFromCStr("foo")];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  [ufs2 addFieldNumber:4 lengthDelimited:DataFromCStr("foo")];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);

  // Group

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:5];
  [group1 addFieldNumber:10 varint:10];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  GPBUnknownFields* group2 = [ufs2 addGroupWithFieldNumber:5];
  [group2 addFieldNumber:10 varint:10];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);
}

- (void)testInequality_Values {
  // Same field number and type, different values.

  GPBUnknownFields* ufs1 = [[[GPBUnknownFields alloc] init] autorelease];
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];

  [ufs1 addFieldNumber:1 varint:1];
  [ufs2 addFieldNumber:1 varint:2];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed32:1];
  [ufs2 addFieldNumber:1 fixed32:2];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed64:1];
  [ufs2 addFieldNumber:1 fixed64:2];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs2 addFieldNumber:1 lengthDelimited:DataFromCStr("bar")];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:1];
  GPBUnknownFields* group2 = [ufs2 addGroupWithFieldNumber:1];
  [group1 addFieldNumber:10 varint:10];
  [group2 addFieldNumber:10 varint:20];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  XCTAssertNotEqualObjects(group1, group2);
}

- (void)testInequality_FieldNumbers {
  // Same type and value, different field numbers.

  GPBUnknownFields* ufs1 = [[[GPBUnknownFields alloc] init] autorelease];
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];

  [ufs1 addFieldNumber:1 varint:1];
  [ufs2 addFieldNumber:2 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed32:1];
  [ufs2 addFieldNumber:2 fixed32:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed64:1];
  [ufs2 addFieldNumber:2 fixed64:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs2 addFieldNumber:2 lengthDelimited:DataFromCStr("fod")];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:1];
  GPBUnknownFields* group2 = [ufs2 addGroupWithFieldNumber:2];
  [group1 addFieldNumber:10 varint:10];
  [group2 addFieldNumber:10 varint:10];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  XCTAssertEqualObjects(group1, group2);
}

- (void)testInequality_Types {
  // Same field number and value when possible, different types.

  GPBUnknownFields* ufs1 = [[[GPBUnknownFields alloc] init] autorelease];
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];

  [ufs1 addFieldNumber:1 varint:1];
  [ufs2 addFieldNumber:1 fixed32:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed32:1];
  [ufs2 addFieldNumber:1 fixed64:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed64:1];
  [ufs2 addFieldNumber:1 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs2 addFieldNumber:1 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:1];
  [group1 addFieldNumber:10 varint:10];
  [ufs2 addFieldNumber:1 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
}

- (void)testGetFirst {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  XCTAssertEqual(0U, ufs.count);
  [ufs addFieldNumber:1 varint:1];
  XCTAssertEqual(1U, ufs.count);
  [ufs addFieldNumber:1 varint:2];
  XCTAssertEqual(2U, ufs.count);
  [ufs addFieldNumber:1 fixed32:3];
  XCTAssertEqual(3U, ufs.count);
  [ufs addFieldNumber:1 fixed32:4];
  XCTAssertEqual(4U, ufs.count);
  [ufs addFieldNumber:1 fixed64:5];
  XCTAssertEqual(5U, ufs.count);
  [ufs addFieldNumber:1 fixed64:6];
  XCTAssertEqual(6U, ufs.count);
  [ufs addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  XCTAssertEqual(7U, ufs.count);
  [ufs addFieldNumber:1 lengthDelimited:DataFromCStr("bar")];
  XCTAssertEqual(8U, ufs.count);
  GPBUnknownFields* group1 = [ufs addGroupWithFieldNumber:1];
  XCTAssertNotNil(group1);
  XCTAssertEqual(9U, ufs.count);
  GPBUnknownFields* group2 = [ufs addGroupWithFieldNumber:1];
  XCTAssertNotNil(group2);
  XCTAssertTrue(group1 != group2);  // Different objects
  XCTAssertEqual(10U, ufs.count);

  [ufs addFieldNumber:11 varint:11];
  [ufs addFieldNumber:12 fixed32:12];
  [ufs addFieldNumber:13 fixed64:13];
  [ufs addFieldNumber:14 lengthDelimited:DataFromCStr("foo2")];
  GPBUnknownFields* group3 = [ufs addGroupWithFieldNumber:15];
  XCTAssertNotNil(group3);
  XCTAssertTrue(group3 != group1);  // Different objects
  XCTAssertTrue(group3 != group2);  // Different objects
  XCTAssertEqual(15U, ufs.count);

  uint64_t varint = 0;
  XCTAssertTrue([ufs getFirst:1 varint:&varint]);
  XCTAssertEqual(1U, varint);
  XCTAssertTrue([ufs getFirst:11 varint:&varint]);
  XCTAssertEqual(11U, varint);
  XCTAssertFalse([ufs getFirst:12 varint:&varint]);  // Different type
  XCTAssertFalse([ufs getFirst:99 varint:&varint]);  // Not present

  uint32_t fixed32 = 0;
  XCTAssertTrue([ufs getFirst:1 fixed32:&fixed32]);
  XCTAssertEqual(3U, fixed32);
  XCTAssertTrue([ufs getFirst:12 fixed32:&fixed32]);
  XCTAssertEqual(12U, fixed32);
  XCTAssertFalse([ufs getFirst:11 fixed32:&fixed32]);  // Different type
  XCTAssertFalse([ufs getFirst:99 fixed32:&fixed32]);  // Not present

  uint64_t fixed64 = 0;
  XCTAssertTrue([ufs getFirst:1 fixed64:&fixed64]);
  XCTAssertEqual(5U, fixed64);
  XCTAssertTrue([ufs getFirst:13 fixed64:&fixed64]);
  XCTAssertEqual(13U, fixed64);
  XCTAssertFalse([ufs getFirst:11 fixed64:&fixed64]);  // Different type
  XCTAssertFalse([ufs getFirst:99 fixed64:&fixed64]);  // Not present

  XCTAssertEqualObjects(DataFromCStr("foo"), [ufs firstLengthDelimited:1]);
  XCTAssertEqualObjects(DataFromCStr("foo2"), [ufs firstLengthDelimited:14]);
  XCTAssertNil([ufs firstLengthDelimited:11]);  // Different type
  XCTAssertNil([ufs firstLengthDelimited:99]);  // Not present

  XCTAssertTrue(group1 == [ufs firstGroup:1]);   // Testing ptr, exact object
  XCTAssertTrue(group3 == [ufs firstGroup:15]);  // Testing ptr, exact object
  XCTAssertNil([ufs firstGroup:11]);             // Different type
  XCTAssertNil([ufs firstGroup:99]);             // Not present
}

- (void)testGetFields {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:1];
  [ufs addFieldNumber:2 varint:2];
  [ufs addFieldNumber:1 fixed32:3];
  [ufs addFieldNumber:2 fixed32:4];
  [ufs addFieldNumber:1 fixed64:5];
  [ufs addFieldNumber:3 fixed64:6];
  [ufs addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs addFieldNumber:2 lengthDelimited:DataFromCStr("bar")];
  GPBUnknownFields* group1 = [ufs addGroupWithFieldNumber:1];
  GPBUnknownFields* group2 = [ufs addGroupWithFieldNumber:3];

  NSArray<GPBUnknownField*>* fields1 = [ufs fields:1];
  XCTAssertEqual(fields1.count, 5);
  GPBUnknownField* field = fields1[0];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeVarint);
  XCTAssertEqual(field.varint, 1);
  field = fields1[1];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed32);
  XCTAssertEqual(field.fixed32, 3);
  field = fields1[2];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed64);
  XCTAssertEqual(field.fixed64, 5);
  field = fields1[3];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeLengthDelimited);
  XCTAssertEqualObjects(field.lengthDelimited, DataFromCStr("foo"));
  field = fields1[4];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeGroup);
  XCTAssertTrue(field.group == group1);  // Exact object.

  NSArray<GPBUnknownField*>* fields2 = [ufs fields:2];
  XCTAssertEqual(fields2.count, 3);
  field = fields2[0];
  XCTAssertEqual(field.number, 2);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeVarint);
  XCTAssertEqual(field.varint, 2);
  field = fields2[1];
  XCTAssertEqual(field.number, 2);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed32);
  XCTAssertEqual(field.fixed32, 4);
  field = fields2[2];
  XCTAssertEqual(field.number, 2);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeLengthDelimited);
  XCTAssertEqualObjects(field.lengthDelimited, DataFromCStr("bar"));

  NSArray<GPBUnknownField*>* fields3 = [ufs fields:3];
  XCTAssertEqual(fields3.count, 2);
  field = fields3[0];
  XCTAssertEqual(field.number, 3);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed64);
  XCTAssertEqual(field.fixed64, 6);
  field = fields3[1];
  XCTAssertEqual(field.number, 3);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeGroup);
  XCTAssertTrue(field.group == group2);  // Exact object.

  XCTAssertNil([ufs fields:99]);  // Not present
}

- (void)testFastEnumeration {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:1];
  [ufs addFieldNumber:2 varint:2];
  [ufs addFieldNumber:1 fixed32:3];
  [ufs addFieldNumber:2 fixed32:4];
  [ufs addFieldNumber:1 fixed64:5];
  [ufs addFieldNumber:3 fixed64:6];
  [ufs addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs addFieldNumber:2 lengthDelimited:DataFromCStr("bar")];
  GPBUnknownFields* group1 = [ufs addGroupWithFieldNumber:1];
  GPBUnknownFields* group2 = [ufs addGroupWithFieldNumber:3];

  // The order added nothing to do with field numbers.
  NSInteger loop = 0;
  for (GPBUnknownField* field in ufs) {
    ++loop;
    switch (loop) {
      case 1:
        XCTAssertEqual(field.number, 1);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeVarint);
        XCTAssertEqual(field.varint, 1);
        break;
      case 2:
        XCTAssertEqual(field.number, 2);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeVarint);
        XCTAssertEqual(field.varint, 2);
        break;
      case 3:
        XCTAssertEqual(field.number, 1);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed32);
        XCTAssertEqual(field.fixed32, 3);
        break;
      case 4:
        XCTAssertEqual(field.number, 2);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed32);
        XCTAssertEqual(field.fixed32, 4);
        break;
      case 5:
        XCTAssertEqual(field.number, 1);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed64);
        XCTAssertEqual(field.fixed64, 5);
        break;
      case 6:
        XCTAssertEqual(field.number, 3);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed64);
        XCTAssertEqual(field.fixed64, 6);
        break;
      case 7:
        XCTAssertEqual(field.number, 1);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeLengthDelimited);
        XCTAssertEqualObjects(field.lengthDelimited, DataFromCStr("foo"));
        break;
      case 8:
        XCTAssertEqual(field.number, 2);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeLengthDelimited);
        XCTAssertEqualObjects(field.lengthDelimited, DataFromCStr("bar"));
        break;
      case 9:
        XCTAssertEqual(field.number, 1);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeGroup);
        XCTAssertTrue(field.group == group1);  // Exact object.
        break;
      case 10:
        XCTAssertEqual(field.number, 3);
        XCTAssertEqual(field.type, GPBUnknownFieldTypeGroup);
        XCTAssertTrue(field.group == group2);  // Exact object.
        break;
      default:
        XCTFail(@"Unexpected");
        break;
    }
  }
  XCTAssertEqual(loop, 10);
}

- (void)testDescriptions {
  // Exercise description for completeness.
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:1];
  [ufs addFieldNumber:2 fixed32:1];
  [ufs addFieldNumber:1 fixed64:1];
  [ufs addFieldNumber:4 lengthDelimited:DataFromCStr("foo")];
  [[ufs addGroupWithFieldNumber:5] addFieldNumber:10 varint:10];
  XCTAssertTrue(ufs.description.length > 10);
  for (GPBUnknownField* field in ufs) {
    XCTAssertTrue(field.description.length > 10);
  }
}

- (void)testInvalidFieldNumbers {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];

  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:0 varint:1], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:0 fixed32:1], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:0 fixed64:1], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:0 lengthDelimited:[NSData data]], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addGroupWithFieldNumber:0], NSException,
                               NSInvalidArgumentException);

  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:-1 varint:1], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:-1 fixed32:1], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:-1 fixed64:1], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addFieldNumber:-1 lengthDelimited:[NSData data]], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs addGroupWithFieldNumber:-1], NSException,
                               NSInvalidArgumentException);

  uint64_t varint;
  uint32_t fixed32;
  uint64_t fixed64;
  XCTAssertThrowsSpecificNamed([ufs getFirst:0 varint:&varint], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs getFirst:0 fixed32:&fixed32], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs getFirst:0 fixed64:&fixed64], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs firstLengthDelimited:0], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs firstGroup:0], NSException, NSInvalidArgumentException);

  XCTAssertThrowsSpecificNamed([ufs getFirst:-1 varint:&varint], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs getFirst:-1 fixed32:&fixed32], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs getFirst:-1 fixed64:&fixed64], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs firstLengthDelimited:-1], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs firstGroup:-1], NSException, NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs fields:0], NSException, NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([ufs fields:-1], NSException, NSInvalidArgumentException);
}

@end
