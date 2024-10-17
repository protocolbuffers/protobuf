// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBTestUtilities.h"
#import "GPBUnknownField.h"
#import "GPBUnknownFields.h"
#import "GPBUnknownFields_PackagePrivate.h"
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
  // This also calls the methods on the `GPBUnknownField` objects for completeness and to
  // make any failure in that class easier to notice/debug.

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
  GPBUnknownField* field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  GPBUnknownField* field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Fixed32

  [ufs1 addFieldNumber:2 fixed32:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  [ufs2 addFieldNumber:2 fixed32:1];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);
  field1 = [[ufs1 fields:2] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:2] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Fixed64

  [ufs1 addFieldNumber:3 fixed64:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  [ufs2 addFieldNumber:3 fixed64:1];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);
  field1 = [[ufs1 fields:3] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:3] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
  XCTAssertEqual([field1 hash], [field2 hash]);

  // LengthDelimited

  [ufs1 addFieldNumber:4 lengthDelimited:DataFromCStr("foo")];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  [ufs2 addFieldNumber:4 lengthDelimited:DataFromCStr("foo")];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);
  field1 = [[ufs1 fields:4] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:4] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
  XCTAssertEqual([field1 hash], [field2 hash]);

  // Group

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:5];
  [group1 addFieldNumber:10 varint:10];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  GPBUnknownFields* group2 = [ufs2 addGroupWithFieldNumber:5];
  [group2 addFieldNumber:10 varint:10];
  XCTAssertEqualObjects(ufs1, ufs2);
  XCTAssertEqual([ufs1 hash], [ufs2 hash]);
  field1 = [[ufs1 fields:5] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:5] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
  XCTAssertEqual([field1 hash], [field2 hash]);
}

- (void)testInequality_Values {
  // Same field number and type, different values.

  // This also calls the methods on the `GPBUnknownField` objects for completeness and to
  // make any failure in that class easier to notice/debug.

  GPBUnknownFields* ufs1 = [[[GPBUnknownFields alloc] init] autorelease];
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];

  [ufs1 addFieldNumber:1 varint:1];
  [ufs2 addFieldNumber:1 varint:2];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  GPBUnknownField* field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  GPBUnknownField* field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed32:1];
  [ufs2 addFieldNumber:1 fixed32:2];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed64:1];
  [ufs2 addFieldNumber:1 fixed64:2];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs2 addFieldNumber:1 lengthDelimited:DataFromCStr("bar")];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:1];
  GPBUnknownFields* group2 = [ufs2 addGroupWithFieldNumber:1];
  [group1 addFieldNumber:10 varint:10];
  [group2 addFieldNumber:10 varint:20];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  XCTAssertNotEqualObjects(group1, group2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
}

- (void)testInequality_FieldNumbers {
  // Same type and value, different field numbers.

  // This also calls the methods on the `GPBUnknownField` objects for completeness and to
  // make any failure in that class easier to notice/debug.

  GPBUnknownFields* ufs1 = [[[GPBUnknownFields alloc] init] autorelease];
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];

  [ufs1 addFieldNumber:1 varint:1];
  [ufs2 addFieldNumber:2 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  GPBUnknownField* field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  GPBUnknownField* field2 = [[ufs2 fields:2] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed32:1];
  [ufs2 addFieldNumber:2 fixed32:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:2] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed64:1];
  [ufs2 addFieldNumber:2 fixed64:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:2] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs2 addFieldNumber:2 lengthDelimited:DataFromCStr("fod")];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:2] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:1];
  GPBUnknownFields* group2 = [ufs2 addGroupWithFieldNumber:2];
  [group1 addFieldNumber:10 varint:10];
  [group2 addFieldNumber:10 varint:10];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  XCTAssertEqualObjects(group1, group2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:2] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
}

