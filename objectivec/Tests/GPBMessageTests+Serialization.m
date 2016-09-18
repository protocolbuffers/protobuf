// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

#import <objc/runtime.h>

#import "GPBMessage.h"

#import "google/protobuf/MapProto2Unittest.pbobjc.h"
#import "google/protobuf/MapUnittest.pbobjc.h"
#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestDropUnknownFields.pbobjc.h"
#import "google/protobuf/UnittestPreserveUnknownEnum.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto2.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto3.pbobjc.h"

static NSData *DataFromCStr(const char *str) {
  return [NSData dataWithBytes:str length:strlen(str)];
}

@interface MessageSerializationTests : GPBTestCase
@end

@implementation MessageSerializationTests

// TODO(thomasvl): Pull tests over from GPBMessageTests that are serialization
// specific.

- (void)testProto3SerializationHandlingDefaults {
  // Proto2 covered in other tests.

  Message3 *msg = [[Message3 alloc] init];

  // Add defaults, no output.

  NSData *data = [msg data];
  XCTAssertEqual([data length], 0U);

  // All zeros, still nothing.

  msg.optionalInt32 = 0;
  msg.optionalInt64 = 0;
  msg.optionalUint32 = 0;
  msg.optionalUint64 = 0;
  msg.optionalSint32 = 0;
  msg.optionalSint64 = 0;
  msg.optionalFixed32 = 0;
  msg.optionalFixed64 = 0;
  msg.optionalSfixed32 = 0;
  msg.optionalSfixed64 = 0;
  msg.optionalFloat = 0.0f;
  msg.optionalDouble = 0.0;
  msg.optionalBool = NO;
  msg.optionalString = @"";
  msg.optionalBytes = [NSData data];
  msg.optionalEnum = Message3_Enum_Foo;  // first value

  data = [msg data];
  XCTAssertEqual([data length], 0U);

  // The two that also take nil as nothing.

  msg.optionalString = nil;
  msg.optionalBytes = nil;

  data = [msg data];
  XCTAssertEqual([data length], 0U);

  // Set one field...

  msg.optionalInt32 = 1;

  data = [msg data];
  const uint8_t expectedBytes[] = {0x08, 0x01};
  NSData *expected = [NSData dataWithBytes:expectedBytes length:2];
  XCTAssertEqualObjects(data, expected);

  // Back to zero...

  msg.optionalInt32 = 0;

  data = [msg data];
  XCTAssertEqual([data length], 0U);

  [msg release];
}

- (void)testProto3DroppingUnknownFields {
  DropUnknownsFooWithExtraFields *fooWithExtras =
      [[DropUnknownsFooWithExtraFields alloc] init];

  fooWithExtras.int32Value = 1;
  fooWithExtras.enumValue = DropUnknownsFooWithExtraFields_NestedEnum_Baz;
  fooWithExtras.extraInt32Value = 2;

  NSData *data = [fooWithExtras data];
  XCTAssertNotNil(data);
  DropUnknownsFoo *foo = [DropUnknownsFoo parseFromData:data error:NULL];

  XCTAssertEqual(foo.int32Value, 1);
  XCTAssertEqual(foo.enumValue, DropUnknownsFoo_NestedEnum_Baz);
  // Nothing should end up in the unknowns.
  XCTAssertEqual([foo.unknownFields countOfFields], 0U);

  [fooWithExtras release];
  data = [foo data];
  fooWithExtras =
      [DropUnknownsFooWithExtraFields parseFromData:data error:NULL];
  XCTAssertEqual(fooWithExtras.int32Value, 1);
  XCTAssertEqual(fooWithExtras.enumValue,
                 DropUnknownsFooWithExtraFields_NestedEnum_Baz);
  // And the extra value is gone (back to the default).
  XCTAssertEqual(fooWithExtras.extraInt32Value, 0);
  XCTAssertEqual([foo.unknownFields countOfFields], 0U);
}

- (void)testProto2UnknownEnumToUnknownField {
  Message3 *orig = [[Message3 alloc] init];

  orig.optionalEnum = Message3_Enum_Extra3;
  orig.repeatedEnumArray =
      [GPBEnumArray arrayWithValidationFunction:Message3_Enum_IsValidValue
                                       rawValue:Message3_Enum_Extra3];
  orig.oneofEnum = Message3_Enum_Extra3;

  NSData *data = [orig data];
  XCTAssertNotNil(data);
  Message2 *msg = [[Message2 alloc] initWithData:data error:NULL];

  // None of the fields should be set.

  XCTAssertFalse(msg.hasOptionalEnum);
  XCTAssertEqual(msg.repeatedEnumArray.count, 0U);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_GPBUnsetOneOfCase);

  // All the values should be in unknown fields.

  GPBUnknownFieldSet *unknownFields = msg.unknownFields;

  XCTAssertEqual([unknownFields countOfFields], 3U);
  XCTAssertTrue([unknownFields hasField:Message2_FieldNumber_OptionalEnum]);
  XCTAssertTrue(
      [unknownFields hasField:Message2_FieldNumber_RepeatedEnumArray]);
  XCTAssertTrue([unknownFields hasField:Message2_FieldNumber_OneofEnum]);

  GPBUnknownField *field =
      [unknownFields getField:Message2_FieldNumber_OptionalEnum];
  XCTAssertEqual(field.varintList.count, 1U);
  XCTAssertEqual([field.varintList valueAtIndex:0],
                 (uint64_t)Message3_Enum_Extra3);

  field = [unknownFields getField:Message2_FieldNumber_RepeatedEnumArray];
  XCTAssertEqual(field.varintList.count, 1U);
  XCTAssertEqual([field.varintList valueAtIndex:0], (uint64_t)Message3_Enum_Extra3);

  field = [unknownFields getField:Message2_FieldNumber_OneofEnum];
  XCTAssertEqual(field.varintList.count, 1U);
  XCTAssertEqual([field.varintList valueAtIndex:0],
                 (uint64_t)Message3_Enum_Extra3);

  [msg release];
  [orig release];
}

