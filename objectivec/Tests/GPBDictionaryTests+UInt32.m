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

//%PDDM-EXPAND TEST_FOR_POD_KEY(UInt32, uint32_t, 1U, 2U, 3U, 4U)
// This block of code is generated, do not edit it directly.

// To let the testing macros work, add some extra methods to simplify things.
@interface GPBUInt32EnumDictionary (TestingTweak)
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const uint32_t [])keys
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

@implementation GPBUInt32EnumDictionary (TestingTweak)
- (instancetype)initWithEnums:(const int32_t [])values
                      forKeys:(const uint32_t [])keys
                        count:(NSUInteger)count {
  return [self initWithValidationFunction:TestingEnum_IsValidValue
                                rawValues:values
                                  forKeys:keys
                                    count:count];
}
@end


#pragma mark - UInt32 -> UInt32

@interface GPBUInt32UInt32DictionaryTests : XCTestCase
@end

@implementation GPBUInt32UInt32DictionaryTests

- (void)testEmpty {
  GPBUInt32UInt32Dictionary *dict = [[GPBUInt32UInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:1U]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint32_t aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32UInt32Dictionary *dict = [[GPBUInt32UInt32Dictionary alloc] init];
  [dict setUInt32:100U forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:2U]);
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint32_t aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, 100U);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const uint32_t kValues[] = { 100U, 101U, 102U };
  GPBUInt32UInt32Dictionary *dict =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:&value forKey:2U]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  uint32_t *seenValues = malloc(3 * sizeof(uint32_t));
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint32_t aKey, uint32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndUInt32sUsingBlock:^(uint32_t aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const uint32_t kValues1[] = { 100U, 101U, 102U };
  const uint32_t kValues2[] = { 100U, 103U, 102U };
  const uint32_t kValues3[] = { 100U, 101U, 102U, 103U };
  GPBUInt32UInt32Dictionary *dict1 =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32UInt32Dictionary *dict1prime =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32UInt32Dictionary *dict2 =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32UInt32Dictionary *dict3 =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32UInt32Dictionary *dict4 =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt32UInt32Dictionary *dict =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32UInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32UInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt32UInt32Dictionary *dict =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32UInt32Dictionary *dict2 =
      [[GPBUInt32UInt32Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32UInt32Dictionary *dict = [[GPBUInt32UInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt32:100U forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const uint32_t kValues[] = { 101U, 102U, 103U };
  GPBUInt32UInt32Dictionary *dict2 =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:&value forKey:2U]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt32:&value forKey:4U]);
  XCTAssertEqual(value, 103U);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt32UInt32Dictionary *dict =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt32ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt32:&value forKey:4U]);
  XCTAssertEqual(value, 103U);

  // Remove again does nothing.
  [dict removeUInt32ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt32:&value forKey:4U]);
  XCTAssertEqual(value, 103U);

  [dict removeUInt32ForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertFalse([dict getUInt32:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt32:NULL forKey:1U]);
  XCTAssertFalse([dict getUInt32:NULL forKey:2U]);
  XCTAssertFalse([dict getUInt32:NULL forKey:3U]);
  XCTAssertFalse([dict getUInt32:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint32_t kValues[] = { 100U, 101U, 102U, 103U };
  GPBUInt32UInt32Dictionary *dict =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint32_t value;
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:&value forKey:2U]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt32:&value forKey:4U]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:103U forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:&value forKey:2U]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt32:&value forKey:4U]);
  XCTAssertEqual(value, 103U);

  [dict setUInt32:101U forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:&value forKey:2U]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt32:&value forKey:4U]);
  XCTAssertEqual(value, 101U);

  const uint32_t kKeys2[] = { 2U, 3U };
  const uint32_t kValues2[] = { 102U, 100U };
  GPBUInt32UInt32Dictionary *dict2 =
      [[GPBUInt32UInt32Dictionary alloc] initWithUInt32s:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt32:&value forKey:1U]);
  XCTAssertEqual(value, 103U);
  XCTAssertTrue([dict getUInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt32:&value forKey:2U]);
  XCTAssertEqual(value, 102U);
  XCTAssertTrue([dict getUInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt32:&value forKey:3U]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict getUInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt32:&value forKey:4U]);
  XCTAssertEqual(value, 101U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Int32

@interface GPBUInt32Int32DictionaryTests : XCTestCase
@end

@implementation GPBUInt32Int32DictionaryTests

- (void)testEmpty {
  GPBUInt32Int32Dictionary *dict = [[GPBUInt32Int32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:1U]);
  [dict enumerateKeysAndInt32sUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32Int32Dictionary *dict = [[GPBUInt32Int32Dictionary alloc] init];
  [dict setInt32:200 forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:2U]);
  [dict enumerateKeysAndInt32sUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, 200);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const int32_t kValues[] = { 200, 201, 202 };
  GPBUInt32Int32Dictionary *dict =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:&value forKey:2U]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndInt32sUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndInt32sUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const int32_t kValues1[] = { 200, 201, 202 };
  const int32_t kValues2[] = { 200, 203, 202 };
  const int32_t kValues3[] = { 200, 201, 202, 203 };
  GPBUInt32Int32Dictionary *dict1 =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32Int32Dictionary *dict1prime =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32Int32Dictionary *dict2 =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32Int32Dictionary *dict3 =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32Int32Dictionary *dict4 =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt32Int32Dictionary *dict =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32Int32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32Int32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt32Int32Dictionary *dict =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32Int32Dictionary *dict2 =
      [[GPBUInt32Int32Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32Int32Dictionary *dict = [[GPBUInt32Int32Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt32:200 forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const int32_t kValues[] = { 201, 202, 203 };
  GPBUInt32Int32Dictionary *dict2 =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:&value forKey:2U]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getInt32:&value forKey:4U]);
  XCTAssertEqual(value, 203);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt32Int32Dictionary *dict =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt32ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getInt32:&value forKey:4U]);
  XCTAssertEqual(value, 203);

  // Remove again does nothing.
  [dict removeInt32ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getInt32:&value forKey:4U]);
  XCTAssertEqual(value, 203);

  [dict removeInt32ForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertFalse([dict getInt32:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt32:NULL forKey:1U]);
  XCTAssertFalse([dict getInt32:NULL forKey:2U]);
  XCTAssertFalse([dict getInt32:NULL forKey:3U]);
  XCTAssertFalse([dict getInt32:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 200, 201, 202, 203 };
  GPBUInt32Int32Dictionary *dict =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:&value forKey:2U]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getInt32:&value forKey:4U]);
  XCTAssertEqual(value, 203);

  [dict setInt32:203 forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:&value forKey:2U]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getInt32:&value forKey:4U]);
  XCTAssertEqual(value, 203);

  [dict setInt32:201 forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:&value forKey:2U]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getInt32:&value forKey:4U]);
  XCTAssertEqual(value, 201);

  const uint32_t kKeys2[] = { 2U, 3U };
  const int32_t kValues2[] = { 202, 200 };
  GPBUInt32Int32Dictionary *dict2 =
      [[GPBUInt32Int32Dictionary alloc] initWithInt32s:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt32:NULL forKey:1U]);
  XCTAssertTrue([dict getInt32:&value forKey:1U]);
  XCTAssertEqual(value, 203);
  XCTAssertTrue([dict getInt32:NULL forKey:2U]);
  XCTAssertTrue([dict getInt32:&value forKey:2U]);
  XCTAssertEqual(value, 202);
  XCTAssertTrue([dict getInt32:NULL forKey:3U]);
  XCTAssertTrue([dict getInt32:&value forKey:3U]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict getInt32:NULL forKey:4U]);
  XCTAssertTrue([dict getInt32:&value forKey:4U]);
  XCTAssertEqual(value, 201);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> UInt64

