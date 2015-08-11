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

// Importing sources here to force the linker to include our category methods in
// the static library. If these were compiled separately, the category methods
// below would be stripped by the linker.

#import "google/protobuf/Timestamp.pbobjc.m"
#import "google/protobuf/Duration.pbobjc.m"
#import "GPBWellKnownTypes.h"

static NSTimeInterval TimeIntervalSince1970FromSecondsAndNanos(int64_t seconds,
                                                               int32_t nanos) {
  return seconds + (NSTimeInterval)nanos / 1e9;
}

static int32_t SecondsAndNanosFromTimeIntervalSince1970(NSTimeInterval time,
                                                        int64_t *outSeconds) {
  NSTimeInterval seconds;
  NSTimeInterval nanos = modf(time, &seconds);
  nanos *= 1e9;
  *outSeconds = (int64_t)seconds;
  return (int32_t)nanos;
}

@implementation GPBTimestamp (GBPWellKnownTypes)

- (instancetype)initWithDate:(NSDate *)date {
  return [self initWithTimeIntervalSince1970:date.timeIntervalSince1970];
}

- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970 {
  if ((self = [super init])) {
    int64_t seconds;
    int32_t nanos = SecondsAndNanosFromTimeIntervalSince1970(
        timeIntervalSince1970, &seconds);
    self.seconds = seconds;
    self.nanos = nanos;
  }
  return self;
}

- (NSDate *)date {
  return [NSDate dateWithTimeIntervalSince1970:self.timeIntervalSince1970];
}

- (void)setDate:(NSDate *)date {
  self.timeIntervalSince1970 = date.timeIntervalSince1970;
}

- (NSTimeInterval)timeIntervalSince1970 {
  return TimeIntervalSince1970FromSecondsAndNanos(self.seconds, self.nanos);
}

- (void)setTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970 {
  int64_t seconds;
  int32_t nanos =
      SecondsAndNanosFromTimeIntervalSince1970(timeIntervalSince1970, &seconds);
  self.seconds = seconds;
  self.nanos = nanos;
}

@end

@implementation GPBDuration (GBPWellKnownTypes)

- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970 {
  if ((self = [super init])) {
    int64_t seconds;
    int32_t nanos = SecondsAndNanosFromTimeIntervalSince1970(
        timeIntervalSince1970, &seconds);
    self.seconds = seconds;
    self.nanos = nanos;
  }
  return self;
}

- (NSTimeInterval)timeIntervalSince1970 {
  return TimeIntervalSince1970FromSecondsAndNanos(self.seconds, self.nanos);
}

- (void)setTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970 {
  int64_t seconds;
  int32_t nanos =
      SecondsAndNanosFromTimeIntervalSince1970(timeIntervalSince1970, &seconds);
  self.seconds = seconds;
  self.nanos = nanos;
}

@end
