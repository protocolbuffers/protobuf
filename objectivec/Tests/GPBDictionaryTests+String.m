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

#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>

#import "GPBDictionary.h"

#import "GPBTestUtilities.h"
#import "objectivec/Tests/UnittestRuntimeProto2.pbobjc.h"

// Disable clang-format for the macros.
// clang-format off

// Pull in the macros (using an external file because expanding all tests
// in a single file makes a file that is failing to work with within Xcode.
//%PDDM-IMPORT-DEFINES GPBDictionaryTests.pddm

//%PDDM-EXPAND TESTS_FOR_POD_VALUES(String,NSString,*,Objects,@"foo",@"bar",@"baz",@"mumble")
// This block of code is generated, do not edit it directly.

// To let the testing macros work, add some extra methods to simplify things.
@interface GPBStringEnumDictionary (TestingTweak)
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const NSString * [])keys
                        count:(NSUInteger)count;
@end

static BOOL TestingEnum_IsValidValue(int32_t value) {
  switch (value) {
    case 700:
    case 701:
    case 702:
    case 703:
      return YES;
    default:
      return NO;
  }
}

@implementation GPBStringEnumDictionary (TestingTweak)
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const NSString * [])keys
                        count:(NSUInteger)count {
  return [self initWithValidationFunction:TestingEnum_IsValidValue
                                rawValues:values
                                  forKeys:keys
                                    count:count];
}
@end


#pragma mark - String -> UInt32

@interface GPBStringUInt32DictionaryTests : XCTestCase
@end

@implementation GPBStringUInt32DictionaryTests

