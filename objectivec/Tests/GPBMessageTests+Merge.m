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

#import "google/protobuf/MapUnittest.pbobjc.h"
#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestPreserveUnknownEnum.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto2.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto3.pbobjc.h"

@interface MessageMergeTests : GPBTestCase
@end

@implementation MessageMergeTests

// TODO(thomasvl): Pull tests over from GPBMessageTests that are merge specific.

- (void)testProto3MergingAndZeroValues {
  // Proto2 covered in other tests.

  Message3 *src = [[Message3 alloc] init];
  Message3 *dst = [[Message3 alloc] init];
  NSData *testData1 = [@"abc" dataUsingEncoding:NSUTF8StringEncoding];
  NSData *testData2 = [@"def" dataUsingEncoding:NSUTF8StringEncoding];

  dst.optionalInt32 = 1;
  dst.optionalInt64 = 1;
  dst.optionalUint32 = 1;
  dst.optionalUint64 = 1;
  dst.optionalSint32 = 1;
  dst.optionalSint64 = 1;
  dst.optionalFixed32 = 1;
  dst.optionalFixed64 = 1;
  dst.optionalSfixed32 = 1;
  dst.optionalSfixed64 = 1;
  dst.optionalFloat = 1.0f;
  dst.optionalDouble = 1.0;
  dst.optionalBool = YES;
  dst.optionalString = @"bar";
  dst.optionalBytes = testData1;
  dst.optionalEnum = Message3_Enum_Bar;

  // All zeros, nothing should overright.

  src.optionalInt32 = 0;
  src.optionalInt64 = 0;
  src.optionalUint32 = 0;
  src.optionalUint64 = 0;
  src.optionalSint32 = 0;
  src.optionalSint64 = 0;
  src.optionalFixed32 = 0;
  src.optionalFixed64 = 0;
  src.optionalSfixed32 = 0;
  src.optionalSfixed64 = 0;
  src.optionalFloat = 0.0f;
  src.optionalDouble = 0.0;
  src.optionalBool = NO;
  src.optionalString = @"";
  src.optionalBytes = [NSData data];
  src.optionalEnum = Message3_Enum_Foo;  // first value

  [dst mergeFrom:src];

  XCTAssertEqual(dst.optionalInt32, 1);
  XCTAssertEqual(dst.optionalInt64, 1);
  XCTAssertEqual(dst.optionalUint32, 1U);
  XCTAssertEqual(dst.optionalUint64, 1U);
  XCTAssertEqual(dst.optionalSint32, 1);
  XCTAssertEqual(dst.optionalSint64, 1);
  XCTAssertEqual(dst.optionalFixed32, 1U);
  XCTAssertEqual(dst.optionalFixed64, 1U);
  XCTAssertEqual(dst.optionalSfixed32, 1);
  XCTAssertEqual(dst.optionalSfixed64, 1);
  XCTAssertEqual(dst.optionalFloat, 1.0f);
  XCTAssertEqual(dst.optionalDouble, 1.0);
  XCTAssertEqual(dst.optionalBool, YES);
  XCTAssertEqualObjects(dst.optionalString, @"bar");
  XCTAssertEqualObjects(dst.optionalBytes, testData1);
  XCTAssertEqual(dst.optionalEnum, Message3_Enum_Bar);

  // Half the values that will replace.

  src.optionalInt32 = 0;
  src.optionalInt64 = 2;
  src.optionalUint32 = 0;
  src.optionalUint64 = 2;
  src.optionalSint32 = 0;
  src.optionalSint64 = 2;
  src.optionalFixed32 = 0;
  src.optionalFixed64 = 2;
  src.optionalSfixed32 = 0;
  src.optionalSfixed64 = 2;
  src.optionalFloat = 0.0f;
  src.optionalDouble = 2.0;
  src.optionalBool = YES;  // No other value to use.  :(
  src.optionalString = @"baz";
  src.optionalBytes = nil;
  src.optionalEnum = Message3_Enum_Baz;

  [dst mergeFrom:src];

  XCTAssertEqual(dst.optionalInt32, 1);
  XCTAssertEqual(dst.optionalInt64, 2);
  XCTAssertEqual(dst.optionalUint32, 1U);
  XCTAssertEqual(dst.optionalUint64, 2U);
  XCTAssertEqual(dst.optionalSint32, 1);
  XCTAssertEqual(dst.optionalSint64, 2);
  XCTAssertEqual(dst.optionalFixed32, 1U);
  XCTAssertEqual(dst.optionalFixed64, 2U);
  XCTAssertEqual(dst.optionalSfixed32, 1);
  XCTAssertEqual(dst.optionalSfixed64, 2);
  XCTAssertEqual(dst.optionalFloat, 1.0f);
  XCTAssertEqual(dst.optionalDouble, 2.0);
  XCTAssertEqual(dst.optionalBool, YES);
  XCTAssertEqualObjects(dst.optionalString, @"baz");
  XCTAssertEqualObjects(dst.optionalBytes, testData1);
  XCTAssertEqual(dst.optionalEnum, Message3_Enum_Baz);

  // Other half the values that will replace.

  src.optionalInt32 = 3;
  src.optionalInt64 = 0;
  src.optionalUint32 = 3;
  src.optionalUint64 = 0;
  src.optionalSint32 = 3;
  src.optionalSint64 = 0;
  src.optionalFixed32 = 3;
  src.optionalFixed64 = 0;
  src.optionalSfixed32 = 3;
  src.optionalSfixed64 = 0;
  src.optionalFloat = 3.0f;
  src.optionalDouble = 0.0;
  src.optionalBool = YES;  // No other value to use.  :(
  src.optionalString = nil;
  src.optionalBytes = testData2;
  src.optionalEnum = Message3_Enum_Foo;

  [dst mergeFrom:src];

  XCTAssertEqual(dst.optionalInt32, 3);
  XCTAssertEqual(dst.optionalInt64, 2);
  XCTAssertEqual(dst.optionalUint32, 3U);
  XCTAssertEqual(dst.optionalUint64, 2U);
  XCTAssertEqual(dst.optionalSint32, 3);
  XCTAssertEqual(dst.optionalSint64, 2);
  XCTAssertEqual(dst.optionalFixed32, 3U);
  XCTAssertEqual(dst.optionalFixed64, 2U);
  XCTAssertEqual(dst.optionalSfixed32, 3);
  XCTAssertEqual(dst.optionalSfixed64, 2);
  XCTAssertEqual(dst.optionalFloat, 3.0f);
  XCTAssertEqual(dst.optionalDouble, 2.0);
  XCTAssertEqual(dst.optionalBool, YES);
  XCTAssertEqualObjects(dst.optionalString, @"baz");
  XCTAssertEqualObjects(dst.optionalBytes, testData2);
  XCTAssertEqual(dst.optionalEnum, Message3_Enum_Baz);

  [src release];
  [dst release];
}

