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

#import "GPBArray.h"
#import "GPBArray_PackagePrivate.h"

#import "GPBTestUtilities.h"

// To let the testing macros work, add some extra methods to simplify things.
@interface GPBEnumArray (TestingTweak)
+ (instancetype)arrayWithValue:(int32_t)value;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;
- (instancetype)initWithValues:(const int32_t[])values count:(NSUInteger)count;
@end

static BOOL TestingEnum_IsValidValue(int32_t value) {
  switch (value) {
    case 71:
    case 72:
    case 73:
    case 74:
      return YES;
    default:
      return NO;
  }
}

static BOOL TestingEnum_IsValidValue2(int32_t value) {
  switch (value) {
    case 71:
    case 72:
    case 73:
      return YES;
    default:
      return NO;
  }
}

@implementation GPBEnumArray (TestingTweak)
+ (instancetype)arrayWithValue:(int32_t)value {
  return [[[self alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                         rawValues:&value
                                             count:1] autorelease];
}
+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                          capacity:count] autorelease];
}
- (instancetype)initWithValues:(const int32_t[])values count:(NSUInteger)count {
  return [self initWithValidationFunction:TestingEnum_IsValidValue rawValues:values count:count];
}
@end

#pragma mark - PDDM Macros

// Disable clang-format for the macros.
// clang-format off

//%PDDM-DEFINE ARRAY_TESTS(NAME, TYPE, VAL1, VAL2, VAL3, VAL4)
//%ARRAY_TESTS2(NAME, TYPE, VAL1, VAL2, VAL3, VAL4, )
//%PDDM-DEFINE ARRAY_TESTS2(NAME, TYPE, VAL1, VAL2, VAL3, VAL4, HELPER)
//%#pragma mark - NAME
//%
//%@interface GPB##NAME##ArrayTests : XCTestCase
//%@end
//%
//%@implementation GPB##NAME##ArrayTests
//%
//%- (void)testEmpty {
//%  GPB##NAME##Array *array = [[GPB##NAME##Array alloc] init];
//%  XCTAssertNotNil(array);
//%  XCTAssertEqual(array.count, 0U);
//%  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
//%  [array enumerateValuesWithBlock:^(__unused TYPE value, __unused NSUInteger idx, __unused BOOL *stop) {
//%    XCTFail(@"Shouldn't get here!");
//%  }];
//%  [array enumerateValuesWithOptions:NSEnumerationReverse
//%                         usingBlock:^(__unused TYPE value, __unused NSUInteger idx, __unused BOOL *stop) {
//%    XCTFail(@"Shouldn't get here!");
//%  }];
//%  [array release];
//%}
//%
//%- (void)testOne {
//%  GPB##NAME##Array *array = [GPB##NAME##Array arrayWithValue:VAL1];
//%  XCTAssertNotNil(array);
//%  XCTAssertEqual(array.count, 1U);
//%  XCTAssertEqual([array valueAtIndex:0], VAL1);
//%  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
//%  [array enumerateValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%    XCTAssertEqual(idx, 0U);
//%    XCTAssertEqual(value, VAL1);
//%    XCTAssertNotEqual(stop, NULL);
//%  }];
//%  [array enumerateValuesWithOptions:NSEnumerationReverse
//%                         usingBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%    XCTAssertEqual(idx, 0U);
//%    XCTAssertEqual(value, VAL1);
//%    XCTAssertNotEqual(stop, NULL);
//%  }];
//%}
//%
//%- (void)testBasics {
//%  static const TYPE kValues[] = { VAL1, VAL2, VAL3, VAL4 };
//%  GPB##NAME##Array *array =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues
//%            NAME$S                     count:GPBARRAYSIZE(kValues)];
//%  XCTAssertNotNil(array);
//%  XCTAssertEqual(array.count, 4U);
//%  XCTAssertEqual([array valueAtIndex:0], VAL1);
//%  XCTAssertEqual([array valueAtIndex:1], VAL2);
//%  XCTAssertEqual([array valueAtIndex:2], VAL3);
//%  XCTAssertEqual([array valueAtIndex:3], VAL4);
//%  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
//%  __block NSUInteger idx2 = 0;
//%  [array enumerateValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%    XCTAssertEqual(idx, idx2);
//%    XCTAssertEqual(value, kValues[idx]);
//%    XCTAssertNotEqual(stop, NULL);
//%    ++idx2;
//%  }];
//%  idx2 = 0;
//%  [array enumerateValuesWithOptions:NSEnumerationReverse
//%                         usingBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%    XCTAssertEqual(idx, (3 - idx2));
//%    XCTAssertEqual(value, kValues[idx]);
//%    XCTAssertNotEqual(stop, NULL);
//%    ++idx2;
//%  }];
//%  // Stopping the enumeration.
//%  idx2 = 0;
//%  [array enumerateValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%    XCTAssertEqual(idx, idx2);
//%    XCTAssertEqual(value, kValues[idx]);
//%    XCTAssertNotEqual(stop, NULL);
//%    if (idx2 == 1) *stop = YES;
//%    XCTAssertNotEqual(idx, 2U);
//%    XCTAssertNotEqual(idx, 3U);
//%    ++idx2;
//%  }];
//%  idx2 = 0;
//%  [array enumerateValuesWithOptions:NSEnumerationReverse
//%                         usingBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%    XCTAssertEqual(idx, (3 - idx2));
//%    XCTAssertEqual(value, kValues[idx]);
//%    XCTAssertNotEqual(stop, NULL);
//%    if (idx2 == 1) *stop = YES;
//%    XCTAssertNotEqual(idx, 1U);
//%    XCTAssertNotEqual(idx, 0U);
//%    ++idx2;
//%  }];
//%  // Ensure description doesn't choke.
//%  XCTAssertTrue(array.description.length > 10);
//%  [array release];
//%}
//%
//%- (void)testEquality {
//%  const TYPE kValues1[] = { VAL1, VAL2, VAL3 };
//%  const TYPE kValues2[] = { VAL1, VAL4, VAL3 };
//%  const TYPE kValues3[] = { VAL1, VAL2, VAL3, VAL4 };
//%  GPB##NAME##Array *array1 =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues1
//%            NAME$S                     count:GPBARRAYSIZE(kValues1)];
//%  XCTAssertNotNil(array1);
//%  GPB##NAME##Array *array1prime =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues1
//%            NAME$S                     count:GPBARRAYSIZE(kValues1)];
//%  XCTAssertNotNil(array1prime);
//%  GPB##NAME##Array *array2 =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues2
//%            NAME$S                     count:GPBARRAYSIZE(kValues2)];
//%  XCTAssertNotNil(array2);
//%  GPB##NAME##Array *array3 =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues3
//%            NAME$S                     count:GPBARRAYSIZE(kValues3)];
//%  XCTAssertNotNil(array3);
//%
//%  // Identity
//%  XCTAssertTrue([array1 isEqual:array1]);
//%  // Wrong type doesn't blow up.
//%  XCTAssertFalse([array1 isEqual:@"bogus"]);
//%  // 1/1Prime should be different objects, but equal.
//%  XCTAssertNotEqual(array1, array1prime);
//%  XCTAssertEqualObjects(array1, array1prime);
//%  // Equal, so they must have same hash.
//%  XCTAssertEqual([array1 hash], [array1prime hash]);
//%
//%  // 1/2/3 shouldn't be equal.
//%  XCTAssertNotEqualObjects(array1, array2);
//%  XCTAssertNotEqualObjects(array1, array3);
//%  XCTAssertNotEqualObjects(array2, array3);
//%
//%  [array1 release];
//%  [array1prime release];
//%  [array2 release];
//%  [array3 release];
//%}
//%
//%- (void)testCopy {
//%  const TYPE kValues[] = { VAL1, VAL2, VAL3, VAL4 };
//%  GPB##NAME##Array *array =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues
//%            NAME$S                     count:GPBARRAYSIZE(kValues)];
//%  XCTAssertNotNil(array);
//%
//%  GPB##NAME##Array *array2 = [array copy];
//%  XCTAssertNotNil(array2);
//%
//%  // Should be new object but equal.
//%  XCTAssertNotEqual(array, array2);
//%  XCTAssertEqualObjects(array, array2);
//%  [array2 release];
//%  [array release];
//%}
//%
//%- (void)testArrayFromArray {
//%  const TYPE kValues[] = { VAL1, VAL2, VAL3, VAL4 };
//%  GPB##NAME##Array *array =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues
//%            NAME$S                     count:GPBARRAYSIZE(kValues)];
//%  XCTAssertNotNil(array);
//%
//%  GPB##NAME##Array *array2 = [GPB##NAME##Array arrayWithValueArray:array];
//%  XCTAssertNotNil(array2);
//%
//%  // Should be new pointer, but equal objects.
//%  XCTAssertNotEqual(array, array2);
//%  XCTAssertEqualObjects(array, array2);
//%  [array release];
//%}
//%
//%- (void)testAdds {
//%  GPB##NAME##Array *array = [GPB##NAME##Array array];
//%  XCTAssertNotNil(array);
//%
//%  XCTAssertEqual(array.count, 0U);
//%  [array addValue:VAL1];
//%  XCTAssertEqual(array.count, 1U);
//%
//%  const TYPE kValues1[] = { VAL2, VAL3 };
//%  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
//%  XCTAssertEqual(array.count, 3U);
//%
//%  const TYPE kValues2[] = { VAL4, VAL1 };
//%  GPB##NAME##Array *array2 =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues2
//%            NAME$S                     count:GPBARRAYSIZE(kValues2)];
//%  XCTAssertNotNil(array2);
//%  [array add##HELPER##ValuesFromArray:array2];
//%  XCTAssertEqual(array.count, 5U);
//%
//%  // Zero/nil inputs do nothing.
//%  [array addValues:kValues1 count:0];
//%  XCTAssertEqual(array.count, 5U);
//%  [array addValues:NULL count:5];
//%  XCTAssertEqual(array.count, 5U);
//%
//%  XCTAssertEqual([array valueAtIndex:0], VAL1);
//%  XCTAssertEqual([array valueAtIndex:1], VAL2);
//%  XCTAssertEqual([array valueAtIndex:2], VAL3);
//%  XCTAssertEqual([array valueAtIndex:3], VAL4);
//%  XCTAssertEqual([array valueAtIndex:4], VAL1);
//%  [array2 release];
//%}
//%
//%- (void)testInsert {
//%  const TYPE kValues[] = { VAL1, VAL2, VAL3 };
//%  GPB##NAME##Array *array =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues
//%            NAME$S                     count:GPBARRAYSIZE(kValues)];
//%  XCTAssertNotNil(array);
//%  XCTAssertEqual(array.count, 3U);
//%
//%  // First
//%  [array insertValue:VAL4 atIndex:0];
//%  XCTAssertEqual(array.count, 4U);
//%
//%  // Middle
//%  [array insertValue:VAL4 atIndex:2];
//%  XCTAssertEqual(array.count, 5U);
//%
//%  // End
//%  [array insertValue:VAL4 atIndex:5];
//%  XCTAssertEqual(array.count, 6U);
//%
//%  // Too far.
//%  XCTAssertThrowsSpecificNamed([array insertValue:VAL4 atIndex:7],
//%                               NSException, NSRangeException);
//%
//%  XCTAssertEqual([array valueAtIndex:0], VAL4);
//%  XCTAssertEqual([array valueAtIndex:1], VAL1);
//%  XCTAssertEqual([array valueAtIndex:2], VAL4);
//%  XCTAssertEqual([array valueAtIndex:3], VAL2);
//%  XCTAssertEqual([array valueAtIndex:4], VAL3);
//%  XCTAssertEqual([array valueAtIndex:5], VAL4);
//%  [array release];
//%}
//%
//%- (void)testRemove {
//%  const TYPE kValues[] = { VAL4, VAL1, VAL2, VAL4, VAL3, VAL4 };
//%  GPB##NAME##Array *array =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues
//%            NAME$S                     count:GPBARRAYSIZE(kValues)];
//%  XCTAssertNotNil(array);
//%  XCTAssertEqual(array.count, 6U);
//%
//%  // First
//%  [array removeValueAtIndex:0];
//%  XCTAssertEqual(array.count, 5U);
//%  XCTAssertEqual([array valueAtIndex:0], VAL1);
//%
//%  // Middle
//%  [array removeValueAtIndex:2];
//%  XCTAssertEqual(array.count, 4U);
//%  XCTAssertEqual([array valueAtIndex:2], VAL3);
//%
//%  // End
//%  [array removeValueAtIndex:3];
//%  XCTAssertEqual(array.count, 3U);
//%
//%  XCTAssertEqual([array valueAtIndex:0], VAL1);
//%  XCTAssertEqual([array valueAtIndex:1], VAL2);
//%  XCTAssertEqual([array valueAtIndex:2], VAL3);
//%
//%  // Too far.
//%  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
//%                               NSException, NSRangeException);
//%
//%  [array removeAll];
//%  XCTAssertEqual(array.count, 0U);
//%  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
//%                               NSException, NSRangeException);
//%  [array release];
//%}
//%
//%- (void)testInplaceMutation {
//%  const TYPE kValues[] = { VAL1, VAL1, VAL3, VAL3 };
//%  GPB##NAME##Array *array =
//%      [[GPB##NAME##Array alloc] initWithValues:kValues
//%            NAME$S                     count:GPBARRAYSIZE(kValues)];
//%  XCTAssertNotNil(array);
//%
//%  [array replaceValueAtIndex:1 withValue:VAL2];
//%  [array replaceValueAtIndex:3 withValue:VAL4];
//%  XCTAssertEqual(array.count, 4U);
//%  XCTAssertEqual([array valueAtIndex:0], VAL1);
//%  XCTAssertEqual([array valueAtIndex:1], VAL2);
//%  XCTAssertEqual([array valueAtIndex:2], VAL3);
//%  XCTAssertEqual([array valueAtIndex:3], VAL4);
//%
//%  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:VAL4],
//%                               NSException, NSRangeException);
//%
//%  [array exchangeValueAtIndex:1 withValueAtIndex:3];
//%  XCTAssertEqual(array.count, 4U);
//%  XCTAssertEqual([array valueAtIndex:0], VAL1);
//%  XCTAssertEqual([array valueAtIndex:1], VAL4);
//%  XCTAssertEqual([array valueAtIndex:2], VAL3);
//%  XCTAssertEqual([array valueAtIndex:3], VAL2);
//%
//%  [array exchangeValueAtIndex:2 withValueAtIndex:0];
//%  XCTAssertEqual(array.count, 4U);
//%  XCTAssertEqual([array valueAtIndex:0], VAL3);
//%  XCTAssertEqual([array valueAtIndex:1], VAL4);
//%  XCTAssertEqual([array valueAtIndex:2], VAL1);
//%  XCTAssertEqual([array valueAtIndex:3], VAL2);
//%
//%  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
//%                               NSException, NSRangeException);
//%  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
//%                               NSException, NSRangeException);
//%  [array release];
//%}
//%
//%- (void)testInternalResizing {
//%  const TYPE kValues[] = { VAL1, VAL2, VAL3, VAL4 };
//%  GPB##NAME##Array *array =
//%      [GPB##NAME##Array arrayWithCapacity:GPBARRAYSIZE(kValues)];
//%  XCTAssertNotNil(array);
//%  [array addValues:kValues count:GPBARRAYSIZE(kValues)];
//%
//%  // Add/remove to trigger the intneral buffer to grow/shrink.
//%  for (int i = 0; i < 100; ++i) {
//%    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
//%  }
//%  XCTAssertEqual(array.count, 404U);
//%  for (int i = 0; i < 100; ++i) {
//%    [array removeValueAtIndex:(i * 2)];
//%  }
//%  XCTAssertEqual(array.count, 304U);
//%  for (int i = 0; i < 100; ++i) {
//%    [array insertValue:VAL4 atIndex:(i * 3)];
//%  }
//%  XCTAssertEqual(array.count, 404U);
//%  [array removeAll];
//%  XCTAssertEqual(array.count, 0U);
//%}
//%
//%@end
//%
//%PDDM-EXPAND ARRAY_TESTS(Int32, int32_t, 1, 2, 3, 4)
// This block of code is generated, do not edit it directly.