- (void)testInequality_Types {
  // Same field number and value when possible, different types.

  // This also calls the methods on the `GPBUnknownField` objects for completeness and to
  // make any failure in that class easier to notice/debug.

  GPBUnknownFields* ufs1 = [[[GPBUnknownFields alloc] init] autorelease];
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];

  [ufs1 addFieldNumber:1 varint:1];
  [ufs2 addFieldNumber:1 fixed32:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  GPBUnknownField* field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  GPBUnknownField* field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed32:1];
  [ufs2 addFieldNumber:1 fixed64:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 fixed64:1];
  [ufs2 addFieldNumber:1 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  [ufs1 addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
  [ufs2 addFieldNumber:1 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.

  [ufs1 clear];
  [ufs2 clear];
  XCTAssertEqualObjects(ufs1, ufs2);

  GPBUnknownFields* group1 = [ufs1 addGroupWithFieldNumber:1];
  [group1 addFieldNumber:10 varint:10];
  [ufs2 addFieldNumber:1 varint:1];
  XCTAssertNotEqualObjects(ufs1, ufs2);
  field1 = [[ufs1 fields:1] firstObject];
  XCTAssertNotNil(field1);
  field2 = [[ufs2 fields:1] firstObject];
  XCTAssertNotNil(field2);
  XCTAssertNotEqualObjects(field1, field2);
  XCTAssertTrue(field1 != field2);  // Different objects.
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

- (void)testRemoveField {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:1];
  [ufs addFieldNumber:1 fixed32:1];
  [ufs addFieldNumber:1 fixed64:1];
  XCTAssertEqual(ufs.count, 3);

  NSArray<GPBUnknownField*>* fields = [ufs fields:1];
  XCTAssertEqual(fields.count, 3);
  GPBUnknownField* field = fields[0];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeVarint);
  XCTAssertEqual(field.varint, 1);
  [ufs removeField:field];  // Remove first (varint)
  XCTAssertEqual(ufs.count, 2);

  fields = [ufs fields:1];
  XCTAssertEqual(fields.count, 2);
  field = fields[0];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed32);
  field = fields[1];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed64);
  [ufs removeField:field];  // Remove the second (fixed64)
  XCTAssertEqual(ufs.count, 1);

  fields = [ufs fields:1];
  XCTAssertEqual(fields.count, 1);
  field = fields[0];
  XCTAssertEqual(field.number, 1);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed32);

  field = [[field retain] autorelease];  // Hold on to this last one.
  [ufs removeField:field];               // Remove the last one (fixed32)
  XCTAssertEqual(ufs.count, 0);

  // Trying to remove something not in the set should fail.
  XCTAssertThrowsSpecificNamed([ufs removeField:field], NSException, NSInvalidArgumentException);
}

- (void)testClearFieldNumber {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:1];
  [ufs addFieldNumber:2 fixed32:2];
  [ufs addFieldNumber:1 fixed64:1];
  [ufs addFieldNumber:3 varint:3];
  XCTAssertEqual(ufs.count, 4);

  [ufs clearFieldNumber:999];  // Not present, noop.
  XCTAssertEqual(ufs.count, 4);

  [ufs clearFieldNumber:1];  // Should remove slot zero and slot two.
  XCTAssertEqual(ufs.count, 2);
  NSArray<GPBUnknownField*>* fields = [ufs fields:2];
  XCTAssertEqual(fields.count, 1);
  GPBUnknownField* field = fields[0];
  XCTAssertEqual(field.number, 2);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeFixed32);
  XCTAssertEqual(field.fixed32, 2);
  fields = [ufs fields:3];
  XCTAssertEqual(fields.count, 1);
  field = fields[0];
  XCTAssertEqual(field.number, 3);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeVarint);
  XCTAssertEqual(field.varint, 3);

  [ufs clearFieldNumber:2];  // Should remove slot one.
  fields = [ufs fields:3];
  XCTAssertEqual(fields.count, 1);
  field = fields[0];
  XCTAssertEqual(field.number, 3);
  XCTAssertEqual(field.type, GPBUnknownFieldTypeVarint);
  XCTAssertEqual(field.varint, 3);
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

- (void)testAddCopyOfField {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:10];
  [ufs addFieldNumber:2 fixed32:11];
  [ufs addFieldNumber:3 fixed64:12];
  [ufs addFieldNumber:4 lengthDelimited:DataFromCStr("foo")];
  GPBUnknownFields* group = [ufs addGroupWithFieldNumber:5];
  [group addFieldNumber:10 varint:100];
  GPBUnknownFields* subGroup = [group addGroupWithFieldNumber:100];
  [subGroup addFieldNumber:50 varint:50];

  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] init] autorelease];
  for (GPBUnknownField* field in ufs) {
    GPBUnknownField* field2 = [ufs2 addCopyOfField:field];
    XCTAssertEqualObjects(field, field2);
    if (field.type == GPBUnknownFieldTypeGroup) {
      // Group does a copy because the `.group` value is mutable.
      XCTAssertTrue(field != field2);        // Pointer comparison.
      XCTAssertTrue(group != field2.group);  // Pointer comparison.
      XCTAssertEqualObjects(group, field2.group);
      GPBUnknownFields* subGroupAdded = [field2.group firstGroup:100];
      XCTAssertTrue(subGroupAdded != subGroup);  // Pointer comparison.
      XCTAssertEqualObjects(subGroupAdded, subGroup);
    } else {
      // All other types are immutable, so they use the same object.
      XCTAssertTrue(field == field2);  // Pointer comparision.
    }
  }
  XCTAssertEqualObjects(ufs, ufs2);
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