- (void)testEmpty {
  GPBStringUInt32Dictionary *dict = [[GPBStringUInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"foo"]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(__unused NSString *aKey, __unused uint32_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringUInt32Dictionary *dict = [[GPBStringUInt32Dictionary alloc] init];
  [dict setUInt32:100U forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"bar"]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(NSString *aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, 100U);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const uint32_t kValues[] = { 100U, 101U, 102U };
  GPBStringUInt32Dictionary *dict =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  uint32_t *seenValues = malloc(3 * sizeof(uint32_t));
  [dict enumerateKeysAndUInt32sUsingBlock:^(NSString *aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndUInt32sUsingBlock:^(__unused NSString *aKey, __unused uint32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const uint32_t kValues1[] = { 100U, 101U, 102U };
  const uint32_t kValues2[] = { 100U, 103U, 102U };
  const uint32_t kValues3[] = { 100U, 101U, 102U, 103U };
  GPBStringUInt32Dictionary *dict1 =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringUInt32Dictionary *dict1prime =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringUInt32Dictionary *dict2 =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringUInt32Dictionary *dict3 =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringUInt32Dictionary *dict4 =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues3
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBStringUInt32Dictionary *dict =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringUInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringUInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBStringUInt32Dictionary *dict =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringUInt32Dictionary *dict2 =
      [[GPBStringUInt32Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringUInt32Dictionary *dict = [[GPBStringUInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt32:100U forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const uint32_t kValues[] = { 101U, 102U, 103U };
  GPBStringUInt32Dictionary *dict2 =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 103U);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBStringUInt32Dictionary *dict =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt32ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 103U);

  // Remove again does nothing.
  [dict removeUInt32ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 103U);

  [dict removeUInt32ForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getUInt32:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBStringUInt32Dictionary *dict =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:103U forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:101U forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 101U);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const uint32_t kValues2[] = { 102U, 100U };
  GPBStringUInt32Dictionary *dict2 =
      [[GPBStringUInt32Dictionary alloc] initWithUInt32s:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 101U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> Int32

@interface GPBStringInt32DictionaryTests : XCTestCase
@end

@implementation GPBStringInt32DictionaryTests

- (void)testEmpty {
  GPBStringInt32Dictionary *dict = [[GPBStringInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:@"foo"]);
  [dict enumerateKeysAndInt32sUsingBlock:^(__unused NSString *aKey, __unused int32_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringInt32Dictionary *dict = [[GPBStringInt32Dictionary alloc] init];
  [dict setInt32:200 forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:@"bar"]);
  [dict enumerateKeysAndInt32sUsingBlock:^(NSString *aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, 200);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const int32_t kValues[] = { 200, 201, 202 };
  GPBStringInt32Dictionary *dict =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndInt32sUsingBlock:^(NSString *aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndInt32sUsingBlock:^(__unused NSString *aKey, __unused int32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const int32_t kValues1[] = { 200, 201, 202 };
  const int32_t kValues2[] = { 200, 203, 202 };
  const int32_t kValues3[] = { 200, 201, 202, 203 };
  GPBStringInt32Dictionary *dict1 =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringInt32Dictionary *dict1prime =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringInt32Dictionary *dict2 =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringInt32Dictionary *dict3 =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringInt32Dictionary *dict4 =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues3
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBStringInt32Dictionary *dict =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBStringInt32Dictionary *dict =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringInt32Dictionary *dict2 =
      [[GPBStringInt32Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringInt32Dictionary *dict = [[GPBStringInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt32:200 forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 201, 202, 203 };
  GPBStringInt32Dictionary *dict2 =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 203);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBStringInt32Dictionary *dict =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt32ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 203);

  // Remove again does nothing.
  [dict removeInt32ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 203);

  [dict removeInt32ForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getInt32:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBStringInt32Dictionary *dict =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 203);

  [dict setInt32:203 forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 203);

  [dict setInt32:201 forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 201);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const int32_t kValues2[] = { 202, 200 };
  GPBStringInt32Dictionary *dict2 =
      [[GPBStringInt32Dictionary alloc] initWithInt32s:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"foo"]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"bar"]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"baz"]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt32:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 201);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> UInt64

@interface GPBStringUInt64DictionaryTests : XCTestCase
@end

@implementation GPBStringUInt64DictionaryTests

- (void)testEmpty {
  GPBStringUInt64Dictionary *dict = [[GPBStringUInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"foo"]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(__unused NSString *aKey, __unused uint64_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringUInt64Dictionary *dict = [[GPBStringUInt64Dictionary alloc] init];
  [dict setUInt64:300U forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"bar"]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(NSString *aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, 300U);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const uint64_t kValues[] = { 300U, 301U, 302U };
  GPBStringUInt64Dictionary *dict =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  uint64_t *seenValues = malloc(3 * sizeof(uint64_t));
  [dict enumerateKeysAndUInt64sUsingBlock:^(NSString *aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndUInt64sUsingBlock:^(__unused NSString *aKey, __unused uint64_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const uint64_t kValues1[] = { 300U, 301U, 302U };
  const uint64_t kValues2[] = { 300U, 303U, 302U };
  const uint64_t kValues3[] = { 300U, 301U, 302U, 303U };
  GPBStringUInt64Dictionary *dict1 =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringUInt64Dictionary *dict1prime =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringUInt64Dictionary *dict2 =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringUInt64Dictionary *dict3 =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringUInt64Dictionary *dict4 =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues3
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBStringUInt64Dictionary *dict =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringUInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringUInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBStringUInt64Dictionary *dict =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringUInt64Dictionary *dict2 =
      [[GPBStringUInt64Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringUInt64Dictionary *dict = [[GPBStringUInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt64:300U forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const uint64_t kValues[] = { 301U, 302U, 303U };
  GPBStringUInt64Dictionary *dict2 =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 303U);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBStringUInt64Dictionary *dict =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt64ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 303U);

  // Remove again does nothing.
  [dict removeUInt64ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 303U);

  [dict removeUInt64ForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getUInt64:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBStringUInt64Dictionary *dict =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:303U forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:301U forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 301U);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const uint64_t kValues2[] = { 302U, 300U };
  GPBStringUInt64Dictionary *dict2 =
      [[GPBStringUInt64Dictionary alloc] initWithUInt64s:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getUInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 301U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> Int64

@interface GPBStringInt64DictionaryTests : XCTestCase
@end

@implementation GPBStringInt64DictionaryTests

- (void)testEmpty {
  GPBStringInt64Dictionary *dict = [[GPBStringInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:@"foo"]);
  [dict enumerateKeysAndInt64sUsingBlock:^(__unused NSString *aKey, __unused int64_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringInt64Dictionary *dict = [[GPBStringInt64Dictionary alloc] init];
  [dict setInt64:400 forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:@"bar"]);
  [dict enumerateKeysAndInt64sUsingBlock:^(NSString *aKey, int64_t aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, 400);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const int64_t kValues[] = { 400, 401, 402 };
  GPBStringInt64Dictionary *dict =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  int64_t *seenValues = malloc(3 * sizeof(int64_t));
  [dict enumerateKeysAndInt64sUsingBlock:^(NSString *aKey, int64_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndInt64sUsingBlock:^(__unused NSString *aKey, __unused int64_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const int64_t kValues1[] = { 400, 401, 402 };
  const int64_t kValues2[] = { 400, 403, 402 };
  const int64_t kValues3[] = { 400, 401, 402, 403 };
  GPBStringInt64Dictionary *dict1 =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringInt64Dictionary *dict1prime =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringInt64Dictionary *dict2 =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringInt64Dictionary *dict3 =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringInt64Dictionary *dict4 =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues3
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBStringInt64Dictionary *dict =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBStringInt64Dictionary *dict =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringInt64Dictionary *dict2 =
      [[GPBStringInt64Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringInt64Dictionary *dict = [[GPBStringInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt64:400 forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const int64_t kValues[] = { 401, 402, 403 };
  GPBStringInt64Dictionary *dict2 =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 403);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBStringInt64Dictionary *dict =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt64ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 403);

  // Remove again does nothing.
  [dict removeInt64ForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 403);

  [dict removeInt64ForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getInt64:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBStringInt64Dictionary *dict =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 403);

  [dict setInt64:403 forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 403);

  [dict setInt64:401 forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 401);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const int64_t kValues2[] = { 402, 400 };
  GPBStringInt64Dictionary *dict2 =
      [[GPBStringInt64Dictionary alloc] initWithInt64s:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"foo"]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"bar"]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"baz"]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getInt64:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 401);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> Bool

@interface GPBStringBoolDictionaryTests : XCTestCase
@end

@implementation GPBStringBoolDictionaryTests

- (void)testEmpty {
  GPBStringBoolDictionary *dict = [[GPBStringBoolDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:@"foo"]);
  [dict enumerateKeysAndBoolsUsingBlock:^(__unused NSString *aKey, __unused BOOL aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringBoolDictionary *dict = [[GPBStringBoolDictionary alloc] init];
  [dict setBool:YES forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:@"bar"]);
  [dict enumerateKeysAndBoolsUsingBlock:^(NSString *aKey, BOOL aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, YES);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const BOOL kValues[] = { YES, YES, NO };
  GPBStringBoolDictionary *dict =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:&value forKey:@"bar"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  BOOL *seenValues = malloc(3 * sizeof(BOOL));
  [dict enumerateKeysAndBoolsUsingBlock:^(NSString *aKey, BOOL aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndBoolsUsingBlock:^(__unused NSString *aKey, __unused BOOL aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const BOOL kValues1[] = { YES, YES, NO };
  const BOOL kValues2[] = { YES, NO, NO };
  const BOOL kValues3[] = { YES, YES, NO, NO };
  GPBStringBoolDictionary *dict1 =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringBoolDictionary *dict1prime =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringBoolDictionary *dict2 =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringBoolDictionary *dict3 =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringBoolDictionary *dict4 =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues3
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBStringBoolDictionary *dict =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringBoolDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringBoolDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBStringBoolDictionary *dict =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringBoolDictionary *dict2 =
      [[GPBStringBoolDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringBoolDictionary *dict = [[GPBStringBoolDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setBool:YES forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const BOOL kValues[] = { YES, NO, NO };
  GPBStringBoolDictionary *dict2 =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:&value forKey:@"bar"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getBool:&value forKey:@"mumble"]);
  XCTAssertEqual(value, NO);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBStringBoolDictionary *dict =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeBoolForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getBool:&value forKey:@"mumble"]);
  XCTAssertEqual(value, NO);

  // Remove again does nothing.
  [dict removeBoolForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getBool:&value forKey:@"mumble"]);
  XCTAssertEqual(value, NO);

  [dict removeBoolForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getBool:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getBool:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getBool:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBStringBoolDictionary *dict =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:&value forKey:@"bar"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getBool:&value forKey:@"mumble"]);
  XCTAssertEqual(value, NO);

  [dict setBool:NO forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:&value forKey:@"bar"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getBool:&value forKey:@"mumble"]);
  XCTAssertEqual(value, NO);

  [dict setBool:YES forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:&value forKey:@"bar"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getBool:&value forKey:@"mumble"]);
  XCTAssertEqual(value, YES);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const BOOL kValues2[] = { NO, YES };
  GPBStringBoolDictionary *dict2 =
      [[GPBStringBoolDictionary alloc] initWithBools:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getBool:&value forKey:@"foo"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getBool:&value forKey:@"bar"]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getBool:&value forKey:@"baz"]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getBool:&value forKey:@"mumble"]);
  XCTAssertEqual(value, YES);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> Float

@interface GPBStringFloatDictionaryTests : XCTestCase
@end

@implementation GPBStringFloatDictionaryTests

- (void)testEmpty {
  GPBStringFloatDictionary *dict = [[GPBStringFloatDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:@"foo"]);
  [dict enumerateKeysAndFloatsUsingBlock:^(__unused NSString *aKey, __unused float aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringFloatDictionary *dict = [[GPBStringFloatDictionary alloc] init];
  [dict setFloat:500.f forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:@"bar"]);
  [dict enumerateKeysAndFloatsUsingBlock:^(NSString *aKey, float aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, 500.f);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const float kValues[] = { 500.f, 501.f, 502.f };
  GPBStringFloatDictionary *dict =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"bar"]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  float *seenValues = malloc(3 * sizeof(float));
  [dict enumerateKeysAndFloatsUsingBlock:^(NSString *aKey, float aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndFloatsUsingBlock:^(__unused NSString *aKey, __unused float aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const float kValues1[] = { 500.f, 501.f, 502.f };
  const float kValues2[] = { 500.f, 503.f, 502.f };
  const float kValues3[] = { 500.f, 501.f, 502.f, 503.f };
  GPBStringFloatDictionary *dict1 =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringFloatDictionary *dict1prime =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringFloatDictionary *dict2 =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringFloatDictionary *dict3 =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringFloatDictionary *dict4 =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues3
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBStringFloatDictionary *dict =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringFloatDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringFloatDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBStringFloatDictionary *dict =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringFloatDictionary *dict2 =
      [[GPBStringFloatDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringFloatDictionary *dict = [[GPBStringFloatDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setFloat:500.f forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const float kValues[] = { 501.f, 502.f, 503.f };
  GPBStringFloatDictionary *dict2 =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"bar"]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 503.f);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBStringFloatDictionary *dict =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeFloatForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 503.f);

  // Remove again does nothing.
  [dict removeFloatForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 503.f);

  [dict removeFloatForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getFloat:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBStringFloatDictionary *dict =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"bar"]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:503.f forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"bar"]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:501.f forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"bar"]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 501.f);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const float kValues2[] = { 502.f, 500.f };
  GPBStringFloatDictionary *dict2 =
      [[GPBStringFloatDictionary alloc] initWithFloats:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"foo"]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"bar"]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"baz"]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getFloat:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 501.f);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> Double

@interface GPBStringDoubleDictionaryTests : XCTestCase
@end

@implementation GPBStringDoubleDictionaryTests

- (void)testEmpty {
  GPBStringDoubleDictionary *dict = [[GPBStringDoubleDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:@"foo"]);
  [dict enumerateKeysAndDoublesUsingBlock:^(__unused NSString *aKey, __unused double aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringDoubleDictionary *dict = [[GPBStringDoubleDictionary alloc] init];
  [dict setDouble:600. forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:@"bar"]);
  [dict enumerateKeysAndDoublesUsingBlock:^(NSString *aKey, double aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, 600.);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const double kValues[] = { 600., 601., 602. };
  GPBStringDoubleDictionary *dict =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"bar"]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  double *seenValues = malloc(3 * sizeof(double));
  [dict enumerateKeysAndDoublesUsingBlock:^(NSString *aKey, double aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndDoublesUsingBlock:^(__unused NSString *aKey, __unused double aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const double kValues1[] = { 600., 601., 602. };
  const double kValues2[] = { 600., 603., 602. };
  const double kValues3[] = { 600., 601., 602., 603. };
  GPBStringDoubleDictionary *dict1 =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringDoubleDictionary *dict1prime =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringDoubleDictionary *dict2 =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringDoubleDictionary *dict3 =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringDoubleDictionary *dict4 =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues3
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBStringDoubleDictionary *dict =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringDoubleDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringDoubleDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBStringDoubleDictionary *dict =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringDoubleDictionary *dict2 =
      [[GPBStringDoubleDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringDoubleDictionary *dict = [[GPBStringDoubleDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setDouble:600. forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const double kValues[] = { 601., 602., 603. };
  GPBStringDoubleDictionary *dict2 =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"bar"]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 603.);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBStringDoubleDictionary *dict =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeDoubleForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 603.);

  // Remove again does nothing.
  [dict removeDoubleForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 603.);

  [dict removeDoubleForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getDouble:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBStringDoubleDictionary *dict =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"bar"]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:603. forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"bar"]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:601. forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"bar"]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 601.);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const double kValues2[] = { 602., 600. };
  GPBStringDoubleDictionary *dict2 =
      [[GPBStringDoubleDictionary alloc] initWithDoubles:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"foo"]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"bar"]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"baz"]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getDouble:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 601.);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> Enum

@interface GPBStringEnumDictionaryTests : XCTestCase
@end

@implementation GPBStringEnumDictionaryTests

- (void)testEmpty {
  GPBStringEnumDictionary *dict = [[GPBStringEnumDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:@"foo"]);
  [dict enumerateKeysAndEnumsUsingBlock:^(__unused NSString *aKey, __unused int32_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBStringEnumDictionary *dict = [[GPBStringEnumDictionary alloc] init];
  [dict setEnum:700 forKey:@"foo"];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  [dict enumerateKeysAndEnumsUsingBlock:^(NSString *aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqualObjects(aKey, @"foo");
    XCTAssertEqual(aValue, 700);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const int32_t kValues[] = { 700, 701, 702 };
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(NSString *aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndEnumsUsingBlock:^(__unused NSString *aKey, __unused int32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const int32_t kValues1[] = { 700, 701, 702 };
  const int32_t kValues2[] = { 700, 703, 702 };
  const int32_t kValues3[] = { 700, 701, 702, 703 };
  GPBStringEnumDictionary *dict1 =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringEnumDictionary *dict1prime =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringEnumDictionary *dict3 =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringEnumDictionary *dict4 =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues3
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringEnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringEnumDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBStringEnumDictionary *dict = [[GPBStringEnumDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setEnum:700 forKey:@"foo"];
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 701, 702, 703 };
  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 703);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 703);

  // Remove again does nothing.
  [dict removeEnumForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 703);

  [dict removeEnumForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getEnum:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutation {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 703);

  [dict setEnum:703 forKey:@"foo"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 703);

  [dict setEnum:701 forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 701);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const int32_t kValues2[] = { 702, 700 };
  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 701);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - String -> Enum (Unknown Enums)

@interface GPBStringEnumDictionaryUnknownEnumTests : XCTestCase
@end

@implementation GPBStringEnumDictionaryUnknownEnumTests

- (void)testRawBasics {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz" };
  const int32_t kValues[] = { 700, 801, 702 };
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue(dict.validationFunc == TestingEnum_IsValidValue);  // Pointer comparison
  int32_t value;
  XCTAssertTrue([dict getRawValue:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"bar"]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getRawValue:NULL forKey:@"mumble"]);

  __block NSUInteger idx = 0;
  NSString **seenKeys = malloc(3 * sizeof(NSString*));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(NSString *aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        if (i == 1) {
          XCTAssertEqual(kGPBUnrecognizedEnumeratorValue, seenValues[j], @"i = %d, j = %d", i, j);
        } else {
          XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
        }
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  idx = 0;
  [dict enumerateKeysAndRawValuesUsingBlock:^(NSString *aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if ([kKeys[i] isEqual:seenKeys[j]]) {
        foundKey = YES;
        XCTAssertEqual(kValues[i], seenValues[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenValues);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndRawValuesUsingBlock:^(__unused NSString *aKey, __unused int32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEqualityWithUnknowns {
  const NSString *kKeys1[] = { @"foo", @"bar", @"baz", @"mumble" };
  const NSString *kKeys2[] = { @"bar", @"foo", @"mumble" };
  const int32_t kValues1[] = { 700, 801, 702 };  // Unknown
  const int32_t kValues2[] = { 700, 803, 702 };  // Unknown
  const int32_t kValues3[] = { 700, 801, 702, 803 };  // Unknowns
  GPBStringEnumDictionary *dict1 =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBStringEnumDictionary *dict1prime =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues2
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBStringEnumDictionary *dict3 =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys2
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBStringEnumDictionary *dict4 =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues3
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different values; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same values; not equal.
  XCTAssertNotEqualObjects(dict1, dict3);

  // 4 extra pair; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopyWithUnknowns {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknown
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringEnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertEqualObjects(dict, dict2);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  [dict2 release];
  [dict release];
}

- (void)testUnknownAdds {
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  XCTAssertThrowsSpecificNamed([dict setEnum:801 forKey:@"bar"],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 0U);
  [dict setRawValue:801 forKey:@"bar"];  // Unknown
  XCTAssertEqual(dict.count, 1U);

  const NSString *kKeys[] = { @"foo", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 702, 803 };  // Unknown
  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithEnums:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"bar"]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 803);
  [dict2 release];
  [dict release];
}

- (void)testUnknownRemove {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 803);

  // Remove again does nothing.
  [dict removeEnumForKey:@"bar"];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 803);

  [dict removeEnumForKey:@"mumble"];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:@"mumble"]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertFalse([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertFalse([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertFalse([dict getEnum:NULL forKey:@"mumble"]);
  [dict release];
}

- (void)testInplaceMutationUnknowns {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"bar"]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 803);

  XCTAssertThrowsSpecificNamed([dict setEnum:803 forKey:@"foo"],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"foo"]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"bar"]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:803 forKey:@"foo"];  // Unknown
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"foo"]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"bar"]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:700 forKey:@"mumble"];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"foo"]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"bar"]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"baz"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 700);

  const NSString *kKeys2[] = { @"bar", @"baz" };
  const int32_t kValues2[] = { 702, 801 };  // Unknown
  GPBStringEnumDictionary *dict2 =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues2
                                                          forKeys:kKeys2
                                                            count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"foo"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"foo"]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getEnum:NULL forKey:@"bar"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"bar"]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:@"baz"]);
  XCTAssertTrue([dict getRawValue:&value forKey:@"baz"]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:@"mumble"]);
  XCTAssertTrue([dict getEnum:&value forKey:@"mumble"]);
  XCTAssertEqual(value, 700);

  [dict2 release];
  [dict release];
}

- (void)testCopyUnknowns {
  const NSString *kKeys[] = { @"foo", @"bar", @"baz", @"mumble" };
  const int32_t kValues[] = { 700, 801, 702, 803 };
  GPBStringEnumDictionary *dict =
      [[GPBStringEnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBStringEnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertTrue([dict2 isKindOfClass:[GPBStringEnumDictionary class]]);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND-END TESTS_FOR_POD_VALUES(String,NSString,*,Objects,@"foo",@"bar",@"baz",@"mumble")
