// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

#import <objc/runtime.h>

#import "GPBDescriptor_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestObjc.pbobjc.h"
#import "objectivec/Tests/UnittestObjcOptions.pbobjc.h"

@interface DescriptorTests : GPBTestCase
@end

@implementation DescriptorTests

- (void)testDescriptor_containingType {
  GPBDescriptor *testAllTypesDesc = [TestAllTypes descriptor];
  GPBDescriptor *nestedMessageDesc = [TestAllTypes_NestedMessage descriptor];
  XCTAssertNil(testAllTypesDesc.containingType);
  XCTAssertNotNil(nestedMessageDesc.containingType);
  XCTAssertEqual(nestedMessageDesc.containingType, testAllTypesDesc);  // Ptr comparison
}

- (void)testDescriptor_fullName {
  GPBDescriptor *testAllTypesDesc = [TestAllTypes descriptor];
  XCTAssertEqualObjects(testAllTypesDesc.fullName, @"objc.protobuf.tests.TestAllTypes");
  GPBDescriptor *nestedMessageDesc = [TestAllTypes_NestedMessage descriptor];
  XCTAssertEqualObjects(nestedMessageDesc.fullName,
                        @"objc.protobuf.tests.TestAllTypes.NestedMessage");

  // Prefixes removed.
  GPBDescriptor *descDesc = [GPBTESTPrefixedParentMessage descriptor];
  XCTAssertEqualObjects(descDesc.fullName, @"objc.protobuf.tests.options.PrefixedParentMessage");
  GPBDescriptor *descExtRngDesc = [GPBTESTPrefixedParentMessage_Child descriptor];
  XCTAssertEqualObjects(descExtRngDesc.fullName,
                        @"objc.protobuf.tests.options.PrefixedParentMessage.Child");

  // Things that get "_Class" added.
  GPBDescriptor *pointDesc = [Point_Class descriptor];
  XCTAssertEqualObjects(pointDesc.fullName, @"objc.protobuf.tests.Point");
  GPBDescriptor *pointRectDesc = [Point_Rect descriptor];
  XCTAssertEqualObjects(pointRectDesc.fullName, @"objc.protobuf.tests.Point.Rect");
}

