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

//%PDDM-EXPAND TEST_FOR_POD_KEY(UInt64, uint64_t, 31ULL, 32ULL, 33ULL, 34ULL)
// This block of code is generated, do not edit it directly.

// To let the testing macros work, add some extra methods to simplify things.
@interface GPBUInt64EnumDictionary (TestingTweak)
+ (instancetype)dictionaryWithEnum:(int32_t)value forKey:(uint64_t)key;
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const uint64_t [])keys
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

@implementation GPBUInt64EnumDictionary (TestingTweak)
+ (instancetype)dictionaryWithEnum:(int32_t)value forKey:(uint64_t)key {
  // Cast is needed to compiler knows what class we are invoking initWithValues: on to get the
  // type correct.
  return [[(GPBUInt64EnumDictionary*)[self alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                                   rawValues:&value
                                                                     forKeys:&key
                                                                       count:1] autorelease];
}
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const uint64_t [])keys
                        count:(NSUInteger)count {
  return [self initWithValidationFunction:TestingEnum_IsValidValue
                                rawValues:values
                                  forKeys:keys
                                    count:count];
}
@end


#pragma mark - UInt64 -> UInt32

@interface GPBUInt64UInt32DictionaryTests : XCTestCase
@end

@implementation GPBUInt64UInt32DictionaryTests

