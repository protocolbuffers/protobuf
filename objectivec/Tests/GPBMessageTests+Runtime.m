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
#import "google/protobuf/UnittestObjcStartup.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto2.pbobjc.h"
#import "google/protobuf/UnittestRuntimeProto3.pbobjc.h"

@interface MessageRuntimeTests : GPBTestCase
@end

@implementation MessageRuntimeTests

// TODO(thomasvl): Pull tests over from GPBMessageTests that are runtime
// specific.

- (void)testStartupOrdering {
  // Just have to create a message.  Nothing else uses the classes from
  // this file, so the first selector invoked on the class will initialize
  // it, which also initializes the root.
  TestObjCStartupMessage *message = [TestObjCStartupMessage message];
  XCTAssertNotNil(message);
}

- (void)testProto2HasMethodSupport {
  NSArray *names = @[
    @"Int32",
    @"Int64",
    @"Uint32",
    @"Uint64",
    @"Sint32",
    @"Sint64",
    @"Fixed32",
    @"Fixed64",
    @"Sfixed32",
    @"Sfixed64",
    @"Float",
    @"Double",
    @"Bool",
    @"String",
    @"Bytes",
    @"Group",
    @"Message",
    @"Enum",
  ];

  // Proto2 gets:

  // Single fields - has*/setHas* is valid.

  for (NSString *name in names) {
    // build the selector, i.e. - hasOptionalInt32/setHasOptionalInt32:
    SEL hasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"hasOptional%@", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasOptional%@:", name]);
    XCTAssertTrue([Message2 instancesRespondToSelector:hasSel], @"field: %@",
                  name);
    XCTAssertTrue([Message2 instancesRespondToSelector:setHasSel], @"field: %@",
                  name);
  }

  // Repeated fields
  //  - no has*/setHas*
  //  - *Count

  for (NSString *name in names) {
    // build the selector, i.e. - hasRepeatedInt32Array/setHasRepeatedInt32Array:
    SEL hasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"hasRepeated%@Array", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasRepeated%@Array:", name]);
    XCTAssertFalse([Message2 instancesRespondToSelector:hasSel], @"field: %@",
                   name);
    XCTAssertFalse([Message2 instancesRespondToSelector:setHasSel],
                   @"field: %@", name);
    // build the selector, i.e. - repeatedInt32Array_Count
    SEL countSel = NSSelectorFromString(
        [NSString stringWithFormat:@"repeated%@Array_Count", name]);
    XCTAssertTrue([Message2 instancesRespondToSelector:countSel], @"field: %@",
                   name);
  }

  // OneOf fields - no has*/setHas*

  for (NSString *name in names) {
    // build the selector, i.e. - hasOneofInt32/setHasOneofInt32:
    SEL hasSel =
        NSSelectorFromString([NSString stringWithFormat:@"hasOneof%@", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasOneof%@:", name]);
    XCTAssertFalse([Message2 instancesRespondToSelector:hasSel], @"field: %@",
                   name);
    XCTAssertFalse([Message2 instancesRespondToSelector:setHasSel],
                   @"field: %@", name);
  }

  // map<> fields
  //  - no has*/setHas*
  //  - *Count

  NSArray *mapNames = @[
    @"Int32Int32",
    @"Int64Int64",
    @"Uint32Uint32",
    @"Uint64Uint64",
    @"Sint32Sint32",
    @"Sint64Sint64",
    @"Fixed32Fixed32",
    @"Fixed64Fixed64",
    @"Sfixed32Sfixed32",
    @"Sfixed64Sfixed64",
    @"Int32Float",
    @"Int32Double",
    @"BoolBool",
    @"StringString",
    @"StringBytes",
    @"StringMessage",
    @"Int32Bytes",
    @"Int32Enum",
    @"Int32Message",
  ];

  for (NSString *name in mapNames) {
    // build the selector, i.e. - hasMapInt32Int32/setHasMapInt32Int32:
    SEL hasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"hasMap%@", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasMap%@:", name]);
    XCTAssertFalse([Message2 instancesRespondToSelector:hasSel], @"field: %@",
                   name);
    XCTAssertFalse([Message2 instancesRespondToSelector:setHasSel],
                   @"field: %@", name);
    // build the selector, i.e. - mapInt32Int32Count
    SEL countSel = NSSelectorFromString(
        [NSString stringWithFormat:@"map%@_Count", name]);
    XCTAssertTrue([Message2 instancesRespondToSelector:countSel], @"field: %@",
                   name);
  }

}

- (void)testProto3HasMethodSupport {
  NSArray *names = @[
    @"Int32",
    @"Int64",
    @"Uint32",
    @"Uint64",
    @"Sint32",
    @"Sint64",
    @"Fixed32",
    @"Fixed64",
    @"Sfixed32",
    @"Sfixed64",
    @"Float",
    @"Double",
    @"Bool",
    @"String",
    @"Bytes",
    @"Message",
    @"Enum",
  ];

  // Proto3 gets:

  // Single fields
  //  - has*/setHas* invalid for primative types.
  //  - has*/setHas* valid for Message.

  for (NSString *name in names) {
    // build the selector, i.e. - hasOptionalInt32/setHasOptionalInt32:
    SEL hasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"hasOptional%@", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasOptional%@:", name]);
    if ([name isEqual:@"Message"]) {
      // Sub messages/groups are the exception.
      XCTAssertTrue([Message3 instancesRespondToSelector:hasSel], @"field: %@",
                    name);
      XCTAssertTrue([Message3 instancesRespondToSelector:setHasSel],
                    @"field: %@", name);
    } else {
      XCTAssertFalse([Message3 instancesRespondToSelector:hasSel], @"field: %@",
                     name);
      XCTAssertFalse([Message3 instancesRespondToSelector:setHasSel],
                     @"field: %@", name);
    }
  }

  // Repeated fields
  //  - no has*/setHas*
  //  - *Count

  for (NSString *name in names) {
    // build the selector, i.e. - hasRepeatedInt32Array/setHasRepeatedInt32Array:
    SEL hasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"hasRepeated%@Array", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasRepeated%@Array:", name]);
    XCTAssertFalse([Message3 instancesRespondToSelector:hasSel], @"field: %@",
                   name);
    XCTAssertFalse([Message3 instancesRespondToSelector:setHasSel],
                   @"field: %@", name);
    // build the selector, i.e. - repeatedInt32Array_Count
    SEL countSel = NSSelectorFromString(
        [NSString stringWithFormat:@"repeated%@Array_Count", name]);
    XCTAssertTrue([Message2 instancesRespondToSelector:countSel], @"field: %@",
                  name);
  }

  // OneOf fields - no has*/setHas*

  for (NSString *name in names) {
    // build the selector, i.e. - hasOneofInt32/setHasOneofInt32:
    SEL hasSel =
        NSSelectorFromString([NSString stringWithFormat:@"hasOneof%@", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasOneof%@:", name]);
    XCTAssertFalse([Message2 instancesRespondToSelector:hasSel], @"field: %@",
                   name);
    XCTAssertFalse([Message2 instancesRespondToSelector:setHasSel],
                   @"field: %@", name);
  }

  // map<> fields
  //  - no has*/setHas*
  //  - *Count

  NSArray *mapNames = @[
    @"Int32Int32",
    @"Int64Int64",
    @"Uint32Uint32",
    @"Uint64Uint64",
    @"Sint32Sint32",
    @"Sint64Sint64",
    @"Fixed32Fixed32",
    @"Fixed64Fixed64",
    @"Sfixed32Sfixed32",
    @"Sfixed64Sfixed64",
    @"Int32Float",
    @"Int32Double",
    @"BoolBool",
    @"StringString",
    @"StringBytes",
    @"StringMessage",
    @"Int32Bytes",
    @"Int32Enum",
    @"Int32Message",
  ];

  for (NSString *name in mapNames) {
    // build the selector, i.e. - hasMapInt32Int32/setHasMapInt32Int32:
    SEL hasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"hasMap%@", name]);
    SEL setHasSel = NSSelectorFromString(
        [NSString stringWithFormat:@"setHasMap%@:", name]);
    XCTAssertFalse([Message2 instancesRespondToSelector:hasSel], @"field: %@",
                   name);
    XCTAssertFalse([Message2 instancesRespondToSelector:setHasSel],
                   @"field: %@", name);
    // build the selector, i.e. - mapInt32Int32Count
    SEL countSel = NSSelectorFromString(
        [NSString stringWithFormat:@"map%@_Count", name]);
    XCTAssertTrue([Message2 instancesRespondToSelector:countSel], @"field: %@",
                   name);
  }
}

- (void)testProto2SingleFieldHasBehavior {
  //
  // Setting to any value including the default value (0) should result has*
  // being true.
  //

//%PDDM-DEFINE PROTO2_TEST_HAS_FIELD(FIELD, NON_ZERO_VALUE, ZERO_VALUE)
//%  {  // optional##FIELD :: NON_ZERO_VALUE
//%    Message2 *msg = [[Message2 alloc] init];
//%    XCTAssertFalse(msg.hasOptional##FIELD);
//%    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_Optional##FIELD));
//%    msg.optional##FIELD = NON_ZERO_VALUE;
//%    XCTAssertTrue(msg.hasOptional##FIELD);
//%    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_Optional##FIELD));
//%    [msg release];
//%  }
//%  {  // optional##FIELD :: ZERO_VALUE
//%    Message2 *msg = [[Message2 alloc] init];
//%    XCTAssertFalse(msg.hasOptional##FIELD);
//%    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_Optional##FIELD));
//%    msg.optional##FIELD = ZERO_VALUE;
//%    XCTAssertTrue(msg.hasOptional##FIELD);
//%    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_Optional##FIELD));
//%    [msg release];
//%  }
//%
//%PDDM-DEFINE PROTO2_TEST_HAS_FIELDS()
//%PROTO2_TEST_HAS_FIELD(Int32, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Int64, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Uint32, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Uint64, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Sint32, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Sint64, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Fixed32, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Fixed64, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Sfixed32, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Sfixed64, 1, 0)
//%PROTO2_TEST_HAS_FIELD(Float, 1.0f, 0.0f)
//%PROTO2_TEST_HAS_FIELD(Double, 1.0, 0.0)
//%PROTO2_TEST_HAS_FIELD(Bool, YES, NO)
//%PROTO2_TEST_HAS_FIELD(String, @"foo", @"")
//%PROTO2_TEST_HAS_FIELD(Bytes, [@"foo" dataUsingEncoding:NSUTF8StringEncoding], [NSData data])
//%  //
//%  // Test doesn't apply to optionalGroup/optionalMessage.
//%  //
//%
//%PROTO2_TEST_HAS_FIELD(Enum, Message2_Enum_Bar, Message2_Enum_Foo)
//%PDDM-EXPAND PROTO2_TEST_HAS_FIELDS()
// This block of code is generated, do not edit it directly.

  {  // optionalInt32 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalInt32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt32));
    msg.optionalInt32 = 1;
    XCTAssertTrue(msg.hasOptionalInt32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt32));
    [msg release];
  }
  {  // optionalInt32 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalInt32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt32));
    msg.optionalInt32 = 0;
    XCTAssertTrue(msg.hasOptionalInt32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt32));
    [msg release];
  }

  {  // optionalInt64 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalInt64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt64));
    msg.optionalInt64 = 1;
    XCTAssertTrue(msg.hasOptionalInt64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt64));
    [msg release];
  }
  {  // optionalInt64 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalInt64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt64));
    msg.optionalInt64 = 0;
    XCTAssertTrue(msg.hasOptionalInt64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalInt64));
    [msg release];
  }

  {  // optionalUint32 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalUint32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint32));
    msg.optionalUint32 = 1;
    XCTAssertTrue(msg.hasOptionalUint32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint32));
    [msg release];
  }
  {  // optionalUint32 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalUint32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint32));
    msg.optionalUint32 = 0;
    XCTAssertTrue(msg.hasOptionalUint32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint32));
    [msg release];
  }

  {  // optionalUint64 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalUint64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint64));
    msg.optionalUint64 = 1;
    XCTAssertTrue(msg.hasOptionalUint64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint64));
    [msg release];
  }
  {  // optionalUint64 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalUint64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint64));
    msg.optionalUint64 = 0;
    XCTAssertTrue(msg.hasOptionalUint64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalUint64));
    [msg release];
  }

  {  // optionalSint32 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSint32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint32));
    msg.optionalSint32 = 1;
    XCTAssertTrue(msg.hasOptionalSint32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint32));
    [msg release];
  }
  {  // optionalSint32 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSint32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint32));
    msg.optionalSint32 = 0;
    XCTAssertTrue(msg.hasOptionalSint32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint32));
    [msg release];
  }

  {  // optionalSint64 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSint64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint64));
    msg.optionalSint64 = 1;
    XCTAssertTrue(msg.hasOptionalSint64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint64));
    [msg release];
  }
  {  // optionalSint64 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSint64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint64));
    msg.optionalSint64 = 0;
    XCTAssertTrue(msg.hasOptionalSint64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSint64));
    [msg release];
  }

  {  // optionalFixed32 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalFixed32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed32));
    msg.optionalFixed32 = 1;
    XCTAssertTrue(msg.hasOptionalFixed32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed32));
    [msg release];
  }
  {  // optionalFixed32 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalFixed32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed32));
    msg.optionalFixed32 = 0;
    XCTAssertTrue(msg.hasOptionalFixed32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed32));
    [msg release];
  }

  {  // optionalFixed64 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalFixed64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed64));
    msg.optionalFixed64 = 1;
    XCTAssertTrue(msg.hasOptionalFixed64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed64));
    [msg release];
  }
  {  // optionalFixed64 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalFixed64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed64));
    msg.optionalFixed64 = 0;
    XCTAssertTrue(msg.hasOptionalFixed64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFixed64));
    [msg release];
  }

  {  // optionalSfixed32 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSfixed32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed32));
    msg.optionalSfixed32 = 1;
    XCTAssertTrue(msg.hasOptionalSfixed32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed32));
    [msg release];
  }
  {  // optionalSfixed32 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSfixed32);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed32));
    msg.optionalSfixed32 = 0;
    XCTAssertTrue(msg.hasOptionalSfixed32);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed32));
    [msg release];
  }

  {  // optionalSfixed64 :: 1
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSfixed64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed64));
    msg.optionalSfixed64 = 1;
    XCTAssertTrue(msg.hasOptionalSfixed64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed64));
    [msg release];
  }
  {  // optionalSfixed64 :: 0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalSfixed64);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed64));
    msg.optionalSfixed64 = 0;
    XCTAssertTrue(msg.hasOptionalSfixed64);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalSfixed64));
    [msg release];
  }

  {  // optionalFloat :: 1.0f
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalFloat);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFloat));
    msg.optionalFloat = 1.0f;
    XCTAssertTrue(msg.hasOptionalFloat);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFloat));
    [msg release];
  }
  {  // optionalFloat :: 0.0f
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalFloat);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFloat));
    msg.optionalFloat = 0.0f;
    XCTAssertTrue(msg.hasOptionalFloat);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalFloat));
    [msg release];
  }

  {  // optionalDouble :: 1.0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalDouble);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalDouble));
    msg.optionalDouble = 1.0;
    XCTAssertTrue(msg.hasOptionalDouble);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalDouble));
    [msg release];
  }
  {  // optionalDouble :: 0.0
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalDouble);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalDouble));
    msg.optionalDouble = 0.0;
    XCTAssertTrue(msg.hasOptionalDouble);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalDouble));
    [msg release];
  }

  {  // optionalBool :: YES
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalBool);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBool));
    msg.optionalBool = YES;
    XCTAssertTrue(msg.hasOptionalBool);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBool));
    [msg release];
  }
  {  // optionalBool :: NO
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalBool);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBool));
    msg.optionalBool = NO;
    XCTAssertTrue(msg.hasOptionalBool);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBool));
    [msg release];
  }

  {  // optionalString :: @"foo"
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalString);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalString));
    msg.optionalString = @"foo";
    XCTAssertTrue(msg.hasOptionalString);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalString));
    [msg release];
  }
  {  // optionalString :: @""
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalString);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalString));
    msg.optionalString = @"";
    XCTAssertTrue(msg.hasOptionalString);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalString));
    [msg release];
  }

  {  // optionalBytes :: [@"foo" dataUsingEncoding:NSUTF8StringEncoding]
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalBytes);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBytes));
    msg.optionalBytes = [@"foo" dataUsingEncoding:NSUTF8StringEncoding];
    XCTAssertTrue(msg.hasOptionalBytes);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBytes));
    [msg release];
  }
  {  // optionalBytes :: [NSData data]
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalBytes);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBytes));
    msg.optionalBytes = [NSData data];
    XCTAssertTrue(msg.hasOptionalBytes);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalBytes));
    [msg release];
  }

  //
  // Test doesn't apply to optionalGroup/optionalMessage.
  //

  {  // optionalEnum :: Message2_Enum_Bar
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalEnum);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalEnum));
    msg.optionalEnum = Message2_Enum_Bar;
    XCTAssertTrue(msg.hasOptionalEnum);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalEnum));
    [msg release];
  }
  {  // optionalEnum :: Message2_Enum_Foo
    Message2 *msg = [[Message2 alloc] init];
    XCTAssertFalse(msg.hasOptionalEnum);
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalEnum));
    msg.optionalEnum = Message2_Enum_Foo;
    XCTAssertTrue(msg.hasOptionalEnum);
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message2_FieldNumber_OptionalEnum));
    [msg release];
  }

