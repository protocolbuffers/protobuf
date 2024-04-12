// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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
#import "GPBDictionary_PackagePrivate.h"

#import "GPBTestUtilities.h"

#pragma mark - GPBAutocreatedDictionary Tests

// These are hand written tests to double check some behaviors of the
// GPBAutocreatedDictionary.  The GPBDictionary+[type]Tests files are generate
// tests.

// NOTE: GPBAutocreatedDictionary is private to the library, users of the
// library should never have to directly deal with this class.

@interface GPBAutocreatedDictionaryTests : XCTestCase
@end

@implementation GPBAutocreatedDictionaryTests

- (void)testEquality {
  GPBAutocreatedDictionary *dict = [[GPBAutocreatedDictionary alloc] init];

  XCTAssertTrue([dict isEqual:@{}]);
  XCTAssertTrue([dict isEqualToDictionary:@{}]);

  XCTAssertFalse([dict isEqual:@{@"foo" : @"bar"}]);
  XCTAssertFalse([dict isEqualToDictionary:@{@"foo" : @"bar"}]);

  [dict setObject:@"bar" forKey:@"foo"];

  XCTAssertFalse([dict isEqual:@{}]);
  XCTAssertFalse([dict isEqualToDictionary:@{}]);
  XCTAssertTrue([dict isEqual:@{@"foo" : @"bar"}]);
  XCTAssertTrue([dict isEqualToDictionary:@{@"foo" : @"bar"}]);
  XCTAssertFalse([dict isEqual:@{@"bar" : @"baz"}]);
  XCTAssertFalse([dict isEqualToDictionary:@{@"bar" : @"baz"}]);

  GPBAutocreatedDictionary *dict2 = [[GPBAutocreatedDictionary alloc] init];

  XCTAssertFalse([dict isEqual:dict2]);
  XCTAssertFalse([dict isEqualToDictionary:dict2]);

  [dict2 setObject:@"mumble" forKey:@"foo"];
  XCTAssertFalse([dict isEqual:dict2]);
  XCTAssertFalse([dict isEqualToDictionary:dict2]);

  [dict2 setObject:@"bar" forKey:@"foo"];
  XCTAssertTrue([dict isEqual:dict2]);
  XCTAssertTrue([dict isEqualToDictionary:dict2]);

  [dict2 release];
  [dict release];
}

- (void)testCopy {
  {
    GPBAutocreatedDictionary *dict = [[GPBAutocreatedDictionary alloc] init];

    NSDictionary *cpy = [dict copy];
    XCTAssertTrue(cpy != dict);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSDictionary class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)0);

    NSDictionary *cpy2 = [dict copy];
    XCTAssertTrue(cpy2 != dict);  // Ptr compare
    XCTAssertTrue(cpy2 != cpy);   // Ptr compare
    XCTAssertTrue([cpy2 isKindOfClass:[NSDictionary class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)0);

    [cpy2 release];
    [cpy release];
    [dict release];
  }

  {
    GPBAutocreatedDictionary *dict = [[GPBAutocreatedDictionary alloc] init];

    NSMutableDictionary *cpy = [dict mutableCopy];
    XCTAssertTrue(cpy != dict);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSMutableDictionary class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)0);

    NSMutableDictionary *cpy2 = [dict mutableCopy];
    XCTAssertTrue(cpy2 != dict);  // Ptr compare
    XCTAssertTrue(cpy2 != cpy);   // Ptr compare
    XCTAssertTrue([cpy2 isKindOfClass:[NSMutableDictionary class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)0);

    [cpy2 release];
    [cpy release];
    [dict release];
  }

  {
    GPBAutocreatedDictionary *dict = [[GPBAutocreatedDictionary alloc] init];
    dict[@"foo"] = @"bar";
    dict[@"baz"] = @"mumble";

    NSDictionary *cpy = [dict copy];
    XCTAssertTrue(cpy != dict);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSDictionary class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy[@"foo"], @"bar");
    XCTAssertEqualObjects(cpy[@"baz"], @"mumble");

    NSDictionary *cpy2 = [dict copy];
    XCTAssertTrue(cpy2 != dict);  // Ptr compare
    XCTAssertTrue(cpy2 != cpy);   // Ptr compare
    XCTAssertTrue([cpy2 isKindOfClass:[NSDictionary class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy2[@"foo"], @"bar");
    XCTAssertEqualObjects(cpy2[@"baz"], @"mumble");

    [cpy2 release];
    [cpy release];
    [dict release];
  }

  {
    GPBAutocreatedDictionary *dict = [[GPBAutocreatedDictionary alloc] init];
    dict[@"foo"] = @"bar";
    dict[@"baz"] = @"mumble";

    NSMutableDictionary *cpy = [dict mutableCopy];
    XCTAssertTrue(cpy != dict);  // Ptr compare
    XCTAssertTrue([cpy isKindOfClass:[NSMutableDictionary class]]);
    XCTAssertFalse([cpy isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy[@"foo"], @"bar");
    XCTAssertEqualObjects(cpy[@"baz"], @"mumble");

    NSMutableDictionary *cpy2 = [dict mutableCopy];
    XCTAssertTrue(cpy2 != dict);  // Ptr compare
    XCTAssertTrue(cpy2 != cpy);   // Ptr compare
    XCTAssertTrue([cpy2 isKindOfClass:[NSMutableDictionary class]]);
    XCTAssertFalse([cpy2 isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(cpy2.count, (NSUInteger)2);
    XCTAssertEqualObjects(cpy2[@"foo"], @"bar");
    XCTAssertEqualObjects(cpy2[@"baz"], @"mumble");

    [cpy2 release];
    [cpy release];
    [dict release];
  }
}

@end
