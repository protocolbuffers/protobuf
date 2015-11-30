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
#import "google/protobuf/UnittestRuntimeProto2.pbobjc.h"

// Pull in the macros (using an external file because expanding all tests
// in a single file makes a file that is failing to work with within Xcode.
//%PDDM-IMPORT-DEFINES GPBDictionaryTests.pddm

//%PDDM-EXPAND TEST_FOR_POD_KEY(Int32, int32_t, 11, 12, 13, 14)
// This block of code is generated, do not edit it directly.

// To let the testing macros work, add some extra methods to simplify things.
@interface GPBInt32EnumDictionary (TestingTweak)
+ (instancetype)dictionaryWithValue:(int32_t)value forKey:(int32_t)key;
- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const int32_t [])keys
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

@implementation GPBInt32EnumDictionary (TestingTweak)
+ (instancetype)dictionaryWithValue:(int32_t)value forKey:(int32_t)key {
  // Cast is needed to compiler knows what class we are invoking initWithValues: on to get the
  // type correct.
  return [[(GPBInt32EnumDictionary*)[self alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                                  rawValues:&value
                                                                    forKeys:&key
                                                                      count:1] autorelease];
}
- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count {
  return [self initWithValidationFunction:TestingEnum_IsValidValue
                                rawValues:values
                                  forKeys:keys
                                    count:count];
}
@end


#pragma mark - Int32 -> UInt32

@interface GPBInt32UInt32DictionaryTests : XCTestCase
@end

@implementation GPBInt32UInt32DictionaryTests

- (void)testEmpty {
  GPBInt32UInt32Dictionary *dict = [[GPBInt32UInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32UInt32Dictionary *dict = [GPBInt32UInt32Dictionary dictionaryWithValue:100U forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, 100U);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const uint32_t kValues[] = { 100U, 101U, 102U };
  GPBInt32UInt32Dictionary *dict =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  uint32_t *seenValues = malloc(3 * sizeof(uint32_t));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const uint32_t kValues1[] = { 100U, 101U, 102U };
  const uint32_t kValues2[] = { 100U, 103U, 102U };
  const uint32_t kValues3[] = { 100U, 101U, 102U, 103U };
  GPBInt32UInt32Dictionary *dict1 =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32UInt32Dictionary *dict1prime =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32UInt32Dictionary *dict2 =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32UInt32Dictionary *dict3 =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32UInt32Dictionary *dict4 =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt32UInt32Dictionary *dict =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32UInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32UInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt32UInt32Dictionary *dict =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32UInt32Dictionary *dict2 =
      [GPBInt32UInt32Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32UInt32Dictionary *dict = [GPBInt32UInt32Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:100U forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const uint32_t kValues[] = { 101U, 102U, 103U };
  GPBInt32UInt32Dictionary *dict2 =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 103U);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt32UInt32Dictionary *dict =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 103U);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 103U);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBInt32UInt32Dictionary *dict =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 103U);

  [dict setValue:103U forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 103U);

  [dict setValue:101U forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 101U);

  const int32_t kKeys2[] = { 12, 13 };
  const uint32_t kValues2[] = { 102U, 100U };
  GPBInt32UInt32Dictionary *dict2 =
      [[GPBInt32UInt32Dictionary alloc] initWithValues:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 101U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Int32

@interface GPBInt32Int32DictionaryTests : XCTestCase
@end

@implementation GPBInt32Int32DictionaryTests

- (void)testEmpty {
  GPBInt32Int32Dictionary *dict = [[GPBInt32Int32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32Int32Dictionary *dict = [GPBInt32Int32Dictionary dictionaryWithValue:200 forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, 200);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const int32_t kValues[] = { 200, 201, 202 };
  GPBInt32Int32Dictionary *dict =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const int32_t kValues1[] = { 200, 201, 202 };
  const int32_t kValues2[] = { 200, 203, 202 };
  const int32_t kValues3[] = { 200, 201, 202, 203 };
  GPBInt32Int32Dictionary *dict1 =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32Int32Dictionary *dict1prime =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32Int32Dictionary *dict2 =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32Int32Dictionary *dict3 =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32Int32Dictionary *dict4 =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt32Int32Dictionary *dict =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32Int32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32Int32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt32Int32Dictionary *dict =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32Int32Dictionary *dict2 =
      [GPBInt32Int32Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32Int32Dictionary *dict = [GPBInt32Int32Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:200 forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const int32_t kValues[] = { 201, 202, 203 };
  GPBInt32Int32Dictionary *dict2 =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 203);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt32Int32Dictionary *dict =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 203);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 203);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBInt32Int32Dictionary *dict =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 203);

  [dict setValue:203 forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 203);

  [dict setValue:201 forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 201);

  const int32_t kKeys2[] = { 12, 13 };
  const int32_t kValues2[] = { 202, 200 };
  GPBInt32Int32Dictionary *dict2 =
      [[GPBInt32Int32Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 201);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> UInt64

@interface GPBInt32UInt64DictionaryTests : XCTestCase
@end

@implementation GPBInt32UInt64DictionaryTests

- (void)testEmpty {
  GPBInt32UInt64Dictionary *dict = [[GPBInt32UInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32UInt64Dictionary *dict = [GPBInt32UInt64Dictionary dictionaryWithValue:300U forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, 300U);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const uint64_t kValues[] = { 300U, 301U, 302U };
  GPBInt32UInt64Dictionary *dict =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  uint64_t *seenValues = malloc(3 * sizeof(uint64_t));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint64_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const uint64_t kValues1[] = { 300U, 301U, 302U };
  const uint64_t kValues2[] = { 300U, 303U, 302U };
  const uint64_t kValues3[] = { 300U, 301U, 302U, 303U };
  GPBInt32UInt64Dictionary *dict1 =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32UInt64Dictionary *dict1prime =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32UInt64Dictionary *dict2 =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32UInt64Dictionary *dict3 =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32UInt64Dictionary *dict4 =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt32UInt64Dictionary *dict =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32UInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32UInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt32UInt64Dictionary *dict =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32UInt64Dictionary *dict2 =
      [GPBInt32UInt64Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32UInt64Dictionary *dict = [GPBInt32UInt64Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:300U forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const uint64_t kValues[] = { 301U, 302U, 303U };
  GPBInt32UInt64Dictionary *dict2 =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 303U);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt32UInt64Dictionary *dict =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 303U);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 303U);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBInt32UInt64Dictionary *dict =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 303U);

  [dict setValue:303U forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 303U);

  [dict setValue:301U forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 301U);

  const int32_t kKeys2[] = { 12, 13 };
  const uint64_t kValues2[] = { 302U, 300U };
  GPBInt32UInt64Dictionary *dict2 =
      [[GPBInt32UInt64Dictionary alloc] initWithValues:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 301U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Int64

@interface GPBInt32Int64DictionaryTests : XCTestCase
@end

@implementation GPBInt32Int64DictionaryTests

- (void)testEmpty {
  GPBInt32Int64Dictionary *dict = [[GPBInt32Int64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32Int64Dictionary *dict = [GPBInt32Int64Dictionary dictionaryWithValue:400 forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, 400);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const int64_t kValues[] = { 400, 401, 402 };
  GPBInt32Int64Dictionary *dict =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  int64_t *seenValues = malloc(3 * sizeof(int64_t));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int64_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const int64_t kValues1[] = { 400, 401, 402 };
  const int64_t kValues2[] = { 400, 403, 402 };
  const int64_t kValues3[] = { 400, 401, 402, 403 };
  GPBInt32Int64Dictionary *dict1 =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32Int64Dictionary *dict1prime =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32Int64Dictionary *dict2 =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32Int64Dictionary *dict3 =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32Int64Dictionary *dict4 =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt32Int64Dictionary *dict =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32Int64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32Int64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt32Int64Dictionary *dict =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32Int64Dictionary *dict2 =
      [GPBInt32Int64Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32Int64Dictionary *dict = [GPBInt32Int64Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:400 forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const int64_t kValues[] = { 401, 402, 403 };
  GPBInt32Int64Dictionary *dict2 =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 403);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt32Int64Dictionary *dict =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 403);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 403);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBInt32Int64Dictionary *dict =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 403);

  [dict setValue:403 forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 403);

  [dict setValue:401 forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 401);

  const int32_t kKeys2[] = { 12, 13 };
  const int64_t kValues2[] = { 402, 400 };
  GPBInt32Int64Dictionary *dict2 =
      [[GPBInt32Int64Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 401);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Bool

@interface GPBInt32BoolDictionaryTests : XCTestCase
@end

@implementation GPBInt32BoolDictionaryTests

- (void)testEmpty {
  GPBInt32BoolDictionary *dict = [[GPBInt32BoolDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32BoolDictionary *dict = [GPBInt32BoolDictionary dictionaryWithValue:YES forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, BOOL aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, YES);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const BOOL kValues[] = { YES, YES, NO };
  GPBInt32BoolDictionary *dict =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  BOOL *seenValues = malloc(3 * sizeof(BOOL));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, BOOL aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const BOOL kValues1[] = { YES, YES, NO };
  const BOOL kValues2[] = { YES, NO, NO };
  const BOOL kValues3[] = { YES, YES, NO, NO };
  GPBInt32BoolDictionary *dict1 =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32BoolDictionary *dict1prime =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32BoolDictionary *dict2 =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32BoolDictionary *dict3 =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32BoolDictionary *dict4 =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt32BoolDictionary *dict =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32BoolDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32BoolDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt32BoolDictionary *dict =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32BoolDictionary *dict2 =
      [GPBInt32BoolDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32BoolDictionary *dict = [GPBInt32BoolDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:YES forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const BOOL kValues[] = { YES, NO, NO };
  GPBInt32BoolDictionary *dict2 =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  BOOL value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, NO);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt32BoolDictionary *dict =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, NO);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, NO);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBInt32BoolDictionary *dict =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, NO);

  [dict setValue:NO forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, NO);

  [dict setValue:YES forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, YES);

  const int32_t kKeys2[] = { 12, 13 };
  const BOOL kValues2[] = { NO, YES };
  GPBInt32BoolDictionary *dict2 =
      [[GPBInt32BoolDictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, YES);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Float

@interface GPBInt32FloatDictionaryTests : XCTestCase
@end

@implementation GPBInt32FloatDictionaryTests

- (void)testEmpty {
  GPBInt32FloatDictionary *dict = [[GPBInt32FloatDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32FloatDictionary *dict = [GPBInt32FloatDictionary dictionaryWithValue:500.f forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  float value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, float aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, 500.f);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const float kValues[] = { 500.f, 501.f, 502.f };
  GPBInt32FloatDictionary *dict =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  float *seenValues = malloc(3 * sizeof(float));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, float aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const float kValues1[] = { 500.f, 501.f, 502.f };
  const float kValues2[] = { 500.f, 503.f, 502.f };
  const float kValues3[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt32FloatDictionary *dict1 =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32FloatDictionary *dict1prime =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32FloatDictionary *dict2 =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32FloatDictionary *dict3 =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32FloatDictionary *dict4 =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt32FloatDictionary *dict =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32FloatDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32FloatDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt32FloatDictionary *dict =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32FloatDictionary *dict2 =
      [GPBInt32FloatDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32FloatDictionary *dict = [GPBInt32FloatDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:500.f forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const float kValues[] = { 501.f, 502.f, 503.f };
  GPBInt32FloatDictionary *dict2 =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  float value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 503.f);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt32FloatDictionary *dict =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 503.f);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 503.f);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBInt32FloatDictionary *dict =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  float value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 503.f);

  [dict setValue:503.f forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 503.f);

  [dict setValue:501.f forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 501.f);

  const int32_t kKeys2[] = { 12, 13 };
  const float kValues2[] = { 502.f, 500.f };
  GPBInt32FloatDictionary *dict2 =
      [[GPBInt32FloatDictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 501.f);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Double

@interface GPBInt32DoubleDictionaryTests : XCTestCase
@end

@implementation GPBInt32DoubleDictionaryTests

- (void)testEmpty {
  GPBInt32DoubleDictionary *dict = [[GPBInt32DoubleDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32DoubleDictionary *dict = [GPBInt32DoubleDictionary dictionaryWithValue:600. forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  double value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, double aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, 600.);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const double kValues[] = { 600., 601., 602. };
  GPBInt32DoubleDictionary *dict =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  double *seenValues = malloc(3 * sizeof(double));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, double aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const double kValues1[] = { 600., 601., 602. };
  const double kValues2[] = { 600., 603., 602. };
  const double kValues3[] = { 600., 601., 602., 603. };
  GPBInt32DoubleDictionary *dict1 =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32DoubleDictionary *dict1prime =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32DoubleDictionary *dict2 =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32DoubleDictionary *dict3 =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32DoubleDictionary *dict4 =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt32DoubleDictionary *dict =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32DoubleDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32DoubleDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt32DoubleDictionary *dict =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32DoubleDictionary *dict2 =
      [GPBInt32DoubleDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32DoubleDictionary *dict = [GPBInt32DoubleDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:600. forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const double kValues[] = { 601., 602., 603. };
  GPBInt32DoubleDictionary *dict2 =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  double value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 603.);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt32DoubleDictionary *dict =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 603.);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 603.);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBInt32DoubleDictionary *dict =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  double value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 603.);

  [dict setValue:603. forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 603.);

  [dict setValue:601. forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 601.);

  const int32_t kKeys2[] = { 12, 13 };
  const double kValues2[] = { 602., 600. };
  GPBInt32DoubleDictionary *dict2 =
      [[GPBInt32DoubleDictionary alloc] initWithValues:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 601.);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Enum

@interface GPBInt32EnumDictionaryTests : XCTestCase
@end

@implementation GPBInt32EnumDictionaryTests

- (void)testEmpty {
  GPBInt32EnumDictionary *dict = [[GPBInt32EnumDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32EnumDictionary *dict = [GPBInt32EnumDictionary dictionaryWithValue:700 forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqual(aValue, 700);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const int32_t kValues[] = { 700, 701, 702 };
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const int32_t kValues1[] = { 700, 701, 702 };
  const int32_t kValues2[] = { 700, 703, 702 };
  const int32_t kValues3[] = { 700, 701, 702, 703 };
  GPBInt32EnumDictionary *dict1 =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32EnumDictionary *dict1prime =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32EnumDictionary *dict2 =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32EnumDictionary *dict3 =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32EnumDictionary *dict4 =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32EnumDictionary *dict2 =
      [GPBInt32EnumDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32EnumDictionary *dict = [GPBInt32EnumDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:700 forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const int32_t kValues[] = { 701, 702, 703 };
  GPBInt32EnumDictionary *dict2 =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 703);
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 703);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 703);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 703);

  [dict setValue:703 forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 703);

  [dict setValue:701 forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 701);

  const int32_t kKeys2[] = { 12, 13 };
  const int32_t kValues2[] = { 702, 700 };
  GPBInt32EnumDictionary *dict2 =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 701);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Enum (Unknown Enums)

@interface GPBInt32EnumDictionaryUnknownEnumTests : XCTestCase
@end

@implementation GPBInt32EnumDictionaryUnknownEnumTests

- (void)testRawBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const int32_t kValues[] = { 700, 801, 702 };
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue(dict.validationFunc == TestingEnum_IsValidValue);  // Pointer comparison
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:11 rawValue:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict valueForKey:12 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:12 rawValue:&value]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict valueForKey:13 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:13 rawValue:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict valueForKey:14 rawValue:NULL]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(int32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEqualityWithUnknowns {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const int32_t kValues1[] = { 700, 801, 702 };  // Unknown
  const int32_t kValues2[] = { 700, 803, 702 };  // Unknown
  const int32_t kValues3[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt32EnumDictionary *dict1 =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues1
                                                         forKeys:kKeys1
                                                           count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBInt32EnumDictionary *dict1prime =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues1
                                                         forKeys:kKeys1
                                                           count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32EnumDictionary *dict2 =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues2
                                                         forKeys:kKeys1
                                                           count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBInt32EnumDictionary *dict3 =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues1
                                                         forKeys:kKeys2
                                                           count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBInt32EnumDictionary *dict4 =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknown
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertEqualObjects(dict, dict2);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32EnumDictionary *dict2 =
      [GPBInt32EnumDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  [dict release];
}

- (void)testUnknownAdds {
  GPBInt32EnumDictionary *dict =
    [GPBInt32EnumDictionary dictionaryWithValidationFunction:TestingEnum_IsValidValue];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  XCTAssertThrowsSpecificNamed([dict setValue:801 forKey:12],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 0U);
  [dict setRawValue:801 forKey:12];  // Unknown
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 11, 13, 14 };
  const int32_t kValues[] = { 700, 702, 803 };  // Unknown
  GPBInt32EnumDictionary *dict2 =
      [[GPBInt32EnumDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict valueForKey:12 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:12 rawValue:&value]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict valueForKey:14 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:14 rawValue:&value]);
  XCTAssertEqual(value, 803);
  [dict2 release];
}

- (void)testUnknownRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:14 rawValue:&value]);
  XCTAssertEqual(value, 803);

  // Remove again does nothing.
  [dict removeValueForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:14 rawValue:&value]);
  XCTAssertEqual(value, 803);

  [dict removeValueForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:11 value:NULL]);
  XCTAssertFalse([dict valueForKey:12 value:NULL]);
  XCTAssertFalse([dict valueForKey:13 value:NULL]);
  XCTAssertFalse([dict valueForKey:14 value:NULL]);
  [dict release];
}

- (void)testInplaceMutationUnknowns {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:12 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:12 rawValue:&value]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:14 rawValue:&value]);
  XCTAssertEqual(value, 803);

  XCTAssertThrowsSpecificNamed([dict setValue:803 forKey:11],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 value:NULL]);
  XCTAssertTrue([dict valueForKey:11 value:&value]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict valueForKey:12 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:12 rawValue:&value]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:14 rawValue:&value]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:803 forKey:11];  // Unknown
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:11 rawValue:&value]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict valueForKey:12 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:12 rawValue:&value]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:14 rawValue:&value]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:700 forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:11 rawValue:&value]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict valueForKey:12 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:12 rawValue:&value]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict valueForKey:13 value:NULL]);
  XCTAssertTrue([dict valueForKey:13 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 700);

  const int32_t kKeys2[] = { 12, 13 };
  const int32_t kValues2[] = { 702, 801 };  // Unknown
  GPBInt32EnumDictionary *dict2 =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues2
                                                         forKeys:kKeys2
                                                           count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict valueForKey:11 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:11 rawValue:&value]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict valueForKey:12 value:NULL]);
  XCTAssertTrue([dict valueForKey:12 value:&value]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict valueForKey:13 rawValue:NULL]);
  XCTAssertTrue([dict valueForKey:13 rawValue:&value]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict valueForKey:14 value:NULL]);
  XCTAssertTrue([dict valueForKey:14 value:&value]);
  XCTAssertEqual(value, 700);

  [dict2 release];
  [dict release];
}