//%PDDM-EXPAND-END PROTO2_TEST_HAS_FIELDS()
}

- (void)testProto3SingleFieldHasBehavior {
  //
  // Setting to any value including the default value (0) should result has*
  // being true.
  //

//%PDDM-DEFINE PROTO3_TEST_HAS_FIELD(FIELD, NON_ZERO_VALUE, ZERO_VALUE)
//%  {  // optional##FIELD
//%    Message3 *msg = [[Message3 alloc] init];
//%    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_Optional##FIELD));
//%    msg.optional##FIELD = NON_ZERO_VALUE;
//%    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_Optional##FIELD));
//%    msg.optional##FIELD = ZERO_VALUE;
//%    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_Optional##FIELD));
//%    [msg release];
//%  }
//%
//%PDDM-DEFINE PROTO3_TEST_HAS_FIELDS()
//%PROTO3_TEST_HAS_FIELD(Int32, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Int64, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Uint32, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Uint64, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Sint32, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Sint64, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Fixed32, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Fixed64, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Sfixed32, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Sfixed64, 1, 0)
//%PROTO3_TEST_HAS_FIELD(Float, 1.0f, 0.0f)
//%PROTO3_TEST_HAS_FIELD(Double, 1.0, 0.0)
//%PROTO3_TEST_HAS_FIELD(Bool, YES, NO)
//%PROTO3_TEST_HAS_FIELD(String, @"foo", @"")
//%PROTO3_TEST_HAS_FIELD(Bytes, [@"foo" dataUsingEncoding:NSUTF8StringEncoding], [NSData data])
//%  //
//%  // Test doesn't apply to optionalGroup/optionalMessage.
//%  //
//%
//%PROTO3_TEST_HAS_FIELD(Enum, Message3_Enum_Bar, Message3_Enum_Foo)
//%PDDM-EXPAND PROTO3_TEST_HAS_FIELDS()
// This block of code is generated, do not edit it directly.

  {  // optionalInt32
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalInt32));
    msg.optionalInt32 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalInt32));
    msg.optionalInt32 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalInt32));
    [msg release];
  }

  {  // optionalInt64
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalInt64));
    msg.optionalInt64 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalInt64));
    msg.optionalInt64 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalInt64));
    [msg release];
  }

  {  // optionalUint32
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalUint32));
    msg.optionalUint32 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalUint32));
    msg.optionalUint32 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalUint32));
    [msg release];
  }

  {  // optionalUint64
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalUint64));
    msg.optionalUint64 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalUint64));
    msg.optionalUint64 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalUint64));
    [msg release];
  }

  {  // optionalSint32
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSint32));
    msg.optionalSint32 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSint32));
    msg.optionalSint32 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSint32));
    [msg release];
  }

  {  // optionalSint64
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSint64));
    msg.optionalSint64 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSint64));
    msg.optionalSint64 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSint64));
    [msg release];
  }

  {  // optionalFixed32
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFixed32));
    msg.optionalFixed32 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFixed32));
    msg.optionalFixed32 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFixed32));
    [msg release];
  }

  {  // optionalFixed64
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFixed64));
    msg.optionalFixed64 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFixed64));
    msg.optionalFixed64 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFixed64));
    [msg release];
  }

  {  // optionalSfixed32
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSfixed32));
    msg.optionalSfixed32 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSfixed32));
    msg.optionalSfixed32 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSfixed32));
    [msg release];
  }

  {  // optionalSfixed64
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSfixed64));
    msg.optionalSfixed64 = 1;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSfixed64));
    msg.optionalSfixed64 = 0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalSfixed64));
    [msg release];
  }

  {  // optionalFloat
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFloat));
    msg.optionalFloat = 1.0f;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFloat));
    msg.optionalFloat = 0.0f;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalFloat));
    [msg release];
  }

  {  // optionalDouble
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalDouble));
    msg.optionalDouble = 1.0;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalDouble));
    msg.optionalDouble = 0.0;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalDouble));
    [msg release];
  }

  {  // optionalBool
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalBool));
    msg.optionalBool = YES;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalBool));
    msg.optionalBool = NO;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalBool));
    [msg release];
  }

  {  // optionalString
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalString));
    msg.optionalString = @"foo";
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalString));
    msg.optionalString = @"";
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalString));
    [msg release];
  }

  {  // optionalBytes
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalBytes));
    msg.optionalBytes = [@"foo" dataUsingEncoding:NSUTF8StringEncoding];
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalBytes));
    msg.optionalBytes = [NSData data];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalBytes));
    [msg release];
  }

  //
  // Test doesn't apply to optionalGroup/optionalMessage.
  //

  {  // optionalEnum
    Message3 *msg = [[Message3 alloc] init];
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalEnum));
    msg.optionalEnum = Message3_Enum_Bar;
    XCTAssertTrue(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalEnum));
    msg.optionalEnum = Message3_Enum_Foo;
    XCTAssertFalse(GPBMessageHasFieldNumberSet(msg, Message3_FieldNumber_OptionalEnum));
    [msg release];
  }