@interface GPBUInt32UInt64DictionaryTests : XCTestCase
@end

@implementation GPBUInt32UInt64DictionaryTests

- (void)testEmpty {
  GPBUInt32UInt64Dictionary *dict = [[GPBUInt32UInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:1U]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint32_t aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32UInt64Dictionary *dict = [[GPBUInt32UInt64Dictionary alloc] init];
  [dict setUInt64:300U forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:2U]);
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint32_t aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, 300U);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const uint64_t kValues[] = { 300U, 301U, 302U };
  GPBUInt32UInt64Dictionary *dict =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:&value forKey:2U]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  uint64_t *seenValues = malloc(3 * sizeof(uint64_t));
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint32_t aKey, uint64_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndUInt64sUsingBlock:^(uint32_t aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const uint64_t kValues1[] = { 300U, 301U, 302U };
  const uint64_t kValues2[] = { 300U, 303U, 302U };
  const uint64_t kValues3[] = { 300U, 301U, 302U, 303U };
  GPBUInt32UInt64Dictionary *dict1 =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32UInt64Dictionary *dict1prime =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32UInt64Dictionary *dict2 =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32UInt64Dictionary *dict3 =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32UInt64Dictionary *dict4 =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt32UInt64Dictionary *dict =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32UInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32UInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt32UInt64Dictionary *dict =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32UInt64Dictionary *dict2 =
      [[GPBUInt32UInt64Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32UInt64Dictionary *dict = [[GPBUInt32UInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setUInt64:300U forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const uint64_t kValues[] = { 301U, 302U, 303U };
  GPBUInt32UInt64Dictionary *dict2 =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:&value forKey:2U]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt64:&value forKey:4U]);
  XCTAssertEqual(value, 303U);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt32UInt64Dictionary *dict =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeUInt64ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt64:&value forKey:4U]);
  XCTAssertEqual(value, 303U);

  // Remove again does nothing.
  [dict removeUInt64ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt64:&value forKey:4U]);
  XCTAssertEqual(value, 303U);

  [dict removeUInt64ForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertFalse([dict getUInt64:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getUInt64:NULL forKey:1U]);
  XCTAssertFalse([dict getUInt64:NULL forKey:2U]);
  XCTAssertFalse([dict getUInt64:NULL forKey:3U]);
  XCTAssertFalse([dict getUInt64:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const uint64_t kValues[] = { 300U, 301U, 302U, 303U };
  GPBUInt32UInt64Dictionary *dict =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  uint64_t value;
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:&value forKey:2U]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt64:&value forKey:4U]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:303U forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:&value forKey:2U]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt64:&value forKey:4U]);
  XCTAssertEqual(value, 303U);

  [dict setUInt64:301U forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:&value forKey:2U]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt64:&value forKey:4U]);
  XCTAssertEqual(value, 301U);

  const uint32_t kKeys2[] = { 2U, 3U };
  const uint64_t kValues2[] = { 302U, 300U };
  GPBUInt32UInt64Dictionary *dict2 =
      [[GPBUInt32UInt64Dictionary alloc] initWithUInt64s:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getUInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getUInt64:&value forKey:1U]);
  XCTAssertEqual(value, 303U);
  XCTAssertTrue([dict getUInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getUInt64:&value forKey:2U]);
  XCTAssertEqual(value, 302U);
  XCTAssertTrue([dict getUInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getUInt64:&value forKey:3U]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict getUInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getUInt64:&value forKey:4U]);
  XCTAssertEqual(value, 301U);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Int64

