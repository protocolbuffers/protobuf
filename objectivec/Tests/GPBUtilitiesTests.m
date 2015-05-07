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

#import "google/protobuf/MapUnittest.pbobjc.h"
#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestObjc.pbobjc.h"

@interface UtilitiesTests : GPBTestCase
@end

// Support code for testing
typedef struct {
  uint32_t _has_storage_[1];
  BOOL aBool;
  int32_t aInt32;
  uint32_t aUInt32;
  int64_t aInt64;
  uint64_t aUInt64;
  float aFloat;
  double aDouble;
  id aObject;
  BOOL _hasTest;
  BOOL stopper;
  BOOL shouldNotBeCounted;
  GPBInt32Array *anArray;
} ApplyFunctionsTest_Storage;

@interface ApplyFunctionsTest : GPBMessage
@property(nonatomic, readwrite) BOOL aBool;
@property(nonatomic, readwrite) int32_t aInt32;
@property(nonatomic, readwrite) uint32_t aUInt32;
@property(nonatomic, readwrite) int64_t aInt64;
@property(nonatomic, readwrite) uint64_t aUInt64;
@property(nonatomic, readwrite) float aFloat;
@property(nonatomic, readwrite) double aDouble;
@property(nonatomic, readwrite, retain) id aObject;
@property(nonatomic, readwrite) BOOL _hasTest;
@property(nonatomic, readwrite) BOOL stopper;
@property(nonatomic, readwrite) BOOL shouldNotBeCounted;
@property(nonatomic, readwrite, retain) GPBInt32Array *anArray;
@end

@implementation ApplyFunctionsTest

@dynamic aBool, aInt32, aUInt32, aInt64, aUInt64, aFloat, aDouble, aObject;
@dynamic _hasTest, stopper, shouldNotBeCounted, anArray;

+ (GPBDescriptor *)descriptor {
  static GPBDescriptor *descriptor = NULL;
  if (!descriptor) {
    static GPBMessageFieldDescription fields[] = {
#define FIELD_ENTRY(NAME, INDEX)                                 \
  {                                                              \
    .name = "a" #NAME, .hasIndex = INDEX, .type = GPBType##NAME, \
    .offset = offsetof(ApplyFunctionsTest_Storage, a##NAME),     \
  }
        FIELD_ENTRY(Bool, 1),
        FIELD_ENTRY(Int32, 2),
        FIELD_ENTRY(UInt32, 3),
        FIELD_ENTRY(Int64, 4),
        FIELD_ENTRY(UInt64, 5),
        FIELD_ENTRY(Float, 6),
        FIELD_ENTRY(Double, 7),
#undef FIELD_ENTRY
        {
         .name = "aObject",
         .type = GPBTypeString,
         .hasIndex = 8,
         .offset = offsetof(ApplyFunctionsTest_Storage, aObject),
        },
        {
         .name = "stopper",
         .type = GPBTypeBool,
         .hasIndex = 9,
         .offset = offsetof(ApplyFunctionsTest_Storage, stopper),
        },
        {
         .name = "shouldNotBeCounted",
         .type = GPBTypeBool,
         .hasIndex = 10,
         .offset = offsetof(ApplyFunctionsTest_Storage, shouldNotBeCounted),
        },
        {
         .name = "anArray",
         .type = GPBTypeInt32,
         .hasIndex = 11,
         .flags = GPBFieldRepeated,
         .offset = offsetof(ApplyFunctionsTest_Storage, anArray),
        },
    };
    descriptor = [GPBDescriptor
        allocDescriptorForClass:[self class]
                      rootClass:Nil
                           file:nil
                         fields:fields
                     fieldCount:sizeof(fields) /
                                sizeof(GPBMessageFieldDescription)
                         oneofs:NULL
                     oneofCount:0
                          enums:NULL
                      enumCount:0
                         ranges:NULL
                     rangeCount:0
                    storageSize:sizeof(ApplyFunctionsTest_Storage)
                     wireFormat:NO];
  }
  return descriptor;
}

@end

typedef struct {
  int calledBool;
  int calledInt32;
  int calledUInt32;
  int calledInt64;
  int calledUInt64;
  int calledFloat;
  int calledDouble;
  int calledObject;
  int hitCount;
} TestApplyFunctionsContext;

// Really, who needs templates?
// Macro for testing apply functions. Declares a variety of different functions
// base on |NAME|.
#define TEST_APPLY_FUNCTIONS_FUNC(NAME)                          \
  static BOOL TestApplyFunction##NAME(GPBFieldDescriptor *field, \
                                      void *voidContext) {       \
    TestApplyFunctionsContext *context = voidContext;            \
    if (field->getSel_ == sel_getUid("stopper")) return NO;      \
    context->called##NAME += 1;                                  \
    context->hitCount += 1;                                      \
    return YES;                                                  \
  }