- (void)testCopy {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:1];
  [ufs addFieldNumber:2 fixed32:2];
  [ufs addFieldNumber:3 fixed64:3];
  [ufs addFieldNumber:4 lengthDelimited:DataFromCStr("foo")];
  GPBUnknownFields* group = [ufs addGroupWithFieldNumber:5];
  [group addFieldNumber:10 varint:10];
  GPBUnknownFields* subGroup = [group addGroupWithFieldNumber:100];
  [subGroup addFieldNumber:20 varint:20];

  GPBUnknownFields* ufs2 = [[ufs copy] autorelease];
  XCTAssertTrue(ufs != ufs2);        // Different objects
  XCTAssertEqualObjects(ufs, ufs2);  // Equal contents
  // All field objects but the group should be the same since they are immutable.
  XCTAssertTrue([[ufs fields:1] firstObject] == [[ufs2 fields:1] firstObject]);  // Same object
  XCTAssertTrue([[ufs fields:2] firstObject] == [[ufs2 fields:2] firstObject]);  // Same object
  XCTAssertTrue([[ufs fields:3] firstObject] == [[ufs2 fields:3] firstObject]);  // Same object
  XCTAssertTrue([[ufs fields:4] firstObject] == [[ufs2 fields:4] firstObject]);  // Same object
  XCTAssertTrue([[ufs fields:4] firstObject].lengthDelimited ==
                [[ufs2 fields:4] firstObject].lengthDelimited);  // Same object
  // Since the group holds another `GPBUnknownFields` object (which is mutable), it will be a
  // different object.
  XCTAssertTrue([[ufs fields:5] firstObject] != [[ufs2 fields:5] firstObject]);
  XCTAssertTrue(group != [[ufs2 fields:5] firstObject].group);
  XCTAssertEqualObjects(group, [[ufs2 fields:5] firstObject].group);
  // And confirm that copy went deep so the nested group also is a different object.
  GPBUnknownFields* groupCopied = [[ufs2 fields:5] firstObject].group;
  XCTAssertTrue([[group fields:100] firstObject] != [[groupCopied fields:100] firstObject]);
  XCTAssertTrue(subGroup != [[groupCopied fields:100] firstObject].group);
  XCTAssertEqualObjects(subGroup, [[groupCopied fields:100] firstObject].group);
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

- (void)testSerialize {
  // Don't need to test CodedOutputStream, just make sure things basically end up there.
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    XCTAssertEqualObjects([ufs serializeAsData], [NSData data]);
  }
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:1 varint:1];
    XCTAssertEqualObjects([ufs serializeAsData], DataFromBytes(0x08, 0x01));
  }
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:1 fixed32:1];
    XCTAssertEqualObjects([ufs serializeAsData], DataFromBytes(0x0d, 0x01, 0x00, 0x00, 0x00));
  }
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:1 fixed64:1];
    XCTAssertEqualObjects([ufs serializeAsData],
                          DataFromBytes(0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
  }
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:1 lengthDelimited:DataFromCStr("foo")];
    XCTAssertEqualObjects([ufs serializeAsData], DataFromBytes(0x0a, 0x03, 0x66, 0x6f, 0x6f));
  }
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addGroupWithFieldNumber:1];  // Empty group
    XCTAssertEqualObjects([ufs serializeAsData], DataFromBytes(0x0b, 0x0c));
  }
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    GPBUnknownFields* group = [ufs addGroupWithFieldNumber:1];  // With some fields
    [group addFieldNumber:10 varint:10];
    [group addFieldNumber:11 fixed32:32];
    [group addFieldNumber:12 fixed32:32];
    XCTAssertEqualObjects([ufs serializeAsData],
                          DataFromBytes(0x0b, 0x50, 0x0a, 0x5d, 0x20, 0x00, 0x00, 0x00, 0x65, 0x20,
                                        0x00, 0x00, 0x00, 0x0c));
  }
}