@interface GPBUInt32Int64DictionaryTests : XCTestCase
@end

@implementation GPBUInt32Int64DictionaryTests

- (void)testEmpty {
  GPBUInt32Int64Dictionary *dict = [[GPBUInt32Int64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:1U]);
  [dict enumerateKeysAndInt64sUsingBlock:^(uint32_t aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32Int64Dictionary *dict = [[GPBUInt32Int64Dictionary alloc] init];
  [dict setInt64:400 forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:2U]);
  [dict enumerateKeysAndInt64sUsingBlock:^(uint32_t aKey, int64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, 400);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const int64_t kValues[] = { 400, 401, 402 };
  GPBUInt32Int64Dictionary *dict =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:&value forKey:2U]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  int64_t *seenValues = malloc(3 * sizeof(int64_t));
  [dict enumerateKeysAndInt64sUsingBlock:^(uint32_t aKey, int64_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndInt64sUsingBlock:^(uint32_t aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const int64_t kValues1[] = { 400, 401, 402 };
  const int64_t kValues2[] = { 400, 403, 402 };
  const int64_t kValues3[] = { 400, 401, 402, 403 };
  GPBUInt32Int64Dictionary *dict1 =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32Int64Dictionary *dict1prime =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32Int64Dictionary *dict2 =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32Int64Dictionary *dict3 =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32Int64Dictionary *dict4 =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt32Int64Dictionary *dict =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32Int64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32Int64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt32Int64Dictionary *dict =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32Int64Dictionary *dict2 =
      [[GPBUInt32Int64Dictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32Int64Dictionary *dict = [[GPBUInt32Int64Dictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setInt64:400 forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const int64_t kValues[] = { 401, 402, 403 };
  GPBUInt32Int64Dictionary *dict2 =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:&value forKey:2U]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getInt64:&value forKey:4U]);
  XCTAssertEqual(value, 403);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt32Int64Dictionary *dict =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeInt64ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getInt64:&value forKey:4U]);
  XCTAssertEqual(value, 403);

  // Remove again does nothing.
  [dict removeInt64ForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getInt64:&value forKey:4U]);
  XCTAssertEqual(value, 403);

  [dict removeInt64ForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertFalse([dict getInt64:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getInt64:NULL forKey:1U]);
  XCTAssertFalse([dict getInt64:NULL forKey:2U]);
  XCTAssertFalse([dict getInt64:NULL forKey:3U]);
  XCTAssertFalse([dict getInt64:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int64_t kValues[] = { 400, 401, 402, 403 };
  GPBUInt32Int64Dictionary *dict =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int64_t value;
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:&value forKey:2U]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getInt64:&value forKey:4U]);
  XCTAssertEqual(value, 403);

  [dict setInt64:403 forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:&value forKey:2U]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getInt64:&value forKey:4U]);
  XCTAssertEqual(value, 403);

  [dict setInt64:401 forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:&value forKey:2U]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getInt64:&value forKey:4U]);
  XCTAssertEqual(value, 401);

  const uint32_t kKeys2[] = { 2U, 3U };
  const int64_t kValues2[] = { 402, 400 };
  GPBUInt32Int64Dictionary *dict2 =
      [[GPBUInt32Int64Dictionary alloc] initWithInt64s:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getInt64:NULL forKey:1U]);
  XCTAssertTrue([dict getInt64:&value forKey:1U]);
  XCTAssertEqual(value, 403);
  XCTAssertTrue([dict getInt64:NULL forKey:2U]);
  XCTAssertTrue([dict getInt64:&value forKey:2U]);
  XCTAssertEqual(value, 402);
  XCTAssertTrue([dict getInt64:NULL forKey:3U]);
  XCTAssertTrue([dict getInt64:&value forKey:3U]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict getInt64:NULL forKey:4U]);
  XCTAssertTrue([dict getInt64:&value forKey:4U]);
  XCTAssertEqual(value, 401);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Bool

@interface GPBUInt32BoolDictionaryTests : XCTestCase
@end

@implementation GPBUInt32BoolDictionaryTests