- (void)testCopyUnknowns {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const int32_t kValues[] = { 700, 801, 702, 803 };
  GPBInt32EnumDictionary *dict =
      [[GPBInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                       rawValues:kValues
                                                         forKeys:kKeys
                                                           count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBInt32EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - Int32 -> Object

@interface GPBInt32ObjectDictionaryTests : XCTestCase
@end

@implementation GPBInt32ObjectDictionaryTests

- (void)testEmpty {
  GPBInt32ObjectDictionary<NSString*> *dict = [[GPBInt32ObjectDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:11]);
  [dict enumerateKeysAndObjectsUsingBlock:^(int32_t aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBInt32ObjectDictionary<NSString*> *dict = [GPBInt32ObjectDictionary dictionaryWithObject:@"abc" forKey:11];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  XCTAssertEqualObjects([dict objectForKey:11], @"abc");
  XCTAssertNil([dict objectForKey:12]);
  [dict enumerateKeysAndObjectsUsingBlock:^(int32_t aKey, NSString* aObject, BOOL *stop) {
    XCTAssertEqual(aKey, 11);
    XCTAssertEqualObjects(aObject, @"abc");
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const int32_t kKeys[] = { 11, 12, 13 };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi" };
  GPBInt32ObjectDictionary<NSString*> *dict =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:11], @"abc");
  XCTAssertEqualObjects([dict objectForKey:12], @"def");
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertNil([dict objectForKey:14]);

  __block NSUInteger idx = 0;
  int32_t *seenKeys = malloc(3 * sizeof(int32_t));
  NSString* *seenObjects = malloc(3 * sizeof(NSString*));
  [dict enumerateKeysAndObjectsUsingBlock:^(int32_t aKey, NSString* aObject, BOOL *stop) {
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
  [dict enumerateKeysAndObjectsUsingBlock:^(int32_t aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const int32_t kKeys1[] = { 11, 12, 13, 14 };
  const int32_t kKeys2[] = { 12, 11, 14 };
  const NSString* kObjects1[] = { @"abc", @"def", @"ghi" };
  const NSString* kObjects2[] = { @"abc", @"jkl", @"ghi" };
  const NSString* kObjects3[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt32ObjectDictionary<NSString*> *dict1 =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1);
  GPBInt32ObjectDictionary<NSString*> *dict1prime =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects1
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1prime);
  GPBInt32ObjectDictionary<NSString*> *dict2 =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects2
                                                forKeys:kKeys1
                                                  count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  GPBInt32ObjectDictionary<NSString*> *dict3 =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects1
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict3);
  GPBInt32ObjectDictionary<NSString*> *dict4 =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects3
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
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt32ObjectDictionary<NSString*> *dict =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBInt32ObjectDictionary<NSString*> *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBInt32ObjectDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt32ObjectDictionary<NSString*> *dict =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBInt32ObjectDictionary<NSString*> *dict2 =
      [GPBInt32ObjectDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBInt32ObjectDictionary<NSString*> *dict = [GPBInt32ObjectDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setObject:@"abc" forKey:11];
  XCTAssertEqual(dict.count, 1U);

  const int32_t kKeys[] = { 12, 13, 14 };
  const NSString* kObjects[] = { @"def", @"ghi", @"jkl" };
  GPBInt32ObjectDictionary<NSString*> *dict2 =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                forKeys:kKeys
                                                  count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  XCTAssertEqualObjects([dict objectForKey:11], @"abc");
  XCTAssertEqualObjects([dict objectForKey:12], @"def");
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:14], @"jkl");
  [dict2 release];
}

- (void)testRemove {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt32ObjectDictionary<NSString*> *dict =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects
                                         forKeys:kKeys
                                           count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeObjectForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:11], @"abc");
  XCTAssertNil([dict objectForKey:12]);
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:14], @"jkl");

  // Remove again does nothing.
  [dict removeObjectForKey:12];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:11], @"abc");
  XCTAssertNil([dict objectForKey:12]);
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:14], @"jkl");

  [dict removeObjectForKey:14];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:11], @"abc");
  XCTAssertNil([dict objectForKey:12]);
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertNil([dict objectForKey:14]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:11]);
  XCTAssertNil([dict objectForKey:12]);
  XCTAssertNil([dict objectForKey:13]);
  XCTAssertNil([dict objectForKey:14]);
  [dict release];
}

- (void)testInplaceMutation {
  const int32_t kKeys[] = { 11, 12, 13, 14 };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBInt32ObjectDictionary<NSString*> *dict =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects
                                         forKeys:kKeys
                                           count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:11], @"abc");
  XCTAssertEqualObjects([dict objectForKey:12], @"def");
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:14], @"jkl");

  [dict setObject:@"jkl" forKey:11];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:11], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:12], @"def");
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:14], @"jkl");

  [dict setObject:@"def" forKey:14];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:11], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:12], @"def");
  XCTAssertEqualObjects([dict objectForKey:13], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:14], @"def");

  const int32_t kKeys2[] = { 12, 13 };
  const NSString* kObjects2[] = { @"ghi", @"abc" };
  GPBInt32ObjectDictionary<NSString*> *dict2 =
      [[GPBInt32ObjectDictionary alloc] initWithObjects:kObjects2
                                                forKeys:kKeys2
                                                  count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:11], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:12], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:13], @"abc");
  XCTAssertEqualObjects([dict objectForKey:14], @"def");

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND-END TEST_FOR_POD_KEY(Int32, int32_t, 11, 12, 13, 14)

