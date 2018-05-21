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

#import <XCTest/XCTest.h>

#import "GPBUtilities_PackagePrivate.h"

#import <objc/runtime.h>

#import "GPBTestUtilities.h"

#import "GPBDescriptor.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBMessage.h"
#import "GPBUnknownField_PackagePrivate.h"

#import "google/protobuf/MapUnittest.pbobjc.h"
#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestObjc.pbobjc.h"

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

  // Long name so multiple decode ops are needed.
  inputStr = @"longFieldNameIsLooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong1000";
  XCTAssertEqualObjects(GPBDecodeTextFormatName(decodeData, 1000, inputStr),
                        @"long_field_name_is_looooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong_1000");
}

- (void)testTextFormat {
  TestAllTypes *message = [TestAllTypes message];

  // Not kGPBDefaultRepeatCount because we are comparing to golden master file
  // which was generated with 2.
  [self setAllFields:message repeatedCount:2];

  NSString *result = GPBTextFormatForMessage(message, nil);

  NSString *fileName = @"text_format_unittest_data.txt";
  NSData *resultData = [result dataUsingEncoding:NSUTF8StringEncoding];
  NSData *expectedData =
      [self getDataFileNamed:fileName dataToWrite:resultData];
  NSString *expected = [[NSString alloc] initWithData:expectedData
                                             encoding:NSUTF8StringEncoding];
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

  NSString *expected = @"_cmd: true\n"
                       @"isProxy: true\n"
                       @"SubEnum: retainCount\n"
                       @"New {\n"
                       @"  copy: \"foo\"\n"
                       @"}\n";
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
  NSData *expectedData =
      [self getDataFileNamed:fileName dataToWrite:resultData];
  NSString *expected = [[NSString alloc] initWithData:expectedData
                                             encoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(expected, result);
  [expected release];
}

// TODO(thomasvl): add test with extensions once those format with correct names.

- (void)testSetRepeatedFields {
  TestAllTypes *message = [TestAllTypes message];

  NSDictionary *repeatedFieldValues = @{
    @"repeatedStringArray" : [@[@"foo", @"bar"] mutableCopy],
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
    GPBFieldDescriptor *field =
        [message.descriptor fieldWithName:fieldName];
    XCTAssertNotNil(field, @"No field with name: %@", fieldName);
    id expectedValues = repeatedFieldValues[fieldName];
    GPBSetMessageRepeatedField(message, field, expectedValues);
    XCTAssertEqualObjects(expectedValues,
                          [message valueForKeyPath:fieldName]);
  }
}

// Helper to make an unknown field set with something in it.
static GPBUnknownFieldSet *UnknownFieldsSetHelper(int num) {
  GPBUnknownFieldSet *result =
      [[[GPBUnknownFieldSet alloc] init] autorelease];

  GPBUnknownField *field =
      [[[GPBUnknownField alloc] initWithNumber:num] autorelease];
  [field addVarint:num];
  [result addField:field];

  return result;
}

- (void)testDropMessageUnknownFieldsRecursively {
  TestAllExtensions *message = [TestAllExtensions message];

  // Give it unknownFields.
  message.unknownFields = UnknownFieldsSetHelper(777);

  // Given it extensions that include a message with unknown fields of its own.
  {
    // Int
    [message setExtension:[UnittestRoot optionalInt32Extension] value:@5];

    // Group
    OptionalGroup_extension *optionalGroup = [OptionalGroup_extension message];
    optionalGroup.a = 123;
    optionalGroup.unknownFields = UnknownFieldsSetHelper(779);
    [message setExtension:[UnittestRoot optionalGroupExtension]
                    value:optionalGroup];

    // Message
    TestAllTypes_NestedMessage *nestedMessage =
        [TestAllTypes_NestedMessage message];
    nestedMessage.bb = 456;
    nestedMessage.unknownFields = UnknownFieldsSetHelper(778);
    [message setExtension:[UnittestRoot optionalNestedMessageExtension]
                    value:nestedMessage];

    // Repeated Group
    RepeatedGroup_extension *repeatedGroup =
      [[RepeatedGroup_extension alloc] init];
    repeatedGroup.a = 567;
    repeatedGroup.unknownFields = UnknownFieldsSetHelper(780);
    [message addExtension:[UnittestRoot repeatedGroupExtension]
                    value:repeatedGroup];
    [repeatedGroup release];

    // Repeated Message
    nestedMessage = [[TestAllTypes_NestedMessage alloc] init];
    nestedMessage.bb = 678;
    nestedMessage.unknownFields = UnknownFieldsSetHelper(781);
    [message addExtension:[UnittestRoot repeatedNestedMessageExtension]
                    value:nestedMessage];
    [nestedMessage release];
  }

  // Confirm everything is there.

  XCTAssertNotNil(message);
  XCTAssertNotNil(message.unknownFields);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalInt32Extension]]);

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalGroupExtension]]);
    OptionalGroup_extension *optionalGroup =
        [message getExtension:[UnittestRoot optionalGroupExtension]];
    XCTAssertNotNil(optionalGroup);
    XCTAssertEqual(optionalGroup.a, 123);
    XCTAssertNotNil(optionalGroup.unknownFields);
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
    TestAllTypes_NestedMessage *nestedMessage =
        [message getExtension:[UnittestRoot optionalNestedMessageExtension]];
    XCTAssertNotNil(nestedMessage);
    XCTAssertEqual(nestedMessage.bb, 456);
    XCTAssertNotNil(nestedMessage.unknownFields);
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedGroupExtension]]);
    NSArray *repeatedGroups = [message getExtension:[UnittestRoot repeatedGroupExtension]];
    XCTAssertEqual(repeatedGroups.count, (NSUInteger)1);
    RepeatedGroup_extension *repeatedGroup = repeatedGroups.firstObject;
    XCTAssertNotNil(repeatedGroup);
    XCTAssertEqual(repeatedGroup.a, 567);
    XCTAssertNotNil(repeatedGroup.unknownFields);
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedNestedMessageExtension]]);
    NSArray *repeatedNestedMessages = [message getExtension:[UnittestRoot repeatedNestedMessageExtension]];
    XCTAssertEqual(repeatedNestedMessages.count, (NSUInteger)1);
    TestAllTypes_NestedMessage *repeatedNestedMessage = repeatedNestedMessages.firstObject;
    XCTAssertNotNil(repeatedNestedMessage);
    XCTAssertEqual(repeatedNestedMessage.bb, 678);
    XCTAssertNotNil(repeatedNestedMessage.unknownFields);
  }

  // Drop them.
  GPBMessageDropUnknownFieldsRecursively(message);

  // Confirm unknowns are gone from within the messages.

  XCTAssertNotNil(message);
  XCTAssertNil(message.unknownFields);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalInt32Extension]]);

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalGroupExtension]]);
    OptionalGroup_extension *optionalGroup =
        [message getExtension:[UnittestRoot optionalGroupExtension]];
    XCTAssertNotNil(optionalGroup);
    XCTAssertEqual(optionalGroup.a, 123);
    XCTAssertNil(optionalGroup.unknownFields);
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
    TestAllTypes_NestedMessage *nestedMessage =
        [message getExtension:[UnittestRoot optionalNestedMessageExtension]];
    XCTAssertNotNil(nestedMessage);
    XCTAssertEqual(nestedMessage.bb, 456);
    XCTAssertNil(nestedMessage.unknownFields);
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedGroupExtension]]);
    NSArray *repeatedGroups = [message getExtension:[UnittestRoot repeatedGroupExtension]];
    XCTAssertEqual(repeatedGroups.count, (NSUInteger)1);
    RepeatedGroup_extension *repeatedGroup = repeatedGroups.firstObject;
    XCTAssertNotNil(repeatedGroup);
    XCTAssertEqual(repeatedGroup.a, 567);
    XCTAssertNil(repeatedGroup.unknownFields);
  }

  {
    XCTAssertTrue([message hasExtension:[UnittestRoot repeatedNestedMessageExtension]]);
    NSArray *repeatedNestedMessages = [message getExtension:[UnittestRoot repeatedNestedMessageExtension]];
    XCTAssertEqual(repeatedNestedMessages.count, (NSUInteger)1);
    TestAllTypes_NestedMessage *repeatedNestedMessage = repeatedNestedMessages.firstObject;
    XCTAssertNotNil(repeatedNestedMessage);
    XCTAssertEqual(repeatedNestedMessage.bb, 678);
    XCTAssertNil(repeatedNestedMessage.unknownFields);
  }

}