- (void)testEmpty {
  GPBUInt64UInt32Dictionary *dict = [[GPBUInt64UInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:31ULL]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint64_t aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64UInt32Dictionary *dict = [GPBUInt64UInt32Dictionary dictionaryWithUInt32:100U forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:32ULL]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint64_t aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, 100U);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const uint32_t kValues[] = { 100U, 101U, 102U };
  GPBUInt64UInt32Dictionary *dict =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  uint32_t *seenValues = malloc(3 * sizeof(uint32_t));
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint64_t aKey, uint32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint64_t aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const uint32_t kValues1[] = { 100U, 101U, 102U };
  const uint32_t kValues2[] = { 100U, 103U, 102U };
  const uint32_t kValues3[] = { 100U, 101U, 102U, 103U };
  GPBUInt64UInt32Dictionary *dict1 =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64UInt32Dictionary *dict1prime =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64UInt32Dictionary *dict2 =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64UInt32Dictionary *dict3 =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64UInt32Dictionary *dict4 =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt64UInt32Dictionary *dict =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64UInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64UInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt64UInt32Dictionary *dict =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64UInt32Dictionary *dict2 =
      [GPBUInt64UInt32Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64UInt32Dictionary *dict = [GPBUInt64UInt32Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt32:100U forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const uint32_t kValues[] = { 101U, 102U, 103U };
  GPBUInt64UInt32Dictionary *dict2 =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 103U);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt64UInt32Dictionary *dict =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt32ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 103U);

  // Remove again does nothing.
  [dict removeUInt32ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 103U);

  [dict removeUInt32ForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertFalse([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertFalse([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertFalse([dict getUInt32:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt64UInt32Dictionary *dict =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:103U forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:101U forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 101U);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const uint32_t kValues2[] = { 102U, 100U };
  GPBUInt64UInt32Dictionary *dict2 =
      [[GPBUInt64UInt32Dictionary alloc] initWithUInt32s:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 101U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Int32

@interface GPBUInt64Int32DictionaryTests : XCTestCase
@end

@implementation GPBUInt64Int32DictionaryTests

- (void)testEmpty {
  GPBUInt64Int32Dictionary *dict = [[GPBUInt64Int32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:31ULL]);
  [dict enumerateKeysAndInt32sUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64Int32Dictionary *dict = [GPBUInt64Int32Dictionary dictionaryWithInt32:200 forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:32ULL]);
  [dict enumerateKeysAndInt32sUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, 200);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const int32_t kValues[] = { 200, 201, 202 };
  GPBUInt64Int32Dictionary *dict =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndInt32sUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndInt32sUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const int32_t kValues1[] = { 200, 201, 202 };
  const int32_t kValues2[] = { 200, 203, 202 };
  const int32_t kValues3[] = { 200, 201, 202, 203 };
  GPBUInt64Int32Dictionary *dict1 =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64Int32Dictionary *dict1prime =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64Int32Dictionary *dict2 =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64Int32Dictionary *dict3 =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64Int32Dictionary *dict4 =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt64Int32Dictionary *dict =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64Int32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64Int32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt64Int32Dictionary *dict =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64Int32Dictionary *dict2 =
      [GPBUInt64Int32Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64Int32Dictionary *dict = [GPBUInt64Int32Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt32:200 forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 201, 202, 203 };
  GPBUInt64Int32Dictionary *dict2 =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 203);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt64Int32Dictionary *dict =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt32ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 203);

  // Remove again does nothing.
  [dict removeInt32ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 203);

  [dict removeInt32ForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:31ULL]);
  XCTAssertFalse([dict getInt32:NULL forKey:32ULL]);
  XCTAssertFalse([dict getInt32:NULL forKey:33ULL]);
  XCTAssertFalse([dict getInt32:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt64Int32Dictionary *dict =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 203);

  [dict setInt32:203 forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 203);

  [dict setInt32:201 forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 201);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const int32_t kValues2[] = { 202, 200 };
  GPBUInt64Int32Dictionary *dict2 =
      [[GPBUInt64Int32Dictionary alloc] initWithInt32s:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:31ULL]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:32ULL]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:33ULL]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt32:&value forKey:34ULL]);
  XCTAssertEqual(value, 201);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> UInt64

@interface GPBUInt64UInt64DictionaryTests : XCTestCase
@end

@implementation GPBUInt64UInt64DictionaryTests

- (void)testEmpty {
  GPBUInt64UInt64Dictionary *dict = [[GPBUInt64UInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:31ULL]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint64_t aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64UInt64Dictionary *dict = [GPBUInt64UInt64Dictionary dictionaryWithUInt64:300U forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:32ULL]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint64_t aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, 300U);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const uint64_t kValues[] = { 300U, 301U, 302U };
  GPBUInt64UInt64Dictionary *dict =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  uint64_t *seenValues = malloc(3 * sizeof(uint64_t));
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint64_t aKey, uint64_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint64_t aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const uint64_t kValues1[] = { 300U, 301U, 302U };
  const uint64_t kValues2[] = { 300U, 303U, 302U };
  const uint64_t kValues3[] = { 300U, 301U, 302U, 303U };
  GPBUInt64UInt64Dictionary *dict1 =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64UInt64Dictionary *dict1prime =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64UInt64Dictionary *dict2 =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64UInt64Dictionary *dict3 =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64UInt64Dictionary *dict4 =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt64UInt64Dictionary *dict =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64UInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64UInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt64UInt64Dictionary *dict =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64UInt64Dictionary *dict2 =
      [GPBUInt64UInt64Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64UInt64Dictionary *dict = [GPBUInt64UInt64Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt64:300U forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const uint64_t kValues[] = { 301U, 302U, 303U };
  GPBUInt64UInt64Dictionary *dict2 =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 303U);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt64UInt64Dictionary *dict =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt64ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 303U);

  // Remove again does nothing.
  [dict removeUInt64ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 303U);

  [dict removeUInt64ForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertFalse([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertFalse([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertFalse([dict getUInt64:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt64UInt64Dictionary *dict =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:303U forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:301U forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 301U);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const uint64_t kValues2[] = { 302U, 300U };
  GPBUInt64UInt64Dictionary *dict2 =
      [[GPBUInt64UInt64Dictionary alloc] initWithUInt64s:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getUInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 301U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Int64

@interface GPBUInt64Int64DictionaryTests : XCTestCase
@end

@implementation GPBUInt64Int64DictionaryTests

- (void)testEmpty {
  GPBUInt64Int64Dictionary *dict = [[GPBUInt64Int64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:31ULL]);
  [dict enumerateKeysAndInt64sUsingBlock:^(uint64_t aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64Int64Dictionary *dict = [GPBUInt64Int64Dictionary dictionaryWithInt64:400 forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:32ULL]);
  [dict enumerateKeysAndInt64sUsingBlock:^(uint64_t aKey, int64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, 400);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const int64_t kValues[] = { 400, 401, 402 };
  GPBUInt64Int64Dictionary *dict =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  int64_t *seenValues = malloc(3 * sizeof(int64_t));
  [dict enumerateKeysAndInt64sUsingBlock:^(uint64_t aKey, int64_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndInt64sUsingBlock:^(uint64_t aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const int64_t kValues1[] = { 400, 401, 402 };
  const int64_t kValues2[] = { 400, 403, 402 };
  const int64_t kValues3[] = { 400, 401, 402, 403 };
  GPBUInt64Int64Dictionary *dict1 =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64Int64Dictionary *dict1prime =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64Int64Dictionary *dict2 =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64Int64Dictionary *dict3 =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64Int64Dictionary *dict4 =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt64Int64Dictionary *dict =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64Int64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64Int64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt64Int64Dictionary *dict =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64Int64Dictionary *dict2 =
      [GPBUInt64Int64Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64Int64Dictionary *dict = [GPBUInt64Int64Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt64:400 forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const int64_t kValues[] = { 401, 402, 403 };
  GPBUInt64Int64Dictionary *dict2 =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 403);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt64Int64Dictionary *dict =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt64ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 403);

  // Remove again does nothing.
  [dict removeInt64ForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 403);

  [dict removeInt64ForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:31ULL]);
  XCTAssertFalse([dict getInt64:NULL forKey:32ULL]);
  XCTAssertFalse([dict getInt64:NULL forKey:33ULL]);
  XCTAssertFalse([dict getInt64:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt64Int64Dictionary *dict =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 403);

  [dict setInt64:403 forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 403);

  [dict setInt64:401 forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 401);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const int64_t kValues2[] = { 402, 400 };
  GPBUInt64Int64Dictionary *dict2 =
      [[GPBUInt64Int64Dictionary alloc] initWithInt64s:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:31ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:31ULL]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:32ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:32ULL]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:33ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:33ULL]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:34ULL]);
  XCTAssertTrue([dict getInt64:&value forKey:34ULL]);
  XCTAssertEqual(value, 401);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Bool

@interface GPBUInt64BoolDictionaryTests : XCTestCase
@end

@implementation GPBUInt64BoolDictionaryTests

- (void)testEmpty {
  GPBUInt64BoolDictionary *dict = [[GPBUInt64BoolDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:31ULL]);
  [dict enumerateKeysAndBoolsUsingBlock:^(uint64_t aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64BoolDictionary *dict = [GPBUInt64BoolDictionary dictionaryWithBool:YES forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:32ULL]);
  [dict enumerateKeysAndBoolsUsingBlock:^(uint64_t aKey, BOOL aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, YES);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const BOOL kValues[] = { YES, YES, NO };
  GPBUInt64BoolDictionary *dict =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:&value forKey:32ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  BOOL *seenValues = malloc(3 * sizeof(BOOL));
  [dict enumerateKeysAndBoolsUsingBlock:^(uint64_t aKey, BOOL aValue, BOOL *stop) {
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
  [dict enumerateKeysAndBoolsUsingBlock:^(uint64_t aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const BOOL kValues1[] = { YES, YES, NO };
  const BOOL kValues2[] = { YES, NO, NO };
  const BOOL kValues3[] = { YES, YES, NO, NO };
  GPBUInt64BoolDictionary *dict1 =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64BoolDictionary *dict1prime =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64BoolDictionary *dict2 =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64BoolDictionary *dict3 =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64BoolDictionary *dict4 =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt64BoolDictionary *dict =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64BoolDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64BoolDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt64BoolDictionary *dict =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64BoolDictionary *dict2 =
      [GPBUInt64BoolDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64BoolDictionary *dict = [GPBUInt64BoolDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setBool:YES forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const BOOL kValues[] = { YES, NO, NO };
  GPBUInt64BoolDictionary *dict2 =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:&value forKey:32ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:34ULL]);
  XCTAssertTrue([dict getBool:&value forKey:34ULL]);
  XCTAssertEqual(value, NO);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt64BoolDictionary *dict =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeBoolForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:34ULL]);
  XCTAssertTrue([dict getBool:&value forKey:34ULL]);
  XCTAssertEqual(value, NO);

  // Remove again does nothing.
  [dict removeBoolForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:34ULL]);
  XCTAssertTrue([dict getBool:&value forKey:34ULL]);
  XCTAssertEqual(value, NO);

  [dict removeBoolForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:31ULL]);
  XCTAssertFalse([dict getBool:NULL forKey:32ULL]);
  XCTAssertFalse([dict getBool:NULL forKey:33ULL]);
  XCTAssertFalse([dict getBool:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt64BoolDictionary *dict =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:&value forKey:32ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:34ULL]);
  XCTAssertTrue([dict getBool:&value forKey:34ULL]);
  XCTAssertEqual(value, NO);

  [dict setBool:NO forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:&value forKey:32ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:34ULL]);
  XCTAssertTrue([dict getBool:&value forKey:34ULL]);
  XCTAssertEqual(value, NO);

  [dict setBool:YES forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:&value forKey:32ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:34ULL]);
  XCTAssertTrue([dict getBool:&value forKey:34ULL]);
  XCTAssertEqual(value, YES);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const BOOL kValues2[] = { NO, YES };
  GPBUInt64BoolDictionary *dict2 =
      [[GPBUInt64BoolDictionary alloc] initWithBools:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:31ULL]);
  XCTAssertTrue([dict getBool:&value forKey:31ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:32ULL]);
  XCTAssertTrue([dict getBool:&value forKey:32ULL]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:33ULL]);
  XCTAssertTrue([dict getBool:&value forKey:33ULL]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:34ULL]);
  XCTAssertTrue([dict getBool:&value forKey:34ULL]);
  XCTAssertEqual(value, YES);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Float

@interface GPBUInt64FloatDictionaryTests : XCTestCase
@end

@implementation GPBUInt64FloatDictionaryTests

- (void)testEmpty {
  GPBUInt64FloatDictionary *dict = [[GPBUInt64FloatDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:31ULL]);
  [dict enumerateKeysAndFloatsUsingBlock:^(uint64_t aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64FloatDictionary *dict = [GPBUInt64FloatDictionary dictionaryWithFloat:500.f forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:32ULL]);
  [dict enumerateKeysAndFloatsUsingBlock:^(uint64_t aKey, float aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, 500.f);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const float kValues[] = { 500.f, 501.f, 502.f };
  GPBUInt64FloatDictionary *dict =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:32ULL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  float *seenValues = malloc(3 * sizeof(float));
  [dict enumerateKeysAndFloatsUsingBlock:^(uint64_t aKey, float aValue, BOOL *stop) {
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
  [dict enumerateKeysAndFloatsUsingBlock:^(uint64_t aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const float kValues1[] = { 500.f, 501.f, 502.f };
  const float kValues2[] = { 500.f, 503.f, 502.f };
  const float kValues3[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt64FloatDictionary *dict1 =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64FloatDictionary *dict1prime =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64FloatDictionary *dict2 =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64FloatDictionary *dict3 =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64FloatDictionary *dict4 =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt64FloatDictionary *dict =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64FloatDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64FloatDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt64FloatDictionary *dict =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64FloatDictionary *dict2 =
      [GPBUInt64FloatDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64FloatDictionary *dict = [GPBUInt64FloatDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setFloat:500.f forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const float kValues[] = { 501.f, 502.f, 503.f };
  GPBUInt64FloatDictionary *dict2 =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:32ULL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:34ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:34ULL]);
  XCTAssertEqual(value, 503.f);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt64FloatDictionary *dict =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeFloatForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:34ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:34ULL]);
  XCTAssertEqual(value, 503.f);

  // Remove again does nothing.
  [dict removeFloatForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:34ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:34ULL]);
  XCTAssertEqual(value, 503.f);

  [dict removeFloatForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:31ULL]);
  XCTAssertFalse([dict getFloat:NULL forKey:32ULL]);
  XCTAssertFalse([dict getFloat:NULL forKey:33ULL]);
  XCTAssertFalse([dict getFloat:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt64FloatDictionary *dict =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:32ULL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:34ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:34ULL]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:503.f forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:32ULL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:34ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:34ULL]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:501.f forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:32ULL]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:34ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:34ULL]);
  XCTAssertEqual(value, 501.f);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const float kValues2[] = { 502.f, 500.f };
  GPBUInt64FloatDictionary *dict2 =
      [[GPBUInt64FloatDictionary alloc] initWithFloats:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:31ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:31ULL]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:32ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:32ULL]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:33ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:33ULL]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:34ULL]);
  XCTAssertTrue([dict getFloat:&value forKey:34ULL]);
  XCTAssertEqual(value, 501.f);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Double

@interface GPBUInt64DoubleDictionaryTests : XCTestCase
@end

@implementation GPBUInt64DoubleDictionaryTests

- (void)testEmpty {
  GPBUInt64DoubleDictionary *dict = [[GPBUInt64DoubleDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:31ULL]);
  [dict enumerateKeysAndDoublesUsingBlock:^(uint64_t aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64DoubleDictionary *dict = [GPBUInt64DoubleDictionary dictionaryWithDouble:600. forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:32ULL]);
  [dict enumerateKeysAndDoublesUsingBlock:^(uint64_t aKey, double aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, 600.);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const double kValues[] = { 600., 601., 602. };
  GPBUInt64DoubleDictionary *dict =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:32ULL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  double *seenValues = malloc(3 * sizeof(double));
  [dict enumerateKeysAndDoublesUsingBlock:^(uint64_t aKey, double aValue, BOOL *stop) {
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
  [dict enumerateKeysAndDoublesUsingBlock:^(uint64_t aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const double kValues1[] = { 600., 601., 602. };
  const double kValues2[] = { 600., 603., 602. };
  const double kValues3[] = { 600., 601., 602., 603. };
  GPBUInt64DoubleDictionary *dict1 =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64DoubleDictionary *dict1prime =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64DoubleDictionary *dict2 =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64DoubleDictionary *dict3 =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64DoubleDictionary *dict4 =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt64DoubleDictionary *dict =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64DoubleDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64DoubleDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt64DoubleDictionary *dict =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64DoubleDictionary *dict2 =
      [GPBUInt64DoubleDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64DoubleDictionary *dict = [GPBUInt64DoubleDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setDouble:600. forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const double kValues[] = { 601., 602., 603. };
  GPBUInt64DoubleDictionary *dict2 =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:32ULL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:34ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:34ULL]);
  XCTAssertEqual(value, 603.);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt64DoubleDictionary *dict =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeDoubleForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:34ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:34ULL]);
  XCTAssertEqual(value, 603.);

  // Remove again does nothing.
  [dict removeDoubleForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:34ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:34ULL]);
  XCTAssertEqual(value, 603.);

  [dict removeDoubleForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:31ULL]);
  XCTAssertFalse([dict getDouble:NULL forKey:32ULL]);
  XCTAssertFalse([dict getDouble:NULL forKey:33ULL]);
  XCTAssertFalse([dict getDouble:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt64DoubleDictionary *dict =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:32ULL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:34ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:34ULL]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:603. forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:32ULL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:34ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:34ULL]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:601. forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:32ULL]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:34ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:34ULL]);
  XCTAssertEqual(value, 601.);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const double kValues2[] = { 602., 600. };
  GPBUInt64DoubleDictionary *dict2 =
      [[GPBUInt64DoubleDictionary alloc] initWithDoubles:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:31ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:31ULL]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:32ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:32ULL]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:33ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:33ULL]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:34ULL]);
  XCTAssertTrue([dict getDouble:&value forKey:34ULL]);
  XCTAssertEqual(value, 601.);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Enum

@interface GPBUInt64EnumDictionaryTests : XCTestCase
@end

@implementation GPBUInt64EnumDictionaryTests

- (void)testEmpty {
  GPBUInt64EnumDictionary *dict = [[GPBUInt64EnumDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:31ULL]);
  [dict enumerateKeysAndEnumsUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64EnumDictionary *dict = [GPBUInt64EnumDictionary dictionaryWithEnum:700 forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  [dict enumerateKeysAndEnumsUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqual(aValue, 700);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const int32_t kValues[] = { 700, 701, 702 };
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndEnumsUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const int32_t kValues1[] = { 700, 701, 702 };
  const int32_t kValues2[] = { 700, 703, 702 };
  const int32_t kValues3[] = { 700, 701, 702, 703 };
  GPBUInt64EnumDictionary *dict1 =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64EnumDictionary *dict1prime =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64EnumDictionary *dict2 =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64EnumDictionary *dict3 =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64EnumDictionary *dict4 =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64EnumDictionary *dict2 =
      [GPBUInt64EnumDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64EnumDictionary *dict = [GPBUInt64EnumDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setEnum:700 forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 701, 702, 703 };
  GPBUInt64EnumDictionary *dict2 =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 703);
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 703);

  // Remove again does nothing.
  [dict removeEnumForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 703);

  [dict removeEnumForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:31ULL]);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertFalse([dict getEnum:NULL forKey:33ULL]);
  XCTAssertFalse([dict getEnum:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 703);

  [dict setEnum:703 forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 703);

  [dict setEnum:701 forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 701);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const int32_t kValues2[] = { 702, 700 };
  GPBUInt64EnumDictionary *dict2 =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 701);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Enum (Unknown Enums)

@interface GPBUInt64EnumDictionaryUnknownEnumTests : XCTestCase
@end

@implementation GPBUInt64EnumDictionaryUnknownEnumTests

- (void)testRawBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const int32_t kValues[] = { 700, 801, 702 };
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue(dict.validationFunc == TestingEnum_IsValidValue);  // Pointer comparison
  int32_t value;
  XCTAssertTrue([dict getRawValue:NULL forKey:31ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:32ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:32ULL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getRawValue:NULL forKey:33ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getRawValue:NULL forKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(uint64_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEqualityWithUnknowns {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const int32_t kValues1[] = { 700, 801, 702 };  // Unknown
  const int32_t kValues2[] = { 700, 803, 702 };  // Unknown
  const int32_t kValues3[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt64EnumDictionary *dict1 =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt64EnumDictionary *dict1prime =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64EnumDictionary *dict2 =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues2
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt64EnumDictionary *dict3 =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys2
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt64EnumDictionary *dict4 =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknown
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertEqualObjects(dict, dict2);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64EnumDictionary *dict2 =
      [GPBUInt64EnumDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  [dict release];
}

- (void)testUnknownAdds {
  GPBUInt64EnumDictionary *dict =
    [GPBUInt64EnumDictionary dictionaryWithValidationFunction:TestingEnum_IsValidValue];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  XCTAssertThrowsSpecificNamed([dict setEnum:801 forKey:32ULL],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 0U);
  [dict setRawValue:801 forKey:32ULL];  // Unknown
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 31ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 702, 803 };  // Unknown
  GPBUInt64EnumDictionary *dict2 =
      [[GPBUInt64EnumDictionary alloc] initWithEnums:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:32ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:32ULL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:34ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:34ULL]);
  XCTAssertEqual(value, 803);
  [dict2 release];
}

- (void)testUnknownRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:34ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:34ULL]);
  XCTAssertEqual(value, 803);

  // Remove again does nothing.
  [dict removeEnumForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:34ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:34ULL]);
  XCTAssertEqual(value, 803);

  [dict removeEnumForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:31ULL]);
  XCTAssertFalse([dict getEnum:NULL forKey:32ULL]);
  XCTAssertFalse([dict getEnum:NULL forKey:33ULL]);
  XCTAssertFalse([dict getEnum:NULL forKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutationUnknowns {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:32ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:32ULL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:34ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:34ULL]);
  XCTAssertEqual(value, 803);

  XCTAssertThrowsSpecificNamed([dict setEnum:803 forKey:31ULL],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:31ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:31ULL]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:32ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:32ULL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:34ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:34ULL]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:803 forKey:31ULL];  // Unknown
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:31ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:31ULL]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:32ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:32ULL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:34ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:34ULL]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:700 forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:31ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:31ULL]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:32ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:32ULL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:33ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:33ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 700);

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const int32_t kValues2[] = { 702, 801 };  // Unknown
  GPBUInt64EnumDictionary *dict2 =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues2
                                                          forKeys:kKeys2
                                                            count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:31ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:31ULL]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getEnum:NULL forKey:32ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:32ULL]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:33ULL]);
  XCTAssertTrue([dict getRawValue:&value forKey:33ULL]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:34ULL]);
  XCTAssertTrue([dict getEnum:&value forKey:34ULL]);
  XCTAssertEqual(value, 700);

  [dict2 release];
  [dict release];
}

