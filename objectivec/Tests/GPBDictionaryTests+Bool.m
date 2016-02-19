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

//%PDDM-EXPAND BOOL_TESTS_FOR_POD_VALUE(UInt32, uint32_t, 100U, 101U)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> UInt32

@interface GPBBoolUInt32DictionaryTests : XCTestCase
@end

@implementation GPBBoolUInt32DictionaryTests

- (void)testEmpty {
  GPBBoolUInt32Dictionary *dict = [[GPBBoolUInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolUInt32Dictionary *dict = [GPBBoolUInt32Dictionary dictionaryWithValue:100U forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqual(aValue, 100U);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const uint32_t kValues[] = { 100U, 101U };
  GPBBoolUInt32Dictionary *dict =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 101U);

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  uint32_t *seenValues = malloc(2 * sizeof(uint32_t));
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const uint32_t kValues1[] = { 100U, 101U };
  const uint32_t kValues2[] = { 101U, 100U };
  const uint32_t kValues3[] = { 101U };
  GPBBoolUInt32Dictionary *dict1 =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBBoolUInt32Dictionary *dict1prime =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolUInt32Dictionary *dict2 =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBBoolUInt32Dictionary *dict3 =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBBoolUInt32Dictionary *dict4 =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const uint32_t kValues[] = { 100U, 101U };
  GPBBoolUInt32Dictionary *dict =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolUInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolUInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const uint32_t kValues[] = { 100U, 101U };
  GPBBoolUInt32Dictionary *dict =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolUInt32Dictionary *dict2 =
      [GPBBoolUInt32Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolUInt32Dictionary *dict = [GPBBoolUInt32Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:100U forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const uint32_t kValues[] = { 101U };
  GPBBoolUInt32Dictionary *dict2 =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  uint32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 101U);
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const uint32_t kValues[] = { 100U, 101U };
  GPBBoolUInt32Dictionary *dict =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  // Remove again does nothing.
  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const uint32_t kValues[] = { 100U, 101U };
  GPBBoolUInt32Dictionary *dict =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  uint32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 101U);

  [dict setValue:101U forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 101U);

  [dict setValue:100U forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 101U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 100U);

  const BOOL kKeys2[] = { NO, YES };
  const uint32_t kValues2[] = { 101U, 100U };
  GPBBoolUInt32Dictionary *dict2 =
      [[GPBBoolUInt32Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 100U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 101U);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND BOOL_TESTS_FOR_POD_VALUE(Int32, int32_t, 200, 201)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Int32

@interface GPBBoolInt32DictionaryTests : XCTestCase
@end

@implementation GPBBoolInt32DictionaryTests

- (void)testEmpty {
  GPBBoolInt32Dictionary *dict = [[GPBBoolInt32Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolInt32Dictionary *dict = [GPBBoolInt32Dictionary dictionaryWithValue:200 forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int32_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqual(aValue, 200);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const int32_t kValues[] = { 200, 201 };
  GPBBoolInt32Dictionary *dict =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 201);

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  int32_t *seenValues = malloc(2 * sizeof(int32_t));
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int32_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int32_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const int32_t kValues1[] = { 200, 201 };
  const int32_t kValues2[] = { 201, 200 };
  const int32_t kValues3[] = { 201 };
  GPBBoolInt32Dictionary *dict1 =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBBoolInt32Dictionary *dict1prime =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolInt32Dictionary *dict2 =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBBoolInt32Dictionary *dict3 =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBBoolInt32Dictionary *dict4 =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const int32_t kValues[] = { 200, 201 };
  GPBBoolInt32Dictionary *dict =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolInt32Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolInt32Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const int32_t kValues[] = { 200, 201 };
  GPBBoolInt32Dictionary *dict =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolInt32Dictionary *dict2 =
      [GPBBoolInt32Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolInt32Dictionary *dict = [GPBBoolInt32Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:200 forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const int32_t kValues[] = { 201 };
  GPBBoolInt32Dictionary *dict2 =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  int32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 201);
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const int32_t kValues[] = { 200, 201 };
  GPBBoolInt32Dictionary *dict =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  // Remove again does nothing.
  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const int32_t kValues[] = { 200, 201 };
  GPBBoolInt32Dictionary *dict =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  int32_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 201);

  [dict setValue:201 forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 201);

  [dict setValue:200 forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 201);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 200);

  const BOOL kKeys2[] = { NO, YES };
  const int32_t kValues2[] = { 201, 200 };
  GPBBoolInt32Dictionary *dict2 =
      [[GPBBoolInt32Dictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 200);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 201);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND BOOL_TESTS_FOR_POD_VALUE(UInt64, uint64_t, 300U, 301U)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> UInt64

@interface GPBBoolUInt64DictionaryTests : XCTestCase
@end

@implementation GPBBoolUInt64DictionaryTests

- (void)testEmpty {
  GPBBoolUInt64Dictionary *dict = [[GPBBoolUInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolUInt64Dictionary *dict = [GPBBoolUInt64Dictionary dictionaryWithValue:300U forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqual(aValue, 300U);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const uint64_t kValues[] = { 300U, 301U };
  GPBBoolUInt64Dictionary *dict =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 301U);

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  uint64_t *seenValues = malloc(2 * sizeof(uint64_t));
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint64_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, uint64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const uint64_t kValues1[] = { 300U, 301U };
  const uint64_t kValues2[] = { 301U, 300U };
  const uint64_t kValues3[] = { 301U };
  GPBBoolUInt64Dictionary *dict1 =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBBoolUInt64Dictionary *dict1prime =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolUInt64Dictionary *dict2 =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBBoolUInt64Dictionary *dict3 =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBBoolUInt64Dictionary *dict4 =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const uint64_t kValues[] = { 300U, 301U };
  GPBBoolUInt64Dictionary *dict =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolUInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolUInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const uint64_t kValues[] = { 300U, 301U };
  GPBBoolUInt64Dictionary *dict =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolUInt64Dictionary *dict2 =
      [GPBBoolUInt64Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolUInt64Dictionary *dict = [GPBBoolUInt64Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:300U forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const uint64_t kValues[] = { 301U };
  GPBBoolUInt64Dictionary *dict2 =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  uint64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 301U);
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const uint64_t kValues[] = { 300U, 301U };
  GPBBoolUInt64Dictionary *dict =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  // Remove again does nothing.
  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const uint64_t kValues[] = { 300U, 301U };
  GPBBoolUInt64Dictionary *dict =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  uint64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 301U);

  [dict setValue:301U forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 301U);

  [dict setValue:300U forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 301U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 300U);

  const BOOL kKeys2[] = { NO, YES };
  const uint64_t kValues2[] = { 301U, 300U };
  GPBBoolUInt64Dictionary *dict2 =
      [[GPBBoolUInt64Dictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 300U);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 301U);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND BOOL_TESTS_FOR_POD_VALUE(Int64, int64_t, 400, 401)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Int64

@interface GPBBoolInt64DictionaryTests : XCTestCase
@end

@implementation GPBBoolInt64DictionaryTests

- (void)testEmpty {
  GPBBoolInt64Dictionary *dict = [[GPBBoolInt64Dictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolInt64Dictionary *dict = [GPBBoolInt64Dictionary dictionaryWithValue:400 forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int64_t aValue, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqual(aValue, 400);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const int64_t kValues[] = { 400, 401 };
  GPBBoolInt64Dictionary *dict =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 401);

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  int64_t *seenValues = malloc(2 * sizeof(int64_t));
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int64_t aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, int64_t aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const int64_t kValues1[] = { 400, 401 };
  const int64_t kValues2[] = { 401, 400 };
  const int64_t kValues3[] = { 401 };
  GPBBoolInt64Dictionary *dict1 =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBBoolInt64Dictionary *dict1prime =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolInt64Dictionary *dict2 =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBBoolInt64Dictionary *dict3 =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBBoolInt64Dictionary *dict4 =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const int64_t kValues[] = { 400, 401 };
  GPBBoolInt64Dictionary *dict =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolInt64Dictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolInt64Dictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const int64_t kValues[] = { 400, 401 };
  GPBBoolInt64Dictionary *dict =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolInt64Dictionary *dict2 =
      [GPBBoolInt64Dictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolInt64Dictionary *dict = [GPBBoolInt64Dictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:400 forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const int64_t kValues[] = { 401 };
  GPBBoolInt64Dictionary *dict2 =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  int64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 401);
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const int64_t kValues[] = { 400, 401 };
  GPBBoolInt64Dictionary *dict =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  // Remove again does nothing.
  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const int64_t kValues[] = { 400, 401 };
  GPBBoolInt64Dictionary *dict =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  int64_t value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 401);

  [dict setValue:401 forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 401);

  [dict setValue:400 forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 401);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 400);

  const BOOL kKeys2[] = { NO, YES };
  const int64_t kValues2[] = { 401, 400 };
  GPBBoolInt64Dictionary *dict2 =
      [[GPBBoolInt64Dictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 400);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 401);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND BOOL_TESTS_FOR_POD_VALUE(Bool, BOOL, NO, YES)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Bool

@interface GPBBoolBoolDictionaryTests : XCTestCase
@end

@implementation GPBBoolBoolDictionaryTests

- (void)testEmpty {
  GPBBoolBoolDictionary *dict = [[GPBBoolBoolDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolBoolDictionary *dict = [GPBBoolBoolDictionary dictionaryWithValue:NO forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, BOOL aValue, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqual(aValue, NO);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const BOOL kValues[] = { NO, YES };
  GPBBoolBoolDictionary *dict =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, YES);

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  BOOL *seenValues = malloc(2 * sizeof(BOOL));
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, BOOL aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, BOOL aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const BOOL kValues1[] = { NO, YES };
  const BOOL kValues2[] = { YES, NO };
  const BOOL kValues3[] = { YES };
  GPBBoolBoolDictionary *dict1 =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues1
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBBoolBoolDictionary *dict1prime =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues1
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolBoolDictionary *dict2 =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues2
                                            forKeys:kKeys1
                                              count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBBoolBoolDictionary *dict3 =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues1
                                            forKeys:kKeys2
                                              count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBBoolBoolDictionary *dict4 =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const BOOL kValues[] = { NO, YES };
  GPBBoolBoolDictionary *dict =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolBoolDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolBoolDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const BOOL kValues[] = { NO, YES };
  GPBBoolBoolDictionary *dict =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolBoolDictionary *dict2 =
      [GPBBoolBoolDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolBoolDictionary *dict = [GPBBoolBoolDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:NO forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const BOOL kValues[] = { YES };
  GPBBoolBoolDictionary *dict2 =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues
                                            forKeys:kKeys
                                              count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  BOOL value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, YES);
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const BOOL kValues[] = { NO, YES };
  GPBBoolBoolDictionary *dict =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues
                                     forKeys:kKeys
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  // Remove again does nothing.
  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const BOOL kValues[] = { NO, YES };
  GPBBoolBoolDictionary *dict =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues
                                     forKeys:kKeys
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  BOOL value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, YES);

  [dict setValue:YES forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, YES);

  [dict setValue:NO forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, YES);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, NO);

  const BOOL kKeys2[] = { NO, YES };
  const BOOL kValues2[] = { YES, NO };
  GPBBoolBoolDictionary *dict2 =
      [[GPBBoolBoolDictionary alloc] initWithValues:kValues2
                                            forKeys:kKeys2
                                              count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, NO);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, YES);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND BOOL_TESTS_FOR_POD_VALUE(Float, float, 500.f, 501.f)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Float

@interface GPBBoolFloatDictionaryTests : XCTestCase
@end

@implementation GPBBoolFloatDictionaryTests

- (void)testEmpty {
  GPBBoolFloatDictionary *dict = [[GPBBoolFloatDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolFloatDictionary *dict = [GPBBoolFloatDictionary dictionaryWithValue:500.f forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  float value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, float aValue, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqual(aValue, 500.f);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const float kValues[] = { 500.f, 501.f };
  GPBBoolFloatDictionary *dict =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  float value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 501.f);

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  float *seenValues = malloc(2 * sizeof(float));
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, float aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, float aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const float kValues1[] = { 500.f, 501.f };
  const float kValues2[] = { 501.f, 500.f };
  const float kValues3[] = { 501.f };
  GPBBoolFloatDictionary *dict1 =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBBoolFloatDictionary *dict1prime =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolFloatDictionary *dict2 =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys1
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBBoolFloatDictionary *dict3 =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues1
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBBoolFloatDictionary *dict4 =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const float kValues[] = { 500.f, 501.f };
  GPBBoolFloatDictionary *dict =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolFloatDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolFloatDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const float kValues[] = { 500.f, 501.f };
  GPBBoolFloatDictionary *dict =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolFloatDictionary *dict2 =
      [GPBBoolFloatDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolFloatDictionary *dict = [GPBBoolFloatDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:500.f forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const float kValues[] = { 501.f };
  GPBBoolFloatDictionary *dict2 =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues
                                             forKeys:kKeys
                                               count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  float value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 501.f);
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const float kValues[] = { 500.f, 501.f };
  GPBBoolFloatDictionary *dict =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  float value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  // Remove again does nothing.
  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const float kValues[] = { 500.f, 501.f };
  GPBBoolFloatDictionary *dict =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues
                                      forKeys:kKeys
                                        count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  float value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 501.f);

  [dict setValue:501.f forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 501.f);

  [dict setValue:500.f forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 501.f);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 500.f);

  const BOOL kKeys2[] = { NO, YES };
  const float kValues2[] = { 501.f, 500.f };
  GPBBoolFloatDictionary *dict2 =
      [[GPBBoolFloatDictionary alloc] initWithValues:kValues2
                                             forKeys:kKeys2
                                               count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 500.f);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 501.f);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND BOOL_TESTS_FOR_POD_VALUE(Double, double, 600., 601.)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Double

@interface GPBBoolDoubleDictionaryTests : XCTestCase
@end

@implementation GPBBoolDoubleDictionaryTests

- (void)testEmpty {
  GPBBoolDoubleDictionary *dict = [[GPBBoolDoubleDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolDoubleDictionary *dict = [GPBBoolDoubleDictionary dictionaryWithValue:600. forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  double value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, double aValue, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqual(aValue, 600.);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const double kValues[] = { 600., 601. };
  GPBBoolDoubleDictionary *dict =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  double value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 601.);

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  double *seenValues = malloc(2 * sizeof(double));
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, double aValue, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenValues[idx] = aValue;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndValuesUsingBlock:^(BOOL aKey, double aValue, BOOL *stop) {
    #pragma unused(aKey, aValue)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const double kValues1[] = { 600., 601. };
  const double kValues2[] = { 601., 600. };
  const double kValues3[] = { 601. };
  GPBBoolDoubleDictionary *dict1 =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1);
  GPBBoolDoubleDictionary *dict1prime =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolDoubleDictionary *dict2 =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys1
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  GPBBoolDoubleDictionary *dict3 =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues1
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(dict3);
  GPBBoolDoubleDictionary *dict4 =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const double kValues[] = { 600., 601. };
  GPBBoolDoubleDictionary *dict =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolDoubleDictionary *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolDoubleDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const double kValues[] = { 600., 601. };
  GPBBoolDoubleDictionary *dict =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);

  GPBBoolDoubleDictionary *dict2 =
      [GPBBoolDoubleDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolDoubleDictionary *dict = [GPBBoolDoubleDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setValue:600. forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const double kValues[] = { 601. };
  GPBBoolDoubleDictionary *dict2 =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues
                                              forKeys:kKeys
                                                count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  double value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 601.);
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const double kValues[] = { 600., 601. };
  GPBBoolDoubleDictionary *dict =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  double value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  // Remove again does nothing.
  [dict removeValueForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertFalse([dict valueForKey:YES value:NULL]);
  XCTAssertFalse([dict valueForKey:NO value:NULL]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const double kValues[] = { 600., 601. };
  GPBBoolDoubleDictionary *dict =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues
                                       forKeys:kKeys
                                         count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  double value;
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 601.);

  [dict setValue:601. forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 601.);

  [dict setValue:600. forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 601.);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 600.);

  const BOOL kKeys2[] = { NO, YES };
  const double kValues2[] = { 601., 600. };
  GPBBoolDoubleDictionary *dict2 =
      [[GPBBoolDoubleDictionary alloc] initWithValues:kValues2
                                              forKeys:kKeys2
                                                count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertTrue([dict valueForKey:YES value:NULL]);
  XCTAssertTrue([dict valueForKey:YES value:&value]);
  XCTAssertEqual(value, 600.);
  XCTAssertTrue([dict valueForKey:NO value:NULL]);
  XCTAssertTrue([dict valueForKey:NO value:&value]);
  XCTAssertEqual(value, 601.);

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND TESTS_FOR_BOOL_KEY_OBJECT_VALUE(Object, NSString*, @"abc", @"def")
// This block of code is generated, do not edit it directly.

#pragma mark - Bool -> Object

@interface GPBBoolObjectDictionaryTests : XCTestCase
@end

@implementation GPBBoolObjectDictionaryTests

- (void)testEmpty {
  GPBBoolObjectDictionary<NSString*> *dict = [[GPBBoolObjectDictionary alloc] init];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:YES]);
  [dict enumerateKeysAndObjectsUsingBlock:^(BOOL aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject, stop)
    XCTFail(@"Shouldn't get here!");
  }];
  [dict release];
}

- (void)testOne {
  GPBBoolObjectDictionary<NSString*> *dict = [GPBBoolObjectDictionary dictionaryWithObject:@"abc" forKey:YES];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 1U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"abc");
  XCTAssertNil([dict objectForKey:NO]);
  [dict enumerateKeysAndObjectsUsingBlock:^(BOOL aKey, NSString* aObject, BOOL *stop) {
    XCTAssertEqual(aKey, YES);
    XCTAssertEqualObjects(aObject, @"abc");
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  const BOOL kKeys[] = { YES, NO };
  const NSString* kObjects[] = { @"abc", @"def" };
  GPBBoolObjectDictionary<NSString*> *dict =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"abc");
  XCTAssertEqualObjects([dict objectForKey:NO], @"def");

  __block NSUInteger idx = 0;
  BOOL *seenKeys = malloc(2 * sizeof(BOOL));
  NSString* *seenObjects = malloc(2 * sizeof(NSString*));
  [dict enumerateKeysAndObjectsUsingBlock:^(BOOL aKey, NSString* aObject, BOOL *stop) {
    XCTAssertLessThan(idx, 2U);
    seenKeys[idx] = aKey;
    seenObjects[idx] = aObject;
    XCTAssertNotEqual(stop, NULL);
    ++idx;
  }];
  for (int i = 0; i < 2; ++i) {
    BOOL foundKey = NO;
    for (int j = 0; (j < 2) && !foundKey; ++j) {
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
  [dict enumerateKeysAndObjectsUsingBlock:^(BOOL aKey, NSString* aObject, BOOL *stop) {
    #pragma unused(aKey, aObject)
    if (idx == 0) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    ++idx;
  }];
  [dict release];
}

- (void)testEquality {
  const BOOL kKeys1[] = { YES, NO };
  const BOOL kKeys2[] = { NO, YES };
  const NSString* kObjects1[] = { @"abc", @"def" };
  const NSString* kObjects2[] = { @"def", @"abc" };
  const NSString* kObjects3[] = { @"def" };
  GPBBoolObjectDictionary<NSString*> *dict1 =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1);
  GPBBoolObjectDictionary<NSString*> *dict1prime =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects1
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict1prime);
  GPBBoolObjectDictionary<NSString*> *dict2 =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects2
                                               forKeys:kKeys1
                                                 count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  GPBBoolObjectDictionary<NSString*> *dict3 =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects1
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kObjects1)];
  XCTAssertNotNil(dict3);
  GPBBoolObjectDictionary<NSString*> *dict4 =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects3
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

  // 4 Fewer pairs; not equal
  XCTAssertNotEqualObjects(dict1, dict4);

  [dict1 release];
  [dict1prime release];
  [dict2 release];
  [dict3 release];
  [dict4 release];
}

- (void)testCopy {
  const BOOL kKeys[] = { YES, NO };
  const NSString* kObjects[] = { @"abc", @"def" };
  GPBBoolObjectDictionary<NSString*> *dict =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBBoolObjectDictionary<NSString*> *dict2 = [dict copy];
  XCTAssertNotNil(dict2);

  // Should be new object but equal.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  XCTAssertTrue([dict2 isKindOfClass:[GPBBoolObjectDictionary class]]);

  [dict2 release];
  [dict release];
}

- (void)testDictionaryFromDictionary {
  const BOOL kKeys[] = { YES, NO };
  const NSString* kObjects[] = { @"abc", @"def" };
  GPBBoolObjectDictionary<NSString*> *dict =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);

  GPBBoolObjectDictionary<NSString*> *dict2 =
      [GPBBoolObjectDictionary dictionaryWithDictionary:dict];
  XCTAssertNotNil(dict2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(dict, dict2);
  XCTAssertEqualObjects(dict, dict2);
  [dict release];
}

- (void)testAdds {
  GPBBoolObjectDictionary<NSString*> *dict = [GPBBoolObjectDictionary dictionary];
  XCTAssertNotNil(dict);

  XCTAssertEqual(dict.count, 0U);
  [dict setObject:@"abc" forKey:YES];
  XCTAssertEqual(dict.count, 1U);

  const BOOL kKeys[] = { NO };
  const NSString* kObjects[] = { @"def" };
  GPBBoolObjectDictionary<NSString*> *dict2 =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects
                                               forKeys:kKeys
                                                 count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);

  XCTAssertEqualObjects([dict objectForKey:YES], @"abc");
  XCTAssertEqualObjects([dict objectForKey:NO], @"def");
  [dict2 release];
}

- (void)testRemove {
  const BOOL kKeys[] = { YES, NO};
  const NSString* kObjects[] = { @"abc", @"def" };
  GPBBoolObjectDictionary<NSString*> *dict =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);

  [dict removeObjectForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"abc");
  XCTAssertNil([dict objectForKey:NO]);

  // Remove again does nothing.
  [dict removeObjectForKey:NO];
  XCTAssertEqual(dict.count, 1U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"abc");
  XCTAssertNil([dict objectForKey:NO]);

  [dict removeAll];
  XCTAssertEqual(dict.count, 0U);
  XCTAssertNil([dict objectForKey:YES]);
  XCTAssertNil([dict objectForKey:NO]);
  [dict release];
}

- (void)testInplaceMutation {
  const BOOL kKeys[] = { YES, NO };
  const NSString* kObjects[] = { @"abc", @"def" };
  GPBBoolObjectDictionary<NSString*> *dict =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects
                                        forKeys:kKeys
                                          count:GPBARRAYSIZE(kObjects)];
  XCTAssertNotNil(dict);
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"abc");
  XCTAssertEqualObjects([dict objectForKey:NO], @"def");

  [dict setObject:@"def" forKey:YES];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"def");
  XCTAssertEqualObjects([dict objectForKey:NO], @"def");

  [dict setObject:@"abc" forKey:NO];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"def");
  XCTAssertEqualObjects([dict objectForKey:NO], @"abc");

  const BOOL kKeys2[] = { NO, YES };
  const NSString* kObjects2[] = { @"def", @"abc" };
  GPBBoolObjectDictionary<NSString*> *dict2 =
      [[GPBBoolObjectDictionary alloc] initWithObjects:kObjects2
                                               forKeys:kKeys2
                                                 count:GPBARRAYSIZE(kObjects2)];
  XCTAssertNotNil(dict2);
  [dict addEntriesFromDictionary:dict2];
  XCTAssertEqual(dict.count, 2U);
  XCTAssertEqualObjects([dict objectForKey:YES], @"abc");
  XCTAssertEqualObjects([dict objectForKey:NO], @"def");

  [dict2 release];
  [dict release];
}

@end

//%PDDM-EXPAND-END (8 expansions)