- (void)testEmpty {
  GPBUInt32BoolDictionary *dict = [[GPBUInt32BoolDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:1U]);
  [dict enumerateKeysAndBoolsUsingBlock:^(uint32_t aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32BoolDictionary *dict = [[GPBUInt32BoolDictionary alloc] init];
  [dict setBool:YES forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:2U]);
  [dict enumerateKeysAndBoolsUsingBlock:^(uint32_t aKey, BOOL aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, YES);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const BOOL kValues[] = { YES, YES, NO };
  GPBUInt32BoolDictionary *dict =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:&value forKey:2U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  BOOL *seenValues = malloc(3 * sizeof(BOOL));
  [dict enumerateKeysAndBoolsUsingBlock:^(uint32_t aKey, BOOL aValue, BOOL *stop) {
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
  [dict enumerateKeysAndBoolsUsingBlock:^(uint32_t aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const BOOL kValues1[] = { YES, YES, NO };
  const BOOL kValues2[] = { YES, NO, NO };
  const BOOL kValues3[] = { YES, YES, NO, NO };
  GPBUInt32BoolDictionary *dict1 =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32BoolDictionary *dict1prime =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32BoolDictionary *dict2 =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32BoolDictionary *dict3 =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32BoolDictionary *dict4 =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt32BoolDictionary *dict =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32BoolDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32BoolDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt32BoolDictionary *dict =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32BoolDictionary *dict2 =
      [[GPBUInt32BoolDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32BoolDictionary *dict = [[GPBUInt32BoolDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setBool:YES forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const BOOL kValues[] = { YES, NO, NO };
  GPBUInt32BoolDictionary *dict2 =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:&value forKey:2U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:4U]);
  XCTAssertTrue([dict getBool:&value forKey:4U]);
  XCTAssertEqual(value, NO);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt32BoolDictionary *dict =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeBoolForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:4U]);
  XCTAssertTrue([dict getBool:&value forKey:4U]);
  XCTAssertEqual(value, NO);

  // Remove again does nothing.
  [dict removeBoolForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:4U]);
  XCTAssertTrue([dict getBool:&value forKey:4U]);
  XCTAssertEqual(value, NO);

  [dict removeBoolForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, YES);
  XCTAssertFalse([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict getBool:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getBool:NULL forKey:1U]);
  XCTAssertFalse([dict getBool:NULL forKey:2U]);
  XCTAssertFalse([dict getBool:NULL forKey:3U]);
  XCTAssertFalse([dict getBool:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const BOOL kValues[] = { YES, YES, NO, NO };
  GPBUInt32BoolDictionary *dict =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  BOOL value;
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:&value forKey:2U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:4U]);
  XCTAssertTrue([dict getBool:&value forKey:4U]);
  XCTAssertEqual(value, NO);

  [dict setBool:NO forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:&value forKey:2U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:4U]);
  XCTAssertTrue([dict getBool:&value forKey:4U]);
  XCTAssertEqual(value, NO);

  [dict setBool:YES forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:&value forKey:2U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:4U]);
  XCTAssertTrue([dict getBool:&value forKey:4U]);
  XCTAssertEqual(value, YES);

  const uint32_t kKeys2[] = { 2U, 3U };
  const BOOL kValues2[] = { NO, YES };
  GPBUInt32BoolDictionary *dict2 =
      [[GPBUInt32BoolDictionary alloc] initWithBools:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getBool:NULL forKey:1U]);
  XCTAssertTrue([dict getBool:&value forKey:1U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:2U]);
  XCTAssertTrue([dict getBool:&value forKey:2U]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict getBool:NULL forKey:3U]);
  XCTAssertTrue([dict getBool:&value forKey:3U]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict getBool:NULL forKey:4U]);
  XCTAssertTrue([dict getBool:&value forKey:4U]);
  XCTAssertEqual(value, YES);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Float

@interface GPBUInt32FloatDictionaryTests : XCTestCase
@end

@implementation GPBUInt32FloatDictionaryTests

- (void)testEmpty {
  GPBUInt32FloatDictionary *dict = [[GPBUInt32FloatDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:1U]);
  [dict enumerateKeysAndFloatsUsingBlock:^(uint32_t aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32FloatDictionary *dict = [[GPBUInt32FloatDictionary alloc] init];
  [dict setFloat:500.f forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:2U]);
  [dict enumerateKeysAndFloatsUsingBlock:^(uint32_t aKey, float aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, 500.f);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const float kValues[] = { 500.f, 501.f, 502.f };
  GPBUInt32FloatDictionary *dict =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:&value forKey:2U]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  float *seenValues = malloc(3 * sizeof(float));
  [dict enumerateKeysAndFloatsUsingBlock:^(uint32_t aKey, float aValue, BOOL *stop) {
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
  [dict enumerateKeysAndFloatsUsingBlock:^(uint32_t aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const float kValues1[] = { 500.f, 501.f, 502.f };
  const float kValues2[] = { 500.f, 503.f, 502.f };
  const float kValues3[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt32FloatDictionary *dict1 =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32FloatDictionary *dict1prime =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32FloatDictionary *dict2 =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32FloatDictionary *dict3 =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32FloatDictionary *dict4 =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt32FloatDictionary *dict =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32FloatDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32FloatDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt32FloatDictionary *dict =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32FloatDictionary *dict2 =
      [[GPBUInt32FloatDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32FloatDictionary *dict = [[GPBUInt32FloatDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setFloat:500.f forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const float kValues[] = { 501.f, 502.f, 503.f };
  GPBUInt32FloatDictionary *dict2 =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:&value forKey:2U]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:4U]);
  XCTAssertTrue([dict getFloat:&value forKey:4U]);
  XCTAssertEqual(value, 503.f);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt32FloatDictionary *dict =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeFloatForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:4U]);
  XCTAssertTrue([dict getFloat:&value forKey:4U]);
  XCTAssertEqual(value, 503.f);

  // Remove again does nothing.
  [dict removeFloatForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:4U]);
  XCTAssertTrue([dict getFloat:&value forKey:4U]);
  XCTAssertEqual(value, 503.f);

  [dict removeFloatForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertFalse([dict getFloat:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getFloat:NULL forKey:1U]);
  XCTAssertFalse([dict getFloat:NULL forKey:2U]);
  XCTAssertFalse([dict getFloat:NULL forKey:3U]);
  XCTAssertFalse([dict getFloat:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const float kValues[] = { 500.f, 501.f, 502.f, 503.f };
  GPBUInt32FloatDictionary *dict =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  float value;
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:&value forKey:2U]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:4U]);
  XCTAssertTrue([dict getFloat:&value forKey:4U]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:503.f forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:&value forKey:2U]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:4U]);
  XCTAssertTrue([dict getFloat:&value forKey:4U]);
  XCTAssertEqual(value, 503.f);

  [dict setFloat:501.f forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:&value forKey:2U]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:4U]);
  XCTAssertTrue([dict getFloat:&value forKey:4U]);
  XCTAssertEqual(value, 501.f);

  const uint32_t kKeys2[] = { 2U, 3U };
  const float kValues2[] = { 502.f, 500.f };
  GPBUInt32FloatDictionary *dict2 =
      [[GPBUInt32FloatDictionary alloc] initWithFloats:kValues2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getFloat:NULL forKey:1U]);
  XCTAssertTrue([dict getFloat:&value forKey:1U]);
  XCTAssertEqual(value, 503.f);
  XCTAssertTrue([dict getFloat:NULL forKey:2U]);
  XCTAssertTrue([dict getFloat:&value forKey:2U]);
  XCTAssertEqual(value, 502.f);
  XCTAssertTrue([dict getFloat:NULL forKey:3U]);
  XCTAssertTrue([dict getFloat:&value forKey:3U]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict getFloat:NULL forKey:4U]);
  XCTAssertTrue([dict getFloat:&value forKey:4U]);
  XCTAssertEqual(value, 501.f);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Double