- (void)testProto3MergingEnums {
  UnknownEnumsMyMessage *src = [UnknownEnumsMyMessage message];
  UnknownEnumsMyMessage *dst = [UnknownEnumsMyMessage message];

  // Known value.

  src.e = UnknownEnumsMyEnum_Bar;
  src.repeatedEArray =
      [GPBEnumArray arrayWithValidationFunction:UnknownEnumsMyEnum_IsValidValue
                                       rawValue:UnknownEnumsMyEnum_Bar];
  src.repeatedPackedEArray =
      [GPBEnumArray arrayWithValidationFunction:UnknownEnumsMyEnum_IsValidValue
                                       rawValue:UnknownEnumsMyEnum_Bar];
  src.oneofE1 = UnknownEnumsMyEnum_Bar;

  [dst mergeFrom:src];

  XCTAssertEqual(dst.e, UnknownEnumsMyEnum_Bar);
  XCTAssertEqual(dst.repeatedEArray.count, 1U);
  XCTAssertEqual([dst.repeatedEArray valueAtIndex:0], UnknownEnumsMyEnum_Bar);
  XCTAssertEqual(dst.repeatedPackedEArray.count, 1U);
  XCTAssertEqual([dst.repeatedPackedEArray valueAtIndex:0],
                 UnknownEnumsMyEnum_Bar);
  XCTAssertEqual(dst.oneofE1, UnknownEnumsMyEnum_Bar);

  // Unknown value.

  const int32_t kUnknownValue = 666;

  SetUnknownEnumsMyMessage_E_RawValue(src, kUnknownValue);
  src.repeatedEArray =
      [GPBEnumArray arrayWithValidationFunction:UnknownEnumsMyEnum_IsValidValue
                                       rawValue:kUnknownValue];
  src.repeatedPackedEArray =
      [GPBEnumArray arrayWithValidationFunction:UnknownEnumsMyEnum_IsValidValue
                                       rawValue:kUnknownValue];
  SetUnknownEnumsMyMessage_OneofE1_RawValue(src, kUnknownValue);

  [dst mergeFrom:src];

  XCTAssertEqual(dst.e, UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual(UnknownEnumsMyMessage_E_RawValue(dst), kUnknownValue);
  XCTAssertEqual(dst.repeatedEArray.count, 2U);
  XCTAssertEqual([dst.repeatedEArray valueAtIndex:0], UnknownEnumsMyEnum_Bar);
  XCTAssertEqual([dst.repeatedEArray valueAtIndex:1],
                 UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([dst.repeatedEArray rawValueAtIndex:1], kUnknownValue);
  XCTAssertEqual(dst.repeatedPackedEArray.count, 2U);
  XCTAssertEqual([dst.repeatedPackedEArray valueAtIndex:0],
                 UnknownEnumsMyEnum_Bar);
  XCTAssertEqual([dst.repeatedPackedEArray valueAtIndex:1],
                 UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([dst.repeatedPackedEArray rawValueAtIndex:1], kUnknownValue);
  XCTAssertEqual(dst.oneofE1,
                 UnknownEnumsMyEnum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual(UnknownEnumsMyMessage_OneofE1_RawValue(dst), kUnknownValue);
}

- (void)testProto2MergeOneof {
  Message2 *src = [Message2 message];
  Message2 *dst = [Message2 message];

  //
  // Make sure whatever is in dst gets cleared out be merging in something else.
  //

  dst.oneofEnum = Message2_Enum_Bar;

//%PDDM-DEFINE MERGE2_TEST(SET_NAME, SET_VALUE, CLEARED_NAME, CLEARED_DEFAULT)
//%  src.oneof##SET_NAME = SET_VALUE;
//%  [dst mergeFrom:src];
//%  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_Oneof##SET_NAME);
//%  XCTAssertEqual(dst.oneof##SET_NAME, SET_VALUE);
//%  XCTAssertEqual(dst.oneof##CLEARED_NAME, CLEARED_DEFAULT);
//%
//%PDDM-EXPAND MERGE2_TEST(Int32, 10, Enum, Message2_Enum_Baz)
// This block of code is generated, do not edit it directly.

  src.oneofInt32 = 10;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofInt32);
  XCTAssertEqual(dst.oneofInt32, 10);
  XCTAssertEqual(dst.oneofEnum, Message2_Enum_Baz);

//%PDDM-EXPAND MERGE2_TEST(Int64, 11, Int32, 100)
// This block of code is generated, do not edit it directly.

  src.oneofInt64 = 11;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofInt64);
  XCTAssertEqual(dst.oneofInt64, 11);
  XCTAssertEqual(dst.oneofInt32, 100);

//%PDDM-EXPAND MERGE2_TEST(Uint32, 12U, Int64, 101)
// This block of code is generated, do not edit it directly.

  src.oneofUint32 = 12U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofUint32);
  XCTAssertEqual(dst.oneofUint32, 12U);
  XCTAssertEqual(dst.oneofInt64, 101);

//%PDDM-EXPAND MERGE2_TEST(Uint64, 13U, Uint32, 102U)
// This block of code is generated, do not edit it directly.

  src.oneofUint64 = 13U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofUint64);
  XCTAssertEqual(dst.oneofUint64, 13U);
  XCTAssertEqual(dst.oneofUint32, 102U);

//%PDDM-EXPAND MERGE2_TEST(Sint32, 14, Uint64, 103U)
// This block of code is generated, do not edit it directly.

  src.oneofSint32 = 14;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofSint32);
  XCTAssertEqual(dst.oneofSint32, 14);
  XCTAssertEqual(dst.oneofUint64, 103U);