//%PDDM-EXPAND-END PROTO3_TEST_HAS_FIELDS()
}

- (void)testAccessingProto2UnknownEnumValues {
  Message2 *msg = [[Message2 alloc] init];

  // Set it to something non zero, try and confirm it doesn't change.

  msg.optionalEnum = Message2_Enum_Bar;
  XCTAssertThrowsSpecificNamed(msg.optionalEnum = 666, NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(msg.optionalEnum, Message2_Enum_Bar);

  msg.oneofEnum = Message2_Enum_Bar;
  XCTAssertThrowsSpecificNamed(msg.oneofEnum = 666, NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Bar);

  [msg release];
}

- (void)testAccessingProto3UnknownEnumValues {
  Message3 *msg = [[Message3 alloc] init];

  // Set it to something non zero, try and confirm it doesn't change.

  msg.optionalEnum = Message3_Enum_Bar;
  XCTAssertThrowsSpecificNamed(msg.optionalEnum = 666, NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(msg.optionalEnum, Message3_Enum_Bar);

  msg.oneofEnum = Message3_Enum_Bar;
  XCTAssertThrowsSpecificNamed(msg.oneofEnum = 666, NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Bar);

  // Set via raw api to confirm it works.

  SetMessage3_OptionalEnum_RawValue(msg, 666);
  XCTAssertEqual(msg.optionalEnum,
                 Message3_Enum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual(Message3_OptionalEnum_RawValue(msg), 666);

  SetMessage3_OneofEnum_RawValue(msg, 666);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_GPBUnrecognizedEnumeratorValue);
  XCTAssertEqual(Message3_OneofEnum_RawValue(msg), 666);

  [msg release];
}

- (void)testProto2OneofBasicBehaviors {
  Message2 *msg = [[Message2 alloc] init];

  NSString *oneofStringDefault = @"string";
  NSData *oneofBytesDefault = [@"data" dataUsingEncoding:NSUTF8StringEncoding];

  // Nothing set.
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_GPBUnsetOneOfCase);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);

  // Set, check the case, check everyone has default but the one, confirm case
  // didn't change.

  msg.oneofInt32 = 1;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofInt32);
  XCTAssertEqual(msg.oneofInt32, 1);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofInt32);

  msg.oneofInt64 = 2;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofInt64);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 2);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofInt64);

  msg.oneofUint32 = 3;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofUint32);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 3U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofUint32);

  msg.oneofUint64 = 4;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofUint64);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 4U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofUint64);

  msg.oneofSint32 = 5;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSint32);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 5);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSint32);

  msg.oneofSint64 = 6;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSint64);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 6);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSint64);

  msg.oneofFixed32 = 7;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFixed32);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 7U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFixed32);

  msg.oneofFixed64 = 8;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFixed64);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 8U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFixed64);

  msg.oneofSfixed32 = 9;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSfixed32);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 9);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSfixed32);

  msg.oneofSfixed64 = 10;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSfixed64);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 10);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofSfixed64);

  msg.oneofFloat = 11.0f;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFloat);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 11.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofFloat);

  msg.oneofDouble = 12.0;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofDouble);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 12.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofDouble);

  msg.oneofBool = NO;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofBool);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofBool);

  msg.oneofString = @"foo";
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofString);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, @"foo");
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofString);

  msg.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofBytes);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes,
                        [@"bar" dataUsingEncoding:NSUTF8StringEncoding]);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofBytes);

  Message2_OneofGroup *group = [Message2_OneofGroup message];
  msg.oneofGroup = group;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofGroup);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertEqual(msg.oneofGroup, group);  // Pointer compare.
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofGroup);

  Message2 *subMessage = [Message2 message];
  msg.oneofMessage = subMessage;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofMessage);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotEqual(msg.oneofGroup, group);      // Pointer compare.
  XCTAssertEqual(msg.oneofMessage, subMessage);  // Pointer compare.
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofMessage);

  msg.oneofEnum = Message2_Enum_Bar;
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofEnum);
  XCTAssertEqual(msg.oneofInt32, 100);
  XCTAssertEqual(msg.oneofInt64, 101);
  XCTAssertEqual(msg.oneofUint32, 102U);
  XCTAssertEqual(msg.oneofUint64, 103U);
  XCTAssertEqual(msg.oneofSint32, 104);
  XCTAssertEqual(msg.oneofSint64, 105);
  XCTAssertEqual(msg.oneofFixed32, 106U);
  XCTAssertEqual(msg.oneofFixed64, 107U);
  XCTAssertEqual(msg.oneofSfixed32, 108);
  XCTAssertEqual(msg.oneofSfixed64, 109);
  XCTAssertEqual(msg.oneofFloat, 110.0f);
  XCTAssertEqual(msg.oneofDouble, 111.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofGroup);
  XCTAssertNotEqual(msg.oneofGroup, group);  // Pointer compare.
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertNotEqual(msg.oneofMessage, subMessage);  // Pointer compare.
  XCTAssertEqual(msg.oneofEnum, Message2_Enum_Bar);
  XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_OneofEnum);

  // Test setting/calling clear clearing.

  [msg release];
  msg = [[Message2 alloc] init];

  uint32_t values[] = {
    Message2_O_OneOfCase_OneofInt32,
    Message2_O_OneOfCase_OneofInt64,
    Message2_O_OneOfCase_OneofUint32,
    Message2_O_OneOfCase_OneofUint64,
    Message2_O_OneOfCase_OneofSint32,
    Message2_O_OneOfCase_OneofSint64,
    Message2_O_OneOfCase_OneofFixed32,
    Message2_O_OneOfCase_OneofFixed64,
    Message2_O_OneOfCase_OneofSfixed32,
    Message2_O_OneOfCase_OneofSfixed64,
    Message2_O_OneOfCase_OneofFloat,
    Message2_O_OneOfCase_OneofDouble,
    Message2_O_OneOfCase_OneofBool,
    Message2_O_OneOfCase_OneofString,
    Message2_O_OneOfCase_OneofBytes,
    Message2_O_OneOfCase_OneofGroup,
    Message2_O_OneOfCase_OneofMessage,
    Message2_O_OneOfCase_OneofEnum,
  };

  for (size_t i = 0; i < GPBARRAYSIZE(values); ++i) {
    switch (values[i]) {
      case Message2_O_OneOfCase_OneofInt32:
        msg.oneofInt32 = 1;
        break;
      case Message2_O_OneOfCase_OneofInt64:
        msg.oneofInt64 = 2;
        break;
      case Message2_O_OneOfCase_OneofUint32:
        msg.oneofUint32 = 3;
        break;
      case Message2_O_OneOfCase_OneofUint64:
        msg.oneofUint64 = 4;
        break;
      case Message2_O_OneOfCase_OneofSint32:
        msg.oneofSint32 = 5;
        break;
      case Message2_O_OneOfCase_OneofSint64:
        msg.oneofSint64 = 6;
        break;
      case Message2_O_OneOfCase_OneofFixed32:
        msg.oneofFixed32 = 7;
        break;
      case Message2_O_OneOfCase_OneofFixed64:
        msg.oneofFixed64 = 8;
        break;
      case Message2_O_OneOfCase_OneofSfixed32:
        msg.oneofSfixed32 = 9;
        break;
      case Message2_O_OneOfCase_OneofSfixed64:
        msg.oneofSfixed64 = 10;
        break;
      case Message2_O_OneOfCase_OneofFloat:
        msg.oneofFloat = 11.0f;
        break;
      case Message2_O_OneOfCase_OneofDouble:
        msg.oneofDouble = 12.0;
        break;
      case Message2_O_OneOfCase_OneofBool:
        msg.oneofBool = YES;
        break;
      case Message2_O_OneOfCase_OneofString:
        msg.oneofString = @"foo";
        break;
      case Message2_O_OneOfCase_OneofBytes:
        msg.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
        break;
      case Message2_O_OneOfCase_OneofGroup:
        msg.oneofGroup = group;
        break;
      case Message2_O_OneOfCase_OneofMessage:
        msg.oneofMessage = subMessage;
        break;
      case Message2_O_OneOfCase_OneofEnum:
        msg.oneofEnum = Message2_Enum_Bar;
        break;
      default:
        XCTFail(@"shouldn't happen, loop: %zd, value: %d", i, values[i]);
        break;
    }

    XCTAssertEqual(msg.oOneOfCase, values[i], "Loop: %zd", i);
    // No need to check the value was set, the above tests did that.
    Message2_ClearOOneOfCase(msg);
    // Nothing in the case.
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase_GPBUnsetOneOfCase,
                   "Loop: %zd", i);
    // Confirm everything is back to defaults after a clear.
    XCTAssertEqual(msg.oneofInt32, 100, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofInt64, 101, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofUint32, 102U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofUint64, 103U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSint32, 104, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSint64, 105, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofFixed32, 106U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofFixed64, 107U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSfixed32, 108, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSfixed64, 109, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofFloat, 110.0f, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofDouble, 111.0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofBool, YES, "Loop: %zd", i);
    XCTAssertEqualObjects(msg.oneofString, oneofStringDefault, "Loop: %zd", i);
    XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault, "Loop: %zd", i);
    XCTAssertNotNil(msg.oneofGroup, "Loop: %zd", i);
    XCTAssertNotEqual(msg.oneofGroup, group, "Loop: %zd",
                      i);  // Pointer compare.
    XCTAssertNotNil(msg.oneofMessage, "Loop: %zd", i);
    XCTAssertNotEqual(msg.oneofMessage, subMessage, "Loop: %zd",
                      i);  // Pointer compare.
    XCTAssertEqual(msg.oneofEnum, Message2_Enum_Baz, "Loop: %zd", i);
  }

  [msg release];
}