@interface GPBUInt32DoubleDictionaryTests : XCTestCase
@end

@implementation GPBUInt32DoubleDictionaryTests

- (void)testEmpty {
  GPBUInt32DoubleDictionary *dict = [[GPBUInt32DoubleDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:1U]);
  [dict enumerateKeysAndDoublesUsingBlock:^(uint32_t aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32DoubleDictionary *dict = [[GPBUInt32DoubleDictionary alloc] init];
  [dict setDouble:600. forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:2U]);
  [dict enumerateKeysAndDoublesUsingBlock:^(uint32_t aKey, double aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, 600.);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const double kValues[] = { 600., 601., 602. };
  GPBUInt32DoubleDictionary *dict =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:&value forKey:2U]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  double *seenValues = malloc(3 * sizeof(double));
  [dict enumerateKeysAndDoublesUsingBlock:^(uint32_t aKey, double aValue, BOOL *stop) {
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
  [dict enumerateKeysAndDoublesUsingBlock:^(uint32_t aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const double kValues1[] = { 600., 601., 602. };
  const double kValues2[] = { 600., 603., 602. };
  const double kValues3[] = { 600., 601., 602., 603. };
  GPBUInt32DoubleDictionary *dict1 =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32DoubleDictionary *dict1prime =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32DoubleDictionary *dict2 =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32DoubleDictionary *dict3 =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32DoubleDictionary *dict4 =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt32DoubleDictionary *dict =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32DoubleDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32DoubleDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt32DoubleDictionary *dict =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32DoubleDictionary *dict2 =
      [[GPBUInt32DoubleDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32DoubleDictionary *dict = [[GPBUInt32DoubleDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setDouble:600. forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const double kValues[] = { 601., 602., 603. };
  GPBUInt32DoubleDictionary *dict2 =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:&value forKey:2U]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:4U]);
  XCTAssertTrue([dict getDouble:&value forKey:4U]);
  XCTAssertEqual(value, 603.);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt32DoubleDictionary *dict =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeDoubleForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:4U]);
  XCTAssertTrue([dict getDouble:&value forKey:4U]);
  XCTAssertEqual(value, 603.);

  // Remove again does nothing.
  [dict removeDoubleForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:4U]);
  XCTAssertTrue([dict getDouble:&value forKey:4U]);
  XCTAssertEqual(value, 603.);

  [dict removeDoubleForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertFalse([dict getDouble:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getDouble:NULL forKey:1U]);
  XCTAssertFalse([dict getDouble:NULL forKey:2U]);
  XCTAssertFalse([dict getDouble:NULL forKey:3U]);
  XCTAssertFalse([dict getDouble:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const double kValues[] = { 600., 601., 602., 603. };
  GPBUInt32DoubleDictionary *dict =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  double value;
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:&value forKey:2U]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:4U]);
  XCTAssertTrue([dict getDouble:&value forKey:4U]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:603. forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:&value forKey:2U]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:4U]);
  XCTAssertTrue([dict getDouble:&value forKey:4U]);
  XCTAssertEqual(value, 603.);

  [dict setDouble:601. forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:&value forKey:2U]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:4U]);
  XCTAssertTrue([dict getDouble:&value forKey:4U]);
  XCTAssertEqual(value, 601.);

  const uint32_t kKeys2[] = { 2U, 3U };
  const double kValues2[] = { 602., 600. };
  GPBUInt32DoubleDictionary *dict2 =
      [[GPBUInt32DoubleDictionary alloc] initWithDoubles:kValues2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getDouble:NULL forKey:1U]);
  XCTAssertTrue([dict getDouble:&value forKey:1U]);
  XCTAssertEqual(value, 603.);
  XCTAssertTrue([dict getDouble:NULL forKey:2U]);
  XCTAssertTrue([dict getDouble:&value forKey:2U]);
  XCTAssertEqual(value, 602.);
  XCTAssertTrue([dict getDouble:NULL forKey:3U]);
  XCTAssertTrue([dict getDouble:&value forKey:3U]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict getDouble:NULL forKey:4U]);
  XCTAssertTrue([dict getDouble:&value forKey:4U]);
  XCTAssertEqual(value, 601.);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Enum