//%PDDM-EXPAND MERGE2_TEST(Sint64, 15, Sint32, 104)
// This block of code is generated, do not edit it directly.

  src.oneofSint64 = 15;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofSint64);
  XCTAssertEqual(dst.oneofSint64, 15);
  XCTAssertEqual(dst.oneofSint32, 104);

//%PDDM-EXPAND MERGE2_TEST(Fixed32, 16U, Sint64, 105)
// This block of code is generated, do not edit it directly.

  src.oneofFixed32 = 16U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofFixed32);
  XCTAssertEqual(dst.oneofFixed32, 16U);
  XCTAssertEqual(dst.oneofSint64, 105);

//%PDDM-EXPAND MERGE2_TEST(Fixed64, 17U, Fixed32, 106U)
// This block of code is generated, do not edit it directly.

  src.oneofFixed64 = 17U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofFixed64);
  XCTAssertEqual(dst.oneofFixed64, 17U);
  XCTAssertEqual(dst.oneofFixed32, 106U);

//%PDDM-EXPAND MERGE2_TEST(Sfixed32, 18, Fixed64, 107U)
// This block of code is generated, do not edit it directly.

  src.oneofSfixed32 = 18;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofSfixed32);
  XCTAssertEqual(dst.oneofSfixed32, 18);
  XCTAssertEqual(dst.oneofFixed64, 107U);