- (void)testProto3OneofBasicBehaviors {
  Message3 *msg = [[Message3 alloc] init];

  NSString *oneofStringDefault = @"";
  NSData *oneofBytesDefault = [NSData data];

  // Nothing set.
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_GPBUnsetOneOfCase);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);

  // Set, check the case, check everyone has default but the one, confirm case
  // didn't change.

  msg.oneofInt32 = 1;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofInt32);
  XCTAssertEqual(msg.oneofInt32, 1);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofInt32);

  msg.oneofInt64 = 2;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofInt64);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 2);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofInt64);

  msg.oneofUint32 = 3;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofUint32);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 3U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofUint32);

  msg.oneofUint64 = 4;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofUint64);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 4U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofUint64);

  msg.oneofSint32 = 5;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSint32);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 5);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSint32);

  msg.oneofSint64 = 6;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSint64);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 6);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSint64);

  msg.oneofFixed32 = 7;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFixed32);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 7U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFixed32);

  msg.oneofFixed64 = 8;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFixed64);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 8U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFixed64);

  msg.oneofSfixed32 = 9;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSfixed32);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 9);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSfixed32);

  msg.oneofSfixed64 = 10;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSfixed64);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 10);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofSfixed64);

  msg.oneofFloat = 11.0f;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFloat);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 11.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofFloat);

  msg.oneofDouble = 12.0;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofDouble);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 12.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofDouble);

  msg.oneofBool = YES;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofBool);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, YES);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofBool);

  msg.oneofString = @"foo";
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofString);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, @"foo");
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofString);

  msg.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofBytes);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes,
                        [@"bar" dataUsingEncoding:NSUTF8StringEncoding]);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofBytes);

  Message3 *subMessage = [Message3 message];
  msg.oneofMessage = subMessage;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofMessage);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertEqual(msg.oneofMessage, subMessage);  // Pointer compare.
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofMessage);

  msg.oneofEnum = Message3_Enum_Bar;
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofEnum);
  XCTAssertEqual(msg.oneofInt32, 0);
  XCTAssertEqual(msg.oneofInt64, 0);
  XCTAssertEqual(msg.oneofUint32, 0U);
  XCTAssertEqual(msg.oneofUint64, 0U);
  XCTAssertEqual(msg.oneofSint32, 0);
  XCTAssertEqual(msg.oneofSint64, 0);
  XCTAssertEqual(msg.oneofFixed32, 0U);
  XCTAssertEqual(msg.oneofFixed64, 0U);
  XCTAssertEqual(msg.oneofSfixed32, 0);
  XCTAssertEqual(msg.oneofSfixed64, 0);
  XCTAssertEqual(msg.oneofFloat, 0.0f);
  XCTAssertEqual(msg.oneofDouble, 0.0);
  XCTAssertEqual(msg.oneofBool, NO);
  XCTAssertEqualObjects(msg.oneofString, oneofStringDefault);
  XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault);
  XCTAssertNotNil(msg.oneofMessage);
  XCTAssertNotEqual(msg.oneofMessage, subMessage);  // Pointer compare.
  XCTAssertEqual(msg.oneofEnum, Message3_Enum_Bar);
  XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_OneofEnum);

  // Test setting/calling clear clearing.

  [msg release];
  msg = [[Message3 alloc] init];

  uint32_t values[] = {
    Message3_O_OneOfCase_OneofInt32,
    Message3_O_OneOfCase_OneofInt64,
    Message3_O_OneOfCase_OneofUint32,
    Message3_O_OneOfCase_OneofUint64,
    Message3_O_OneOfCase_OneofSint32,
    Message3_O_OneOfCase_OneofSint64,
    Message3_O_OneOfCase_OneofFixed32,
    Message3_O_OneOfCase_OneofFixed64,
    Message3_O_OneOfCase_OneofSfixed32,
    Message3_O_OneOfCase_OneofSfixed64,
    Message3_O_OneOfCase_OneofFloat,
    Message3_O_OneOfCase_OneofDouble,
    Message3_O_OneOfCase_OneofBool,
    Message3_O_OneOfCase_OneofString,
    Message3_O_OneOfCase_OneofBytes,
    Message3_O_OneOfCase_OneofMessage,
    Message3_O_OneOfCase_OneofEnum,
  };

  for (size_t i = 0; i < GPBARRAYSIZE(values); ++i) {
    switch (values[i]) {
      case Message3_O_OneOfCase_OneofInt32:
        msg.oneofInt32 = 1;
        break;
      case Message3_O_OneOfCase_OneofInt64:
        msg.oneofInt64 = 2;
        break;
      case Message3_O_OneOfCase_OneofUint32:
        msg.oneofUint32 = 3;
        break;
      case Message3_O_OneOfCase_OneofUint64:
        msg.oneofUint64 = 4;
        break;
      case Message3_O_OneOfCase_OneofSint32:
        msg.oneofSint32 = 5;
        break;
      case Message3_O_OneOfCase_OneofSint64:
        msg.oneofSint64 = 6;
        break;
      case Message3_O_OneOfCase_OneofFixed32:
        msg.oneofFixed32 = 7;
        break;
      case Message3_O_OneOfCase_OneofFixed64:
        msg.oneofFixed64 = 8;
        break;
      case Message3_O_OneOfCase_OneofSfixed32:
        msg.oneofSfixed32 = 9;
        break;
      case Message3_O_OneOfCase_OneofSfixed64:
        msg.oneofSfixed64 = 10;
        break;
      case Message3_O_OneOfCase_OneofFloat:
        msg.oneofFloat = 11.0f;
        break;
      case Message3_O_OneOfCase_OneofDouble:
        msg.oneofDouble = 12.0;
        break;
      case Message3_O_OneOfCase_OneofBool:
        msg.oneofBool = YES;
        break;
      case Message3_O_OneOfCase_OneofString:
        msg.oneofString = @"foo";
        break;
      case Message3_O_OneOfCase_OneofBytes:
        msg.oneofBytes = [@"bar" dataUsingEncoding:NSUTF8StringEncoding];
        break;
      case Message3_O_OneOfCase_OneofMessage:
        msg.oneofMessage = subMessage;
        break;
      case Message3_O_OneOfCase_OneofEnum:
        msg.oneofEnum = Message3_Enum_Baz;
        break;
      default:
        XCTFail(@"shouldn't happen, loop: %zd, value: %d", i, values[i]);
        break;
    }

    XCTAssertEqual(msg.oOneOfCase, values[i], "Loop: %zd", i);
    // No need to check the value was set, the above tests did that.
    Message3_ClearOOneOfCase(msg);
    // Nothing in the case.
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase_GPBUnsetOneOfCase,
                   "Loop: %zd", i);
    // Confirm everything is back to defaults after a clear.
    XCTAssertEqual(msg.oneofInt32, 0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofInt64, 0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofUint32, 0U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofUint64, 0U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSint32, 0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSint64, 0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofFixed32, 0U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofFixed64, 0U, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSfixed32, 0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofSfixed64, 0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofFloat, 0.0f, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofDouble, 0.0, "Loop: %zd", i);
    XCTAssertEqual(msg.oneofBool, NO, "Loop: %zd", i);
    XCTAssertEqualObjects(msg.oneofString, oneofStringDefault, "Loop: %zd", i);
    XCTAssertEqualObjects(msg.oneofBytes, oneofBytesDefault, "Loop: %zd", i);
    XCTAssertNotNil(msg.oneofMessage, "Loop: %zd", i);
    XCTAssertNotEqual(msg.oneofMessage, subMessage, "Loop: %zd",
                      i);  // Pointer compare.
    XCTAssertEqual(msg.oneofEnum, Message3_Enum_Foo, "Loop: %zd", i);
  }

  [msg release];
}

