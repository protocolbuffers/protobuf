// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import "GPBDescriptor.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBMessage.h"
#import "GPBTestUtilities.h"
#import "GPBUnknownField.h"
#import "GPBUnknownField_PackagePrivate.h"
#import "GPBUtilities.h"
#import "GPBUtilities_PackagePrivate.h"
#import "objectivec/Tests/MapUnittest.pbobjc.h"
#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestObjc.pbobjc.h"

@interface UtilitiesTests : GPBTestCase
@end

@implementation UtilitiesTests

- (void)testRightShiftFunctions {
  XCTAssertEqual((1UL << 31) >> 31, 1UL);
  XCTAssertEqual((int32_t)(1U << 31) >> 31, -1);
  XCTAssertEqual((1ULL << 63) >> 63, 1ULL);
  XCTAssertEqual((int64_t)(1ULL << 63) >> 63, -1LL);

  XCTAssertEqual(GPBLogicalRightShift32((1U << 31), 31), 1);
  XCTAssertEqual(GPBLogicalRightShift64((1ULL << 63), 63), 1LL);
}

- (void)testGPBDecodeTextFormatName {
  uint8_t decodeData[] = {
      // clang-format off
    0x6,
    // An inlined string (first to make sure the leading null is handled
    // correctly, and with a key of zero to check that).
    0x0, 0x0, 'z', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'I', 'J', 0x0,
    // All as is (00 op)
    0x1, 0x0A, 0x0,
    // Underscore, upper + 9 (10 op)
    0x3, 0xCA, 0x0,
    //  Upper + 3 (10 op), underscore, upper + 5 (10 op)
    0x2, 0x44, 0xC6, 0x0,
    // All Upper for 4 (11 op), underscore, underscore, upper + 5 (10 op),
    // underscore, lower + 0 (01 op)
    0x4, 0x64, 0x80, 0xC5, 0xA1, 0x0,
    // 2 byte key: as is + 3 (00 op), underscore, lower + 4 (01 op),
    //   underscore, lower + 3 (01 op), underscore, lower + 1 (01 op),
    //   underscore, lower + 30 (01 op), as is + 30 (00 op), as is + 13 (00 op),
    //   underscore, as is + 3 (00 op)
    0xE8, 0x07, 0x04, 0xA5, 0xA4, 0xA2, 0xBF, 0x1F, 0x0E, 0x84, 0x0,
      // clang-format on
  };
  NSString *inputStr = @"abcdefghIJ";

  // Empty inputs

  XCTAssertNil(GPBDecodeTextFormatName(nil, 1, NULL));
  XCTAssertNil(GPBDecodeTextFormatName(decodeData, 1, NULL));
  XCTAssertNil(GPBDecodeTextFormatName(nil, 1, inputStr));

  // Keys not found.

  XCTAssertNil(GPBDecodeTextFormatName(decodeData, 5, inputStr));
  XCTAssertNil(GPBDecodeTextFormatName(decodeData, -1, inputStr));

  // Some name decodes.

  XCTAssertEqualObjects(GPBDecodeTextFormatName(decodeData, 1, inputStr), @"abcdefghIJ");
  XCTAssertEqualObjects(GPBDecodeTextFormatName(decodeData, 2, inputStr), @"Abcd_EfghIJ");
  XCTAssertEqualObjects(GPBDecodeTextFormatName(decodeData, 3, inputStr), @"_AbcdefghIJ");
  XCTAssertEqualObjects(GPBDecodeTextFormatName(decodeData, 4, inputStr), @"ABCD__EfghI_j");

  // An inlined string (and key of zero).
  XCTAssertEqualObjects(GPBDecodeTextFormatName(decodeData, 0, inputStr), @"zbcdefghIJ");

  // clang-format off
  // Long name so multiple decode ops are needed.
  inputStr = @"longFieldNameIsLooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong1000";
  XCTAssertEqualObjects(GPBDecodeTextFormatName(decodeData, 1000, inputStr),
                        @"long_field_name_is_looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong_1000");
  // clang-format on
}