- (void)testMessageMergeUnknowns {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:TestAllTypes_FieldNumber_OptionalInt64 varint:100];
  [ufs addFieldNumber:TestAllTypes_FieldNumber_OptionalFixed32 fixed32:200];
  [ufs addFieldNumber:TestAllTypes_FieldNumber_OptionalFixed64 fixed64:300];
  [ufs addFieldNumber:TestAllTypes_FieldNumber_OptionalBytes lengthDelimited:DataFromCStr("foo")];
  GPBUnknownFields* group = [ufs addGroupWithFieldNumber:TestAllTypes_FieldNumber_OptionalGroup];
  [group addFieldNumber:TestAllTypes_OptionalGroup_FieldNumber_A varint:55];
  [ufs addFieldNumber:123456 varint:4321];
  [group addFieldNumber:123456 varint:5432];

  TestAllTypes* msg = [TestAllTypes message];
  XCTAssertTrue([msg mergeUnknownFields:ufs extensionRegistry:nil error:NULL]);
  XCTAssertEqual(msg.optionalInt64, 100);
  XCTAssertEqual(msg.optionalFixed32, 200);
  XCTAssertEqual(msg.optionalFixed64, 300);
  XCTAssertEqualObjects(msg.optionalBytes, DataFromCStr("foo"));
  XCTAssertEqual(msg.optionalGroup.a, 55);
  GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] initFromMessage:msg] autorelease];
  XCTAssertEqual(ufs2.count, 1);  // The unknown at the root
  uint64_t varint = 0;
  XCTAssertTrue([ufs2 getFirst:123456 varint:&varint]);
  XCTAssertEqual(varint, 4321);
  GPBUnknownFields* ufs2group =
      [[[GPBUnknownFields alloc] initFromMessage:msg.optionalGroup] autorelease];
  XCTAssertEqual(ufs2group.count, 1);  // The unknown at in group
  XCTAssertTrue([ufs2group getFirst:123456 varint:&varint]);
  XCTAssertEqual(varint, 5432);

  TestEmptyMessage* emptyMessage = [TestEmptyMessage message];
  XCTAssertTrue([emptyMessage mergeUnknownFields:ufs extensionRegistry:nil error:NULL]);
  GPBUnknownFields* ufs3 = [[[GPBUnknownFields alloc] initFromMessage:emptyMessage] autorelease];
  XCTAssertEqualObjects(ufs3, ufs);  // Round trip through an empty message got us same fields back.
  XCTAssertTrue(ufs3 != ufs);        // But they are different objects.
}

- (void)testRoundTripLotsOfFields {
  // Usage a message with everything, into an empty message to get a lot of unknown fields,
  // and confirm it comes back to match.
  TestAllTypes* allFields = [self allSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* allFieldsData = [allFields data];
  TestEmptyMessage* emptyMessage = [TestEmptyMessage parseFromData:allFieldsData error:NULL];
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] initFromMessage:emptyMessage] autorelease];
  TestAllTypes* allFields2 = [TestAllTypes message];
  XCTAssertTrue([allFields2 mergeUnknownFields:ufs extensionRegistry:nil error:NULL]);
  XCTAssertEqualObjects(allFields2, allFields);

  // Confirm that the they still all end up in unknowns when parsed into a message with extensions
  // support for the field numbers (but no registry).
  {
    TestEmptyMessageWithExtensions* msgWithExts =
        [TestEmptyMessageWithExtensions parseFromData:allFieldsData error:NULL];
    GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] initFromMessage:msgWithExts] autorelease];
    XCTAssertEqualObjects(ufs2, ufs);
  }

  // Sanity check that with the registry, they go into the extension fields.
  {
    TestAllExtensions* msgWithExts =
        [TestAllExtensions parseFromData:allFieldsData
                       extensionRegistry:[UnittestRoot extensionRegistry]
                                   error:NULL];
    GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] initFromMessage:msgWithExts] autorelease];
    XCTAssertEqual(ufs2.count, 0);
  }
}