- (void)testFieldDescriptor {
  GPBDescriptor *descriptor = [TestAllTypes descriptor];

  // Nested Enum
  GPBFieldDescriptor *fieldDescriptorWithName = [descriptor fieldWithName:@"optionalNestedEnum"];
  XCTAssertNotNil(fieldDescriptorWithName);
  GPBFieldDescriptor *fieldDescriptorWithNumber = [descriptor fieldWithNumber:21];
  XCTAssertNotNil(fieldDescriptorWithNumber);
  XCTAssertEqual(fieldDescriptorWithName, fieldDescriptorWithNumber);
  XCTAssertNotNil(fieldDescriptorWithNumber.enumDescriptor);
  XCTAssertEqualObjects(fieldDescriptorWithNumber.enumDescriptor.name, @"TestAllTypes_NestedEnum");
  XCTAssertEqual(fieldDescriptorWithName.number, fieldDescriptorWithNumber.number);
  XCTAssertEqual(fieldDescriptorWithName.dataType, GPBDataTypeEnum);

  // Foreign Enum
  fieldDescriptorWithName = [descriptor fieldWithName:@"optionalForeignEnum"];
  XCTAssertNotNil(fieldDescriptorWithName);
  fieldDescriptorWithNumber = [descriptor fieldWithNumber:22];
  XCTAssertNotNil(fieldDescriptorWithNumber);
  XCTAssertEqual(fieldDescriptorWithName, fieldDescriptorWithNumber);
  XCTAssertNotNil(fieldDescriptorWithNumber.enumDescriptor);
  XCTAssertEqualObjects(fieldDescriptorWithNumber.enumDescriptor.name, @"ForeignEnum");
  XCTAssertEqual(fieldDescriptorWithName.number, fieldDescriptorWithNumber.number);
  XCTAssertEqual(fieldDescriptorWithName.dataType, GPBDataTypeEnum);

  // Import Enum
  fieldDescriptorWithName = [descriptor fieldWithName:@"optionalImportEnum"];
  XCTAssertNotNil(fieldDescriptorWithName);
  fieldDescriptorWithNumber = [descriptor fieldWithNumber:23];
  XCTAssertNotNil(fieldDescriptorWithNumber);
  XCTAssertEqual(fieldDescriptorWithName, fieldDescriptorWithNumber);
  XCTAssertNotNil(fieldDescriptorWithNumber.enumDescriptor);
  XCTAssertEqualObjects(fieldDescriptorWithNumber.enumDescriptor.name, @"ImportEnum");
  XCTAssertEqual(fieldDescriptorWithName.number, fieldDescriptorWithNumber.number);
  XCTAssertEqual(fieldDescriptorWithName.dataType, GPBDataTypeEnum);

  // Nested Message
  fieldDescriptorWithName = [descriptor fieldWithName:@"optionalNestedMessage"];
  XCTAssertNotNil(fieldDescriptorWithName);
  fieldDescriptorWithNumber = [descriptor fieldWithNumber:18];
  XCTAssertNotNil(fieldDescriptorWithNumber);
  XCTAssertEqual(fieldDescriptorWithName, fieldDescriptorWithNumber);
  XCTAssertNil(fieldDescriptorWithNumber.enumDescriptor);
  XCTAssertEqual(fieldDescriptorWithName.number, fieldDescriptorWithNumber.number);
  XCTAssertEqual(fieldDescriptorWithName.dataType, GPBDataTypeMessage);

  // Foreign Message
  fieldDescriptorWithName = [descriptor fieldWithName:@"optionalForeignMessage"];
  XCTAssertNotNil(fieldDescriptorWithName);
  fieldDescriptorWithNumber = [descriptor fieldWithNumber:19];
  XCTAssertNotNil(fieldDescriptorWithNumber);
  XCTAssertEqual(fieldDescriptorWithName, fieldDescriptorWithNumber);
  XCTAssertNil(fieldDescriptorWithNumber.enumDescriptor);
  XCTAssertEqual(fieldDescriptorWithName.number, fieldDescriptorWithNumber.number);
  XCTAssertEqual(fieldDescriptorWithName.dataType, GPBDataTypeMessage);

  // Import Message
  fieldDescriptorWithName = [descriptor fieldWithName:@"optionalImportMessage"];
  XCTAssertNotNil(fieldDescriptorWithName);
  fieldDescriptorWithNumber = [descriptor fieldWithNumber:20];
  XCTAssertNotNil(fieldDescriptorWithNumber);
  XCTAssertEqual(fieldDescriptorWithName, fieldDescriptorWithNumber);
  XCTAssertNil(fieldDescriptorWithNumber.enumDescriptor);
  XCTAssertEqual(fieldDescriptorWithName.number, fieldDescriptorWithNumber.number);
  XCTAssertEqual(fieldDescriptorWithName.dataType, GPBDataTypeMessage);

  // Some failed lookups.
  XCTAssertNil([descriptor fieldWithName:@"NOT THERE"]);
  XCTAssertNil([descriptor fieldWithNumber:9876543]);
}

- (void)testEnumDescriptor {
  GPBEnumDescriptor *descriptor = TestAllTypes_NestedEnum_EnumDescriptor();

  NSString *enumName = [descriptor enumNameForValue:1];
  XCTAssertNotNil(enumName);
  int32_t value;
  XCTAssertTrue([descriptor getValue:&value forEnumName:@"TestAllTypes_NestedEnum_Foo"]);
  XCTAssertTrue([descriptor getValue:NULL forEnumName:@"TestAllTypes_NestedEnum_Foo"]);
  XCTAssertEqual(value, TestAllTypes_NestedEnum_Foo);

  enumName = [descriptor enumNameForValue:2];
  XCTAssertNotNil(enumName);
  XCTAssertTrue([descriptor getValue:&value forEnumName:@"TestAllTypes_NestedEnum_Bar"]);
  XCTAssertEqual(value, TestAllTypes_NestedEnum_Bar);

  enumName = [descriptor enumNameForValue:3];
  XCTAssertNotNil(enumName);
  XCTAssertTrue([descriptor getValue:&value forEnumName:@"TestAllTypes_NestedEnum_Baz"]);
  XCTAssertEqual(value, TestAllTypes_NestedEnum_Baz);

  // TextFormat
  enumName = [descriptor textFormatNameForValue:1];
  XCTAssertNotNil(enumName);
  XCTAssertTrue([descriptor getValue:&value forEnumTextFormatName:@"FOO"]);
  XCTAssertEqual(value, TestAllTypes_NestedEnum_Foo);
  XCTAssertNil([descriptor textFormatNameForValue:99999]);

  // Bad values
  enumName = [descriptor enumNameForValue:0];
  XCTAssertNil(enumName);
  XCTAssertFalse([descriptor getValue:&value forEnumName:@"Unknown"]);
  XCTAssertFalse([descriptor getValue:NULL forEnumName:@"Unknown"]);
  XCTAssertFalse([descriptor getValue:&value forEnumName:@"TestAllTypes_NestedEnum_Unknown"]);
  XCTAssertFalse([descriptor getValue:NULL forEnumName:@"TestAllTypes_NestedEnum_Unknown"]);
  XCTAssertFalse([descriptor getValue:NULL forEnumTextFormatName:@"Unknown"]);
  XCTAssertFalse([descriptor getValue:&value forEnumTextFormatName:@"Unknown"]);
}