- (void)testProto3UnknownEnumPreserving {
  UnknownEnumsMyMessagePlusExtra *orig =
      [UnknownEnumsMyMessagePlusExtra message];

  orig.e = UnknownEnumsMyEnumPlusExtra_EExtra;
  orig.repeatedEArray = [GPBEnumArray
      arrayWithValidationFunction:UnknownEnumsMyEnumPlusExtra_IsValidValue
                         rawValue:UnknownEnumsMyEnumPlusExtra_EExtra];
  orig.repeatedPackedEArray = [GPBEnumArray
      arrayWithValidationFunction:UnknownEnumsMyEnumPlusExtra_IsValidValue
                         rawValue:UnknownEnumsMyEnumPlusExtra_EExtra];
  orig.oneofE1 = UnknownEnumsMyEnumPlusExtra_EExtra;

  // Everything should be there via raw values.

  NSData *data = [orig data];
  XCTAssertNotNil(data);
  UnknownEnumsMyMessage *msg =
      [UnknownEnumsMyMessage parseFromData:data error:NULL];

  XCTAssertEqual(msg.e, UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual(UnknownEnumsMyMessage_E_RawValue(msg),
                 UnknownEnumsMyEnumPlusExtra_EExtra);
  XCTAssertEqual(msg.repeatedEArray.count, 1U);
  XCTAssertEqual([msg.repeatedEArray valueAtIndex:0],
                 UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([msg.repeatedEArray rawValueAtIndex:0],
                 (UnknownEnumsMyEnum)UnknownEnumsMyEnumPlusExtra_EExtra);
  XCTAssertEqual(msg.repeatedPackedEArray.count, 1U);
  XCTAssertEqual([msg.repeatedPackedEArray valueAtIndex:0],
                 UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([msg.repeatedPackedEArray rawValueAtIndex:0],
                 (UnknownEnumsMyEnum)UnknownEnumsMyEnumPlusExtra_EExtra);
  XCTAssertEqual(msg.oneofE1,
                 UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual(UnknownEnumsMyMessage_OneofE1_RawValue(msg),
                 UnknownEnumsMyEnumPlusExtra_EExtra);

  // Everything should go out and come back.

  data = [msg data];
  orig = [UnknownEnumsMyMessagePlusExtra parseFromData:data error:NULL];

  XCTAssertEqual(orig.e, UnknownEnumsMyEnumPlusExtra_EExtra);
  XCTAssertEqual(orig.repeatedEArray.count, 1U);
  XCTAssertEqual([orig.repeatedEArray valueAtIndex:0],
                 UnknownEnumsMyEnumPlusExtra_EExtra);
  XCTAssertEqual(orig.repeatedPackedEArray.count, 1U);
  XCTAssertEqual([orig.repeatedPackedEArray valueAtIndex:0],
                 UnknownEnumsMyEnumPlusExtra_EExtra);
  XCTAssertEqual(orig.oneofE1, UnknownEnumsMyEnumPlusExtra_EExtra);
}

//%PDDM-DEFINE TEST_ROUNDTRIP_ONEOF(MESSAGE, FIELD, VALUE)
//%TEST_ROUNDTRIP_ONEOF_ADV(MESSAGE, FIELD, VALUE, )
//%PDDM-DEFINE TEST_ROUNDTRIP_ONEOF_ADV(MESSAGE, FIELD, VALUE, EQ_SUFFIX)
//%  {  // oneof##FIELD
//%    MESSAGE *orig = [[MESSAGE alloc] init];
//%    orig.oneof##FIELD = VALUE;
//%    XCTAssertEqual(orig.oOneOfCase, MESSAGE##_O_OneOfCase_Oneof##FIELD);
//%    NSData *data = [orig data];
//%    XCTAssertNotNil(data);
//%    MESSAGE *msg = [MESSAGE parseFromData:data error:NULL];
//%    XCTAssertEqual(msg.oOneOfCase, MESSAGE##_O_OneOfCase_Oneof##FIELD);
//%    XCTAssertEqual##EQ_SUFFIX(msg.oneof##FIELD, VALUE);
//%    [orig release];
//%  }
//%
//%PDDM-DEFINE TEST_ROUNDTRIP_ONEOFS(SYNTAX, BOOL_NON_DEFAULT)
//%- (void)testProto##SYNTAX##RoundTripOneof {
//%
//%GROUP_INIT##SYNTAX()  Message##SYNTAX *subMessage = [[Message##SYNTAX alloc] init];
//%  XCTAssertNotNil(subMessage);
//%  subMessage.optionalInt32 = 666;
//%
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Int32, 1)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Int64, 2)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Uint32, 3U)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Uint64, 4U)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Sint32, 5)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Sint64, 6)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Fixed32, 7U)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Fixed64, 8U)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Sfixed32, 9)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Sfixed64, 10)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Float, 11.0f)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Double, 12.0)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Bool, BOOL_NON_DEFAULT)
//%TEST_ROUNDTRIP_ONEOF_ADV(Message##SYNTAX, String, @"foo", Objects)
//%TEST_ROUNDTRIP_ONEOF_ADV(Message##SYNTAX, Bytes, [@"bar" dataUsingEncoding:NSUTF8StringEncoding], Objects)
//%GROUP_TEST##SYNTAX()TEST_ROUNDTRIP_ONEOF_ADV(Message##SYNTAX, Message, subMessage, Objects)
//%TEST_ROUNDTRIP_ONEOF(Message##SYNTAX, Enum, Message2_Enum_Bar)
//%GROUP_CLEANUP##SYNTAX()  [subMessage release];
//%}
//%
//%PDDM-DEFINE GROUP_INIT2()
//%  Message2_OneofGroup *group = [[Message2_OneofGroup alloc] init];
//%  XCTAssertNotNil(group);
//%  group.a = 777;
//%
//%PDDM-DEFINE GROUP_CLEANUP2()
//%  [group release];
//%
//%PDDM-DEFINE GROUP_TEST2()
//%TEST_ROUNDTRIP_ONEOF_ADV(Message2, Group, group, Objects)
//%
//%PDDM-DEFINE GROUP_INIT3()
// Empty
//%PDDM-DEFINE GROUP_CLEANUP3()
// Empty
//%PDDM-DEFINE GROUP_TEST3()
//%  // Not "group" in proto3.
//%
//%
//%PDDM-EXPAND TEST_ROUNDTRIP_ONEOFS(2, NO)
// This block of code is generated, do not edit it directly.

