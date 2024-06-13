// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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

//%PDDM-EXPAND TEST_FOR_POD_KEY(Int64, int64_t, 21LL, 22LL, 23LL, 24LL)
// This block of code is generated, do not edit it directly.

// To let the testing macros work, add some extra methods to simplify things.
@interface GPBInt64EnumDictionary (TestingTweak)
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const int64_t [])keys
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

@implementation GPBInt64EnumDictionary (TestingTweak)
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const int64_t [])keys
                        count:(NSUInteger)count {
  return [self initWithValidationFunction:TestingEnum_IsValidValue
                                rawValues:values
                                  forKeys:keys
                                    count:count];
}
@end


#pragma mark - Int64 -> UInt32

@interface GPBInt64UInt32DictionaryTests : XCTestCase
@end

@implementation GPBInt64UInt32DictionaryTests

- (void)testEmpty {
  GPBInt64UInt32Dictionary *dict = [[GPBInt64UInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:21LL]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(__unused int64_t aKey, __unused uint32_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64UInt32Dictionary *dict = [[GPBInt64UInt32Dictionary alloc] init];
  [dict setUInt32:100U forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:22LL]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(int64_t aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, 100U);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const uint32_t kValues[] = { 100U, 101U, 102U };
  GPBInt64UInt32Dictionary *dict =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  uint32_t *seenValues = malloc(3 * sizeof(uint32_t));
  [dict enumerateKeysAndUInt32sUsingBlock:^(int64_t aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndUInt32sUsingBlock:^(__unused int64_t aKey, __unused uint32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const uint32_t kValues1[] = { 100U, 101U, 102U };
  const uint32_t kValues2[] = { 100U, 103U, 102U };
  const uint32_t kValues3[] = { 100U, 101U, 102U, 103U };
  GPBInt64UInt32Dictionary *dict1 =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64UInt32Dictionary *dict1prime =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64UInt32Dictionary *dict2 =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues2
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64UInt32Dictionary *dict3 =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64UInt32Dictionary *dict4 =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt64UInt32Dictionary *dict =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64UInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64UInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt64UInt32Dictionary *dict =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64UInt32Dictionary *dict2 =
      [[GPBInt64UInt32Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64UInt32Dictionary *dict = [[GPBInt64UInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt32:100U forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const uint32_t kValues[] = { 101U, 102U, 103U };
  GPBInt64UInt32Dictionary *dict2 =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 103U);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt64UInt32Dictionary *dict =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt32ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 103U);

  // Remove again does nothing.
  [dict removeUInt32ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 103U);

  [dict removeUInt32ForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:21LL]);
  XCTAssertFalse([dict getUInt32:NULL forKey:22LL]);
  XCTAssertFalse([dict getUInt32:NULL forKey:23LL]);
  XCTAssertFalse([dict getUInt32:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt64UInt32Dictionary *dict =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:103U forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:101U forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 101U);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const uint32_t kValues2[] = { 102U, 100U };
  GPBInt64UInt32Dictionary *dict2 =
      [[GPBInt64UInt32Dictionary alloc] initWithUInt32s:kValues2
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 101U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Int32

@interface GPBInt64Int32DictionaryTests : XCTestCase
@end

@implementation GPBInt64Int32DictionaryTests

- (void)testEmpty {
  GPBInt64Int32Dictionary *dict = [[GPBInt64Int32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:21LL]);
  [dict enumerateKeysAndInt32sUsingBlock:^(__unused int64_t aKey, __unused int32_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64Int32Dictionary *dict = [[GPBInt64Int32Dictionary alloc] init];
  [dict setInt32:200 forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:22LL]);
  [dict enumerateKeysAndInt32sUsingBlock:^(int64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, 200);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const int32_t kValues[] = { 200, 201, 202 };
  GPBInt64Int32Dictionary *dict =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndInt32sUsingBlock:^(int64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndInt32sUsingBlock:^(__unused int64_t aKey, __unused int32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const int32_t kValues1[] = { 200, 201, 202 };
  const int32_t kValues2[] = { 200, 203, 202 };
  const int32_t kValues3[] = { 200, 201, 202, 203 };
  GPBInt64Int32Dictionary *dict1 =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64Int32Dictionary *dict1prime =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64Int32Dictionary *dict2 =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64Int32Dictionary *dict3 =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64Int32Dictionary *dict4 =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt64Int32Dictionary *dict =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64Int32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64Int32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt64Int32Dictionary *dict =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64Int32Dictionary *dict2 =
      [[GPBInt64Int32Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64Int32Dictionary *dict = [[GPBInt64Int32Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt32:200 forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 201, 202, 203 };
  GPBInt64Int32Dictionary *dict2 =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 203);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt64Int32Dictionary *dict =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt32ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 203);

  // Remove again does nothing.
  [dict removeInt32ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 203);

  [dict removeInt32ForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:21LL]);
  XCTAssertFalse([dict getInt32:NULL forKey:22LL]);
  XCTAssertFalse([dict getInt32:NULL forKey:23LL]);
  XCTAssertFalse([dict getInt32:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt64Int32Dictionary *dict =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 203);

  [dict setInt32:203 forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 203);

  [dict setInt32:201 forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 201);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const int32_t kValues2[] = { 202, 200 };
  GPBInt64Int32Dictionary *dict2 =
      [[GPBInt64Int32Dictionary alloc] initWithInt32s:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt32:&value forKey:21LL]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt32:&value forKey:22LL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt32:&value forKey:23LL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt32:&value forKey:24LL]);
  XCTAssertEqual(value, 201);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> UInt64

@interface GPBInt64UInt64DictionaryTests : XCTestCase
@end

@implementation GPBInt64UInt64DictionaryTests

- (void)testEmpty {
  GPBInt64UInt64Dictionary *dict = [[GPBInt64UInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:21LL]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(__unused int64_t aKey, __unused uint64_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64UInt64Dictionary *dict = [[GPBInt64UInt64Dictionary alloc] init];
  [dict setUInt64:300U forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:22LL]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(int64_t aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, 300U);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const uint64_t kValues[] = { 300U, 301U, 302U };
  GPBInt64UInt64Dictionary *dict =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  uint64_t *seenValues = malloc(3 * sizeof(uint64_t));
  [dict enumerateKeysAndUInt64sUsingBlock:^(int64_t aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndUInt64sUsingBlock:^(__unused int64_t aKey, __unused uint64_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const uint64_t kValues1[] = { 300U, 301U, 302U };
  const uint64_t kValues2[] = { 300U, 303U, 302U };
  const uint64_t kValues3[] = { 300U, 301U, 302U, 303U };
  GPBInt64UInt64Dictionary *dict1 =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64UInt64Dictionary *dict1prime =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64UInt64Dictionary *dict2 =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues2
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64UInt64Dictionary *dict3 =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64UInt64Dictionary *dict4 =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt64UInt64Dictionary *dict =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64UInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64UInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt64UInt64Dictionary *dict =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64UInt64Dictionary *dict2 =
      [[GPBInt64UInt64Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64UInt64Dictionary *dict = [[GPBInt64UInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt64:300U forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const uint64_t kValues[] = { 301U, 302U, 303U };
  GPBInt64UInt64Dictionary *dict2 =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 303U);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt64UInt64Dictionary *dict =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt64ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 303U);

  // Remove again does nothing.
  [dict removeUInt64ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 303U);

  [dict removeUInt64ForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:21LL]);
  XCTAssertFalse([dict getUInt64:NULL forKey:22LL]);
  XCTAssertFalse([dict getUInt64:NULL forKey:23LL]);
  XCTAssertFalse([dict getUInt64:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt64UInt64Dictionary *dict =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:303U forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:301U forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 301U);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const uint64_t kValues2[] = { 302U, 300U };
  GPBInt64UInt64Dictionary *dict2 =
      [[GPBInt64UInt64Dictionary alloc] initWithUInt64s:kValues2
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getUInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 301U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Int64

@interface GPBInt64Int64DictionaryTests : XCTestCase
@end

@implementation GPBInt64Int64DictionaryTests

- (void)testEmpty {
  GPBInt64Int64Dictionary *dict = [[GPBInt64Int64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:21LL]);
  [dict enumerateKeysAndInt64sUsingBlock:^(__unused int64_t aKey, __unused int64_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64Int64Dictionary *dict = [[GPBInt64Int64Dictionary alloc] init];
  [dict setInt64:400 forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:22LL]);
  [dict enumerateKeysAndInt64sUsingBlock:^(int64_t aKey, int64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, 400);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const int64_t kValues[] = { 400, 401, 402 };
  GPBInt64Int64Dictionary *dict =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  int64_t *seenValues = malloc(3 * sizeof(int64_t));
  [dict enumerateKeysAndInt64sUsingBlock:^(int64_t aKey, int64_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndInt64sUsingBlock:^(__unused int64_t aKey, __unused int64_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const int64_t kValues1[] = { 400, 401, 402 };
  const int64_t kValues2[] = { 400, 403, 402 };
  const int64_t kValues3[] = { 400, 401, 402, 403 };
  GPBInt64Int64Dictionary *dict1 =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64Int64Dictionary *dict1prime =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64Int64Dictionary *dict2 =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64Int64Dictionary *dict3 =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64Int64Dictionary *dict4 =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt64Int64Dictionary *dict =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64Int64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64Int64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt64Int64Dictionary *dict =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64Int64Dictionary *dict2 =
      [[GPBInt64Int64Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64Int64Dictionary *dict = [[GPBInt64Int64Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt64:400 forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const int64_t kValues[] = { 401, 402, 403 };
  GPBInt64Int64Dictionary *dict2 =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 403);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt64Int64Dictionary *dict =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt64ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 403);

  // Remove again does nothing.
  [dict removeInt64ForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 403);

  [dict removeInt64ForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:21LL]);
  XCTAssertFalse([dict getInt64:NULL forKey:22LL]);
  XCTAssertFalse([dict getInt64:NULL forKey:23LL]);
  XCTAssertFalse([dict getInt64:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt64Int64Dictionary *dict =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 403);

  [dict setInt64:403 forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 403);

  [dict setInt64:401 forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 401);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const int64_t kValues2[] = { 402, 400 };
  GPBInt64Int64Dictionary *dict2 =
      [[GPBInt64Int64Dictionary alloc] initWithInt64s:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:21LL]);
  XCTAssertTrue([dict getInt64:&value forKey:21LL]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:22LL]);
  XCTAssertTrue([dict getInt64:&value forKey:22LL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:23LL]);
  XCTAssertTrue([dict getInt64:&value forKey:23LL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:24LL]);
  XCTAssertTrue([dict getInt64:&value forKey:24LL]);
  XCTAssertEqual(value, 401);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Bool

@interface GPBInt64BoolDictionaryTests : XCTestCase
@end

@implementation GPBInt64BoolDictionaryTests

- (void)testEmpty {
  GPBInt64BoolDictionary *dict = [[GPBInt64BoolDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:21LL]);
  [dict enumerateKeysAndBoolsUsingBlock:^(__unused int64_t aKey, __unused BOOL aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64BoolDictionary *dict = [[GPBInt64BoolDictionary alloc] init];
  [dict setBool:YES forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:22LL]);
  [dict enumerateKeysAndBoolsUsingBlock:^(int64_t aKey, BOOL aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, YES);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const BOOL kValues[] = { YES, YES, NO };
  GPBInt64BoolDictionary *dict =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:&value forKey:22LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  BOOL *seenValues = malloc(3 * sizeof(BOOL));
  [dict enumerateKeysAndBoolsUsingBlock:^(int64_t aKey, BOOL aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndBoolsUsingBlock:^(__unused int64_t aKey, __unused BOOL aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const BOOL kValues1[] = { YES, YES, NO };
  const BOOL kValues2[] = { YES, NO, NO };
  const BOOL kValues3[] = { YES, YES, NO, NO };
  GPBInt64BoolDictionary *dict1 =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues1
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64BoolDictionary *dict1prime =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues1
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64BoolDictionary *dict2 =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues2
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64BoolDictionary *dict3 =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues1
                                            forKeys:kKeys2
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64BoolDictionary *dict4 =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt64BoolDictionary *dict =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64BoolDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64BoolDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt64BoolDictionary *dict =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64BoolDictionary *dict2 =
      [[GPBInt64BoolDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64BoolDictionary *dict = [[GPBInt64BoolDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setBool:YES forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const BOOL kValues[] = { YES, NO, NO };
  GPBInt64BoolDictionary *dict2 =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:&value forKey:22LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:24LL]);
  XCTAssertTrue([dict getBool:&value forKey:24LL]);
  XCTAssertEqual(value, NO);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt64BoolDictionary *dict =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeBoolForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:24LL]);
  XCTAssertTrue([dict getBool:&value forKey:24LL]);
  XCTAssertEqual(value, NO);

  // Remove again does nothing.
  [dict removeBoolForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:24LL]);
  XCTAssertTrue([dict getBool:&value forKey:24LL]);
  XCTAssertEqual(value, NO);

  [dict removeBoolForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:21LL]);
  XCTAssertFalse([dict getBool:NULL forKey:22LL]);
  XCTAssertFalse([dict getBool:NULL forKey:23LL]);
  XCTAssertFalse([dict getBool:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt64BoolDictionary *dict =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:&value forKey:22LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:24LL]);
  XCTAssertTrue([dict getBool:&value forKey:24LL]);
  XCTAssertEqual(value, NO);

  [dict setBool:NO forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:&value forKey:22LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:24LL]);
  XCTAssertTrue([dict getBool:&value forKey:24LL]);
  XCTAssertEqual(value, NO);

  [dict setBool:YES forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:&value forKey:22LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:24LL]);
  XCTAssertTrue([dict getBool:&value forKey:24LL]);
  XCTAssertEqual(value, YES);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const BOOL kValues2[] = { NO, YES };
  GPBInt64BoolDictionary *dict2 =
      [[GPBInt64BoolDictionary alloc] initWithBools:kValues2
                                            forKeys:kKeys2
                                              count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:21LL]);
  XCTAssertTrue([dict getBool:&value forKey:21LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:22LL]);
  XCTAssertTrue([dict getBool:&value forKey:22LL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:23LL]);
  XCTAssertTrue([dict getBool:&value forKey:23LL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:24LL]);
  XCTAssertTrue([dict getBool:&value forKey:24LL]);
  XCTAssertEqual(value, YES);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Float

@interface GPBInt64FloatDictionaryTests : XCTestCase
@end

@implementation GPBInt64FloatDictionaryTests

- (void)testEmpty {
  GPBInt64FloatDictionary *dict = [[GPBInt64FloatDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:21LL]);
  [dict enumerateKeysAndFloatsUsingBlock:^(__unused int64_t aKey, __unused float aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64FloatDictionary *dict = [[GPBInt64FloatDictionary alloc] init];
  [dict setFloat:500.f forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:22LL]);
  [dict enumerateKeysAndFloatsUsingBlock:^(int64_t aKey, float aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, 500.f);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const float kValues[] = { 500.f, 501.f, 502.f };
  GPBInt64FloatDictionary *dict =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:&value forKey:22LL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  float *seenValues = malloc(3 * sizeof(float));
  [dict enumerateKeysAndFloatsUsingBlock:^(int64_t aKey, float aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndFloatsUsingBlock:^(__unused int64_t aKey, __unused float aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const float kValues1[] = { 500.f, 501.f, 502.f };
  const float kValues2[] = { 500.f, 503.f, 502.f };
  const float kValues3[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt64FloatDictionary *dict1 =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64FloatDictionary *dict1prime =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64FloatDictionary *dict2 =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64FloatDictionary *dict3 =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64FloatDictionary *dict4 =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt64FloatDictionary *dict =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64FloatDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64FloatDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt64FloatDictionary *dict =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64FloatDictionary *dict2 =
      [[GPBInt64FloatDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64FloatDictionary *dict = [[GPBInt64FloatDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setFloat:500.f forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const float kValues[] = { 501.f, 502.f, 503.f };
  GPBInt64FloatDictionary *dict2 =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:&value forKey:22LL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:24LL]);
  XCTAssertTrue([dict getFloat:&value forKey:24LL]);
  XCTAssertEqual(value, 503.f);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt64FloatDictionary *dict =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeFloatForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:24LL]);
  XCTAssertTrue([dict getFloat:&value forKey:24LL]);
  XCTAssertEqual(value, 503.f);

  // Remove again does nothing.
  [dict removeFloatForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:24LL]);
  XCTAssertTrue([dict getFloat:&value forKey:24LL]);
  XCTAssertEqual(value, 503.f);

  [dict removeFloatForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:21LL]);
  XCTAssertFalse([dict getFloat:NULL forKey:22LL]);
  XCTAssertFalse([dict getFloat:NULL forKey:23LL]);
  XCTAssertFalse([dict getFloat:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt64FloatDictionary *dict =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:&value forKey:22LL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:24LL]);
  XCTAssertTrue([dict getFloat:&value forKey:24LL]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:503.f forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:&value forKey:22LL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:24LL]);
  XCTAssertTrue([dict getFloat:&value forKey:24LL]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:501.f forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:&value forKey:22LL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:24LL]);
  XCTAssertTrue([dict getFloat:&value forKey:24LL]);
  XCTAssertEqual(value, 501.f);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const float kValues2[] = { 502.f, 500.f };
  GPBInt64FloatDictionary *dict2 =
      [[GPBInt64FloatDictionary alloc] initWithFloats:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:21LL]);
  XCTAssertTrue([dict getFloat:&value forKey:21LL]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:22LL]);
  XCTAssertTrue([dict getFloat:&value forKey:22LL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:23LL]);
  XCTAssertTrue([dict getFloat:&value forKey:23LL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:24LL]);
  XCTAssertTrue([dict getFloat:&value forKey:24LL]);
  XCTAssertEqual(value, 501.f);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Double

@interface GPBInt64DoubleDictionaryTests : XCTestCase
@end

@implementation GPBInt64DoubleDictionaryTests

- (void)testEmpty {
  GPBInt64DoubleDictionary *dict = [[GPBInt64DoubleDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:21LL]);
  [dict enumerateKeysAndDoublesUsingBlock:^(__unused int64_t aKey, __unused double aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64DoubleDictionary *dict = [[GPBInt64DoubleDictionary alloc] init];
  [dict setDouble:600. forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:22LL]);
  [dict enumerateKeysAndDoublesUsingBlock:^(int64_t aKey, double aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, 600.);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const double kValues[] = { 600., 601., 602. };
  GPBInt64DoubleDictionary *dict =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:&value forKey:22LL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  double *seenValues = malloc(3 * sizeof(double));
  [dict enumerateKeysAndDoublesUsingBlock:^(int64_t aKey, double aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndDoublesUsingBlock:^(__unused int64_t aKey, __unused double aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const double kValues1[] = { 600., 601., 602. };
  const double kValues2[] = { 600., 603., 602. };
  const double kValues3[] = { 600., 601., 602., 603. };
  GPBInt64DoubleDictionary *dict1 =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64DoubleDictionary *dict1prime =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64DoubleDictionary *dict2 =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues2
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64DoubleDictionary *dict3 =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues1
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64DoubleDictionary *dict4 =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt64DoubleDictionary *dict =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64DoubleDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64DoubleDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt64DoubleDictionary *dict =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64DoubleDictionary *dict2 =
      [[GPBInt64DoubleDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64DoubleDictionary *dict = [[GPBInt64DoubleDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setDouble:600. forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const double kValues[] = { 601., 602., 603. };
  GPBInt64DoubleDictionary *dict2 =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:&value forKey:22LL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:24LL]);
  XCTAssertTrue([dict getDouble:&value forKey:24LL]);
  XCTAssertEqual(value, 603.);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt64DoubleDictionary *dict =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeDoubleForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:24LL]);
  XCTAssertTrue([dict getDouble:&value forKey:24LL]);
  XCTAssertEqual(value, 603.);

  // Remove again does nothing.
  [dict removeDoubleForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:24LL]);
  XCTAssertTrue([dict getDouble:&value forKey:24LL]);
  XCTAssertEqual(value, 603.);

  [dict removeDoubleForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:21LL]);
  XCTAssertFalse([dict getDouble:NULL forKey:22LL]);
  XCTAssertFalse([dict getDouble:NULL forKey:23LL]);
  XCTAssertFalse([dict getDouble:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt64DoubleDictionary *dict =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:&value forKey:22LL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:24LL]);
  XCTAssertTrue([dict getDouble:&value forKey:24LL]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:603. forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:&value forKey:22LL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:24LL]);
  XCTAssertTrue([dict getDouble:&value forKey:24LL]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:601. forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:&value forKey:22LL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:24LL]);
  XCTAssertTrue([dict getDouble:&value forKey:24LL]);
  XCTAssertEqual(value, 601.);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const double kValues2[] = { 602., 600. };
  GPBInt64DoubleDictionary *dict2 =
      [[GPBInt64DoubleDictionary alloc] initWithDoubles:kValues2
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:21LL]);
  XCTAssertTrue([dict getDouble:&value forKey:21LL]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:22LL]);
  XCTAssertTrue([dict getDouble:&value forKey:22LL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:23LL]);
  XCTAssertTrue([dict getDouble:&value forKey:23LL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:24LL]);
  XCTAssertTrue([dict getDouble:&value forKey:24LL]);
  XCTAssertEqual(value, 601.);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Enum

@interface GPBInt64EnumDictionaryTests : XCTestCase
@end

@implementation GPBInt64EnumDictionaryTests

- (void)testEmpty {
  GPBInt64EnumDictionary *dict = [[GPBInt64EnumDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:21LL]);
  [dict enumerateKeysAndEnumsUsingBlock:^(__unused int64_t aKey, __unused int32_t aValue, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64EnumDictionary *dict = [[GPBInt64EnumDictionary alloc] init];
  [dict setEnum:700 forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  [dict enumerateKeysAndEnumsUsingBlock:^(int64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqual(aValue, 700);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const int32_t kValues[] = { 700, 701, 702 };
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(int64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndEnumsUsingBlock:^(__unused int64_t aKey, __unused int32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const int32_t kValues1[] = { 700, 701, 702 };
  const int32_t kValues2[] = { 700, 703, 702 };
  const int32_t kValues3[] = { 700, 701, 702, 703 };
  GPBInt64EnumDictionary *dict1 =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues1
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64EnumDictionary *dict1prime =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues1
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues2
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64EnumDictionary *dict3 =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues1
                                            forKeys:kKeys2
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64EnumDictionary *dict4 =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues3
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64EnumDictionary *dict = [[GPBInt64EnumDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setEnum:700 forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 701, 702, 703 };
  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 703);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 703);

  // Remove again does nothing.
  [dict removeEnumForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 703);

  [dict removeEnumForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:21LL]);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertFalse([dict getEnum:NULL forKey:23LL]);
  XCTAssertFalse([dict getEnum:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 703);

  [dict setEnum:703 forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 703);

  [dict setEnum:701 forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 701);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const int32_t kValues2[] = { 702, 700 };
  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues2
                                            forKeys:kKeys2
                                              count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 701);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Enum (Unknown Enums)

@interface GPBInt64EnumDictionaryUnknownEnumTests : XCTestCase
@end

@implementation GPBInt64EnumDictionaryUnknownEnumTests

- (void)testRawBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const int32_t kValues[] = { 700, 801, 702 };
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue(dict.validationFunc == TestingEnum_IsValidValue);  // Pointer comparison
  int32_t value;
  XCTAssertTrue([dict getRawValue:NULL forKey:21LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:22LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:22LL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getRawValue:NULL forKey:23LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getRawValue:NULL forKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(int64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(int64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(__unused int64_t aKey, __unused int32_t aValue, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEqualityWithUnknowns {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const int32_t kValues1[] = { 700, 801, 702 };  // Unknown
  const int32_t kValues2[] = { 700, 803, 702 };  // Unknown
  const int32_t kValues3[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt64EnumDictionary *dict1 =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues1
                                                         forKeys:kKeys1
                                                           count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt64EnumDictionary *dict1prime =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues1
                                                         forKeys:kKeys1
                                                           count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues2
                                                         forKeys:kKeys1
                                                           count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt64EnumDictionary *dict3 =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues1
                                                         forKeys:kKeys2
                                                           count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt64EnumDictionary *dict4 =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknown
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertEqualObjects(dict, dict2);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  [dict2 release];
  [dict release];
}

- (void)testUnknownAdds {
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  XCTAssertThrowsSpecificNamed([dict setEnum:801 forKey:22LL],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 0U);
  [dict setRawValue:801 forKey:22LL];  // Unknown
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 21LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 702, 803 };  // Unknown
  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithEnums:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:22LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:22LL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:24LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:24LL]);
  XCTAssertEqual(value, 803);
  [dict2 release];
  [dict release];
}

- (void)testUnknownRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:24LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:24LL]);
  XCTAssertEqual(value, 803);

  // Remove again does nothing.
  [dict removeEnumForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:24LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:24LL]);
  XCTAssertEqual(value, 803);

  [dict removeEnumForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:21LL]);
  XCTAssertFalse([dict getEnum:NULL forKey:22LL]);
  XCTAssertFalse([dict getEnum:NULL forKey:23LL]);
  XCTAssertFalse([dict getEnum:NULL forKey:24LL]);
  [dict release];
}

- (void)testInplaceMutationUnknowns {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:22LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:22LL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:24LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:24LL]);
  XCTAssertEqual(value, 803);

  XCTAssertThrowsSpecificNamed([dict setEnum:803 forKey:21LL],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:21LL]);
  XCTAssertTrue([dict getEnum:&value forKey:21LL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:22LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:22LL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:24LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:24LL]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:803 forKey:21LL];  // Unknown
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:21LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:21LL]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:22LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:22LL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:24LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:24LL]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:700 forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:21LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:21LL]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:22LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:22LL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:23LL]);
  XCTAssertTrue([dict getEnum:&value forKey:23LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 700);

  const int64_t kKeys2[] = { 22LL, 23LL };
  const int32_t kValues2[] = { 702, 801 };  // Unknown
  GPBInt64EnumDictionary *dict2 =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues2
                                                         forKeys:kKeys2
                                                           count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:21LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:21LL]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getEnum:NULL forKey:22LL]);
  XCTAssertTrue([dict getEnum:&value forKey:22LL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:23LL]);
  XCTAssertTrue([dict getRawValue:&value forKey:23LL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:24LL]);
  XCTAssertTrue([dict getEnum:&value forKey:24LL]);
  XCTAssertEqual(value, 700);

  [dict2 release];
  [dict release];
}

- (void)testCopyUnknowns {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const int32_t kValues[] = { 700, 801, 702, 803 };
  GPBInt64EnumDictionary *dict =
      [[GPBInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt64EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int64 -> Object

@interface GPBInt64ObjectDictionaryTests : XCTestCase
@end

@implementation GPBInt64ObjectDictionaryTests

- (void)testEmpty {
  GPBInt64ObjectDictionary<NSString*> *dict = [[GPBInt64ObjectDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:21LL]);
  [dict enumerateKeysAndObjectsUsingBlock:^(__unused int64_t aKey, __unused NSString* aObject, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt64ObjectDictionary<NSString*> *dict = [[GPBInt64ObjectDictionary alloc] init];
  [dict setObject:@"abc" forKey:21LL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"abc");
  XCTAssertNil([dict objectForKey:22LL]);
  [dict enumerateKeysAndObjectsUsingBlock:^(int64_t aKey, NSString* aObject, BOOL *stop) {
    XCTAssertEqual(aKey, 21LL);
    XCTAssertEqualObjects(aObject, @"abc");
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi" };
  GPBInt64ObjectDictionary<NSString*> *dict =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:22LL], @"def");
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertNil([dict objectForKey:24LL]);

  __block NSUInteger idx = 0;
  int64_t *seenKeys = malloc(3 * sizeof(int64_t));
  NSString* *seenObjects = malloc(3 * sizeof(NSString*));
  [dict enumerateKeysAndObjectsUsingBlock:^(int64_t aKey, NSString* aObject, BOOL *stop) {
    XCTAssertLessThan(idx, 3U);
    seenKeys[idx] = aKey;
    seenObjects[idx] = aObject;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 3; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 3) && !foundKey; ++j) {
      if (kKeys[i] == seenKeys[j]) {
        foundKey = YES;
        XCTAssertEqualObjects(kObjects[i], seenObjects[j], @"i = %d, j = %d", i, j);
      }
    }
    XCTAssertTrue(foundKey, @"i = %d", i);
  }
  free(seenKeys);
  free(seenObjects);

  // Stopping the enumeration.
  idx = 0;
  [dict enumerateKeysAndObjectsUsingBlock:^(__unused int64_t aKey, __unused NSString* aObject, BOOL *stop) {
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int64_t kKeys1[] = { 21LL, 22LL, 23LL, 24LL };
  const int64_t kKeys2[] = { 22LL, 21LL, 24LL };
  const NSString* kObjects1[] = { @"abc", @"def", @"ghi" };
  const NSString* kObjects2[] = { @"abc", @"jkl", @"ghi" };
  const NSString* kObjects3[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt64ObjectDictionary<NSString*> *dict1 =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1);
  GPBInt64ObjectDictionary<NSString*> *dict1prime =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1prime);
  GPBInt64ObjectDictionary<NSString*> *dict2 =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects2
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  GPBInt64ObjectDictionary<NSString*> *dict3 =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects1
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict3);
  GPBInt64ObjectDictionary<NSString*> *dict4 =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects3
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kObjects3)];
  XCTAssertNotNil(dict4);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(dict1, dict1prime);
  XCTAssertEqualObjects(dict1, dict1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([dict1 hash], [dict1prime hash]);

  // 2 is same keys, different objects; not equal.
  XCTAssertNotEqualObjects(dict1, dict2);

  // 3 is different keys, same objects; not equal.
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
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt64ObjectDictionary<NSString*> *dict =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBInt64ObjectDictionary<NSString*> *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt64ObjectDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt64ObjectDictionary<NSString*> *dict =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBInt64ObjectDictionary<NSString*> *dict2 =
      [[GPBInt64ObjectDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBInt64ObjectDictionary<NSString*> *dict = [[GPBInt64ObjectDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setObject:@"abc" forKey:21LL];
  XCTAssertEqual(dict.count, 1U);

  const int64_t kKeys[] = { 22LL, 23LL, 24LL };
  const NSString* kObjects[] = { @"def", @"ghi", @"jkl" };
  GPBInt64ObjectDictionary<NSString*> *dict2 =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  XCTAssertEqualObjects([dict objectForKey:21LL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:22LL], @"def");
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:24LL], @"jkl");
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt64ObjectDictionary<NSString*> *dict =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeObjectForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"abc");
  XCTAssertNil([dict objectForKey:22LL]);
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:24LL], @"jkl");

  // Remove again does nothing.
  [dict removeObjectForKey:22LL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"abc");
  XCTAssertNil([dict objectForKey:22LL]);
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:24LL], @"jkl");

  [dict removeObjectForKey:24LL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"abc");
  XCTAssertNil([dict objectForKey:22LL]);
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertNil([dict objectForKey:24LL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:21LL]);
  XCTAssertNil([dict objectForKey:22LL]);
  XCTAssertNil([dict objectForKey:23LL]);
  XCTAssertNil([dict objectForKey:24LL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int64_t kKeys[] = { 21LL, 22LL, 23LL, 24LL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt64ObjectDictionary<NSString*> *dict =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:22LL], @"def");
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:24LL], @"jkl");

  [dict setObject:@"jkl" forKey:21LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:22LL], @"def");
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:24LL], @"jkl");

  [dict setObject:@"def" forKey:24LL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:22LL], @"def");
  XCTAssertEqualObjects([dict objectForKey:23LL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:24LL], @"def");

  const int64_t kKeys2[] = { 22LL, 23LL };
  const NSString* kObjects2[] = { @"ghi", @"abc" };
  GPBInt64ObjectDictionary<NSString*> *dict2 =
      [[GPBInt64ObjectDictionary alloc] initWithObjects:kObjects2
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:21LL], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:22LL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:23LL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:24LL], @"def");

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND-END TEST_FOR_POD_KEY(Int64, int64_t, 21LL, 22LL, 23LL, 24LL)