- (void)testEnumDescriptorIntrospection {
  GPBEnumDescriptor *descriptor = TestAllTypes_NestedEnum_EnumDescriptor();

  XCTAssertEqual(descriptor.enumNameCount, 4U);
  XCTAssertEqualObjects([descriptor getEnumNameForIndex:0], @"TestAllTypes_NestedEnum_Foo");
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:0], @"FOO");
  XCTAssertEqualObjects([descriptor getEnumNameForIndex:1], @"TestAllTypes_NestedEnum_Bar");
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:1], @"BAR");
  XCTAssertEqualObjects([descriptor getEnumNameForIndex:2], @"TestAllTypes_NestedEnum_Baz");
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:2], @"BAZ");
  XCTAssertEqualObjects([descriptor getEnumNameForIndex:3], @"TestAllTypes_NestedEnum_Neg");
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:3], @"NEG");
}

- (void)testEnumDescriptorIntrospectionWithAlias {
  GPBEnumDescriptor *descriptor = TestEnumWithDupValue_EnumDescriptor();
  NSString *enumName;
  int32_t value;

  XCTAssertEqual(descriptor.enumNameCount, 5U);

  enumName = [descriptor getEnumNameForIndex:0];
  XCTAssertEqualObjects(enumName, @"TestEnumWithDupValue_Foo1");
  XCTAssertTrue([descriptor getValue:&value forEnumName:enumName]);
  XCTAssertEqual(value, 1);
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:0], @"FOO1");

  enumName = [descriptor getEnumNameForIndex:1];
  XCTAssertEqualObjects(enumName, @"TestEnumWithDupValue_Bar1");
  XCTAssertTrue([descriptor getValue:&value forEnumName:enumName]);
  XCTAssertEqual(value, 2);
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:1], @"BAR1");

  enumName = [descriptor getEnumNameForIndex:2];
  XCTAssertEqualObjects(enumName, @"TestEnumWithDupValue_Baz");
  XCTAssertTrue([descriptor getValue:&value forEnumName:enumName]);
  XCTAssertEqual(value, 3);
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:2], @"BAZ");

  enumName = [descriptor getEnumNameForIndex:3];
  XCTAssertEqualObjects(enumName, @"TestEnumWithDupValue_Foo2");
  XCTAssertTrue([descriptor getValue:&value forEnumName:enumName]);
  XCTAssertEqual(value, 1);
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:3], @"FOO2");

  enumName = [descriptor getEnumNameForIndex:4];
  XCTAssertEqualObjects(enumName, @"TestEnumWithDupValue_Bar2");
  XCTAssertTrue([descriptor getValue:&value forEnumName:enumName]);
  XCTAssertEqual(value, 2);
  XCTAssertEqualObjects([descriptor getEnumTextFormatNameForIndex:4], @"BAR2");
}