- (void)testProto2RoundTripOneof {

  Message2_OneofGroup *group = [[Message2_OneofGroup alloc] init];
  XCTAssertNotNil(group);
  group.a = 777;
  Message2 *subMessage = [[Message2 alloc] init];
  XCTAssertNotNil(subMessage);
  subMessage.optionalInt32 = 666;

  {  // oneofInt32
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofInt32 = 1;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofInt32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofInt32);
    XCTAssertEqual(msg.oneofInt32, 1);
    [orig release];
  }

  {  // oneofInt64
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofInt64 = 2;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofInt64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofInt64);
    XCTAssertEqual(msg.oneofInt64, 2);
    [orig release];
  }

  {  // oneofUint32
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofUint32 = 3U;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofUint32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofUint32);
    XCTAssertEqual(msg.oneofUint32, 3U);
    [orig release];
  }

  {  // oneofUint64
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofUint64 = 4U;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofUint64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofUint64);
    XCTAssertEqual(msg.oneofUint64, 4U);
    [orig release];
  }

  {  // oneofSint32
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofSint32 = 5;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofSint32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSint32);
    XCTAssertEqual(msg.oneofSint32, 5);
    [orig release];
  }

  {  // oneofSint64
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofSint64 = 6;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofSint64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSint64);
    XCTAssertEqual(msg.oneofSint64, 6);
    [orig release];
  }

  {  // oneofFixed32
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofFixed32 = 7U;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofFixed32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFixed32);
    XCTAssertEqual(msg.oneofFixed32, 7U);
    [orig release];
  }

  {  // oneofFixed64
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofFixed64 = 8U;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofFixed64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFixed64);
    XCTAssertEqual(msg.oneofFixed64, 8U);
    [orig release];
  }

  {  // oneofSfixed32
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofSfixed32 = 9;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofSfixed32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSfixed32);
    XCTAssertEqual(msg.oneofSfixed32, 9);
    [orig release];
  }

  {  // oneofSfixed64
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofSfixed64 = 10;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofSfixed64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSfixed64);
    XCTAssertEqual(msg.oneofSfixed64, 10);
    [orig release];
  }

  {  // oneofFloat
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofFloat = 11.0f;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofFloat);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFloat);
    XCTAssertEqual(msg.oneofFloat, 11.0f);
    [orig release];
  }

  {  // oneofDouble
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofDouble = 12.0;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofDouble);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofDouble);
    XCTAssertEqual(msg.oneofDouble, 12.0);
    [orig release];
  }

  {  // oneofBool
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofBool = NO;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofBool);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofBool);
    XCTAssertEqual(msg.oneofBool, NO);
    [orig release];
  }

  {  // oneofString
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofString = @"foo";
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofString);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofString);
    XCTAssertEqualObjects(msg.oneofString, @"foo");
    [orig release];
  }

  {  // oneofBytes
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofBytes);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofBytes);
    XCTAssertEqualObjects(msg.oneofBytes, [@"bar" dataUsingEncoding:NSUTF8StringEncoding]);
    [orig release];
  }

  {  // oneofGroup
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofGroup = group;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofGroup);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofGroup);
    XCTAssertEqualObjects(msg.oneofGroup, group);
    [orig release];
  }

  {  // oneofMessage
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofMessage = subMessage;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofMessage);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofMessage);
    XCTAssertEqualObjects(msg.oneofMessage, subMessage);
    [orig release];
  }

  {  // oneofEnum
    Message2 *orig = [[Message2 alloc] init];
    orig.oneofEnum = Message2_Enum_Bar;
    XCTAssertEqual(orig.oOneOfCase, Message2_O_OneOfCase_OneofEnum);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message2 *msg = [Message2 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofEnum);
    XCTAssertEqual(msg.oneofEnum, Message2_Enum_Bar);
    [orig release];
  }

  [group release];
  [subMessage release];
}