#pragma mark - Int32

@interface GPBInt32ArrayTests : XCTestCase
@end

@implementation GPBInt32ArrayTests

- (void)testEmpty {
  GPBInt32Array *array = [[GPBInt32Array alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused int32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused int32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBInt32Array *array = [GPBInt32Array arrayWithValue:1];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], 1);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 1);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 1);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const int32_t kValues[] = { 1, 2, 3, 4 };
  GPBInt32Array *array =
      [[GPBInt32Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 1);
  XCTAssertEqual([array valueAtIndex:1], 2);
  XCTAssertEqual([array valueAtIndex:2], 3);
  XCTAssertEqual([array valueAtIndex:3], 4);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const int32_t kValues1[] = { 1, 2, 3 };
  const int32_t kValues2[] = { 1, 4, 3 };
  const int32_t kValues3[] = { 1, 2, 3, 4 };
  GPBInt32Array *array1 =
      [[GPBInt32Array alloc] initWithValues:kValues1
                                      count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBInt32Array *array1prime =
      [[GPBInt32Array alloc] initWithValues:kValues1
                                      count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBInt32Array *array2 =
      [[GPBInt32Array alloc] initWithValues:kValues2
                                      count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBInt32Array *array3 =
      [[GPBInt32Array alloc] initWithValues:kValues3
                                      count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const int32_t kValues[] = { 1, 2, 3, 4 };
  GPBInt32Array *array =
      [[GPBInt32Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBInt32Array *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const int32_t kValues[] = { 1, 2, 3, 4 };
  GPBInt32Array *array =
      [[GPBInt32Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBInt32Array *array2 = [GPBInt32Array arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBInt32Array *array = [GPBInt32Array array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:1];
  XCTAssertEqual(array.count, 1U);

  const int32_t kValues1[] = { 2, 3 };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const int32_t kValues2[] = { 4, 1 };
  GPBInt32Array *array2 =
      [[GPBInt32Array alloc] initWithValues:kValues2
                                      count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], 1);
  XCTAssertEqual([array valueAtIndex:1], 2);
  XCTAssertEqual([array valueAtIndex:2], 3);
  XCTAssertEqual([array valueAtIndex:3], 4);
  XCTAssertEqual([array valueAtIndex:4], 1);
  [array2 release];
}

- (void)testInsert {
  const int32_t kValues[] = { 1, 2, 3 };
  GPBInt32Array *array =
      [[GPBInt32Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:4 atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:4 atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:4 atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:4 atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], 4);
  XCTAssertEqual([array valueAtIndex:1], 1);
  XCTAssertEqual([array valueAtIndex:2], 4);
  XCTAssertEqual([array valueAtIndex:3], 2);
  XCTAssertEqual([array valueAtIndex:4], 3);
  XCTAssertEqual([array valueAtIndex:5], 4);
  [array release];
}

- (void)testRemove {
  const int32_t kValues[] = { 4, 1, 2, 4, 3, 4 };
  GPBInt32Array *array =
      [[GPBInt32Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], 1);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], 3);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], 1);
  XCTAssertEqual([array valueAtIndex:1], 2);
  XCTAssertEqual([array valueAtIndex:2], 3);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const int32_t kValues[] = { 1, 1, 3, 3 };
  GPBInt32Array *array =
      [[GPBInt32Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:2];
  [array replaceValueAtIndex:3 withValue:4];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 1);
  XCTAssertEqual([array valueAtIndex:1], 2);
  XCTAssertEqual([array valueAtIndex:2], 3);
  XCTAssertEqual([array valueAtIndex:3], 4);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:4],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 1);
  XCTAssertEqual([array valueAtIndex:1], 4);
  XCTAssertEqual([array valueAtIndex:2], 3);
  XCTAssertEqual([array valueAtIndex:3], 2);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 3);
  XCTAssertEqual([array valueAtIndex:1], 4);
  XCTAssertEqual([array valueAtIndex:2], 1);
  XCTAssertEqual([array valueAtIndex:3], 2);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const int32_t kValues[] = { 1, 2, 3, 4 };
  GPBInt32Array *array =
      [GPBInt32Array arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:4 atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND ARRAY_TESTS(UInt32, uint32_t, 11U, 12U, 13U, 14U)
// This block of code is generated, do not edit it directly.

#pragma mark - UInt32

@interface GPBUInt32ArrayTests : XCTestCase
@end

@implementation GPBUInt32ArrayTests

- (void)testEmpty {
  GPBUInt32Array *array = [[GPBUInt32Array alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused uint32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused uint32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBUInt32Array *array = [GPBUInt32Array arrayWithValue:11U];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], 11U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 11U);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 11U);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const uint32_t kValues[] = { 11U, 12U, 13U, 14U };
  GPBUInt32Array *array =
      [[GPBUInt32Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 11U);
  XCTAssertEqual([array valueAtIndex:1], 12U);
  XCTAssertEqual([array valueAtIndex:2], 13U);
  XCTAssertEqual([array valueAtIndex:3], 14U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const uint32_t kValues1[] = { 11U, 12U, 13U };
  const uint32_t kValues2[] = { 11U, 14U, 13U };
  const uint32_t kValues3[] = { 11U, 12U, 13U, 14U };
  GPBUInt32Array *array1 =
      [[GPBUInt32Array alloc] initWithValues:kValues1
                                       count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBUInt32Array *array1prime =
      [[GPBUInt32Array alloc] initWithValues:kValues1
                                       count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBUInt32Array *array2 =
      [[GPBUInt32Array alloc] initWithValues:kValues2
                                       count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBUInt32Array *array3 =
      [[GPBUInt32Array alloc] initWithValues:kValues3
                                       count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const uint32_t kValues[] = { 11U, 12U, 13U, 14U };
  GPBUInt32Array *array =
      [[GPBUInt32Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBUInt32Array *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const uint32_t kValues[] = { 11U, 12U, 13U, 14U };
  GPBUInt32Array *array =
      [[GPBUInt32Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBUInt32Array *array2 = [GPBUInt32Array arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBUInt32Array *array = [GPBUInt32Array array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:11U];
  XCTAssertEqual(array.count, 1U);

  const uint32_t kValues1[] = { 12U, 13U };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const uint32_t kValues2[] = { 14U, 11U };
  GPBUInt32Array *array2 =
      [[GPBUInt32Array alloc] initWithValues:kValues2
                                       count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], 11U);
  XCTAssertEqual([array valueAtIndex:1], 12U);
  XCTAssertEqual([array valueAtIndex:2], 13U);
  XCTAssertEqual([array valueAtIndex:3], 14U);
  XCTAssertEqual([array valueAtIndex:4], 11U);
  [array2 release];
}

- (void)testInsert {
  const uint32_t kValues[] = { 11U, 12U, 13U };
  GPBUInt32Array *array =
      [[GPBUInt32Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:14U atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:14U atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:14U atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:14U atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], 14U);
  XCTAssertEqual([array valueAtIndex:1], 11U);
  XCTAssertEqual([array valueAtIndex:2], 14U);
  XCTAssertEqual([array valueAtIndex:3], 12U);
  XCTAssertEqual([array valueAtIndex:4], 13U);
  XCTAssertEqual([array valueAtIndex:5], 14U);
  [array release];
}

- (void)testRemove {
  const uint32_t kValues[] = { 14U, 11U, 12U, 14U, 13U, 14U };
  GPBUInt32Array *array =
      [[GPBUInt32Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], 11U);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], 13U);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], 11U);
  XCTAssertEqual([array valueAtIndex:1], 12U);
  XCTAssertEqual([array valueAtIndex:2], 13U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const uint32_t kValues[] = { 11U, 11U, 13U, 13U };
  GPBUInt32Array *array =
      [[GPBUInt32Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:12U];
  [array replaceValueAtIndex:3 withValue:14U];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 11U);
  XCTAssertEqual([array valueAtIndex:1], 12U);
  XCTAssertEqual([array valueAtIndex:2], 13U);
  XCTAssertEqual([array valueAtIndex:3], 14U);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:14U],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 11U);
  XCTAssertEqual([array valueAtIndex:1], 14U);
  XCTAssertEqual([array valueAtIndex:2], 13U);
  XCTAssertEqual([array valueAtIndex:3], 12U);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 13U);
  XCTAssertEqual([array valueAtIndex:1], 14U);
  XCTAssertEqual([array valueAtIndex:2], 11U);
  XCTAssertEqual([array valueAtIndex:3], 12U);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const uint32_t kValues[] = { 11U, 12U, 13U, 14U };
  GPBUInt32Array *array =
      [GPBUInt32Array arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:14U atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND ARRAY_TESTS(Int64, int64_t, 31LL, 32LL, 33LL, 34LL)
// This block of code is generated, do not edit it directly.

#pragma mark - Int64

@interface GPBInt64ArrayTests : XCTestCase
@end

@implementation GPBInt64ArrayTests

- (void)testEmpty {
  GPBInt64Array *array = [[GPBInt64Array alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused int64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused int64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBInt64Array *array = [GPBInt64Array arrayWithValue:31LL];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], 31LL);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 31LL);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 31LL);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const int64_t kValues[] = { 31LL, 32LL, 33LL, 34LL };
  GPBInt64Array *array =
      [[GPBInt64Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 31LL);
  XCTAssertEqual([array valueAtIndex:1], 32LL);
  XCTAssertEqual([array valueAtIndex:2], 33LL);
  XCTAssertEqual([array valueAtIndex:3], 34LL);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const int64_t kValues1[] = { 31LL, 32LL, 33LL };
  const int64_t kValues2[] = { 31LL, 34LL, 33LL };
  const int64_t kValues3[] = { 31LL, 32LL, 33LL, 34LL };
  GPBInt64Array *array1 =
      [[GPBInt64Array alloc] initWithValues:kValues1
                                      count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBInt64Array *array1prime =
      [[GPBInt64Array alloc] initWithValues:kValues1
                                      count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBInt64Array *array2 =
      [[GPBInt64Array alloc] initWithValues:kValues2
                                      count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBInt64Array *array3 =
      [[GPBInt64Array alloc] initWithValues:kValues3
                                      count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const int64_t kValues[] = { 31LL, 32LL, 33LL, 34LL };
  GPBInt64Array *array =
      [[GPBInt64Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBInt64Array *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const int64_t kValues[] = { 31LL, 32LL, 33LL, 34LL };
  GPBInt64Array *array =
      [[GPBInt64Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBInt64Array *array2 = [GPBInt64Array arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBInt64Array *array = [GPBInt64Array array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:31LL];
  XCTAssertEqual(array.count, 1U);

  const int64_t kValues1[] = { 32LL, 33LL };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const int64_t kValues2[] = { 34LL, 31LL };
  GPBInt64Array *array2 =
      [[GPBInt64Array alloc] initWithValues:kValues2
                                      count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], 31LL);
  XCTAssertEqual([array valueAtIndex:1], 32LL);
  XCTAssertEqual([array valueAtIndex:2], 33LL);
  XCTAssertEqual([array valueAtIndex:3], 34LL);
  XCTAssertEqual([array valueAtIndex:4], 31LL);
  [array2 release];
}

- (void)testInsert {
  const int64_t kValues[] = { 31LL, 32LL, 33LL };
  GPBInt64Array *array =
      [[GPBInt64Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:34LL atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:34LL atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:34LL atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:34LL atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], 34LL);
  XCTAssertEqual([array valueAtIndex:1], 31LL);
  XCTAssertEqual([array valueAtIndex:2], 34LL);
  XCTAssertEqual([array valueAtIndex:3], 32LL);
  XCTAssertEqual([array valueAtIndex:4], 33LL);
  XCTAssertEqual([array valueAtIndex:5], 34LL);
  [array release];
}

- (void)testRemove {
  const int64_t kValues[] = { 34LL, 31LL, 32LL, 34LL, 33LL, 34LL };
  GPBInt64Array *array =
      [[GPBInt64Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], 31LL);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], 33LL);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], 31LL);
  XCTAssertEqual([array valueAtIndex:1], 32LL);
  XCTAssertEqual([array valueAtIndex:2], 33LL);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const int64_t kValues[] = { 31LL, 31LL, 33LL, 33LL };
  GPBInt64Array *array =
      [[GPBInt64Array alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:32LL];
  [array replaceValueAtIndex:3 withValue:34LL];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 31LL);
  XCTAssertEqual([array valueAtIndex:1], 32LL);
  XCTAssertEqual([array valueAtIndex:2], 33LL);
  XCTAssertEqual([array valueAtIndex:3], 34LL);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:34LL],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 31LL);
  XCTAssertEqual([array valueAtIndex:1], 34LL);
  XCTAssertEqual([array valueAtIndex:2], 33LL);
  XCTAssertEqual([array valueAtIndex:3], 32LL);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 33LL);
  XCTAssertEqual([array valueAtIndex:1], 34LL);
  XCTAssertEqual([array valueAtIndex:2], 31LL);
  XCTAssertEqual([array valueAtIndex:3], 32LL);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const int64_t kValues[] = { 31LL, 32LL, 33LL, 34LL };
  GPBInt64Array *array =
      [GPBInt64Array arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:34LL atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND ARRAY_TESTS(UInt64, uint64_t, 41ULL, 42ULL, 43ULL, 44ULL)
// This block of code is generated, do not edit it directly.

#pragma mark - UInt64

@interface GPBUInt64ArrayTests : XCTestCase
@end

@implementation GPBUInt64ArrayTests

- (void)testEmpty {
  GPBUInt64Array *array = [[GPBUInt64Array alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused uint64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused uint64_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBUInt64Array *array = [GPBUInt64Array arrayWithValue:41ULL];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], 41ULL);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 41ULL);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 41ULL);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const uint64_t kValues[] = { 41ULL, 42ULL, 43ULL, 44ULL };
  GPBUInt64Array *array =
      [[GPBUInt64Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 41ULL);
  XCTAssertEqual([array valueAtIndex:1], 42ULL);
  XCTAssertEqual([array valueAtIndex:2], 43ULL);
  XCTAssertEqual([array valueAtIndex:3], 44ULL);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const uint64_t kValues1[] = { 41ULL, 42ULL, 43ULL };
  const uint64_t kValues2[] = { 41ULL, 44ULL, 43ULL };
  const uint64_t kValues3[] = { 41ULL, 42ULL, 43ULL, 44ULL };
  GPBUInt64Array *array1 =
      [[GPBUInt64Array alloc] initWithValues:kValues1
                                       count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBUInt64Array *array1prime =
      [[GPBUInt64Array alloc] initWithValues:kValues1
                                       count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBUInt64Array *array2 =
      [[GPBUInt64Array alloc] initWithValues:kValues2
                                       count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBUInt64Array *array3 =
      [[GPBUInt64Array alloc] initWithValues:kValues3
                                       count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const uint64_t kValues[] = { 41ULL, 42ULL, 43ULL, 44ULL };
  GPBUInt64Array *array =
      [[GPBUInt64Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBUInt64Array *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const uint64_t kValues[] = { 41ULL, 42ULL, 43ULL, 44ULL };
  GPBUInt64Array *array =
      [[GPBUInt64Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBUInt64Array *array2 = [GPBUInt64Array arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBUInt64Array *array = [GPBUInt64Array array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:41ULL];
  XCTAssertEqual(array.count, 1U);

  const uint64_t kValues1[] = { 42ULL, 43ULL };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const uint64_t kValues2[] = { 44ULL, 41ULL };
  GPBUInt64Array *array2 =
      [[GPBUInt64Array alloc] initWithValues:kValues2
                                       count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], 41ULL);
  XCTAssertEqual([array valueAtIndex:1], 42ULL);
  XCTAssertEqual([array valueAtIndex:2], 43ULL);
  XCTAssertEqual([array valueAtIndex:3], 44ULL);
  XCTAssertEqual([array valueAtIndex:4], 41ULL);
  [array2 release];
}

- (void)testInsert {
  const uint64_t kValues[] = { 41ULL, 42ULL, 43ULL };
  GPBUInt64Array *array =
      [[GPBUInt64Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:44ULL atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:44ULL atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:44ULL atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:44ULL atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], 44ULL);
  XCTAssertEqual([array valueAtIndex:1], 41ULL);
  XCTAssertEqual([array valueAtIndex:2], 44ULL);
  XCTAssertEqual([array valueAtIndex:3], 42ULL);
  XCTAssertEqual([array valueAtIndex:4], 43ULL);
  XCTAssertEqual([array valueAtIndex:5], 44ULL);
  [array release];
}

- (void)testRemove {
  const uint64_t kValues[] = { 44ULL, 41ULL, 42ULL, 44ULL, 43ULL, 44ULL };
  GPBUInt64Array *array =
      [[GPBUInt64Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], 41ULL);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], 43ULL);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], 41ULL);
  XCTAssertEqual([array valueAtIndex:1], 42ULL);
  XCTAssertEqual([array valueAtIndex:2], 43ULL);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const uint64_t kValues[] = { 41ULL, 41ULL, 43ULL, 43ULL };
  GPBUInt64Array *array =
      [[GPBUInt64Array alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:42ULL];
  [array replaceValueAtIndex:3 withValue:44ULL];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 41ULL);
  XCTAssertEqual([array valueAtIndex:1], 42ULL);
  XCTAssertEqual([array valueAtIndex:2], 43ULL);
  XCTAssertEqual([array valueAtIndex:3], 44ULL);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:44ULL],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 41ULL);
  XCTAssertEqual([array valueAtIndex:1], 44ULL);
  XCTAssertEqual([array valueAtIndex:2], 43ULL);
  XCTAssertEqual([array valueAtIndex:3], 42ULL);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 43ULL);
  XCTAssertEqual([array valueAtIndex:1], 44ULL);
  XCTAssertEqual([array valueAtIndex:2], 41ULL);
  XCTAssertEqual([array valueAtIndex:3], 42ULL);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const uint64_t kValues[] = { 41ULL, 42ULL, 43ULL, 44ULL };
  GPBUInt64Array *array =
      [GPBUInt64Array arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:44ULL atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND ARRAY_TESTS(Float, float, 51.f, 52.f, 53.f, 54.f)
// This block of code is generated, do not edit it directly.

#pragma mark - Float

@interface GPBFloatArrayTests : XCTestCase
@end

@implementation GPBFloatArrayTests

- (void)testEmpty {
  GPBFloatArray *array = [[GPBFloatArray alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused float value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused float value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBFloatArray *array = [GPBFloatArray arrayWithValue:51.f];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], 51.f);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(float value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 51.f);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(float value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 51.f);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const float kValues[] = { 51.f, 52.f, 53.f, 54.f };
  GPBFloatArray *array =
      [[GPBFloatArray alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 51.f);
  XCTAssertEqual([array valueAtIndex:1], 52.f);
  XCTAssertEqual([array valueAtIndex:2], 53.f);
  XCTAssertEqual([array valueAtIndex:3], 54.f);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(float value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(float value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(float value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(float value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const float kValues1[] = { 51.f, 52.f, 53.f };
  const float kValues2[] = { 51.f, 54.f, 53.f };
  const float kValues3[] = { 51.f, 52.f, 53.f, 54.f };
  GPBFloatArray *array1 =
      [[GPBFloatArray alloc] initWithValues:kValues1
                                      count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBFloatArray *array1prime =
      [[GPBFloatArray alloc] initWithValues:kValues1
                                      count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBFloatArray *array2 =
      [[GPBFloatArray alloc] initWithValues:kValues2
                                      count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBFloatArray *array3 =
      [[GPBFloatArray alloc] initWithValues:kValues3
                                      count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const float kValues[] = { 51.f, 52.f, 53.f, 54.f };
  GPBFloatArray *array =
      [[GPBFloatArray alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBFloatArray *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const float kValues[] = { 51.f, 52.f, 53.f, 54.f };
  GPBFloatArray *array =
      [[GPBFloatArray alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBFloatArray *array2 = [GPBFloatArray arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBFloatArray *array = [GPBFloatArray array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:51.f];
  XCTAssertEqual(array.count, 1U);

  const float kValues1[] = { 52.f, 53.f };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const float kValues2[] = { 54.f, 51.f };
  GPBFloatArray *array2 =
      [[GPBFloatArray alloc] initWithValues:kValues2
                                      count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], 51.f);
  XCTAssertEqual([array valueAtIndex:1], 52.f);
  XCTAssertEqual([array valueAtIndex:2], 53.f);
  XCTAssertEqual([array valueAtIndex:3], 54.f);
  XCTAssertEqual([array valueAtIndex:4], 51.f);
  [array2 release];
}

- (void)testInsert {
  const float kValues[] = { 51.f, 52.f, 53.f };
  GPBFloatArray *array =
      [[GPBFloatArray alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:54.f atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:54.f atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:54.f atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:54.f atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], 54.f);
  XCTAssertEqual([array valueAtIndex:1], 51.f);
  XCTAssertEqual([array valueAtIndex:2], 54.f);
  XCTAssertEqual([array valueAtIndex:3], 52.f);
  XCTAssertEqual([array valueAtIndex:4], 53.f);
  XCTAssertEqual([array valueAtIndex:5], 54.f);
  [array release];
}

- (void)testRemove {
  const float kValues[] = { 54.f, 51.f, 52.f, 54.f, 53.f, 54.f };
  GPBFloatArray *array =
      [[GPBFloatArray alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], 51.f);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], 53.f);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], 51.f);
  XCTAssertEqual([array valueAtIndex:1], 52.f);
  XCTAssertEqual([array valueAtIndex:2], 53.f);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const float kValues[] = { 51.f, 51.f, 53.f, 53.f };
  GPBFloatArray *array =
      [[GPBFloatArray alloc] initWithValues:kValues
                                      count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:52.f];
  [array replaceValueAtIndex:3 withValue:54.f];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 51.f);
  XCTAssertEqual([array valueAtIndex:1], 52.f);
  XCTAssertEqual([array valueAtIndex:2], 53.f);
  XCTAssertEqual([array valueAtIndex:3], 54.f);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:54.f],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 51.f);
  XCTAssertEqual([array valueAtIndex:1], 54.f);
  XCTAssertEqual([array valueAtIndex:2], 53.f);
  XCTAssertEqual([array valueAtIndex:3], 52.f);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 53.f);
  XCTAssertEqual([array valueAtIndex:1], 54.f);
  XCTAssertEqual([array valueAtIndex:2], 51.f);
  XCTAssertEqual([array valueAtIndex:3], 52.f);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const float kValues[] = { 51.f, 52.f, 53.f, 54.f };
  GPBFloatArray *array =
      [GPBFloatArray arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:54.f atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND ARRAY_TESTS(Double, double, 61., 62., 63., 64.)
// This block of code is generated, do not edit it directly.

#pragma mark - Double

@interface GPBDoubleArrayTests : XCTestCase
@end

@implementation GPBDoubleArrayTests

- (void)testEmpty {
  GPBDoubleArray *array = [[GPBDoubleArray alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused double value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused double value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBDoubleArray *array = [GPBDoubleArray arrayWithValue:61.];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], 61.);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(double value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 61.);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(double value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 61.);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const double kValues[] = { 61., 62., 63., 64. };
  GPBDoubleArray *array =
      [[GPBDoubleArray alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 61.);
  XCTAssertEqual([array valueAtIndex:1], 62.);
  XCTAssertEqual([array valueAtIndex:2], 63.);
  XCTAssertEqual([array valueAtIndex:3], 64.);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(double value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(double value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(double value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(double value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const double kValues1[] = { 61., 62., 63. };
  const double kValues2[] = { 61., 64., 63. };
  const double kValues3[] = { 61., 62., 63., 64. };
  GPBDoubleArray *array1 =
      [[GPBDoubleArray alloc] initWithValues:kValues1
                                       count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBDoubleArray *array1prime =
      [[GPBDoubleArray alloc] initWithValues:kValues1
                                       count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBDoubleArray *array2 =
      [[GPBDoubleArray alloc] initWithValues:kValues2
                                       count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBDoubleArray *array3 =
      [[GPBDoubleArray alloc] initWithValues:kValues3
                                       count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const double kValues[] = { 61., 62., 63., 64. };
  GPBDoubleArray *array =
      [[GPBDoubleArray alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBDoubleArray *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const double kValues[] = { 61., 62., 63., 64. };
  GPBDoubleArray *array =
      [[GPBDoubleArray alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBDoubleArray *array2 = [GPBDoubleArray arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBDoubleArray *array = [GPBDoubleArray array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:61.];
  XCTAssertEqual(array.count, 1U);

  const double kValues1[] = { 62., 63. };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const double kValues2[] = { 64., 61. };
  GPBDoubleArray *array2 =
      [[GPBDoubleArray alloc] initWithValues:kValues2
                                       count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], 61.);
  XCTAssertEqual([array valueAtIndex:1], 62.);
  XCTAssertEqual([array valueAtIndex:2], 63.);
  XCTAssertEqual([array valueAtIndex:3], 64.);
  XCTAssertEqual([array valueAtIndex:4], 61.);
  [array2 release];
}

- (void)testInsert {
  const double kValues[] = { 61., 62., 63. };
  GPBDoubleArray *array =
      [[GPBDoubleArray alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:64. atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:64. atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:64. atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:64. atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], 64.);
  XCTAssertEqual([array valueAtIndex:1], 61.);
  XCTAssertEqual([array valueAtIndex:2], 64.);
  XCTAssertEqual([array valueAtIndex:3], 62.);
  XCTAssertEqual([array valueAtIndex:4], 63.);
  XCTAssertEqual([array valueAtIndex:5], 64.);
  [array release];
}

- (void)testRemove {
  const double kValues[] = { 64., 61., 62., 64., 63., 64. };
  GPBDoubleArray *array =
      [[GPBDoubleArray alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], 61.);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], 63.);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], 61.);
  XCTAssertEqual([array valueAtIndex:1], 62.);
  XCTAssertEqual([array valueAtIndex:2], 63.);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const double kValues[] = { 61., 61., 63., 63. };
  GPBDoubleArray *array =
      [[GPBDoubleArray alloc] initWithValues:kValues
                                       count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:62.];
  [array replaceValueAtIndex:3 withValue:64.];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 61.);
  XCTAssertEqual([array valueAtIndex:1], 62.);
  XCTAssertEqual([array valueAtIndex:2], 63.);
  XCTAssertEqual([array valueAtIndex:3], 64.);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:64.],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 61.);
  XCTAssertEqual([array valueAtIndex:1], 64.);
  XCTAssertEqual([array valueAtIndex:2], 63.);
  XCTAssertEqual([array valueAtIndex:3], 62.);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 63.);
  XCTAssertEqual([array valueAtIndex:1], 64.);
  XCTAssertEqual([array valueAtIndex:2], 61.);
  XCTAssertEqual([array valueAtIndex:3], 62.);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const double kValues[] = { 61., 62., 63., 64. };
  GPBDoubleArray *array =
      [GPBDoubleArray arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:64. atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND ARRAY_TESTS(Bool, BOOL, TRUE, TRUE, FALSE, FALSE)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool

@interface GPBBoolArrayTests : XCTestCase
@end

@implementation GPBBoolArrayTests

- (void)testEmpty {
  GPBBoolArray *array = [[GPBBoolArray alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused BOOL value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused BOOL value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBBoolArray *array = [GPBBoolArray arrayWithValue:TRUE];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], TRUE);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, TRUE);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, TRUE);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const BOOL kValues[] = { TRUE, TRUE, FALSE, FALSE };
  GPBBoolArray *array =
      [[GPBBoolArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], TRUE);
  XCTAssertEqual([array valueAtIndex:1], TRUE);
  XCTAssertEqual([array valueAtIndex:2], FALSE);
  XCTAssertEqual([array valueAtIndex:3], FALSE);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const BOOL kValues1[] = { TRUE, TRUE, FALSE };
  const BOOL kValues2[] = { TRUE, FALSE, FALSE };
  const BOOL kValues3[] = { TRUE, TRUE, FALSE, FALSE };
  GPBBoolArray *array1 =
      [[GPBBoolArray alloc] initWithValues:kValues1
                                     count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBBoolArray *array1prime =
      [[GPBBoolArray alloc] initWithValues:kValues1
                                     count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBBoolArray *array2 =
      [[GPBBoolArray alloc] initWithValues:kValues2
                                     count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBBoolArray *array3 =
      [[GPBBoolArray alloc] initWithValues:kValues3
                                     count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const BOOL kValues[] = { TRUE, TRUE, FALSE, FALSE };
  GPBBoolArray *array =
      [[GPBBoolArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBBoolArray *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const BOOL kValues[] = { TRUE, TRUE, FALSE, FALSE };
  GPBBoolArray *array =
      [[GPBBoolArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBBoolArray *array2 = [GPBBoolArray arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBBoolArray *array = [GPBBoolArray array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:TRUE];
  XCTAssertEqual(array.count, 1U);

  const BOOL kValues1[] = { TRUE, FALSE };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const BOOL kValues2[] = { FALSE, TRUE };
  GPBBoolArray *array2 =
      [[GPBBoolArray alloc] initWithValues:kValues2
                                     count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], TRUE);
  XCTAssertEqual([array valueAtIndex:1], TRUE);
  XCTAssertEqual([array valueAtIndex:2], FALSE);
  XCTAssertEqual([array valueAtIndex:3], FALSE);
  XCTAssertEqual([array valueAtIndex:4], TRUE);
  [array2 release];
}

- (void)testInsert {
  const BOOL kValues[] = { TRUE, TRUE, FALSE };
  GPBBoolArray *array =
      [[GPBBoolArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:FALSE atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:FALSE atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:FALSE atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:FALSE atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], FALSE);
  XCTAssertEqual([array valueAtIndex:1], TRUE);
  XCTAssertEqual([array valueAtIndex:2], FALSE);
  XCTAssertEqual([array valueAtIndex:3], TRUE);
  XCTAssertEqual([array valueAtIndex:4], FALSE);
  XCTAssertEqual([array valueAtIndex:5], FALSE);
  [array release];
}

- (void)testRemove {
  const BOOL kValues[] = { FALSE, TRUE, TRUE, FALSE, FALSE, FALSE };
  GPBBoolArray *array =
      [[GPBBoolArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], TRUE);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], FALSE);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], TRUE);
  XCTAssertEqual([array valueAtIndex:1], TRUE);
  XCTAssertEqual([array valueAtIndex:2], FALSE);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const BOOL kValues[] = { TRUE, TRUE, FALSE, FALSE };
  GPBBoolArray *array =
      [[GPBBoolArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:TRUE];
  [array replaceValueAtIndex:3 withValue:FALSE];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], TRUE);
  XCTAssertEqual([array valueAtIndex:1], TRUE);
  XCTAssertEqual([array valueAtIndex:2], FALSE);
  XCTAssertEqual([array valueAtIndex:3], FALSE);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:FALSE],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], TRUE);
  XCTAssertEqual([array valueAtIndex:1], FALSE);
  XCTAssertEqual([array valueAtIndex:2], FALSE);
  XCTAssertEqual([array valueAtIndex:3], TRUE);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], FALSE);
  XCTAssertEqual([array valueAtIndex:1], FALSE);
  XCTAssertEqual([array valueAtIndex:2], TRUE);
  XCTAssertEqual([array valueAtIndex:3], TRUE);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const BOOL kValues[] = { TRUE, TRUE, FALSE, FALSE };
  GPBBoolArray *array =
      [GPBBoolArray arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:FALSE atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND ARRAY_TESTS2(Enum, int32_t, 71, 72, 73, 74, Raw)
// This block of code is generated, do not edit it directly.

#pragma mark - Enum

@interface GPBEnumArrayTests : XCTestCase
@end

@implementation GPBEnumArrayTests

- (void)testEmpty {
  GPBEnumArray *array = [[GPBEnumArray alloc] init];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:0], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(__unused int32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(__unused int32_t value, __unused NSUInteger idx, __unused BOOL *stop) {
    XCTFail(@"Shouldn't get here!");
  }];
  [array release];
}

- (void)testOne {
  GPBEnumArray *array = [GPBEnumArray arrayWithValue:71];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 1U);
  XCTAssertEqual([array valueAtIndex:0], 71);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:1], NSException, NSRangeException);
  [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 71);
    XCTAssertNotEqual(stop, NULL);
  }];
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, 0U);
    XCTAssertEqual(value, 71);
    XCTAssertNotEqual(stop, NULL);
  }];
}

- (void)testBasics {
  static const int32_t kValues[] = { 71, 72, 73, 74 };
  GPBEnumArray *array =
      [[GPBEnumArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 71);
  XCTAssertEqual([array valueAtIndex:1], 72);
  XCTAssertEqual([array valueAtIndex:2], 73);
  XCTAssertEqual([array valueAtIndex:3], 74);
  XCTAssertThrowsSpecificNamed([array valueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, (3 - idx2));
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 1U);
    XCTAssertNotEqual(idx, 0U);
    ++idx2;
  }];
  // Ensure description doesn't choke.
  XCTAssertTrue(array.description.length > 10);
  [array release];
}

- (void)testEquality {
  const int32_t kValues1[] = { 71, 72, 73 };
  const int32_t kValues2[] = { 71, 74, 73 };
  const int32_t kValues3[] = { 71, 72, 73, 74 };
  GPBEnumArray *array1 =
      [[GPBEnumArray alloc] initWithValues:kValues1
                                     count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBEnumArray *array1prime =
      [[GPBEnumArray alloc] initWithValues:kValues1
                                     count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBEnumArray *array2 =
      [[GPBEnumArray alloc] initWithValues:kValues2
                                     count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBEnumArray *array3 =
      [[GPBEnumArray alloc] initWithValues:kValues3
                                     count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // Identity
  XCTAssertTrue([array1 isEqual:array1]);
  // Wrong type doesn't blow up.
  XCTAssertFalse([array1 isEqual:@"bogus"]);
  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const int32_t kValues[] = { 71, 72, 73, 74 };
  GPBEnumArray *array =
      [[GPBEnumArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBEnumArray *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const int32_t kValues[] = { 71, 72, 73, 74 };
  GPBEnumArray *array =
      [[GPBEnumArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBEnumArray *array2 = [GPBEnumArray arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  [array release];
}

- (void)testAdds {
  GPBEnumArray *array = [GPBEnumArray array];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addValue:71];
  XCTAssertEqual(array.count, 1U);

  const int32_t kValues1[] = { 72, 73 };
  [array addValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const int32_t kValues2[] = { 74, 71 };
  GPBEnumArray *array2 =
      [[GPBEnumArray alloc] initWithValues:kValues2
                                     count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addRawValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  // Zero/nil inputs do nothing.
  [array addValues:kValues1 count:0];
  XCTAssertEqual(array.count, 5U);
  [array addValues:NULL count:5];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array valueAtIndex:0], 71);
  XCTAssertEqual([array valueAtIndex:1], 72);
  XCTAssertEqual([array valueAtIndex:2], 73);
  XCTAssertEqual([array valueAtIndex:3], 74);
  XCTAssertEqual([array valueAtIndex:4], 71);
  [array2 release];
}

- (void)testInsert {
  const int32_t kValues[] = { 71, 72, 73 };
  GPBEnumArray *array =
      [[GPBEnumArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertValue:74 atIndex:0];
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertValue:74 atIndex:2];
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertValue:74 atIndex:5];
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertValue:74 atIndex:7],
                               NSException, NSRangeException);

  XCTAssertEqual([array valueAtIndex:0], 74);
  XCTAssertEqual([array valueAtIndex:1], 71);
  XCTAssertEqual([array valueAtIndex:2], 74);
  XCTAssertEqual([array valueAtIndex:3], 72);
  XCTAssertEqual([array valueAtIndex:4], 73);
  XCTAssertEqual([array valueAtIndex:5], 74);
  [array release];
}

- (void)testRemove {
  const int32_t kValues[] = { 74, 71, 72, 74, 73, 74 };
  GPBEnumArray *array =
      [[GPBEnumArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 6U);

  // First
  [array removeValueAtIndex:0];
  XCTAssertEqual(array.count, 5U);
  XCTAssertEqual([array valueAtIndex:0], 71);

  // Middle
  [array removeValueAtIndex:2];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:2], 73);

  // End
  [array removeValueAtIndex:3];
  XCTAssertEqual(array.count, 3U);

  XCTAssertEqual([array valueAtIndex:0], 71);
  XCTAssertEqual([array valueAtIndex:1], 72);
  XCTAssertEqual([array valueAtIndex:2], 73);

  // Too far.
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:3],
                               NSException, NSRangeException);

  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  XCTAssertThrowsSpecificNamed([array removeValueAtIndex:0],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInplaceMutation {
  const int32_t kValues[] = { 71, 71, 73, 73 };
  GPBEnumArray *array =
      [[GPBEnumArray alloc] initWithValues:kValues
                                     count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withValue:72];
  [array replaceValueAtIndex:3 withValue:74];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 71);
  XCTAssertEqual([array valueAtIndex:1], 72);
  XCTAssertEqual([array valueAtIndex:2], 73);
  XCTAssertEqual([array valueAtIndex:3], 74);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withValue:74],
                               NSException, NSRangeException);

  [array exchangeValueAtIndex:1 withValueAtIndex:3];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 71);
  XCTAssertEqual([array valueAtIndex:1], 74);
  XCTAssertEqual([array valueAtIndex:2], 73);
  XCTAssertEqual([array valueAtIndex:3], 72);

  [array exchangeValueAtIndex:2 withValueAtIndex:0];
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 73);
  XCTAssertEqual([array valueAtIndex:1], 74);
  XCTAssertEqual([array valueAtIndex:2], 71);
  XCTAssertEqual([array valueAtIndex:3], 72);

  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:4 withValueAtIndex:1],
                               NSException, NSRangeException);
  XCTAssertThrowsSpecificNamed([array exchangeValueAtIndex:1 withValueAtIndex:4],
                               NSException, NSRangeException);
  [array release];
}

- (void)testInternalResizing {
  const int32_t kValues[] = { 71, 72, 73, 74 };
  GPBEnumArray *array =
      [GPBEnumArray arrayWithCapacity:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  [array addValues:kValues count:GPBARRAYSIZE(kValues)];

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertValue:74 atIndex:(i * 3)];
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
}

@end

//%PDDM-EXPAND-END (8 expansions)

// clang-format on

#pragma mark - Non macro-based Enum tests

// These are hand written tests to cover the verification and raw methods.

@interface GPBEnumArrayCustomTests : XCTestCase
@end

@implementation GPBEnumArrayCustomTests

- (void)testRawBasics {
  static const int32_t kValues[] = {71, 272, 73, 374};
  static const int32_t kValuesFiltered[] = {71, kGPBUnrecognizedEnumeratorValue, 73,
                                            kGPBUnrecognizedEnumeratorValue};
  XCTAssertEqual(GPBARRAYSIZE(kValues), GPBARRAYSIZE(kValuesFiltered));
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                               rawValues:kValues
                                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 4U);
  GPBEnumValidationFunc func = TestingEnum_IsValidValue;
  XCTAssertEqual(array.validationFunc, func);
  XCTAssertEqual([array rawValueAtIndex:0], 71);
  XCTAssertEqual([array rawValueAtIndex:1], 272);
  XCTAssertEqual([array valueAtIndex:1], kGPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([array rawValueAtIndex:2], 73);
  XCTAssertEqual([array rawValueAtIndex:3], 374);
  XCTAssertEqual([array valueAtIndex:3], kGPBUnrecognizedEnumeratorValue);
  XCTAssertThrowsSpecificNamed([array rawValueAtIndex:4], NSException, NSRangeException);
  __block NSUInteger idx2 = 0;
  [array enumerateRawValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValuesFiltered[idx]);
    XCTAssertNotEqual(stop, NULL);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateRawValuesWithOptions:NSEnumerationReverse
                            usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
                              XCTAssertEqual(idx, (3 - idx2));
                              XCTAssertEqual(value, kValues[idx]);
                              XCTAssertNotEqual(stop, NULL);
                              ++idx2;
                            }];
  idx2 = 0;
  [array enumerateValuesWithOptions:NSEnumerationReverse
                         usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
                           XCTAssertEqual(idx, (3 - idx2));
                           XCTAssertEqual(value, kValuesFiltered[idx]);
                           XCTAssertNotEqual(stop, NULL);
                           ++idx2;
                         }];
  // Stopping the enumeration.
  idx2 = 0;
  [array enumerateRawValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
    XCTAssertEqual(idx, idx2);
    XCTAssertEqual(value, kValues[idx]);
    XCTAssertNotEqual(stop, NULL);
    if (idx2 == 1) *stop = YES;
    XCTAssertNotEqual(idx, 2U);
    XCTAssertNotEqual(idx, 3U);
    ++idx2;
  }];
  idx2 = 0;
  [array enumerateRawValuesWithOptions:NSEnumerationReverse
                            usingBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
                              XCTAssertEqual(idx, (3 - idx2));
                              XCTAssertEqual(value, kValues[idx]);
                              XCTAssertNotEqual(stop, NULL);
                              if (idx2 == 1) *stop = YES;
                              XCTAssertNotEqual(idx, 1U);
                              XCTAssertNotEqual(idx, 0U);
                              ++idx2;
                            }];
  [array release];
}

- (void)testEquality {
  const int32_t kValues1[] = {71, 72, 173};      // With unknown value
  const int32_t kValues2[] = {71, 74, 173};      // With unknown value
  const int32_t kValues3[] = {71, 72, 173, 74};  // With unknown value
  GPBEnumArray *array1 = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                                rawValues:kValues1
                                                                    count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1);
  GPBEnumArray *array1prime =
      [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue2
                                             rawValues:kValues1
                                                 count:GPBARRAYSIZE(kValues1)];
  XCTAssertNotNil(array1prime);
  GPBEnumArray *array2 = [[GPBEnumArray alloc] initWithValues:kValues2
                                                        count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  GPBEnumArray *array3 = [[GPBEnumArray alloc] initWithValues:kValues3
                                                        count:GPBARRAYSIZE(kValues3)];
  XCTAssertNotNil(array3);

  // 1/1Prime should be different objects, but equal.
  XCTAssertNotEqual(array1, array1prime);
  XCTAssertEqualObjects(array1, array1prime);
  // Equal, so they must have same hash.
  XCTAssertEqual([array1 hash], [array1prime hash]);
  // But different validation functions.
  XCTAssertNotEqual(array1.validationFunc, array1prime.validationFunc);

  // 1/2/3 shouldn't be equal.
  XCTAssertNotEqualObjects(array1, array2);
  XCTAssertNotEqualObjects(array1, array3);
  XCTAssertNotEqualObjects(array2, array3);

  [array1 release];
  [array1prime release];
  [array2 release];
  [array3 release];
}

- (void)testCopy {
  const int32_t kValues[] = {71, 72};
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValues:kValues count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array addRawValue:1000];  // Unknown
  XCTAssertEqual(array.count, 3U);
  XCTAssertEqual([array rawValueAtIndex:0], 71);
  XCTAssertEqual([array rawValueAtIndex:1], 72);
  XCTAssertEqual([array rawValueAtIndex:2], 1000);
  XCTAssertEqual([array valueAtIndex:2], kGPBUnrecognizedEnumeratorValue);

  GPBEnumArray *array2 = [array copy];
  XCTAssertNotNil(array2);

  // Should be new object but equal.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  XCTAssertEqual(array.validationFunc, array2.validationFunc);
  XCTAssertTrue([array2 isKindOfClass:[GPBEnumArray class]]);
  XCTAssertEqual(array2.count, 3U);
  XCTAssertEqual([array2 rawValueAtIndex:0], 71);
  XCTAssertEqual([array2 rawValueAtIndex:1], 72);
  XCTAssertEqual([array2 rawValueAtIndex:2], 1000);
  XCTAssertEqual([array2 valueAtIndex:2], kGPBUnrecognizedEnumeratorValue);
  [array2 release];
  [array release];
}

- (void)testArrayFromArray {
  const int32_t kValues[] = {71, 172, 173, 74};  // Unknowns
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                               rawValues:kValues
                                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  GPBEnumArray *array2 = [GPBEnumArray arrayWithValueArray:array];
  XCTAssertNotNil(array2);

  // Should be new pointer, but equal objects.
  XCTAssertNotEqual(array, array2);
  XCTAssertEqualObjects(array, array2);
  XCTAssertEqual(array.validationFunc, array2.validationFunc);
  [array release];
}

- (void)testUnknownAdds {
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue];
  XCTAssertNotNil(array);

  XCTAssertThrowsSpecificNamed([array addValue:172], NSException, NSInvalidArgumentException);
  XCTAssertEqual(array.count, 0U);

  const int32_t kValues1[] = {172, 173};  // Unknown
  XCTAssertThrowsSpecificNamed([array addValues:kValues1 count:GPBARRAYSIZE(kValues1)], NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(array.count, 0U);
  [array release];
}

- (void)testRawAdds {
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue];
  XCTAssertNotNil(array);

  XCTAssertEqual(array.count, 0U);
  [array addRawValue:71];  // Valid
  XCTAssertEqual(array.count, 1U);

  const int32_t kValues1[] = {172, 173};  // Unknown
  [array addRawValues:kValues1 count:GPBARRAYSIZE(kValues1)];
  XCTAssertEqual(array.count, 3U);

  const int32_t kValues2[] = {74, 71};
  GPBEnumArray *array2 = [[GPBEnumArray alloc] initWithValues:kValues2
                                                        count:GPBARRAYSIZE(kValues2)];
  XCTAssertNotNil(array2);
  [array addRawValuesFromArray:array2];
  XCTAssertEqual(array.count, 5U);

  XCTAssertEqual([array rawValueAtIndex:0], 71);
  XCTAssertEqual([array rawValueAtIndex:1], 172);
  XCTAssertEqual([array valueAtIndex:1], kGPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([array rawValueAtIndex:2], 173);
  XCTAssertEqual([array valueAtIndex:2], kGPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([array rawValueAtIndex:3], 74);
  XCTAssertEqual([array rawValueAtIndex:4], 71);
  [array release];
}

- (void)testUnknownInserts {
  const int32_t kValues[] = {71, 72, 73};
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                               rawValues:kValues
                                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  XCTAssertThrowsSpecificNamed([array insertValue:174 atIndex:0], NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(array.count, 3U);

  // Middle
  XCTAssertThrowsSpecificNamed([array insertValue:274 atIndex:1], NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(array.count, 3U);

  // End
  XCTAssertThrowsSpecificNamed([array insertValue:374 atIndex:3], NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(array.count, 3U);
  [array release];
}

- (void)testRawInsert {
  const int32_t kValues[] = {71, 72, 73};
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                               rawValues:kValues
                                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);
  XCTAssertEqual(array.count, 3U);

  // First
  [array insertRawValue:174 atIndex:0];  // Unknown
  XCTAssertEqual(array.count, 4U);

  // Middle
  [array insertRawValue:274 atIndex:2];  // Unknown
  XCTAssertEqual(array.count, 5U);

  // End
  [array insertRawValue:374 atIndex:5];  // Unknown
  XCTAssertEqual(array.count, 6U);

  // Too far.
  XCTAssertThrowsSpecificNamed([array insertRawValue:74 atIndex:7], NSException, NSRangeException);

  XCTAssertEqual([array rawValueAtIndex:0], 174);
  XCTAssertEqual([array valueAtIndex:0], kGPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([array rawValueAtIndex:1], 71);
  XCTAssertEqual([array rawValueAtIndex:2], 274);
  XCTAssertEqual([array valueAtIndex:2], kGPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([array rawValueAtIndex:3], 72);
  XCTAssertEqual([array rawValueAtIndex:4], 73);
  XCTAssertEqual([array rawValueAtIndex:5], 374);
  XCTAssertEqual([array valueAtIndex:5], kGPBUnrecognizedEnumeratorValue);
  [array release];
}

- (void)testUnknownInplaceMutation {
  const int32_t kValues[] = {71, 72, 73, 74};
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                               rawValues:kValues
                                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:1 withValue:172], NSException,
                               NSInvalidArgumentException);
  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:3 withValue:274], NSException,
                               NSInvalidArgumentException);
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array valueAtIndex:0], 71);
  XCTAssertEqual([array valueAtIndex:1], 72);
  XCTAssertEqual([array valueAtIndex:2], 73);
  XCTAssertEqual([array valueAtIndex:3], 74);
  [array release];
}

- (void)testRawInplaceMutation {
  const int32_t kValues[] = {71, 72, 73, 74};
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValidationFunction:TestingEnum_IsValidValue
                                                               rawValues:kValues
                                                                   count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  [array replaceValueAtIndex:1 withRawValue:172];  // Unknown
  [array replaceValueAtIndex:3 withRawValue:274];  // Unknown
  XCTAssertEqual(array.count, 4U);
  XCTAssertEqual([array rawValueAtIndex:0], 71);
  XCTAssertEqual([array rawValueAtIndex:1], 172);
  XCTAssertEqual([array valueAtIndex:1], kGPBUnrecognizedEnumeratorValue);
  XCTAssertEqual([array rawValueAtIndex:2], 73);
  XCTAssertEqual([array rawValueAtIndex:3], 274);
  XCTAssertEqual([array valueAtIndex:3], kGPBUnrecognizedEnumeratorValue);

  XCTAssertThrowsSpecificNamed([array replaceValueAtIndex:4 withRawValue:74], NSException,
                               NSRangeException);
  [array release];
}

- (void)testRawInternalResizing {
  const int32_t kValues[] = {71, 172, 173, 74};  // Unknown
  GPBEnumArray *array = [[GPBEnumArray alloc] initWithValues:kValues count:GPBARRAYSIZE(kValues)];
  XCTAssertNotNil(array);

  // Add/remove to trigger the intneral buffer to grow/shrink.
  for (int i = 0; i < 100; ++i) {
    [array addRawValues:kValues count:GPBARRAYSIZE(kValues)];
  }
  XCTAssertEqual(array.count, 404U);
  for (int i = 0; i < 100; ++i) {
    [array removeValueAtIndex:(i * 2)];
  }
  XCTAssertEqual(array.count, 304U);
  for (int i = 0; i < 100; ++i) {
    [array insertRawValue:274 atIndex:(i * 3)];  // Unknown
  }
  XCTAssertEqual(array.count, 404U);
  [array removeAll];
  XCTAssertEqual(array.count, 0U);
  [array release];
}

@end

#pragma mark - GPBAutocreatedArray Tests

// These are hand written tests to double check some behaviors of the
// GPBAutocreatedArray.

// NOTE: GPBAutocreatedArray is private to the library, users of the library
// should never have to directly deal with this class.

@interface GPBAutocreatedArrayTests : XCTestCase
@end

@implementation GPBAutocreatedArrayTests

- (void)testEquality {
  GPBAutocreatedArray *array = [[GPBAutocreatedArray alloc] init];

  XCTAssertTrue([array isEqual:@[]]);
  XCTAssertTrue([array isEqualToArray:@[]]);

  XCTAssertFalse([array isEqual:@[ @"foo" ]]);
  XCTAssertFalse([array isEqualToArray:@[ @"foo" ]]);

  [array addObject:@"foo"];

  XCTAssertFalse([array isEqual:@[]]);
  XCTAssertFalse([array isEqualToArray:@[]]);
  XCTAssertTrue([array isEqual:@[ @"foo" ]]);
  XCTAssertTrue([array isEqualToArray:@[ @"foo" ]]);
  XCTAssertFalse([array isEqual:@[ @"bar" ]]);
  XCTAssertFalse([array isEqualToArray:@[ @"bar" ]]);

  GPBAutocreatedArray *array2 = [[GPBAutocreatedArray alloc] init];

  XCTAssertFalse([array isEqual:array2]);
  XCTAssertFalse([array isEqualToArray:array2]);

  [array2 addObject:@"bar"];
  XCTAssertFalse([array isEqual:array2]);
  XCTAssertFalse([array isEqualToArray:array2]);

  [array2 replaceObjectAtIndex:0 withObject:@"foo"];
  XCTAssertTrue([array isEqual:array2]);
  XCTAssertTrue([array isEqualToArray:array2]);

  [array2 release];
  [array release];
}

- (void)testCopy {
  {
    GPBAutocreatedArray *array = [[GPBAutocreatedArray alloc] init];

    NSArray *cpy = [array copy];
    XCTAssertTrue(cpy != array);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSArray class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)0);

    NSArray *cpy2 = [array copy];
    XCTAssertTrue(cpy2 != array);  // Ptr compare
    // Can't compare cpy and cpy2 because NSArray has a singleton empty
    // array it uses, so the ptrs are the same.
    XCTAssertTrue([cpy2 isKindOfClass:[NSArray class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)0);

    [cpy2 release];
    [cpy release];
    [array release];
  }

  {
    GPBAutocreatedArray *array = [[GPBAutocreatedArray alloc] init];

    NSMutableArray *cpy = [array mutableCopy];
    XCTAssertTrue(cpy != array);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSMutableArray class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)0);

    NSMutableArray *cpy2 = [array mutableCopy];
    XCTAssertTrue(cpy2 != array);  // Ptr compare
    XCTAssertTrue(cpy2 != cpy);    // Ptr compare
    XCTAssertTrue([cpy2 isKindOfClass:[NSMutableArray class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)0);

    [cpy2 release];
    [cpy release];
    [array release];
  }

  {
    GPBAutocreatedArray *array = [[GPBAutocreatedArray alloc] init];
    [array addObject:@"foo"];
    [array addObject:@"bar"];

    NSArray *cpy = [array copy];
    XCTAssertTrue(cpy != array);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSArray class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy[0], @"foo");
    XCTAssertEqualObjects(cpy[1], @"bar");

    NSArray *cpy2 = [array copy];
    XCTAssertTrue(cpy2 != array);  // Ptr compare
    XCTAssertTrue(cpy2 != cpy);    // Ptr compare
    XCTAssertTrue([cpy2 isKindOfClass:[NSArray class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy2[0], @"foo");
    XCTAssertEqualObjects(cpy2[1], @"bar");

    [cpy2 release];
    [cpy release];
    [array release];
  }

  {
    GPBAutocreatedArray *array = [[GPBAutocreatedArray alloc] init];
    [array addObject:@"foo"];
    [array addObject:@"bar"];

    NSMutableArray *cpy = [array mutableCopy];
    XCTAssertTrue(cpy != array);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSArray class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy[0], @"foo");
    XCTAssertEqualObjects(cpy[1], @"bar");

    NSMutableArray *cpy2 = [array mutableCopy];
    XCTAssertTrue(cpy2 != array);  // Ptr compare
    XCTAssertTrue(cpy2 != cpy);    // Ptr compare
    XCTAssertTrue([cpy2 isKindOfClass:[NSArray class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy2[0], @"foo");
    XCTAssertEqualObjects(cpy2[1], @"bar");

    [cpy2 release];
    [cpy release];
    [array release];
  }
}

- (void)testIndexedSubscriptSupport {
  // The base NSArray/NSMutableArray behaviors for *IndexedSubscript methods
  // should still work via the methods that one has to override to make an
  // NSMutableArray subclass.  i.e. - this should "just work" and if these
  // crash/fail, then something is wrong in how NSMutableArray is subclassed.

  GPBAutocreatedArray *array = [[GPBAutocreatedArray alloc] init];

  [array addObject:@"foo"];
  [array addObject:@"bar"];
  XCTAssertEqual(array.count, (NSUInteger)2);
  XCTAssertEqualObjects(array[0], @"foo");
  XCTAssertEqualObjects(array[1], @"bar");
  array[0] = @"foo2";
  array[2] = @"baz";
  XCTAssertEqual(array.count, (NSUInteger)3);
  XCTAssertEqualObjects(array[0], @"foo2");
  XCTAssertEqualObjects(array[1], @"bar");
  XCTAssertEqualObjects(array[2], @"baz");

  [array release];
}

@end