- (void)testTextFormat {
  TestAllTypes *message = [TestAllTypes message];

  // Not kGPBDefaultRepeatCount because we are comparing to golden master file
  // which was generated with 2.
  [self setAllFields:message repeatedCount:2];

  NSString *result = GPBTextFormatForMessage(message, nil);

  NSString *fileName = @"text_format_unittest_data.txt";
  NSData *resultData = [result dataUsingEncoding:NSUTF8StringEncoding];
  NSData *expectedData = [self getDataFileNamed:fileName dataToWrite:resultData];
  NSString *expected = [[NSString alloc] initWithData:expectedData encoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(expected, result);
  [expected release];
}

- (void)testTextFormatExtra {
  // -testTextFormat uses all protos with fields that don't require special
  // handing for figuring out the names.  The ObjC proto has a bunch of oddball
  // field and enum names that require the decode info to get right, so this
  // confirms they generated and decoded correctly.

  self_Class *message = [self_Class message];
  message.cmd = YES;
  message.isProxy_p = YES;
  message.subEnum = self_autorelease_RetainCount;
  message.new_p.copy_p = @"foo";

  // clang-format off
  NSString *expected = @"_cmd: true\n"
                       @"isProxy: true\n"
                       @"SubEnum: retainCount\n"
                       @"New {\n"
                       @"  copy: \"foo\"\n"
                       @"}\n";
  // clang-format on
  NSString *result = GPBTextFormatForMessage(message, nil);
  XCTAssertEqualObjects(expected, result);
}

- (void)testTextFormatMaps {
  TestMap *message = [TestMap message];

  // Map iteration order doesn't have to be stable, so use only one entry.
  [self setAllMapFields:message numEntries:1];

  NSString *result = GPBTextFormatForMessage(message, nil);

  NSString *fileName = @"text_format_map_unittest_data.txt";
  NSData *resultData = [result dataUsingEncoding:NSUTF8StringEncoding];
  NSData *expectedData = [self getDataFileNamed:fileName dataToWrite:resultData];
  NSString *expected = [[NSString alloc] initWithData:expectedData encoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(expected, result);
  [expected release];
}

- (void)testTextFormatExtensions {
  TestAllExtensions *message = [TestAllExtensions message];

  // Not kGPBDefaultRepeatCount because we are comparing to golden master file
  // which was generated with 2.
  [self setAllExtensions:message repeatedCount:2];

  NSString *result = GPBTextFormatForMessage(message, nil);

  // NOTE: ObjC TextFormat doesn't have the proper extension names so it
  // uses comments for the ObjC name and raw numbers for the fields instead
  // of the bracketed extension name.
  NSString *fileName = @"text_format_extensions_unittest_data.txt";
  NSData *resultData = [result dataUsingEncoding:NSUTF8StringEncoding];
  NSData *expectedData = [self getDataFileNamed:fileName dataToWrite:resultData];
  NSString *expected = [[NSString alloc] initWithData:expectedData encoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(expected, result);
  [expected release];
}

- (void)testTextFormatUnknownFields {
  GPBUnknownFields *ufs = [[[GPBUnknownFields alloc] init] autorelease];
  [ufs addFieldNumber:100 varint:5];
  [ufs addFieldNumber:100 varint:4];
  [ufs addFieldNumber:10 varint:1];
  [ufs addFieldNumber:300 fixed32:0x50];
  [ufs addFieldNumber:300 fixed32:0x40];
  [ufs addFieldNumber:10 fixed32:0x10];
  [ufs addFieldNumber:200 fixed64:0x5000];
  [ufs addFieldNumber:200 fixed64:0x4000];
  [ufs addFieldNumber:10 fixed64:0x1000];
  [ufs addFieldNumber:10 lengthDelimited:DataFromCStr("foo")];
  [ufs addFieldNumber:10 lengthDelimited:DataFromCStr("bar")];
  GPBUnknownFields *group = [ufs addGroupWithFieldNumber:150];
  [group addFieldNumber:2 varint:2];
  [group addFieldNumber:1 varint:1];
  group = [ufs addGroupWithFieldNumber:150];
  [group addFieldNumber:1 varint:1];
  [group addFieldNumber:3 fixed32:0x3];
  [group addFieldNumber:2 fixed64:0x2];
  TestEmptyMessage *message = [TestEmptyMessage message];
  XCTAssertTrue([message mergeUnknownFields:ufs extensionRegistry:nil error:NULL]);

  NSString *expected = @"# --- Unknown fields ---\n"
                       @"10: 1\n"
                       @"10: 0x10\n"
                       @"10: 0x1000\n"
                       @"10: \"foo\"\n"
                       @"10: \"bar\"\n"
                       @"100: 5\n"
                       @"100: 4\n"
                       @"150: {\n"
                       @"  1: 1\n"
                       @"  2: 2\n"
                       @"}\n"
                       @"150: {\n"
                       @"  1: 1\n"
                       @"  2: 0x2\n"
                       @"  3: 0x3\n"
                       @"}\n"
                       @"200: 0x5000\n"
                       @"200: 0x4000\n"
                       @"300: 0x50\n"
                       @"300: 0x40\n";
  NSString *result = GPBTextFormatForMessage(message, nil);
  XCTAssertEqualObjects(expected, result);
}

- (void)testSetRepeatedFields {
  TestAllTypes *message = [TestAllTypes message];

  NSDictionary *repeatedFieldValues = @{
    @"repeatedStringArray" : [@[ @"foo", @"bar" ] mutableCopy],
    @"repeatedBoolArray" : [GPBBoolArray arrayWithValue:YES],
    @"repeatedInt32Array" : [GPBInt32Array arrayWithValue:14],
    @"repeatedInt64Array" : [GPBInt64Array arrayWithValue:15],
    @"repeatedUint32Array" : [GPBUInt32Array arrayWithValue:16],
    @"repeatedUint64Array" : [GPBUInt64Array arrayWithValue:16],
    @"repeatedFloatArray" : [GPBFloatArray arrayWithValue:16],
    @"repeatedDoubleArray" : [GPBDoubleArray arrayWithValue:16],
    @"repeatedNestedEnumArray" :
        [GPBEnumArray arrayWithValidationFunction:TestAllTypes_NestedEnum_IsValidValue
                                         rawValue:TestAllTypes_NestedEnum_Foo],
  };
  for (NSString *fieldName in repeatedFieldValues) {
    GPBFieldDescriptor *field = [message.descriptor fieldWithName:fieldName];
    XCTAssertNotNil(field, @"No field with name: %@", fieldName);
    id expectedValues = repeatedFieldValues[fieldName];
    GPBSetMessageRepeatedField(message, field, expectedValues);
    XCTAssertEqualObjects(expectedValues, [message valueForKeyPath:fieldName]);
  }
}

// Helper to add an unknown field data to messages.
static void AddUnknownFields(GPBMessage *message, int num) {
  GPBUnknownFields *ufs = [[GPBUnknownFields alloc] init];
  [ufs addFieldNumber:num varint:num];
  // Can't fail since it is a varint.
  [message mergeUnknownFields:ufs extensionRegistry:nil error:NULL];
  [ufs release];
}

static BOOL HasUnknownFields(GPBMessage *message) {
  GPBUnknownFields *ufs = [[GPBUnknownFields alloc] initFromMessage:message];
  BOOL result = !ufs.empty;
  [ufs release];
  return result;
}

- (void)testDropMessageUnknownFieldsRecursively {
  TestAllExtensions *message = [TestAllExtensions message];

  // Give it unknownFields.
  AddUnknownFields(message, 1777);

  // Given it extensions that include a message with unknown fields of its own.
  {
    // Int
    [message setExtension:[UnittestRoot optionalInt32Extension] value:@5];

    // Group
    OptionalGroup_extension *optionalGroup = [OptionalGroup_extension message];
    optionalGroup.a = 123;
    AddUnknownFields(optionalGroup, 1779);
    [message setExtension:[UnittestRoot optionalGroupExtension] value:optionalGroup];

    // Message
    TestAllTypes_NestedMessage *nestedMessage = [TestAllTypes_NestedMessage message];
    nestedMessage.bb = 456;
    AddUnknownFields(nestedMessage, 1778);
    [message setExtension:[UnittestRoot optionalNestedMessageExtension] value:nestedMessage];

    // Repeated Group
    RepeatedGroup_extension *repeatedGroup = [[RepeatedGroup_extension alloc] init];
    repeatedGroup.a = 567;
    AddUnknownFields(repeatedGroup, 1780);
    [message addExtension:[UnittestRoot repeatedGroupExtension] value:repeatedGroup];
    [repeatedGroup release];

    // Repeated Message
    nestedMessage = [[TestAllTypes_NestedMessage alloc] init];
    nestedMessage.bb = 678;
    AddUnknownFields(nestedMessage, 1781);
    [message addExtension:[UnittestRoot repeatedNestedMessageExtension] value:nestedMessage];
    [nestedMessage release];
  }

  // Confirm everything is there.

  XCTAssertNotNil(message);
  XCTAssertTrue(HasUnknownFields(message));
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalInt32Extension]]);

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalGroupExtension]]);
    OptionalGroup_extension *optionalGroup =
        [message getExtension:[UnittestRoot optionalGroupExtension]];
    XCTAssertNotNil(optionalGroup);
    XCTAssertEqual(optionalGroup.a, 123);
    XCTAssertTrue(HasUnknownFields(optionalGroup));
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
    TestAllTypes_NestedMessage *nestedMessage =
        [message getExtension:[UnittestRoot optionalNestedMessageExtension]];
    XCTAssertNotNil(nestedMessage);
    XCTAssertEqual(nestedMessage.bb, 456);
    XCTAssertTrue(HasUnknownFields(nestedMessage));
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedGroupExtension]]);
    NSArray *repeatedGroups = [message getExtension:[UnittestRoot repeatedGroupExtension]];
    XCTAssertEqual(repeatedGroups.count, (NSUInteger)1);
    RepeatedGroup_extension *repeatedGroup = repeatedGroups.firstObject;
    XCTAssertNotNil(repeatedGroup);
    XCTAssertEqual(repeatedGroup.a, 567);
    XCTAssertTrue(HasUnknownFields(repeatedGroup));
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedNestedMessageExtension]]);
    NSArray *repeatedNestedMessages =
        [message getExtension:[UnittestRoot repeatedNestedMessageExtension]];
    XCTAssertEqual(repeatedNestedMessages.count, (NSUInteger)1);
    TestAllTypes_NestedMessage *repeatedNestedMessage = repeatedNestedMessages.firstObject;
    XCTAssertNotNil(repeatedNestedMessage);
    XCTAssertEqual(repeatedNestedMessage.bb, 678);
    XCTAssertTrue(HasUnknownFields(repeatedNestedMessage));
  }

  // Drop them.
  GPBMessageDropUnknownFieldsRecursively(message);

  // Confirm unknowns are gone from within the messages.

  XCTAssertNotNil(message);
  XCTAssertFalse(HasUnknownFields(message));
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalInt32Extension]]);

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalGroupExtension]]);
    OptionalGroup_extension *optionalGroup =
        [message getExtension:[UnittestRoot optionalGroupExtension]];
    XCTAssertNotNil(optionalGroup);
    XCTAssertEqual(optionalGroup.a, 123);
    XCTAssertFalse(HasUnknownFields(optionalGroup));
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
    TestAllTypes_NestedMessage *nestedMessage =
        [message getExtension:[UnittestRoot optionalNestedMessageExtension]];
    XCTAssertNotNil(nestedMessage);
    XCTAssertEqual(nestedMessage.bb, 456);
    XCTAssertFalse(HasUnknownFields(nestedMessage));
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedGroupExtension]]);
    NSArray *repeatedGroups = [message getExtension:[UnittestRoot repeatedGroupExtension]];
    XCTAssertEqual(repeatedGroups.count, (NSUInteger)1);
    RepeatedGroup_extension *repeatedGroup = repeatedGroups.firstObject;
    XCTAssertNotNil(repeatedGroup);
    XCTAssertEqual(repeatedGroup.a, 567);
    XCTAssertFalse(HasUnknownFields(repeatedGroup));
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedNestedMessageExtension]]);
    NSArray *repeatedNestedMessages =
        [message getExtension:[UnittestRoot repeatedNestedMessageExtension]];
    XCTAssertEqual(repeatedNestedMessages.count, (NSUInteger)1);
    TestAllTypes_NestedMessage *repeatedNestedMessage = repeatedNestedMessages.firstObject;
    XCTAssertNotNil(repeatedNestedMessage);
    XCTAssertEqual(repeatedNestedMessage.bb, 678);
    XCTAssertFalse(HasUnknownFields(repeatedNestedMessage));
  }
}