@interface GPBUInt32EnumDictionaryTests : XCTestCase
@end

@implementation GPBUInt32EnumDictionaryTests

- (void)testEmpty {
  GPBUInt32EnumDictionary *dict = [[GPBUInt32EnumDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:1U]);
  [dict enumerateKeysAndEnumsUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32EnumDictionary *dict = [[GPBUInt32EnumDictionary alloc] init];
  [dict setEnum:700 forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  [dict enumerateKeysAndEnumsUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqual(aValue, 700);
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const int32_t kValues[] = { 700, 701, 702 };
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndEnumsUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const int32_t kValues1[] = { 700, 701, 702 };
  const int32_t kValues2[] = { 700, 703, 702 };
  const int32_t kValues3[] = { 700, 701, 702, 703 };
  GPBUInt32EnumDictionary *dict1 =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32EnumDictionary *dict1prime =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32EnumDictionary *dict3 =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32EnumDictionary *dict4 =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32EnumDictionary *dict = [[GPBUInt32EnumDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setEnum:700 forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const int32_t kValues[] = { 701, 702, 703 };
  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 703);
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 703);

  // Remove again does nothing.
  [dict removeEnumForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 703);

  [dict removeEnumForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:1U]);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertFalse([dict getEnum:NULL forKey:3U]);
  XCTAssertFalse([dict getEnum:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 701, 702, 703 };
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 703);

  [dict setEnum:703 forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 703);

  [dict setEnum:701 forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, 701);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 701);

  const uint32_t kKeys2[] = { 2U, 3U };
  const int32_t kValues2[] = { 702, 700 };
  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 703);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 701);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Enum (Unknown Enums)

@interface GPBUInt32EnumDictionaryUnknownEnumTests : XCTestCase
@end

@implementation GPBUInt32EnumDictionaryUnknownEnumTests

- (void)testRawBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const int32_t kValues[] = { 700, 801, 702 };
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue(dict.validationFunc == TestingEnum_IsValidValue);  // Pointer comparison
  int32_t value;
  XCTAssertTrue([dict getRawValue:NULL forKey:1U]);
  XCTAssertTrue([dict getRawValue:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:2U]);
  XCTAssertTrue([dict getRawValue:&value forKey:2U]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getRawValue:NULL forKey:3U]);
  XCTAssertTrue([dict getRawValue:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getRawValue:NULL forKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  int32_t *seenValues = malloc(3 * sizeof(int32_t));
  [dict enumerateKeysAndEnumsUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
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
  [dict enumerateKeysAndRawValuesUsingBlock:^(uint32_t aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEqualityWithUnknowns {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const int32_t kValues1[] = { 700, 801, 702 };  // Unknown
  const int32_t kValues2[] = { 700, 803, 702 };  // Unknown
  const int32_t kValues3[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt32EnumDictionary *dict1 =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBUInt32EnumDictionary *dict1prime =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues2
                                                          forKeys:kKeys1
                                                            count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBUInt32EnumDictionary *dict3 =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues1
                                                          forKeys:kKeys2
                                                            count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBUInt32EnumDictionary *dict4 =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknown
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertEqualObjects(dict, dict2);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  [dict2 release];
  [dict release];
}

- (void)testUnknownAdds {
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  XCTAssertThrowsSpecificNamed([dict setEnum:801 forKey:2U],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 0U);
  [dict setRawValue:801 forKey:2U];  // Unknown
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 1U, 3U, 4U };
  const int32_t kValues[] = { 700, 702, 803 };  // Unknown
  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithEnums:kValues
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:2U]);
  XCTAssertTrue([dict getRawValue:&value forKey:2U]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, kGPBUnrecognizedEnumeratorValue);
  XCTAssertTrue([dict getRawValue:NULL forKey:4U]);
  XCTAssertTrue([dict getRawValue:&value forKey:4U]);
  XCTAssertEqual(value, 803);
  [dict2 release];
  [dict release];
}

- (void)testUnknownRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeEnumForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:4U]);
  XCTAssertTrue([dict getRawValue:&value forKey:4U]);
  XCTAssertEqual(value, 803);

  // Remove again does nothing.
  [dict removeEnumForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:4U]);
  XCTAssertTrue([dict getRawValue:&value forKey:4U]);
  XCTAssertEqual(value, 803);

  [dict removeEnumForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertFalse([dict getEnum:NULL forKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict getEnum:NULL forKey:1U]);
  XCTAssertFalse([dict getEnum:NULL forKey:2U]);
  XCTAssertFalse([dict getEnum:NULL forKey:3U]);
  XCTAssertFalse([dict getEnum:NULL forKey:4U]);
  [dict release];
}