//%PDDM-EXPAND TEST_ROUNDTRIP_ONEOFS(3, YES)
// This block of code is generated, do not edit it directly.

- (void)testProto3RoundTripOneof {

  Message3 *subMessage = [[Message3 alloc] init];
  XCTAssertNotNil(subMessage);
  subMessage.optionalInt32 = 666;

  {  // oneofInt32
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofInt32 = 1;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofInt32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofInt32);
    XCTAssertEqual(msg.oneofInt32, 1);
    [orig release];
  }

  {  // oneofInt64
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofInt64 = 2;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofInt64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofInt64);
    XCTAssertEqual(msg.oneofInt64, 2);
    [orig release];
  }

  {  // oneofUint32
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofUint32 = 3U;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofUint32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofUint32);
    XCTAssertEqual(msg.oneofUint32, 3U);
    [orig release];
  }

  {  // oneofUint64
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofUint64 = 4U;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofUint64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofUint64);
    XCTAssertEqual(msg.oneofUint64, 4U);
    [orig release];
  }

  {  // oneofSint32
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofSint32 = 5;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofSint32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSint32);
    XCTAssertEqual(msg.oneofSint32, 5);
    [orig release];
  }

  {  // oneofSint64
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofSint64 = 6;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofSint64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSint64);
    XCTAssertEqual(msg.oneofSint64, 6);
    [orig release];
  }

  {  // oneofFixed32
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofFixed32 = 7U;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofFixed32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFixed32);
    XCTAssertEqual(msg.oneofFixed32, 7U);
    [orig release];
  }

  {  // oneofFixed64
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofFixed64 = 8U;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofFixed64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFixed64);
    XCTAssertEqual(msg.oneofFixed64, 8U);
    [orig release];
  }

  {  // oneofSfixed32
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofSfixed32 = 9;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofSfixed32);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSfixed32);
    XCTAssertEqual(msg.oneofSfixed32, 9);
    [orig release];
  }

  {  // oneofSfixed64
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofSfixed64 = 10;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofSfixed64);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSfixed64);
    XCTAssertEqual(msg.oneofSfixed64, 10);
    [orig release];
  }

  {  // oneofFloat
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofFloat = 11.0f;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofFloat);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFloat);
    XCTAssertEqual(msg.oneofFloat, 11.0f);
    [orig release];
  }

  {  // oneofDouble
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofDouble = 12.0;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofDouble);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofDouble);
    XCTAssertEqual(msg.oneofDouble, 12.0);
    [orig release];
  }

  {  // oneofBool
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofBool = YES;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofBool);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofBool);
    XCTAssertEqual(msg.oneofBool, YES);
    [orig release];
  }

  {  // oneofString
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofString = @"foo";
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofString);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofString);
    XCTAssertEqualObjects(msg.oneofString, @"foo");
    [orig release];
  }

  {  // oneofBytes
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofBytes);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofBytes);
    XCTAssertEqualObjects(msg.oneofBytes, [@"bar" dataUsingEncoding:NSUTF8StringEncoding]);
    [orig release];
  }

  // Not "group" in proto3.

  {  // oneofMessage
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofMessage = subMessage;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofMessage);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofMessage);
    XCTAssertEqualObjects(msg.oneofMessage, subMessage);
    [orig release];
  }

  {  // oneofEnum
    Message3 *orig = [[Message3 alloc] init];
    orig.oneofEnum = Message2_Enum_Bar;
    XCTAssertEqual(orig.oOneOfCase, Message3_O_OneOfCase_OneofEnum);
    NSData *data = [orig data];
    XCTAssertNotNil(data);
    Message3 *msg = [Message3 parseFromData:data error:NULL];
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofEnum);
    XCTAssertEqual(msg.oneofEnum, Message2_Enum_Bar);
    [orig release];
  }

  [subMessage release];
}