TEST_APPLY_FUNCTIONS_FUNC(Bool)
TEST_APPLY_FUNCTIONS_FUNC(Int32)
TEST_APPLY_FUNCTIONS_FUNC(UInt32)
TEST_APPLY_FUNCTIONS_FUNC(Int64)
TEST_APPLY_FUNCTIONS_FUNC(UInt64)
TEST_APPLY_FUNCTIONS_FUNC(Float)
TEST_APPLY_FUNCTIONS_FUNC(Double)
TEST_APPLY_FUNCTIONS_FUNC(Object)

@implementation UtilitiesTests

- (void)testRightShiftFunctions {
  XCTAssertEqual((1UL << 31) >> 31, 1UL);
  XCTAssertEqual((1 << 31) >> 31, -1);
  XCTAssertEqual((1ULL << 63) >> 63, 1ULL);
  XCTAssertEqual((1LL << 63) >> 63, -1LL);

  XCTAssertEqual(GPBLogicalRightShift32((1 << 31), 31), 1);
  XCTAssertEqual(GPBLogicalRightShift64((1LL << 63), 63), 1LL);
}

- (void)testMutability {
  ApplyFunctionsTest *foo_message = [ApplyFunctionsTest message];
  XCTAssertEqual(0, [foo_message aInt32]);
  [foo_message setAInt32:100];
  XCTAssertEqual(100, [foo_message aInt32]);
}

- (void)testSerializedSize {
  ApplyFunctionsTest *foo_message = [ApplyFunctionsTest message];
  [foo_message setAInt32:100];
  size_t size1 = [foo_message serializedSize];
  [foo_message setAInt64:100];
  size_t size2 = [foo_message serializedSize];

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(size1, size2);
}

- (void)testCopying {
  ApplyFunctionsTest *foo_message = [ApplyFunctionsTest message];
  [foo_message setAInt32:100];
  [foo_message setAObject:@"Happy"];
  ApplyFunctionsTest *foo = [[foo_message copy] autorelease];
  XCTAssertNotEqual(foo, foo_message);  // Pointer comparision
  XCTAssertEqualObjects(foo, foo_message);
}

- (void)testApplyFunctions {
  // Covers ApplyFunctionsToProtoVariables and
  // ApplyFunctionsBasedOnEncodingType.
  // This test depends on the layout of the ivars to be in the order
  // declared in the interface. If this is not true, it will fail and will
  // need to be rewritten to accomodate.
  TestApplyFunctionsContext context;
  memset(&context, 0, sizeof(context));
  GPBApplyFunctions foo = GPBAPPLY_FUNCTIONS_INIT(TestApplyFunction);
  ApplyFunctionsTest *msg = [ApplyFunctionsTest message];
  GPBApplyFunctionsToMessageFields(&foo, msg, &context);

  // Only eight vars should be set.
  // "stopper" should cause the loop to quit so it and shouldNotBeCounted should
  // not be counted.
  // "_hasTest" should be skipped over.
  // Each of the vars should only be set once.
  XCTAssertEqual(context.hitCount, 8);
  XCTAssertEqual(context.calledBool, 1);
  XCTAssertEqual(context.calledInt32, 1);
  XCTAssertEqual(context.calledUInt32, 1);
  XCTAssertEqual(context.calledInt64, 1);
  XCTAssertEqual(context.calledUInt64, 1);
  XCTAssertEqual(context.calledFloat, 1);
  XCTAssertEqual(context.calledDouble, 1);
  XCTAssertEqual(context.calledObject, 1);
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

@end