- (void)testDropMessageUnknownFieldsRecursively_Maps {
  TestMap *message = [TestMap message];

  {
    ForeignMessage *foreignMessage = [ForeignMessage message];
    AddUnknownFields(foreignMessage, 1000);
    [message.mapInt32ForeignMessage setObject:foreignMessage forKey:100];

    foreignMessage = [ForeignMessage message];
    AddUnknownFields(foreignMessage, 1001);
    [message.mapStringForeignMessage setObject:foreignMessage forKey:@"101"];
  }

  // Confirm everything is there.

  XCTAssertNotNil(message);

  {
    ForeignMessage *foreignMessage = [message.mapInt32ForeignMessage objectForKey:100];
    XCTAssertNotNil(foreignMessage);
    XCTAssertTrue(HasUnknownFields(foreignMessage));
  }

  {
    ForeignMessage *foreignMessage = [message.mapStringForeignMessage objectForKey:@"101"];
    XCTAssertNotNil(foreignMessage);
    XCTAssertTrue(HasUnknownFields(foreignMessage));
  }

  GPBMessageDropUnknownFieldsRecursively(message);

  // Confirm unknowns are gone from within the messages.

  XCTAssertNotNil(message);

  {
    ForeignMessage *foreignMessage = [message.mapInt32ForeignMessage objectForKey:100];
    XCTAssertNotNil(foreignMessage);
    XCTAssertFalse(HasUnknownFields(foreignMessage));
  }

  {
    ForeignMessage *foreignMessage = [message.mapStringForeignMessage objectForKey:@"101"];
    XCTAssertNotNil(foreignMessage);
    XCTAssertFalse(HasUnknownFields(foreignMessage));
  }
}

@end