//%PDDM-EXPAND-END (2 expansions)

- (void)testPackedUnpackedMessageParsing {
  // packed is optional, a repeated field should parse when packed or unpacked.

  TestPackedTypes *packedOrig = [TestPackedTypes message];
  TestUnpackedTypes *unpackedOrig = [TestUnpackedTypes message];
  [self setPackedFields:packedOrig repeatedCount:4];
  [self setUnpackedFields:unpackedOrig repeatedCount:4];

  NSData *packedData = [packedOrig data];
  NSData *unpackedData = [unpackedOrig data];
  XCTAssertNotNil(packedData);
  XCTAssertNotNil(unpackedData);
  XCTAssertNotEqualObjects(packedData, unpackedData,
                           @"Data should differ (packed vs unpacked) use");

  NSError *error = nil;
  TestPackedTypes *packedParse =
      [TestPackedTypes parseFromData:unpackedData error:&error];
  XCTAssertNotNil(packedParse);
  XCTAssertNil(error);
  XCTAssertEqualObjects(packedParse, packedOrig);

  error = nil;
  TestUnpackedTypes *unpackedParsed =
      [TestUnpackedTypes parseFromData:packedData error:&error];
  XCTAssertNotNil(unpackedParsed);
  XCTAssertNil(error);
  XCTAssertEqualObjects(unpackedParsed, unpackedOrig);
}

- (void)testPackedUnpackedExtensionParsing {
  // packed is optional, a repeated extension should parse when packed or
  // unpacked.

  TestPackedExtensions *packedOrig = [TestPackedExtensions message];
  TestUnpackedExtensions *unpackedOrig = [TestUnpackedExtensions message];
  [self setPackedExtensions:packedOrig repeatedCount:kGPBDefaultRepeatCount];
  [self setUnpackedExtensions:unpackedOrig repeatedCount:kGPBDefaultRepeatCount];

  NSData *packedData = [packedOrig data];
  NSData *unpackedData = [unpackedOrig data];
  XCTAssertNotNil(packedData);
  XCTAssertNotNil(unpackedData);
  XCTAssertNotEqualObjects(packedData, unpackedData,
                           @"Data should differ (packed vs unpacked) use");

  NSError *error = nil;
  TestPackedExtensions *packedParse =
      [TestPackedExtensions parseFromData:unpackedData
                        extensionRegistry:[UnittestRoot extensionRegistry]
                                    error:&error];
  XCTAssertNotNil(packedParse);
  XCTAssertNil(error);
  XCTAssertEqualObjects(packedParse, packedOrig);

  error = nil;
  TestUnpackedExtensions *unpackedParsed =
      [TestUnpackedExtensions parseFromData:packedData
                          extensionRegistry:[UnittestRoot extensionRegistry]
                                      error:&error];
  XCTAssertNotNil(unpackedParsed);
  XCTAssertNil(error);
  XCTAssertEqualObjects(unpackedParsed, unpackedOrig);
}

- (void)testPackedExtensionVsFieldParsing {
  // Extensions and fields end up on the wire the same way, so they can parse
  // each other.

  TestPackedTypes *fieldsOrig = [TestPackedTypes message];
  TestPackedExtensions *extsOrig = [TestPackedExtensions message];
  [self setPackedFields:fieldsOrig repeatedCount:kGPBDefaultRepeatCount];
  [self setPackedExtensions:extsOrig repeatedCount:kGPBDefaultRepeatCount];

  NSData *fieldsData = [fieldsOrig data];
  NSData *extsData = [extsOrig data];
  XCTAssertNotNil(fieldsData);
  XCTAssertNotNil(extsData);
  XCTAssertEqualObjects(fieldsData, extsData);

  NSError *error = nil;
  TestPackedTypes *fieldsParse =
      [TestPackedTypes parseFromData:extsData error:&error];
  XCTAssertNotNil(fieldsParse);
  XCTAssertNil(error);
  XCTAssertEqualObjects(fieldsParse, fieldsOrig);

  error = nil;
  TestPackedExtensions *extsParse =
      [TestPackedExtensions parseFromData:fieldsData
                        extensionRegistry:[UnittestRoot extensionRegistry]
                                    error:&error];
  XCTAssertNotNil(extsParse);
  XCTAssertNil(error);
  XCTAssertEqualObjects(extsParse, extsOrig);
}