- (void)testInplaceMutationUnknowns {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 801, 702, 803 };  // Unknowns
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  int32_t value;
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:2U]);
  XCTAssertTrue([dict getRawValue:&value forKey:2U]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:4U]);
  XCTAssertTrue([dict getRawValue:&value forKey:4U]);
  XCTAssertEqual(value, 803);

  XCTAssertThrowsSpecificNamed([dict setEnum:803 forKey:1U],  // Unknown
                               NSException, NSInvalidArgumentException);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getEnum:NULL forKey:1U]);
  XCTAssertTrue([dict getEnum:&value forKey:1U]);
  XCTAssertEqual(value, 700);
  XCTAssertTrue([dict getRawValue:NULL forKey:2U]);
  XCTAssertTrue([dict getRawValue:&value forKey:2U]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:4U]);
  XCTAssertTrue([dict getRawValue:&value forKey:4U]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:803 forKey:1U];  // Unknown
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:1U]);
  XCTAssertTrue([dict getRawValue:&value forKey:1U]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:2U]);
  XCTAssertTrue([dict getRawValue:&value forKey:2U]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:4U]);
  XCTAssertTrue([dict getRawValue:&value forKey:4U]);
  XCTAssertEqual(value, 803);

  [dict setRawValue:700 forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:1U]);
  XCTAssertTrue([dict getRawValue:&value forKey:1U]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getRawValue:NULL forKey:2U]);
  XCTAssertTrue([dict getRawValue:&value forKey:2U]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:3U]);
  XCTAssertTrue([dict getEnum:&value forKey:3U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 700);

  const uint32_t kKeys2[] = { 2U, 3U };
  const int32_t kValues2[] = { 702, 801 };  // Unknown
  GPBUInt32EnumDictionary *dict2 =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues2
                                                          forKeys:kKeys2
                                                            count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addRawEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertTrue([dict getRawValue:NULL forKey:1U]);
  XCTAssertTrue([dict getRawValue:&value forKey:1U]);
  XCTAssertEqual(value, 803);
  XCTAssertTrue([dict getEnum:NULL forKey:2U]);
  XCTAssertTrue([dict getEnum:&value forKey:2U]);
  XCTAssertEqual(value, 702);
  XCTAssertTrue([dict getRawValue:NULL forKey:3U]);
  XCTAssertTrue([dict getRawValue:&value forKey:3U]);
  XCTAssertEqual(value, 801);
  XCTAssertTrue([dict getEnum:NULL forKey:4U]);
  XCTAssertTrue([dict getEnum:&value forKey:4U]);
  XCTAssertEqual(value, 700);

  [dict2 release];
  [dict release];
}

- (void)testCopyUnknowns {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const int32_t kValues[] = { 700, 801, 702, 803 };
  GPBUInt32EnumDictionary *dict =
      [[GPBUInt32EnumDictionary alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                        rawValues:kValues
                                                          forKeys:kKeys
                                                            count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBUInt32EnumDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertEqual(dict.validationFunc, dict2.validationFunc);  // Pointer comparison
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32EnumDictionary class]]);

  [dict2 release];
  [dict release];
}

@end

#pragma mark - UInt32 -> Object

@interface GPBUInt32ObjectDictionaryTests : XCTestCase
@end

@implementation GPBUInt32ObjectDictionaryTests