- (void)testDropMessageUnknownFieldsRecursively_Maps {
  TestMap *message = [TestMap message];

  {
    ForeignMessage *foreignMessage = [ForeignMessage message];
    foreignMessage.unknownFields = UnknownFieldsSetHelper(100);
    [message.mapInt32ForeignMessage setObject:foreignMessage forKey:100];

    foreignMessage = [ForeignMessage message];
    foreignMessage.unknownFields = UnknownFieldsSetHelper(101);
    [message.mapStringForeignMessage setObject:foreignMessage forKey:@"101"];
  }

  // Confirm everything is there.

  XCTAssertNotNil(message);

  {
    ForeignMessage *foreignMessage = [message.mapInt32ForeignMessage objectForKey:100];
    XCTAssertNotNil(foreignMessage);
    XCTAssertNotNil(foreignMessage.unknownFields);
  }

  {
    ForeignMessage *foreignMessage = [message.mapStringForeignMessage objectForKey:@"101"];
    XCTAssertNotNil(foreignMessage);
    XCTAssertNotNil(foreignMessage.unknownFields);
  }

  GPBMessageDropUnknownFieldsRecursively(message);

  // Confirm unknowns are gone from within the messages.

  XCTAssertNotNil(message);

  {
    ForeignMessage *foreignMessage = [message.mapInt32ForeignMessage objectForKey:100];
    XCTAssertNotNil(foreignMessage);
    XCTAssertNil(foreignMessage.unknownFields);
  }

  {
    ForeignMessage *foreignMessage = [message.mapStringForeignMessage objectForKey:@"101"];
    XCTAssertNotNil(foreignMessage);
    XCTAssertNil(foreignMessage.unknownFields);
  }

}

@end