- (void)testUnpackedExtensionVsFieldParsing {
  // Extensions and fields end up on the wire the same way, so they can parse
  // each other.

  TestUnpackedTypes *fieldsOrig = [TestUnpackedTypes message];
  TestUnpackedExtensions *extsOrig = [TestUnpackedExtensions message];
  [self setUnpackedFields:fieldsOrig repeatedCount:3];
  [self setUnpackedExtensions:extsOrig repeatedCount:3];

  NSData *fieldsData = [fieldsOrig data];
  NSData *extsData = [extsOrig data];
  XCTAssertNotNil(fieldsData);
  XCTAssertNotNil(extsData);
  XCTAssertEqualObjects(fieldsData, extsData);

  TestUnpackedTypes *fieldsParse =
      [TestUnpackedTypes parseFromData:extsData error:NULL];
  XCTAssertNotNil(fieldsParse);
  XCTAssertEqualObjects(fieldsParse, fieldsOrig);

  TestUnpackedExtensions *extsParse =
      [TestUnpackedExtensions parseFromData:fieldsData
                          extensionRegistry:[UnittestRoot extensionRegistry]
                                      error:NULL];
  XCTAssertNotNil(extsParse);
  XCTAssertEqualObjects(extsParse, extsOrig);
}

- (void)testErrorSubsectionInvalidLimit {
  NSData *data = DataFromCStr(
      "\x0A\x08\x0A\x07\x12\x04\x72\x02\x4B\x50\x12\x04\x72\x02\x4B\x50");
  NSError *error = nil;
  NestedTestAllTypes *msg = [NestedTestAllTypes parseFromData:data
                                                        error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorInvalidSubsectionLimit);
}

- (void)testErrorSubsectionLimitReached {
  NSData *data = DataFromCStr("\x0A\x06\x12\x03\x72\x02\x4B\x50");
  NSError *error = nil;
  NestedTestAllTypes *msg = [NestedTestAllTypes parseFromData:data
                                                        error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorSubsectionLimitReached);
}

- (void)testErrorInvalidVarint {
  NSData *data = DataFromCStr("\x72\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF");
  NSError *error = nil;
  TestAllTypes *msg = [TestAllTypes parseFromData:data error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorInvalidVarInt);
}

- (void)testErrorInvalidUTF8 {
  NSData *data = DataFromCStr("\x72\x04\xF4\xFF\xFF\xFF");
  NSError *error = nil;
  TestAllTypes *msg = [TestAllTypes parseFromData:data error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorInvalidUTF8);
}

- (void)testErrorInvalidSize {
  NSData *data = DataFromCStr("\x72\x03\x4B\x50");
  NSError *error = nil;
  NestedTestAllTypes *msg = [NestedTestAllTypes parseFromData:data
                                                        error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorInvalidSize);
}

- (void)testErrorInvalidTag {
  NSData *data = DataFromCStr("\x0F");
  NSError *error = nil;
  NestedTestAllTypes *msg = [NestedTestAllTypes parseFromData:data
                                                        error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorInvalidTag);
}

- (void)testErrorRecursionDepthReached {
  NSData *data = DataFromCStr(
      "\x0A\x86\x01\x0A\x83\x01\x0A\x80\x01\x0A\x7E\x0A\x7C\x0A\x7A\x0A\x78"
      "\x0A\x76\x0A\x74\x0A\x72\x0A\x70\x0A\x6E\x0A\x6C\x0A\x6A\x0A\x68"
      "\x0A\x66\x0A\x64\x0A\x62\x0A\x60\x0A\x5E\x0A\x5C\x0A\x5A\x0A\x58"
      "\x0A\x56\x0A\x54\x0A\x52\x0A\x50\x0A\x4E\x0A\x4C\x0A\x4A\x0A\x48"
      "\x0A\x46\x0A\x44\x0A\x42\x0A\x40\x0A\x3E\x0A\x3C\x0A\x3A\x0A\x38"
      "\x0A\x36\x0A\x34\x0A\x32\x0A\x30\x0A\x2E\x0A\x2C\x0A\x2A\x0A\x28"
      "\x0A\x26\x0A\x24\x0A\x22\x0A\x20\x0A\x1E\x0A\x1C\x0A\x1A\x0A\x18"
      "\x0A\x16\x0A\x14\x0A\x12\x0A\x10\x0A\x0E\x0A\x0C\x0A\x0A\x0A\x08"
      "\x0A\x06\x12\x04\x72\x02\x4B\x50");
  NSError *error = nil;
  NestedTestAllTypes *msg = [NestedTestAllTypes parseFromData:data
                                                        error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorRecursionDepthExceeded);
}

#ifdef DEBUG
- (void)testErrorMissingRequiredField {
  NSData *data = DataFromCStr("");
  NSError *error = nil;
  TestRequired *msg = [TestRequired parseFromData:data error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBMessageErrorDomain);
  XCTAssertEqual(error.code, GPBMessageErrorCodeMissingRequiredField);
}
#endif

#pragma mark - Subset from from map_tests.cc

// TEST(GeneratedMapFieldTest, StandardWireFormat)
- (void)testMap_StandardWireFormat {
  NSData *data = DataFromCStr("\x0A\x04\x08\x01\x10\x01");

  TestMap *msg = [[TestMap alloc] initWithData:data error:NULL];
  XCTAssertEqual(msg.mapInt32Int32.count, 1U);
  int32_t val = 666;
  XCTAssertTrue([msg.mapInt32Int32 getInt32:&val forKey:1]);
  XCTAssertEqual(val, 1);

  [msg release];
}