- (void)testEmpty {
  GPBUInt32ObjectDictionary<NSString*> *dict = [[GPBUInt32ObjectDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:1U]);
  [dict enumerateKeysAndObjectsUsingBlock:^(uint32_t aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBUInt32ObjectDictionary<NSString*> *dict = [[GPBUInt32ObjectDictionary alloc] init];
  [dict setObject:@"abc" forKey:1U];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"abc");
  XCTAssertNil([dict objectForKey:2U]);
  [dict enumerateKeysAndObjectsUsingBlock:^(uint32_t aKey, NSString* aObject, BOOL *stop) {
    XCTAssertEqual(aKey, 1U);
    XCTAssertEqualObjects(aObject, @"abc");
    XCTAssertNotEqual(stop, NULL);
  }];
  [dict release];
}

- (void)testBasics {
  const uint32_t kKeys[] = { 1U, 2U, 3U };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi" };
  GPBUInt32ObjectDictionary<NSString*> *dict =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"abc");
  XCTAssertEqualObjects([dict objectForKey:2U], @"def");
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertNil([dict objectForKey:4U]);

  __block NSUInteger idx = 0;
  uint32_t *seenKeys = malloc(3 * sizeof(uint32_t));
  NSString* *seenObjects = malloc(3 * sizeof(NSString*));
  [dict enumerateKeysAndObjectsUsingBlock:^(uint32_t aKey, NSString* aObject, BOOL *stop) {
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
  [dict enumerateKeysAndObjectsUsingBlock:^(uint32_t aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject)
    if (idx == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const uint32_t kKeys1[] = { 1U, 2U, 3U, 4U };
  const uint32_t kKeys2[] = { 2U, 1U, 4U };
  const NSString* kObjects1[] = { @"abc", @"def", @"ghi" };
  const NSString* kObjects2[] = { @"abc", @"jkl", @"ghi" };
  const NSString* kObjects3[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt32ObjectDictionary<NSString*> *dict1 =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1);
  GPBUInt32ObjectDictionary<NSString*> *dict1prime =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects1
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1prime);
  GPBUInt32ObjectDictionary<NSString*> *dict2 =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects2
                                                 forKeys:kKeys1
                                                   count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  GPBUInt32ObjectDictionary<NSString*> *dict3 =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects1
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict3);
  GPBUInt32ObjectDictionary<NSString*> *dict4 =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects3
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
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt32ObjectDictionary<NSString*> *dict =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBUInt32ObjectDictionary<NSString*> *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBUInt32ObjectDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt32ObjectDictionary<NSString*> *dict =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBUInt32ObjectDictionary<NSString*> *dict2 =
      [[GPBUInt32ObjectDictionary alloc] initWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict2 release];
  [dict release];
}

- (void)testAdds {
  GPBUInt32ObjectDictionary<NSString*> *dict = [[GPBUInt32ObjectDictionary alloc] init];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setObject:@"abc" forKey:1U];
  XCTAssertEqual(dict.count, 1U);

  const uint32_t kKeys[] = { 2U, 3U, 4U };
  const NSString* kObjects[] = { @"def", @"ghi", @"jkl" };
  GPBUInt32ObjectDictionary<NSString*> *dict2 =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);

  XCTAssertEqualObjects([dict objectForKey:1U], @"abc");
  XCTAssertEqualObjects([dict objectForKey:2U], @"def");
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:4U], @"jkl");
  [dict2 release];
  [dict release];
}

- (void)testRemove {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt32ObjectDictionary<NSString*> *dict =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);

  [dict removeObjectForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"abc");
  XCTAssertNil([dict objectForKey:2U]);
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:4U], @"jkl");

  // Remove again does nothing.
  [dict removeObjectForKey:2U];
  XCTAssertEqual(dict.count, 3U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"abc");
  XCTAssertNil([dict objectForKey:2U]);
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:4U], @"jkl");

  [dict removeObjectForKey:4U];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"abc");
  XCTAssertNil([dict objectForKey:2U]);
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertNil([dict objectForKey:4U]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:1U]);
  XCTAssertNil([dict objectForKey:2U]);
  XCTAssertNil([dict objectForKey:3U]);
  XCTAssertNil([dict objectForKey:4U]);
  [dict release];
}

- (void)testInplaceMutation {
  const uint32_t kKeys[] = { 1U, 2U, 3U, 4U };
  const NSString* kObjects[] = { @"abc", @"def", @"ghi", @"jkl" };
  GPBUInt32ObjectDictionary<NSString*> *dict =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects
                                                 forKeys:kKeys
                                                   count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"abc");
  XCTAssertEqualObjects([dict objectForKey:2U], @"def");
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:4U], @"jkl");

  [dict setObject:@"jkl" forKey:1U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:2U], @"def");
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:4U], @"jkl");

  [dict setObject:@"def" forKey:4U];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:2U], @"def");
  XCTAssertEqualObjects([dict objectForKey:3U], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:4U], @"def");

  const uint32_t kKeys2[] = { 2U, 3U };
  const NSString* kObjects2[] = { @"ghi", @"abc" };
  GPBUInt32ObjectDictionary<NSString*> *dict2 =
      [[GPBUInt32ObjectDictionary alloc] initWithObjects:kObjects2
                                                 forKeys:kKeys2
                                                   count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 4U);
  XCTAssertEqualObjects([dict objectForKey:1U], @"jkl");
  XCTAssertEqualObjects([dict objectForKey:2U], @"ghi");
  XCTAssertEqualObjects([dict objectForKey:3U], @"abc");
  XCTAssertEqualObjects([dict objectForKey:4U], @"def");

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND-END TEST_FOR_POD_KEY(UInt32, uint32_t, 1U, 2U, 3U, 4U)

