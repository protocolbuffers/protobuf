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

#import "google/protobuf/Unittest.pbobjc.h"
#import "GPBWellKnownTypes.h"
#import "GPBTestUtilities.h"

#import <XCTest/XCTest.h>

// A basically random interval into the future for testing with.
static const NSTimeInterval kFutureOffsetInterval = 15000;

// Nanosecond time accuracy
static const NSTimeInterval kTimeAccuracy = 1e-9;

@interface WellKnownTypesTest : GPBTestCase
@end

@implementation WellKnownTypesTest

- (void)testTimeStamp {
  // Test Creation.
  NSDate *date = [NSDate date];
  GPBTimestamp *timeStamp = [[GPBTimestamp alloc] initWithDate:date];
  NSDate *timeStampDate = timeStamp.date;

  // Comparing timeIntervals instead of directly comparing dates because date
  // equality requires the time intervals to be exactly the same, and the
  // timeintervals go through a bit of floating point error as they are
  // converted back and forth from the internal representation.
  XCTAssertEqualWithAccuracy(date.timeIntervalSince1970,
                             timeStampDate.timeIntervalSince1970,
                             kTimeAccuracy);

  NSTimeInterval time = [date timeIntervalSince1970];
  GPBTimestamp *timeStamp2 =
      [[GPBTimestamp alloc] initWithTimeIntervalSince1970:time];
  NSTimeInterval durationTime = timeStamp2.timeIntervalSince1970;
  XCTAssertEqualWithAccuracy(time, durationTime, kTimeAccuracy);
  [timeStamp release];

  // Test Mutation.
  date = [NSDate dateWithTimeIntervalSinceNow:kFutureOffsetInterval];
  timeStamp2.date = date;
  timeStampDate = timeStamp2.date;
  XCTAssertEqualWithAccuracy(date.timeIntervalSince1970,
                             timeStampDate.timeIntervalSince1970,
                             kTimeAccuracy);

  time = date.timeIntervalSince1970;
  timeStamp2.timeIntervalSince1970 = time;
  durationTime = timeStamp2.timeIntervalSince1970;
  XCTAssertEqualWithAccuracy(time, durationTime, kTimeAccuracy);
  [timeStamp2 release];
}

- (void)testDuration {
  // Test Creation.
  NSTimeInterval time = [[NSDate date] timeIntervalSince1970];
  GPBDuration *duration =
      [[GPBDuration alloc] initWithTimeIntervalSince1970:time];
  NSTimeInterval durationTime = duration.timeIntervalSince1970;
  XCTAssertEqualWithAccuracy(time, durationTime, kTimeAccuracy);
  [duration release];

  // Test Mutation.
  GPBDuration *duration2 =
      [[GPBDuration alloc] initWithTimeIntervalSince1970:time];
  NSDate *date = [NSDate dateWithTimeIntervalSinceNow:kFutureOffsetInterval];
  time = date.timeIntervalSince1970;
  duration2.timeIntervalSince1970 = time;
  durationTime = duration2.timeIntervalSince1970;
  XCTAssertEqualWithAccuracy(time, durationTime, kTimeAccuracy);
  [duration2 release];
}

- (void)testAnyPackingAndUnpacking {
  TestAllTypes *from = [TestAllTypes message];
  [self setAllFields:from repeatedCount:1];
  NSData *data = from.data;

  // Test initWithMessage
  GPBAny *anyInited = [[GPBAny alloc] initWithMessage:from];
  XCTAssertEqualObjects(
      [GPBTypeGoogleApisComPrefix stringByAppendingString:from.descriptor.name],
      anyInited.typeURL);
  XCTAssertEqualObjects(data, anyInited.value);
  [anyInited release];

  // Test setMessage.
  GPBAny *any = [GPBAny message];
  [any setMessage:from];
  XCTAssertEqualObjects(
      [GPBTypeGoogleApisComPrefix stringByAppendingString:from.descriptor.name],
      any.typeURL);
  XCTAssertEqualObjects(data, any.value);

  // Test messageOfClass
  TestAllTypes *to = (TestAllTypes*)[any messageOfClass:[TestAllTypes class]];
  XCTAssertEqualObjects(from, to);
  XCTAssertEqual([any messageOfClass:[ForeignMessage class]], nil);
  XCTAssertEqual([[GPBAny message] messageOfClass:[TestAllTypes class]], nil);

  // Test setMessage with another type.
  ForeignMessage *from2 = [ForeignMessage message];
  [any setMessage:from2];
  XCTAssertEqualObjects(
      [GPBTypeGoogleApisComPrefix stringByAppendingString:from2.descriptor.name],
      any.typeURL);
  XCTAssertEqual(0UL, [any.value length]);

  // Test wrapsMessageOfClass
  XCTAssertTrue([any wrapsMessageOfClass:[from2 class]]);
  XCTAssertFalse([any wrapsMessageOfClass:[from class]]);
#if !defined(NS_BLOCK_ASSERTIONS)
  // If assert is enabled, throw exception when the passed message class to
  // wrapsMessageOfClass is not a child of GPBMessage.
  XCTAssertThrows([any wrapsMessageOfClass:[NSString class]]);
#else
  // If assert is disabled, return false when the passed message class to
  // wrapsMessageOfClass is not a child of GPBMessage.
  XCTAssertFalse([any wrapsMessageOfClass:[NSString class]]);
#endif
}

@end