- (void)testMismatchedFieldTypes {
  // Start with a valid set of field data, and map it into unknown fields.
  TestAllTypes* allFields = [self allSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData* allFieldsData = [allFields data];
  TestEmptyMessage* emptyMessage = [TestEmptyMessage parseFromData:allFieldsData error:NULL];
  GPBUnknownFields* ufsRightTypes =
      [[[GPBUnknownFields alloc] initFromMessage:emptyMessage] autorelease];

  // Now build a new set of unknown fields where all the data types are wrong for the original
  // fields.
  GPBUnknownFields* ufsWrongTypes = [[[GPBUnknownFields alloc] init] autorelease];
  for (GPBUnknownField* field in ufsRightTypes) {
    if (field.type != GPBUnknownFieldTypeVarint) {
      // Original field is not a varint, so use a varint.
      [ufsWrongTypes addFieldNumber:field.number varint:1];
    } else {
      // Original field *is* a varint, so use something else.
      [ufsWrongTypes addFieldNumber:field.number fixed32:1];
    }
  }

  // Parse into a message with the field numbers, the wrong types should force everything into
  // unknown fields again.
  {
    TestAllTypes* msg = [TestAllTypes message];
    XCTAssertTrue([msg mergeUnknownFields:ufsWrongTypes extensionRegistry:nil error:NULL]);
    GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] initFromMessage:msg] autorelease];
    XCTAssertFalse(ufs2.empty);
    XCTAssertEqualObjects(ufs2, ufsWrongTypes);  // All back as unknown fields.
  }

  // Parse into a message with extension registiry, the wrong types should still force everything
  // into unknown fields.
  {
    TestAllExtensions* msg = [TestAllExtensions message];
    XCTAssertTrue([msg mergeUnknownFields:ufsWrongTypes
                        extensionRegistry:[UnittestRoot extensionRegistry]
                                    error:NULL]);
    GPBUnknownFields* ufs2 = [[[GPBUnknownFields alloc] initFromMessage:msg] autorelease];
    XCTAssertFalse(ufs2.empty);
    XCTAssertEqualObjects(ufs2, ufsWrongTypes);  // All back as unknown fields.
  }
}

- (void)testMergeFailures {
  // Valid data, pushes to the fields just fine.
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_OptionalString
        lengthDelimited:DataFromCStr("abc")];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_RepeatedInt32Array
        lengthDelimited:DataFromBytes(0x01, 0x02)];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_RepeatedFixed32Array
        lengthDelimited:DataFromBytes(0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00)];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_RepeatedFixed64Array
        lengthDelimited:DataFromBytes(0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00)];
    TestAllTypes* msg = [TestAllTypes message];
    NSError* error = nil;
    XCTAssertTrue([msg mergeUnknownFields:ufs extensionRegistry:nil error:&error]);
    XCTAssertNil(error);
    XCTAssertEqualObjects(msg.optionalString, @"abc");
    XCTAssertEqual(msg.repeatedInt32Array.count, 2);
    XCTAssertEqual([msg.repeatedInt32Array valueAtIndex:0], 1);
    XCTAssertEqual([msg.repeatedInt32Array valueAtIndex:1], 2);
    XCTAssertEqual(msg.repeatedFixed32Array.count, 2);
    XCTAssertEqual([msg.repeatedFixed32Array valueAtIndex:0], 3);
    XCTAssertEqual([msg.repeatedFixed32Array valueAtIndex:1], 4);
    XCTAssertEqual(msg.repeatedFixed64Array.count, 2);
    XCTAssertEqual([msg.repeatedFixed64Array valueAtIndex:0], 5);
    XCTAssertEqual([msg.repeatedFixed64Array valueAtIndex:1], 6);
  }

  // Invalid UTF-8 causes a failure when pushed to the message.
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_OptionalString
        lengthDelimited:DataFromBytes(0xC2, 0xF2, 0x0, 0x0, 0x0)];
    TestAllTypes* msg = [TestAllTypes message];
    NSError* error = nil;
    XCTAssertFalse([msg mergeUnknownFields:ufs extensionRegistry:nil error:&error]);
    XCTAssertNotNil(error);
  }

  // Invalid packed varint causes a failure when pushed to the message.
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_RepeatedInt32Array
        lengthDelimited:DataFromBytes(0xff)];  // Invalid varint
    TestAllTypes* msg = [TestAllTypes message];
    NSError* error = nil;
    XCTAssertFalse([msg mergeUnknownFields:ufs extensionRegistry:nil error:&error]);
    XCTAssertNotNil(error);
  }

  // Invalid packed fixed32 causes a failure when pushed to the message.
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_RepeatedFixed32Array
        lengthDelimited:DataFromBytes(0x01, 0x00, 0x00)];  // Truncated fixed32
    TestAllTypes* msg = [TestAllTypes message];
    NSError* error = nil;
    XCTAssertFalse([msg mergeUnknownFields:ufs extensionRegistry:nil error:&error]);
    XCTAssertNotNil(error);
  }

  // Invalid packed fixed64 causes a failure when pushed to the message.
  {
    GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
    [ufs addFieldNumber:TestAllTypes_FieldNumber_RepeatedFixed64Array
        lengthDelimited:DataFromBytes(0x01, 0x00, 0x00, 0x00, 0x00)];  // Truncated fixed64
    TestAllTypes* msg = [TestAllTypes message];
    NSError* error = nil;
    XCTAssertFalse([msg mergeUnknownFields:ufs extensionRegistry:nil error:&error]);
    XCTAssertNotNil(error);
  }
}