//%PDDM-EXPAND MERGE2_TEST(Sfixed64, 19, Sfixed32, 108)
// This block of code is generated, do not edit it directly.

  src.oneofSfixed64 = 19;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofSfixed64);
  XCTAssertEqual(dst.oneofSfixed64, 19);
  XCTAssertEqual(dst.oneofSfixed32, 108);

//%PDDM-EXPAND MERGE2_TEST(Float, 20.0f, Sfixed64, 109)
// This block of code is generated, do not edit it directly.

  src.oneofFloat = 20.0f;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofFloat);
  XCTAssertEqual(dst.oneofFloat, 20.0f);
  XCTAssertEqual(dst.oneofSfixed64, 109);

//%PDDM-EXPAND MERGE2_TEST(Double, 21.0, Float, 110.0f)
// This block of code is generated, do not edit it directly.

  src.oneofDouble = 21.0;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofDouble);
  XCTAssertEqual(dst.oneofDouble, 21.0);
  XCTAssertEqual(dst.oneofFloat, 110.0f);

//%PDDM-EXPAND MERGE2_TEST(Bool, NO, Double, 111.0)
// This block of code is generated, do not edit it directly.

  src.oneofBool = NO;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofBool);
  XCTAssertEqual(dst.oneofBool, NO);
  XCTAssertEqual(dst.oneofDouble, 111.0);

//%PDDM-EXPAND MERGE2_TEST(Enum, Message2_Enum_Bar, Bool, YES)
// This block of code is generated, do not edit it directly.

  src.oneofEnum = Message2_Enum_Bar;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofEnum);
  XCTAssertEqual(dst.oneofEnum, Message2_Enum_Bar);
  XCTAssertEqual(dst.oneofBool, YES);