- (void)testCopyingMakesUniqueObjects {
  const int repeatCount = 5;
  TestAllTypes *msg1 = [TestAllTypes message];
  [self setAllFields:msg1 repeatedCount:repeatCount];

  TestAllTypes *msg2 = [[msg1 copy] autorelease];

  XCTAssertNotEqual(msg1, msg2);      // Ptr compare, new object.
  XCTAssertEqualObjects(msg1, msg2);  // Equal values.

  // Pointer comparisions, different objects.

  XCTAssertNotEqual(msg1.optionalGroup, msg2.optionalGroup);
  XCTAssertNotEqual(msg1.optionalNestedMessage, msg2.optionalNestedMessage);
  XCTAssertNotEqual(msg1.optionalForeignMessage, msg2.optionalForeignMessage);
  XCTAssertNotEqual(msg1.optionalImportMessage, msg2.optionalImportMessage);

  XCTAssertNotEqual(msg1.repeatedInt32Array, msg2.repeatedInt32Array);
  XCTAssertNotEqual(msg1.repeatedInt64Array, msg2.repeatedInt64Array);
  XCTAssertNotEqual(msg1.repeatedUint32Array, msg2.repeatedUint32Array);
  XCTAssertNotEqual(msg1.repeatedUint64Array, msg2.repeatedUint64Array);
  XCTAssertNotEqual(msg1.repeatedSint32Array, msg2.repeatedSint32Array);
  XCTAssertNotEqual(msg1.repeatedSint64Array, msg2.repeatedSint64Array);
  XCTAssertNotEqual(msg1.repeatedFixed32Array, msg2.repeatedFixed32Array);
  XCTAssertNotEqual(msg1.repeatedFixed64Array, msg2.repeatedFixed64Array);
  XCTAssertNotEqual(msg1.repeatedSfixed32Array, msg2.repeatedSfixed32Array);
  XCTAssertNotEqual(msg1.repeatedSfixed64Array, msg2.repeatedSfixed64Array);
  XCTAssertNotEqual(msg1.repeatedFloatArray, msg2.repeatedFloatArray);
  XCTAssertNotEqual(msg1.repeatedDoubleArray, msg2.repeatedDoubleArray);
  XCTAssertNotEqual(msg1.repeatedBoolArray, msg2.repeatedBoolArray);
  XCTAssertNotEqual(msg1.repeatedStringArray, msg2.repeatedStringArray);
  XCTAssertNotEqual(msg1.repeatedBytesArray, msg2.repeatedBytesArray);
  XCTAssertNotEqual(msg1.repeatedGroupArray, msg2.repeatedGroupArray);
  XCTAssertNotEqual(msg1.repeatedNestedMessageArray,
                    msg2.repeatedNestedMessageArray);
  XCTAssertNotEqual(msg1.repeatedForeignMessageArray,
                    msg2.repeatedForeignMessageArray);
  XCTAssertNotEqual(msg1.repeatedImportMessageArray,
                    msg2.repeatedImportMessageArray);
  XCTAssertNotEqual(msg1.repeatedNestedEnumArray, msg2.repeatedNestedEnumArray);
  XCTAssertNotEqual(msg1.repeatedForeignEnumArray,
                    msg2.repeatedForeignEnumArray);
  XCTAssertNotEqual(msg1.repeatedImportEnumArray, msg2.repeatedImportEnumArray);
  XCTAssertNotEqual(msg1.repeatedStringPieceArray,
                    msg2.repeatedStringPieceArray);
  XCTAssertNotEqual(msg1.repeatedCordArray, msg2.repeatedCordArray);

  for (int i = 0; i < repeatCount; i++) {
    XCTAssertNotEqual(msg1.repeatedNestedMessageArray[i],
                      msg2.repeatedNestedMessageArray[i]);
    XCTAssertNotEqual(msg1.repeatedForeignMessageArray[i],
                      msg2.repeatedForeignMessageArray[i]);
    XCTAssertNotEqual(msg1.repeatedImportMessageArray[i],
                      msg2.repeatedImportMessageArray[i]);
  }
}