// TEST(GeneratedMapFieldTest, UnorderedWireFormat)
- (void)testMap_UnorderedWireFormat {
  // put value before key in wire format
  NSData *data = DataFromCStr("\x0A\x04\x10\x01\x08\x02");

  TestMap *msg = [[TestMap alloc] initWithData:data error:NULL];
  XCTAssertEqual(msg.mapInt32Int32.count, 1U);
  int32_t val = 666;
  XCTAssertTrue([msg.mapInt32Int32 getInt32:&val forKey:2]);
  XCTAssertEqual(val, 1);

  [msg release];
}

// TEST(GeneratedMapFieldTest, DuplicatedKeyWireFormat)
- (void)testMap_DuplicatedKeyWireFormat {
  // Two key fields in wire format
  NSData *data = DataFromCStr("\x0A\x06\x08\x01\x08\x02\x10\x01");

  TestMap *msg = [[TestMap alloc] initWithData:data error:NULL];
  XCTAssertEqual(msg.mapInt32Int32.count, 1U);
  int32_t val = 666;
  XCTAssertTrue([msg.mapInt32Int32 getInt32:&val forKey:2]);
  XCTAssertEqual(val, 1);

  [msg release];
}

// TEST(GeneratedMapFieldTest, DuplicatedValueWireFormat)
- (void)testMap_DuplicatedValueWireFormat {
  // Two value fields in wire format
  NSData *data = DataFromCStr("\x0A\x06\x08\x01\x10\x01\x10\x02");

  TestMap *msg = [[TestMap alloc] initWithData:data error:NULL];
  XCTAssertEqual(msg.mapInt32Int32.count, 1U);
  int32_t val = 666;
  XCTAssertTrue([msg.mapInt32Int32 getInt32:&val forKey:1]);
  XCTAssertEqual(val, 2);

  [msg release];
}

// TEST(GeneratedMapFieldTest, MissedKeyWireFormat)
- (void)testMap_MissedKeyWireFormat {
  // No key field in wire format
  NSData *data = DataFromCStr("\x0A\x02\x10\x01");

  TestMap *msg = [[TestMap alloc] initWithData:data error:NULL];
  XCTAssertEqual(msg.mapInt32Int32.count, 1U);
  int32_t val = 666;
  XCTAssertTrue([msg.mapInt32Int32 getInt32:&val forKey:0]);
  XCTAssertEqual(val, 1);

  [msg release];
}

// TEST(GeneratedMapFieldTest, MissedValueWireFormat)
- (void)testMap_MissedValueWireFormat {
  // No value field in wire format
  NSData *data = DataFromCStr("\x0A\x02\x08\x01");

  TestMap *msg = [[TestMap alloc] initWithData:data error:NULL];
  XCTAssertEqual(msg.mapInt32Int32.count, 1U);
  int32_t val = 666;
  XCTAssertTrue([msg.mapInt32Int32 getInt32:&val forKey:1]);
  XCTAssertEqual(val, 0);

  [msg release];
}

// TEST(GeneratedMapFieldTest, UnknownFieldWireFormat)
- (void)testMap_UnknownFieldWireFormat {
  // Unknown field in wire format
  NSData *data = DataFromCStr("\x0A\x06\x08\x02\x10\x03\x18\x01");

  TestMap *msg = [[TestMap alloc] initWithData:data error:NULL];
  XCTAssertEqual(msg.mapInt32Int32.count, 1U);
  int32_t val = 666;
  XCTAssertTrue([msg.mapInt32Int32 getInt32:&val forKey:2]);
  XCTAssertEqual(val, 3);

  [msg release];
}

// TEST(GeneratedMapFieldTest, CorruptedWireFormat)
- (void)testMap_CorruptedWireFormat {
  // corrupted data in wire format
  NSData *data = DataFromCStr("\x0A\x06\x08\x02\x11\x03");

  NSError *error = nil;
  TestMap *msg = [TestMap parseFromData:data error:&error];
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBCodedInputStreamErrorDomain);
  XCTAssertEqual(error.code, GPBCodedInputStreamErrorInvalidSubsectionLimit);
}