//%PDDM-EXPAND-END (14 expansions)

  NSString *oneofStringDefault = @"string";
  NSData *oneofBytesDefault = [@"data" dataUsingEncoding:NSUTF8StringEncoding];

  src.oneofString = @"foo";
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofString);
  XCTAssertEqualObjects(dst.oneofString, @"foo");
  XCTAssertEqual(dst.oneofEnum, Message2_Enum_Baz);

  src.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofBytes);
  XCTAssertEqualObjects(dst.oneofBytes,
                        [@"bar" dataUsingEncoding:NSUTF8StringEncoding]);
  XCTAssertEqualObjects(dst.oneofString, oneofStringDefault);

  Message2_OneofGroup *group = [Message2_OneofGroup message];
  group.a = 666;
  src.oneofGroup = group;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofGroup);
  Message2_OneofGroup *mergedGroup = [[dst.oneofGroup retain] autorelease];
  XCTAssertNotNil(mergedGroup);
  XCTAssertNotEqual(mergedGroup, group);  // Pointer comparision.
  XCTAssertEqualObjects(mergedGroup, group);
  XCTAssertEqualObjects(dst.oneofBytes, oneofBytesDefault);

  Message2 *subMessage = [Message2 message];
  subMessage.optionalInt32 = 777;
  src.oneofMessage = subMessage;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofMessage);
  Message2 *mergedSubMessage = [[dst.oneofMessage retain] autorelease];
  XCTAssertNotNil(mergedSubMessage);
  XCTAssertNotEqual(mergedSubMessage, subMessage);  // Pointer comparision.
  XCTAssertEqualObjects(mergedSubMessage, subMessage);
  XCTAssertNotNil(dst.oneofGroup);
  XCTAssertNotEqual(dst.oneofGroup, mergedGroup);  // Pointer comparision.

  // Back to something else to make sure message clears out ok.

  src.oneofInt32 = 10;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofInt32);
  XCTAssertNotNil(dst.oneofMessage);
  XCTAssertNotEqual(dst.oneofMessage,
                    mergedSubMessage);  // Pointer comparision.

  //
  // Test merging in to message/group when they already had something.
  //

  src.oneofGroup = group;
  mergedGroup = [Message2_OneofGroup message];
  mergedGroup.b = 888;
  dst.oneofGroup = mergedGroup;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofGroup);
  // Shouldn't have been a new object.
  XCTAssertEqual(dst.oneofGroup, mergedGroup);  // Pointer comparision.
  XCTAssertEqual(dst.oneofGroup.a, 666);        // Pointer comparision.
  XCTAssertEqual(dst.oneofGroup.b, 888);        // Pointer comparision.

  src.oneofMessage = subMessage;
  mergedSubMessage = [Message2 message];
  mergedSubMessage.optionalInt64 = 999;
  dst.oneofMessage = mergedSubMessage;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message2_O_OneOfCase_OneofMessage);
  // Shouldn't have been a new object.
  XCTAssertEqual(dst.oneofMessage, mergedSubMessage);   // Pointer comparision.
  XCTAssertEqual(dst.oneofMessage.optionalInt32, 777);  // Pointer comparision.
  XCTAssertEqual(dst.oneofMessage.optionalInt64, 999);  // Pointer comparision.
}

- (void)testProto3MergeOneof {
  Message3 *src = [Message3 message];
  Message3 *dst = [Message3 message];

  //
  // Make sure whatever is in dst gets cleared out be merging in something else.
  //

  dst.oneofEnum = Message3_Enum_Bar;

//%PDDM-DEFINE MERGE3_TEST(SET_NAME, SET_VALUE, CLEARED_NAME, CLEARED_DEFAULT)
//%  src.oneof##SET_NAME = SET_VALUE;
//%  [dst mergeFrom:src];
//%  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_Oneof##SET_NAME);
//%  XCTAssertEqual(dst.oneof##SET_NAME, SET_VALUE);
//%  XCTAssertEqual(dst.oneof##CLEARED_NAME, CLEARED_DEFAULT);
//%
//%PDDM-EXPAND MERGE3_TEST(Int32, 10, Enum, Message3_Enum_Foo)
// This block of code is generated, do not edit it directly.

  src.oneofInt32 = 10;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofInt32);
  XCTAssertEqual(dst.oneofInt32, 10);
  XCTAssertEqual(dst.oneofEnum, Message3_Enum_Foo);