- (void)testEnumAliasNameCollisions {
  GPBEnumDescriptor *descriptor = TestEnumObjCNameCollision_EnumDescriptor();
  NSString *textFormatName;
  int32_t value;

  XCTAssertEqual(descriptor.enumNameCount, 5U);

  XCTAssertEqualObjects([descriptor getEnumNameForIndex:0], @"TestEnumObjCNameCollision_Foo");
  textFormatName = [descriptor getEnumTextFormatNameForIndex:0];
  XCTAssertEqualObjects(textFormatName, @"FOO");
  XCTAssertTrue([descriptor getValue:&value forEnumTextFormatName:textFormatName]);
  XCTAssertEqual(value, 1);

  XCTAssertEqualObjects([descriptor getEnumNameForIndex:1], @"TestEnumObjCNameCollision_Foo");
  textFormatName = [descriptor getEnumTextFormatNameForIndex:1];
  XCTAssertEqualObjects(textFormatName, @"foo");
  XCTAssertTrue([descriptor getValue:&value forEnumTextFormatName:textFormatName]);
  XCTAssertEqual(value, 1);

  XCTAssertEqualObjects([descriptor getEnumNameForIndex:2], @"TestEnumObjCNameCollision_Bar");
  textFormatName = [descriptor getEnumTextFormatNameForIndex:2];
  XCTAssertEqualObjects(textFormatName, @"BAR");
  XCTAssertTrue([descriptor getValue:&value forEnumTextFormatName:textFormatName]);
  XCTAssertEqual(value, 2);

  XCTAssertEqualObjects([descriptor getEnumNameForIndex:3], @"TestEnumObjCNameCollision_Mumble");
  textFormatName = [descriptor getEnumTextFormatNameForIndex:3];
  XCTAssertEqualObjects(textFormatName, @"mumble");
  XCTAssertTrue([descriptor getValue:&value forEnumTextFormatName:textFormatName]);
  XCTAssertEqual(value, 2);

  XCTAssertEqualObjects([descriptor getEnumNameForIndex:4], @"TestEnumObjCNameCollision_Mumble");
  textFormatName = [descriptor getEnumTextFormatNameForIndex:4];
  XCTAssertEqualObjects(textFormatName, @"MUMBLE");
  XCTAssertTrue([descriptor getValue:&value forEnumTextFormatName:textFormatName]);
  XCTAssertEqual(value, 2);
}

- (void)testEnumValueValidator {
  GPBDescriptor *descriptor = [TestAllTypes descriptor];
  GPBFieldDescriptor *fieldDescriptor = [descriptor fieldWithName:@"optionalNestedEnum"];

  // Valid values
  XCTAssertTrue([fieldDescriptor isValidEnumValue:1]);
  XCTAssertTrue([fieldDescriptor isValidEnumValue:2]);
  XCTAssertTrue([fieldDescriptor isValidEnumValue:3]);
  XCTAssertTrue([fieldDescriptor isValidEnumValue:-1]);

  // Invalid values
  XCTAssertFalse([fieldDescriptor isValidEnumValue:4]);
  XCTAssertFalse([fieldDescriptor isValidEnumValue:0]);
  XCTAssertFalse([fieldDescriptor isValidEnumValue:-2]);
}

- (void)testOneofDescriptor {
  GPBDescriptor *descriptor = [TestOneof2 descriptor];

  GPBFieldDescriptor *fooStringField =
      [descriptor fieldWithNumber:TestOneof2_FieldNumber_FooString];
  XCTAssertNotNil(fooStringField);
  GPBFieldDescriptor *barStringField =
      [descriptor fieldWithNumber:TestOneof2_FieldNumber_BarString];
  XCTAssertNotNil(barStringField);

  // Check the oneofs to have what is expected but not other onesofs

  GPBOneofDescriptor *oneofFoo = [descriptor oneofWithName:@"foo"];
  XCTAssertNotNil(oneofFoo);
  XCTAssertNotNil([oneofFoo fieldWithName:@"fooString"]);
  XCTAssertNil([oneofFoo fieldWithName:@"barString"]);

  GPBOneofDescriptor *oneofBar = [descriptor oneofWithName:@"bar"];
  XCTAssertNotNil(oneofBar);
  XCTAssertNil([oneofBar fieldWithName:@"fooString"]);
  XCTAssertNotNil([oneofBar fieldWithName:@"barString"]);

  // Pointer comparisons against lookups from message.

  XCTAssertEqual([oneofFoo fieldWithNumber:TestOneof2_FieldNumber_FooString], fooStringField);
  XCTAssertEqual([oneofFoo fieldWithName:@"fooString"], fooStringField);

  XCTAssertEqual([oneofBar fieldWithNumber:TestOneof2_FieldNumber_BarString], barStringField);
  XCTAssertEqual([oneofBar fieldWithName:@"barString"], barStringField);

  // Unknown oneof not found.

  XCTAssertNil([descriptor oneofWithName:@"mumble"]);
  XCTAssertNil([descriptor oneofWithName:@"Foo"]);

  // Unknown oneof item.

  XCTAssertNil([oneofFoo fieldWithName:@"mumble"]);
  XCTAssertNil([oneofFoo fieldWithNumber:666]);

  // Field exists, but not in this oneof.

  XCTAssertNil([oneofFoo fieldWithName:@"barString"]);
  XCTAssertNil([oneofFoo fieldWithNumber:TestOneof2_FieldNumber_BarString]);
  XCTAssertNil([oneofBar fieldWithName:@"fooString"]);
  XCTAssertNil([oneofBar fieldWithNumber:TestOneof2_FieldNumber_FooString]);

  // Check pointers back to the enclosing oneofs.
  // (pointer comparisons)
  XCTAssertEqual(fooStringField.containingOneof, oneofFoo);
  XCTAssertEqual(barStringField.containingOneof, oneofBar);
  GPBFieldDescriptor *bazString = [descriptor fieldWithNumber:TestOneof2_FieldNumber_BazString];
  XCTAssertNotNil(bazString);
  XCTAssertNil(bazString.containingOneof);
}