// TEST(GeneratedMapFieldTest, Proto2UnknownEnum)
- (void)testMap_Proto2UnknownEnum {
  TestEnumMapPlusExtra *orig = [[TestEnumMapPlusExtra alloc] init];

  orig.knownMapField = [GPBInt32EnumDictionary
      dictionaryWithValidationFunction:Proto2MapEnumPlusExtra_IsValidValue];
  orig.unknownMapField = [GPBInt32EnumDictionary
      dictionaryWithValidationFunction:Proto2MapEnumPlusExtra_IsValidValue];
  [orig.knownMapField setEnum:Proto2MapEnumPlusExtra_EProto2MapEnumFoo
                       forKey:0];
  [orig.unknownMapField setEnum:Proto2MapEnumPlusExtra_EProto2MapEnumExtra
                         forKey:0];

  NSData *data = [orig data];
  XCTAssertNotNil(data);
  TestEnumMap *msg1 = [TestEnumMap parseFromData:data error:NULL];
  XCTAssertEqual(msg1.knownMapField.count, 1U);
  int32_t val = -1;
  XCTAssertTrue([msg1.knownMapField getEnum:&val forKey:0]);
  XCTAssertEqual(val, Proto2MapEnum_Proto2MapEnumFoo);
  XCTAssertEqual(msg1.unknownFields.countOfFields, 1U);

  data = [msg1 data];
  TestEnumMapPlusExtra *msg2 =
      [TestEnumMapPlusExtra parseFromData:data error:NULL];
  val = -1;
  XCTAssertEqual(msg2.knownMapField.count, 1U);
  XCTAssertTrue([msg2.knownMapField getEnum:&val forKey:0]);
  XCTAssertEqual(val, Proto2MapEnumPlusExtra_EProto2MapEnumFoo);
  val = -1;
  XCTAssertEqual(msg2.unknownMapField.count, 1U);
  XCTAssertTrue([msg2.unknownMapField getEnum:&val forKey:0]);
  XCTAssertEqual(val, Proto2MapEnumPlusExtra_EProto2MapEnumExtra);
  XCTAssertEqual(msg2.unknownFields.countOfFields, 0U);

  XCTAssertEqualObjects(orig, msg2);

  [orig release];
}

#pragma mark - Map Round Tripping

- (void)testProto2MapRoundTripping {
  Message2 *msg = [[Message2 alloc] init];

  // Key/Value data should result in different byte lengths on wire to ensure
  // everything is right.
  [msg.mapInt32Int32 setInt32:1000 forKey:200];
  [msg.mapInt32Int32 setInt32:101 forKey:2001];
  [msg.mapInt64Int64 setInt64:1002 forKey:202];
  [msg.mapInt64Int64 setInt64:103 forKey:2003];
  [msg.mapUint32Uint32 setUInt32:1004 forKey:204];
  [msg.mapUint32Uint32 setUInt32:105 forKey:2005];
  [msg.mapUint64Uint64 setUInt64:1006 forKey:206];
  [msg.mapUint64Uint64 setUInt64:107 forKey:2007];
  [msg.mapSint32Sint32 setInt32:1008 forKey:208];
  [msg.mapSint32Sint32 setInt32:109 forKey:2009];
  [msg.mapSint64Sint64 setInt64:1010 forKey:210];
  [msg.mapSint64Sint64 setInt64:111 forKey:2011];
  [msg.mapFixed32Fixed32 setUInt32:1012 forKey:212];
  [msg.mapFixed32Fixed32 setUInt32:113 forKey:2013];
  [msg.mapFixed64Fixed64 setUInt64:1014 forKey:214];
  [msg.mapFixed64Fixed64 setUInt64:115 forKey:2015];
  [msg.mapSfixed32Sfixed32 setInt32:1016 forKey:216];
  [msg.mapSfixed32Sfixed32 setInt32:117 forKey:2017];
  [msg.mapSfixed64Sfixed64 setInt64:1018 forKey:218];
  [msg.mapSfixed64Sfixed64 setInt64:119 forKey:2019];
  [msg.mapInt32Float setFloat:1020.f forKey:220];
  [msg.mapInt32Float setFloat:121.f forKey:2021];
  [msg.mapInt32Double setDouble:1022. forKey:222];
  [msg.mapInt32Double setDouble:123. forKey:2023];
  [msg.mapBoolBool setBool:false forKey:true];
  [msg.mapBoolBool setBool:true forKey:false];
  msg.mapStringString[@"224"] = @"1024";
  msg.mapStringString[@"2025"] = @"125";
  msg.mapStringBytes[@"226"] = DataFromCStr("1026");
  msg.mapStringBytes[@"2027"] = DataFromCStr("127");
  Message2 *val1 = [[Message2 alloc] init];
  val1.optionalInt32 = 1028;
  Message2 *val2 = [[Message2 alloc] init];
  val2.optionalInt32 = 129;
  [msg.mapStringMessage setObject:val1 forKey:@"228"];
  [msg.mapStringMessage setObject:val2 forKey:@"2029"];
  [msg.mapInt32Bytes setObject:DataFromCStr("1030 bytes") forKey:230];
  [msg.mapInt32Bytes setObject:DataFromCStr("131") forKey:2031];
  [msg.mapInt32Enum setEnum:Message2_Enum_Bar forKey:232];
  [msg.mapInt32Enum setEnum:Message2_Enum_Baz forKey:2033];
  Message2 *val3 = [[Message2 alloc] init];
  val3.optionalInt32 = 1034;
  Message2 *val4 = [[Message2 alloc] init];
  val4.optionalInt32 = 135;
  [msg.mapInt32Message setObject:val3 forKey:234];
  [msg.mapInt32Message setObject:val4 forKey:2035];

  NSData *data = [msg data];
  XCTAssertNotNil(data);
  Message2 *msg2 = [[Message2 alloc] initWithData:data error:NULL];

  XCTAssertNotEqual(msg2, msg);  // Pointer comparison
  XCTAssertEqualObjects(msg2, msg);

  [val4 release];
  [val3 release];
  [val2 release];
  [val1 release];
  [msg2 release];
  [msg release];
}

@end