- (void)testCopyingMapsMakesUniqueObjects {
  TestMap *msg1 = [TestMap message];
  [self setAllMapFields:msg1 numEntries:5];

  TestMap *msg2 = [[msg1 copy] autorelease];

  XCTAssertNotEqual(msg1, msg2);      // Ptr compare, new object.
  XCTAssertEqualObjects(msg1, msg2);  // Equal values.

  // Pointer comparisions, different objects.
  XCTAssertNotEqual(msg1.mapInt32Int32, msg2.mapInt32Int32);
  XCTAssertNotEqual(msg1.mapInt64Int64, msg2.mapInt64Int64);
  XCTAssertNotEqual(msg1.mapUint32Uint32, msg2.mapUint32Uint32);
  XCTAssertNotEqual(msg1.mapUint64Uint64, msg2.mapUint64Uint64);
  XCTAssertNotEqual(msg1.mapSint32Sint32, msg2.mapSint32Sint32);
  XCTAssertNotEqual(msg1.mapSint64Sint64, msg2.mapSint64Sint64);
  XCTAssertNotEqual(msg1.mapFixed32Fixed32, msg2.mapFixed32Fixed32);
  XCTAssertNotEqual(msg1.mapFixed64Fixed64, msg2.mapFixed64Fixed64);
  XCTAssertNotEqual(msg1.mapSfixed32Sfixed32, msg2.mapSfixed32Sfixed32);
  XCTAssertNotEqual(msg1.mapSfixed64Sfixed64, msg2.mapSfixed64Sfixed64);
  XCTAssertNotEqual(msg1.mapInt32Float, msg2.mapInt32Float);
  XCTAssertNotEqual(msg1.mapInt32Double, msg2.mapInt32Double);
  XCTAssertNotEqual(msg1.mapBoolBool, msg2.mapBoolBool);
  XCTAssertNotEqual(msg1.mapStringString, msg2.mapStringString);
  XCTAssertNotEqual(msg1.mapInt32Bytes, msg2.mapInt32Bytes);
  XCTAssertNotEqual(msg1.mapInt32Enum, msg2.mapInt32Enum);
  XCTAssertNotEqual(msg1.mapInt32ForeignMessage, msg2.mapInt32ForeignMessage);

  // Ensure the messages are unique per map.
  [msg1.mapInt32ForeignMessage
      enumerateKeysAndObjectsUsingBlock:^(int32_t key, id value, BOOL *stop) {
#pragma unused(stop)
        ForeignMessage *subMsg2 = [msg2.mapInt32ForeignMessage objectForKey:key];
        XCTAssertNotEqual(value, subMsg2);  // Ptr compare, new object.
      }];
}

#pragma mark - Subset from from map_tests.cc

// TEST(GeneratedMapFieldTest, IsInitialized)
- (void)testMap_IsInitialized {
  TestRequiredMessageMap *msg = [[TestRequiredMessageMap alloc] init];

  // Add an uninitialized message.
  TestRequired *subMsg = [[TestRequired alloc] init];
  [msg.mapField setObject:subMsg forKey:0];
  XCTAssertFalse(msg.initialized);

  // Initialize uninitialized message
  subMsg.a = 0;
  subMsg.b = 0;
  subMsg.c = 0;
  XCTAssertTrue(msg.initialized);

  [subMsg release];
  [msg release];
}

@end