- (void)testExtensiondDescriptor {
  Class msgClass = [TestAllExtensions class];
  Class packedMsgClass = [TestPackedExtensions class];

  // Int

  GPBExtensionDescriptor *descriptor = [UnittestRoot optionalInt32Extension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertEqualObjects(descriptor.defaultValue, @0);
  XCTAssertNil(descriptor.enumDescriptor);

  descriptor = [UnittestRoot defaultInt32Extension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertEqualObjects(descriptor.defaultValue, @41);
  XCTAssertNil(descriptor.enumDescriptor);

  // Enum

  descriptor = [UnittestRoot optionalNestedEnumExtension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertEqual(descriptor.defaultValue, @1);
  XCTAssertEqualObjects(descriptor.enumDescriptor.name, @"TestAllTypes_NestedEnum");

  descriptor = [UnittestRoot defaultNestedEnumExtension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertEqual(descriptor.defaultValue, @2);
  XCTAssertEqualObjects(descriptor.enumDescriptor.name, @"TestAllTypes_NestedEnum");

  // Message

  descriptor = [UnittestRoot optionalNestedMessageExtension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertNil(descriptor.defaultValue);
  XCTAssertNil(descriptor.enumDescriptor);

  // Repeated Int

  descriptor = [UnittestRoot repeatedInt32Extension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertNil(descriptor.defaultValue);
  XCTAssertNil(descriptor.enumDescriptor);

  descriptor = [UnittestRoot packedInt32Extension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, packedMsgClass);  // ptr equality
  XCTAssertTrue(descriptor.isPackable);
  XCTAssertNil(descriptor.defaultValue);
  XCTAssertNil(descriptor.enumDescriptor);

  // Repeated Enum

  descriptor = [UnittestRoot repeatedNestedEnumExtension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertNil(descriptor.defaultValue);
  XCTAssertEqualObjects(descriptor.enumDescriptor.name, @"TestAllTypes_NestedEnum");

  descriptor = [UnittestRoot packedEnumExtension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, packedMsgClass);  // ptr equality
  XCTAssertTrue(descriptor.isPackable);
  XCTAssertNil(descriptor.defaultValue);
  XCTAssertEqualObjects(descriptor.enumDescriptor.name, @"ForeignEnum");

  // Repeated Message

  descriptor = [UnittestRoot repeatedNestedMessageExtension];
  XCTAssertNotNil(descriptor);
  XCTAssertEqual(descriptor.containingMessageClass, msgClass);  // ptr equality
  XCTAssertFalse(descriptor.isPackable);
  XCTAssertNil(descriptor.defaultValue);
  XCTAssertNil(descriptor.enumDescriptor);

  // Compare (used internally for serialization).

  GPBExtensionDescriptor *ext1 = [UnittestRoot optionalInt32Extension];
  XCTAssertEqual(ext1.fieldNumber, 1u);
  GPBExtensionDescriptor *ext2 = [UnittestRoot optionalInt64Extension];
  XCTAssertEqual(ext2.fieldNumber, 2u);

  XCTAssertEqual([ext1 compareByFieldNumber:ext2], NSOrderedAscending);
  XCTAssertEqual([ext2 compareByFieldNumber:ext1], NSOrderedDescending);
  XCTAssertEqual([ext1 compareByFieldNumber:ext1], NSOrderedSame);
}

@end
