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

#import "GPBWellKnownTypes.h"

#import <XCTest/XCTest.h>

#import "GPBTestUtilities.h"
#import "objectivec/Tests/AnyTest.pbobjc.h"

// Nanosecond time accuracy
static const NSTimeInterval kTimeAccuracy = 1e-9;

@interface WellKnownTypesTest : XCTestCase
@end

@implementation WellKnownTypesTest

- (void)testTimeStamp {
  // Test negative and positive values.
  NSTimeInterval values[] = {
      -428027599.483999967, -1234567.0, -0.5, 0, 0.75, 54321.0, 2468086, 483999967};
  for (size_t i = 0; i < GPBARRAYSIZE(values); ++i) {
    NSTimeInterval value = values[i];

    // Test Creation - date.
    NSDate *date = [NSDate dateWithTimeIntervalSince1970:value];
    GPBTimestamp *timeStamp = [[GPBTimestamp alloc] initWithDate:date];

    XCTAssertGreaterThanOrEqual(timeStamp.nanos, 0, @"Offset %f - Date: %@", (double)value, date);
    XCTAssertLessThan(timeStamp.nanos, 1e9, @"Offset %f - Date: %@", (double)value, date);

    // Comparing timeIntervals instead of directly comparing dates because date
    // equality requires the time intervals to be exactly the same, and the
    // timeintervals go through a bit of floating point error as they are
    // converted back and forth from the internal representation.
    XCTAssertEqualWithAccuracy(value, timeStamp.date.timeIntervalSince1970, kTimeAccuracy,
                               @"Offset %f - Date: %@", (double)value, date);
    [timeStamp release];

    // Test Creation - timeIntervalSince1970.
    timeStamp = [[GPBTimestamp alloc] initWithTimeIntervalSince1970:value];

    XCTAssertGreaterThanOrEqual(timeStamp.nanos, 0, @"Offset %f - Date: %@", (double)value, date);
    XCTAssertLessThan(timeStamp.nanos, 1e9, @"Offset %f - Date: %@", (double)value, date);

    XCTAssertEqualWithAccuracy(value, timeStamp.timeIntervalSince1970, kTimeAccuracy,
                               @"Offset %f - Date: %@", (double)value, date);
    [timeStamp release];

    // Test Mutation - date.
    timeStamp = [[GPBTimestamp alloc] init];
    timeStamp.date = date;

    XCTAssertGreaterThanOrEqual(timeStamp.nanos, 0, @"Offset %f - Date: %@", (double)value, date);
    XCTAssertLessThan(timeStamp.nanos, 1e9, @"Offset %f - Date: %@", (double)value, date);

    XCTAssertEqualWithAccuracy(value, timeStamp.date.timeIntervalSince1970, kTimeAccuracy,
                               @"Offset %f - Date: %@", (double)value, date);
    [timeStamp release];

    // Test Mutation - timeIntervalSince1970.
    timeStamp = [[GPBTimestamp alloc] init];
    timeStamp.timeIntervalSince1970 = value;

    XCTAssertGreaterThanOrEqual(timeStamp.nanos, 0, @"Offset %f - Date: %@", (double)value, date);
    XCTAssertLessThan(timeStamp.nanos, 1e9, @"Offset %f - Date: %@", (double)value, date);

    XCTAssertEqualWithAccuracy(value, timeStamp.date.timeIntervalSince1970, kTimeAccuracy,
                               @"Offset %f - Date: %@", (double)value, date);

    [timeStamp release];
  }
}

- (void)testDuration {
  // Test negative and positive values.
  NSTimeInterval values[] = {-1000.0001, -500.0, -0.5, 0, 0.75, 1000.0, 2000.0002};
  for (size_t i = 0; i < GPBARRAYSIZE(values); ++i) {
    NSTimeInterval value = values[i];

    // Test Creation.
    GPBDuration *duration = [[GPBDuration alloc] initWithTimeInterval:value];
    XCTAssertEqualWithAccuracy(value, duration.timeInterval, kTimeAccuracy, @"For interval %f",
                               (double)value);
    if (value > 0) {
      XCTAssertGreaterThanOrEqual(duration.seconds, 0, @"For interval %f", (double)value);
      XCTAssertGreaterThanOrEqual(duration.nanos, 0, @"For interval %f", (double)value);
    } else {
      XCTAssertLessThanOrEqual(duration.seconds, 0, @"For interval %f", (double)value);
      XCTAssertLessThanOrEqual(duration.nanos, 0, @"For interval %f", (double)value);
    }
    [duration release];

    // Test Mutation.
    duration = [[GPBDuration alloc] init];
    duration.timeInterval = value;
    XCTAssertEqualWithAccuracy(value, duration.timeInterval, kTimeAccuracy, @"For interval %f",
                               (double)value);
    if (value > 0) {
      XCTAssertGreaterThanOrEqual(duration.seconds, 0, @"For interval %f", (double)value);
      XCTAssertGreaterThanOrEqual(duration.nanos, 0, @"For interval %f", (double)value);
    } else {
      XCTAssertLessThanOrEqual(duration.seconds, 0, @"For interval %f", (double)value);
      XCTAssertLessThanOrEqual(duration.nanos, 0, @"For interval %f", (double)value);
    }
    [duration release];
  }
}

- (void)testAnyHelpers {
  // Set and extract covers most of the code.

  AnyTestMessage *subMessage = [AnyTestMessage message];
  subMessage.int32Value = 12345;
  AnyTestMessage *message = [AnyTestMessage message];
  NSError *err = nil;
  message.anyValue = [GPBAny anyWithMessage:subMessage error:&err];
  XCTAssertNil(err);

  NSData *data = message.data;
  XCTAssertNotNil(data);

  AnyTestMessage *message2 = [AnyTestMessage parseFromData:data error:&err];
  XCTAssertNil(err);
  XCTAssertNotNil(message2);
  XCTAssertTrue(message2.hasAnyValue);

  AnyTestMessage *subMessage2 =
      (AnyTestMessage *)[message.anyValue unpackMessageClass:[AnyTestMessage class] error:&err];
  XCTAssertNil(err);
  XCTAssertNotNil(subMessage2);
  XCTAssertEqual(subMessage2.int32Value, 12345);

  // NULL errorPtr in the two calls.

  message.anyValue = [GPBAny anyWithMessage:subMessage error:NULL];
  NSData *data2 = message.data;
  XCTAssertEqualObjects(data2, data);

  AnyTestMessage *subMessage3 =
      (AnyTestMessage *)[message.anyValue unpackMessageClass:[AnyTestMessage class] error:NULL];
  XCTAssertNotNil(subMessage3);
  XCTAssertEqualObjects(subMessage2, subMessage3);

  // Try to extract wrong type.

  GPBTimestamp *wrongMessage =
      (GPBTimestamp *)[message.anyValue unpackMessageClass:[GPBTimestamp class] error:&err];
  XCTAssertNotNil(err);
  XCTAssertNil(wrongMessage);
  XCTAssertEqualObjects(err.domain, GPBWellKnownTypesErrorDomain);
  XCTAssertEqual(err.code, GPBWellKnownTypesErrorCodeTypeURLMismatch);

  wrongMessage = (GPBTimestamp *)[message.anyValue unpackMessageClass:[GPBTimestamp class]
                                                                error:NULL];
  XCTAssertNil(wrongMessage);
}

@end