//%PDDM-EXPAND MERGE3_TEST(Int64, 11, Int32, 0)
// This block of code is generated, do not edit it directly.

  src.oneofInt64 = 11;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofInt64);
  XCTAssertEqual(dst.oneofInt64, 11);
  XCTAssertEqual(dst.oneofInt32, 0);

//%PDDM-EXPAND MERGE3_TEST(Uint32, 12U, Int64, 0)
// This block of code is generated, do not edit it directly.

  src.oneofUint32 = 12U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofUint32);
  XCTAssertEqual(dst.oneofUint32, 12U);
  XCTAssertEqual(dst.oneofInt64, 0);

//%PDDM-EXPAND MERGE3_TEST(Uint64, 13U, Uint32, 0U)
// This block of code is generated, do not edit it directly.

  src.oneofUint64 = 13U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofUint64);
  XCTAssertEqual(dst.oneofUint64, 13U);
  XCTAssertEqual(dst.oneofUint32, 0U);

//%PDDM-EXPAND MERGE3_TEST(Sint32, 14, Uint64, 0U)
// This block of code is generated, do not edit it directly.

  src.oneofSint32 = 14;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofSint32);
  XCTAssertEqual(dst.oneofSint32, 14);
  XCTAssertEqual(dst.oneofUint64, 0U);

//%PDDM-EXPAND MERGE3_TEST(Sint64, 15, Sint32, 0)
// This block of code is generated, do not edit it directly.

  src.oneofSint64 = 15;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofSint64);
  XCTAssertEqual(dst.oneofSint64, 15);
  XCTAssertEqual(dst.oneofSint32, 0);

//%PDDM-EXPAND MERGE3_TEST(Fixed32, 16U, Sint64, 0)
// This block of code is generated, do not edit it directly.

  src.oneofFixed32 = 16U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofFixed32);
  XCTAssertEqual(dst.oneofFixed32, 16U);
  XCTAssertEqual(dst.oneofSint64, 0);

//%PDDM-EXPAND MERGE3_TEST(Fixed64, 17U, Fixed32, 0U)
// This block of code is generated, do not edit it directly.

  src.oneofFixed64 = 17U;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofFixed64);
  XCTAssertEqual(dst.oneofFixed64, 17U);
  XCTAssertEqual(dst.oneofFixed32, 0U);

//%PDDM-EXPAND MERGE3_TEST(Sfixed32, 18, Fixed64, 0U)
// This block of code is generated, do not edit it directly.

  src.oneofSfixed32 = 18;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofSfixed32);
  XCTAssertEqual(dst.oneofSfixed32, 18);
  XCTAssertEqual(dst.oneofFixed64, 0U);

//%PDDM-EXPAND MERGE3_TEST(Sfixed64, 19, Sfixed32, 0)
// This block of code is generated, do not edit it directly.

  src.oneofSfixed64 = 19;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofSfixed64);
  XCTAssertEqual(dst.oneofSfixed64, 19);
  XCTAssertEqual(dst.oneofSfixed32, 0);

//%PDDM-EXPAND MERGE3_TEST(Float, 20.0f, Sfixed64, 0)
// This block of code is generated, do not edit it directly.

  src.oneofFloat = 20.0f;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofFloat);
  XCTAssertEqual(dst.oneofFloat, 20.0f);
  XCTAssertEqual(dst.oneofSfixed64, 0);

//%PDDM-EXPAND MERGE3_TEST(Double, 21.0, Float, 0.0f)
// This block of code is generated, do not edit it directly.

  src.oneofDouble = 21.0;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofDouble);
  XCTAssertEqual(dst.oneofDouble, 21.0);
  XCTAssertEqual(dst.oneofFloat, 0.0f);

//%PDDM-EXPAND MERGE3_TEST(Bool, YES, Double, 0.0)
// This block of code is generated, do not edit it directly.

  src.oneofBool = YES;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofBool);
  XCTAssertEqual(dst.oneofBool, YES);
  XCTAssertEqual(dst.oneofDouble, 0.0);