- (void)testLargeVarint {
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:1 varint:0x7FFFFFFFFFFFFFFFL];

  TestEmptyMessage* emptyMessage = [TestEmptyMessage message];
  XCTAssertTrue([emptyMessage mergeUnknownFields:ufs extensionRegistry:nil error:NULL]);

  GPBUnknownFields* ufsParsed =
      [[[GPBUnknownFields alloc] initFromMessage:emptyMessage] autorelease];
  XCTAssertEqual(ufsParsed.count, 1);
  uint64_t varint = 0;
  XCTAssertTrue([ufsParsed getFirst:1 varint:&varint]);
  XCTAssertEqual(varint, 0x7FFFFFFFFFFFFFFFL);
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
  GPBUnknownFields* ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  GPBUnknownFields* group = [ufs firstGroup:4];
  XCTAssertTrue(group.empty);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 8, 1, 36) error:NULL];  // varint
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  XCTAssertEqual(group.count, (NSUInteger)1);
  uint64_t varint = 0;
  XCTAssertTrue([group getFirst:1 varint:&varint]);
  XCTAssertEqual(varint, 1);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 21, 0x78, 0x56, 0x34, 0x12, 36)
                                error:NULL];  // fixed32
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  XCTAssertEqual(group.count, (NSUInteger)1);
  uint32_t fixed32 = 0;
  XCTAssertTrue([group getFirst:2 fixed32:&fixed32]);
  XCTAssertEqual(fixed32, 0x12345678);

  m = [TestEmptyMessage
      parseFromData:DataFromBytes(35, 25, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                                  36)
              error:NULL];  // fixed64
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  XCTAssertEqual(group.count, (NSUInteger)1);
  uint64_t fixed64 = 0;
  XCTAssertTrue([group getFirst:3 fixed64:&fixed64]);
  XCTAssertEqual(fixed64, 0x123456789abcdef0LL);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 50, 0, 36)
                                error:NULL];  // length delimited, length 0
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  XCTAssertEqual(group.count, (NSUInteger)1);
  NSData* lengthDelimited = [group firstLengthDelimited:6];
  XCTAssertEqualObjects(lengthDelimited, [NSData data]);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 50, 1, 42, 36)
                                error:NULL];  // length delimited, length 1, byte 42
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  XCTAssertEqual(group.count, (NSUInteger)1);
  lengthDelimited = [group firstLengthDelimited:6];
  XCTAssertEqualObjects(lengthDelimited, DataFromBytes(42));

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 43, 44, 36) error:NULL];  // Sub group
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  XCTAssertEqual(group.count, (NSUInteger)1);
  group = [group firstGroup:5];
  XCTAssertTrue(group.empty);

  m = [TestEmptyMessage parseFromData:DataFromBytes(35, 8, 1, 43, 8, 2, 44, 36)
                                error:NULL];  // varint and sub group with varint
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  XCTAssertEqual(group.count, (NSUInteger)2);
  varint = 0;
  XCTAssertTrue([group getFirst:1 varint:&varint]);
  XCTAssertEqual(varint, 1);
  group = [group firstGroup:5];
  XCTAssertEqual(group.count, (NSUInteger)1);
  XCTAssertTrue([group getFirst:1 varint:&varint]);
  XCTAssertEqual(varint, 2);

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

  // Missing end group
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
  ufs = [[[GPBUnknownFields alloc] initFromMessage:m] autorelease];
  XCTAssertEqual(ufs.count, (NSUInteger)1);
  group = [ufs firstGroup:4];
  for (NSUInteger i = 1; i < kDefaultRecursionLimit; ++i) {
    XCTAssertEqual(group.count, (NSUInteger)1);
    group = [group firstGroup:4];
  }
  // group is now the inner most group.
  XCTAssertEqual(group.count, (NSUInteger)1);
  varint = 0;
  XCTAssertTrue([group getFirst:1 varint:&varint]);
  XCTAssertEqual(varint, 1);
  // One more level deep fails.
  testData = DataForGroupsOfDepth(kDefaultRecursionLimit + 1);
  XCTAssertNil([TestEmptyMessage parseFromData:testData error:NULL]);
}

@end