- (void)testCopyUnknowns {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const int32_t kValues[] = { 700, 801, 702, 803 };
  GPBUInt64EnumDictionary *dict =
      [[GPBUInt64EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt64EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt64 -> Object

@interface GPBUInt64ObjectDictionaryTests : XCTestCase
@end

@implementation GPBUInt64ObjectDictionaryTests

- (void)testEmpty {
  GPBUInt64ObjectDictionary<NSString*> *dict = [[GPBUInt64ObjectDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:31ULL]);
  [dict enumerateKeysAndObjectsUsingBlock:^(uint64_t aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt64ObjectDictionary<NSString*> *dict = [GPBUInt64ObjectDictionary dictionaryWithObject:@"abc" forKey:31ULL];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"abc");
  XCTAssertNil([dict objectForKey:32ULL]);
  [dict enumerateKeysAndObjectsUsingBlock:^(uint64_t aKey, NSString* aObject, BOOL *stop) {
    XCTAssertEqual(aKey, 31ULL);
    XCTAssertEqualObjects(aObject, @"abc");
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi" };
  GPBUInt64ObjectDictionary<NSString*> *dict =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:32ULL], @"def");
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertNil([dict objectForKey:34ULL]);

  __block NSUInteger idx = 0;
  uint64_t *seenKeys = malloc(3 * sizeof(uint64_t));
  NSString* *seenObjects = malloc(3 * sizeof(NSString*));
  [dict enumerateKeysAndObjectsUsingBlock:^(uint64_t aKey, NSString* aObject, BOOL *stop) {
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
  [dict enumerateKeysAndObjectsUsingBlock:^(uint64_t aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint64_t kKeys1[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const uint64_t kKeys2[] = { 32ULL, 31ULL, 34ULL };
  const NSString* kObjects1[] = { @"abc", @"def", @"ghi" };
  const NSString* kObjects2[] = { @"abc", @"jkl", @"ghi" };
  const NSString* kObjects3[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt64ObjectDictionary<NSString*> *dict1 =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1);
  GPBUInt64ObjectDictionary<NSString*> *dict1prime =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt64ObjectDictionary<NSString*> *dict2 =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  GPBUInt64ObjectDictionary<NSString*> *dict3 =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict3);
  GPBUInt64ObjectDictionary<NSString*> *dict4 =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects3
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
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt64ObjectDictionary<NSString*> *dict =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBUInt64ObjectDictionary<NSString*> *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt64ObjectDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt64ObjectDictionary<NSString*> *dict =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBUInt64ObjectDictionary<NSString*> *dict2 =
      [GPBUInt64ObjectDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBUInt64ObjectDictionary<NSString*> *dict = [GPBUInt64ObjectDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setObject:@"abc" forKey:31ULL];
  XCTAssertEqual(dict.count, 1U);

  const uint64_t kKeys[] = { 32ULL, 33ULL, 34ULL };
  const NSString* kObjects[] = { @"def", @"ghi", @"jkl" };
  GPBUInt64ObjectDictionary<NSString*> *dict2 =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  XCTAssertEqualObjects([dict objectForKey:31ULL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:32ULL], @"def");
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:34ULL], @"jkl");
  [dict2 release];
}

- (void)testRemove {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt64ObjectDictionary<NSString*> *dict =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeObjectForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"abc");
  XCTAssertNil([dict objectForKey:32ULL]);
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:34ULL], @"jkl");

  // Remove again does nothing.
  [dict removeObjectForKey:32ULL];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"abc");
  XCTAssertNil([dict objectForKey:32ULL]);
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:34ULL], @"jkl");

  [dict removeObjectForKey:34ULL];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"abc");
  XCTAssertNil([dict objectForKey:32ULL]);
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertNil([dict objectForKey:34ULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:31ULL]);
  XCTAssertNil([dict objectForKey:32ULL]);
  XCTAssertNil([dict objectForKey:33ULL]);
  XCTAssertNil([dict objectForKey:34ULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint64_t kKeys[] = { 31ULL, 32ULL, 33ULL, 34ULL };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt64ObjectDictionary<NSString*> *dict =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:32ULL], @"def");
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:34ULL], @"jkl");

  [dict setObject:@"jkl" forKey:31ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:32ULL], @"def");
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:34ULL], @"jkl");

  [dict setObject:@"def" forKey:34ULL];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:32ULL], @"def");
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:34ULL], @"def");

  const uint64_t kKeys2[] = { 32ULL, 33ULL };
  const NSString* kObjects2[] = { @"ghi", @"abc" };
  GPBUInt64ObjectDictionary<NSString*> *dict2 =
      [[GPBUInt64ObjectDictionary alloc] initWithObjects:kObjects2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:31ULL], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:32ULL], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:33ULL], @"abc");
  XCTAssertEqualObjects([dict objectForKey:34ULL], @"def");

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND-END TEST_FOR_POD_KEY(UInt64, uint64_t, 31ULL, 32ULL, 33ULL, 34ULL)