//%PDDM-EXPAND MERGE3_TEST(Enum, Message3_Enum_Bar, Bool, NO)
// This block of code is generated, do not edit it directly.

  src.oneofEnum = Message3_Enum_Bar;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofEnum);
  XCTAssertEqual(dst.oneofEnum, Message3_Enum_Bar);
  XCTAssertEqual(dst.oneofBool, NO);

//%PDDM-EXPAND-END (14 expansions)

  NSString *oneofStringDefault = @"";
  NSData *oneofBytesDefault = [NSData data];

  src.oneofString = @"foo";
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofString);
  XCTAssertEqualObjects(dst.oneofString, @"foo");
  XCTAssertEqual(dst.oneofEnum, Message3_Enum_Foo);

  src.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofBytes);
  XCTAssertEqualObjects(dst.oneofBytes,
                        [@"bar" dataUsingEncoding:NSUTF8StringEncoding]);
  XCTAssertEqualObjects(dst.oneofString, oneofStringDefault);


  Message3 *subMessage = [Message3 message];
  subMessage.optionalInt32 = 777;
  src.oneofMessage = subMessage;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofMessage);
  Message3 *mergedSubMessage = [[dst.oneofMessage retain] autorelease];
  XCTAssertNotNil(mergedSubMessage);
  XCTAssertNotEqual(mergedSubMessage, subMessage);  // Pointer comparision.
  XCTAssertEqualObjects(mergedSubMessage, subMessage);
  XCTAssertEqualObjects(dst.oneofBytes, oneofBytesDefault);

  // Back to something else to make sure message clears out ok.

  src.oneofInt32 = 10;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofInt32);
  XCTAssertNotNil(dst.oneofMessage);
  XCTAssertNotEqual(dst.oneofMessage,
                    mergedSubMessage);  // Pointer comparision.

  //
  // Test merging in to message when they already had something.
  //

  src.oneofMessage = subMessage;
  mergedSubMessage = [Message3 message];
  mergedSubMessage.optionalInt64 = 999;
  dst.oneofMessage = mergedSubMessage;
  [dst mergeFrom:src];
  XCTAssertEqual(dst.oOneOfCase, Message3_O_OneOfCase_OneofMessage);
  // Shouldn't have been a new object.
  XCTAssertEqual(dst.oneofMessage, mergedSubMessage);   // Pointer comparision.
  XCTAssertEqual(dst.oneofMessage.optionalInt32, 777);  // Pointer comparision.
  XCTAssertEqual(dst.oneofMessage.optionalInt64, 999);  // Pointer comparision.
}

#pragma mark - Subset from from map_tests.cc

// TEST(GeneratedMapFieldTest, CopyFromMessageMap)
- (void)testMap_CopyFromMessageMap {
  TestMessageMap *msg1 = [[TestMessageMap alloc] init];
  TestMessageMap *msg2 = [[TestMessageMap alloc] init];

  TestAllTypes *subMsg = [TestAllTypes message];
  subMsg.repeatedInt32Array = [GPBInt32Array arrayWithValue:100];
  [msg1.mapInt32Message setObject:subMsg forKey:0];
  subMsg = nil;

  subMsg = [TestAllTypes message];
  subMsg.repeatedInt32Array = [GPBInt32Array arrayWithValue:101];

  [msg2.mapInt32Message setObject:subMsg forKey:0];
  subMsg = nil;

  [msg1 mergeFrom:msg2];

  // Checks repeated field is overwritten.
  XCTAssertEqual(msg1.mapInt32Message.count, 1U);
  subMsg = [msg1.mapInt32Message objectForKey:0];
  XCTAssertNotNil(subMsg);
  XCTAssertEqual(subMsg.repeatedInt32Array.count, 1U);
  XCTAssertEqual([subMsg.repeatedInt32Array valueAtIndex:0], 101);

  [msg2 release];
  [msg1 release];
}

@end
