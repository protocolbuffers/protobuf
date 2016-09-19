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

#import "GPBWellKnownTypes.h"

#import "GPBUtilities_PackagePrivate.h"

NSString *const GPBWellKnownTypesErrorDomain =
    GPBNSStringifySymbol(GPBWellKnownTypesErrorDomain);

static NSString *kTypePrefixGoogleApisCom = @"type.googleapis.com/";

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

static NSString *BuildTypeURL(NSString *typeURLPrefix, NSString *fullName) {
  if (typeURLPrefix.length == 0) {
    return fullName;
  }

  if ([typeURLPrefix hasSuffix:@"/"]) {
    return [typeURLPrefix stringByAppendingString:fullName];
  }

  return [NSString stringWithFormat:@"%@/%@", typeURLPrefix, fullName];
}

static NSString *ParseTypeFromURL(NSString *typeURLString) {
  NSRange range = [typeURLString rangeOfString:@"/" options:NSBackwardsSearch];
  if ((range.location == NSNotFound) ||
      (NSMaxRange(range) == typeURLString.length)) {
    return nil;
  }
  NSString *result = [typeURLString substringFromIndex:range.location + 1];
  return result;
}

#pragma mark - GPBTimestamp

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

#pragma mark - GPBDuration

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

#pragma mark - GPBAny

@implementation GPBAny (GBPWellKnownTypes)

+ (instancetype)anyWithMessage:(GPBMessage *)message
                         error:(NSError **)errorPtr {
  return [self anyWithMessage:message
                typeURLPrefix:kTypePrefixGoogleApisCom
                        error:errorPtr];
}

+ (instancetype)anyWithMessage:(GPBMessage *)message
                 typeURLPrefix:(NSString *)typeURLPrefix
                         error:(NSError **)errorPtr {
  return [[[self alloc] initWithMessage:message
                          typeURLPrefix:typeURLPrefix
                                  error:errorPtr] autorelease];
}

- (instancetype)initWithMessage:(GPBMessage *)message
                          error:(NSError **)errorPtr {
  return [self initWithMessage:message
                 typeURLPrefix:kTypePrefixGoogleApisCom
                         error:errorPtr];
}

- (instancetype)initWithMessage:(GPBMessage *)message
                  typeURLPrefix:(NSString *)typeURLPrefix
                          error:(NSError **)errorPtr {
  self = [self init];
  if (self) {
    if (![self packWithMessage:message
                 typeURLPrefix:typeURLPrefix
                         error:errorPtr]) {
      [self release];
      self = nil;
    }
  }
  return self;
}

- (BOOL)packWithMessage:(GPBMessage *)message
                  error:(NSError **)errorPtr {
  return [self packWithMessage:message
                 typeURLPrefix:kTypePrefixGoogleApisCom
                         error:errorPtr];
}

- (BOOL)packWithMessage:(GPBMessage *)message
          typeURLPrefix:(NSString *)typeURLPrefix
                  error:(NSError **)errorPtr {
  NSString *fullName = [message descriptor].fullName;
  if (fullName.length == 0) {
    if (errorPtr) {
      *errorPtr =
          [NSError errorWithDomain:GPBWellKnownTypesErrorDomain
                              code:GPBWellKnownTypesErrorCodeFailedToComputeTypeURL
                          userInfo:nil];
    }
    return NO;
  }
  if (errorPtr) {
    *errorPtr = nil;
  }
  self.typeURL = BuildTypeURL(typeURLPrefix, fullName);
  self.value = message.data;
  return YES;
}

- (GPBMessage *)unpackMessageClass:(Class)messageClass
                             error:(NSError **)errorPtr {
  NSString *fullName = [messageClass descriptor].fullName;
  if (fullName.length == 0) {
    if (errorPtr) {
      *errorPtr =
          [NSError errorWithDomain:GPBWellKnownTypesErrorDomain
                              code:GPBWellKnownTypesErrorCodeFailedToComputeTypeURL
                      userInfo:nil];
    }
    return nil;
  }

  NSString *expectedFullName = ParseTypeFromURL(self.typeURL);
  if ((expectedFullName == nil) || ![expectedFullName isEqual:fullName]) {
    if (errorPtr) {
      *errorPtr =
          [NSError errorWithDomain:GPBWellKnownTypesErrorDomain
                              code:GPBWellKnownTypesErrorCodeTypeURLMismatch
                          userInfo:nil];
    }
    return nil;
  }

  // Any is proto3, which means no extensions, so this assumes anything put
  // within an any also won't need extensions. A second helper could be added
  // if needed.
  return [messageClass parseFromData:self.value
                               error:errorPtr];
}

@end
