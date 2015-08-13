// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#import <XCTest/XCTest.h>

#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBTestUtilities.h"

@interface TestClass : NSObject
@property(nonatomic, retain) NSString *foo;
@end

@implementation TestClass
@synthesize foo;
@end

@interface GPBStringTests : XCTestCase {
  NSMutableArray *nsStrings_;
  NSMutableArray *gpbStrings_;
}

@end

@implementation GPBStringTests

- (void)setUp {
  [super setUp];
  const char *strings[] = {
      "ascii string",
      "non-ascii string \xc3\xa9",  // e with acute accent
      "\xe2\x99\xa1",               // White Heart
      "mix \xe2\x99\xa4 string",    // White Spade

      // Decomposed forms from http://www.unicode.org/reports/tr15/
      // 1.2 Singletons
      "\xe2\x84\xa8 = A\xcc\x8a = \xc3\x85", "\xe2\x84\xa6 = \xce\xa9",

      // 1.2 Canonical Composites
      "A\xcc\x8a = \xc3\x85",
      "o\xcc\x82 = \xc3\xb4",

      // 1.2 Multiple Combining Marks
      "s\xcc\xa3\xcc\x87 = \xe1\xb9\xa9",
      "\xe1\xb8\x8b\xcc\xa3 = d\xcc\xa3\xcc\x87 = \xe1\xb8\x8d \xcc\x87",
      "q\xcc\x87\xcc\xa3 = q\xcc\xa3\xcc\x87",

      // BOM
      "\xEF\xBB\xBF String with BOM",
      "String with \xEF\xBB\xBF in middle",
      "String with end bom \xEF\xBB\xBF",
      "\xEF\xBB\xBF\xe2\x99\xa1",  // BOM White Heart
      "\xEF\xBB\xBF\xEF\xBB\xBF String with Two BOM",

      // Supplementary Plane
      "\xf0\x9d\x84\x9e",  // MUSICAL SYMBOL G CLEF

      // Tags
      "\xf3\xa0\x80\x81",  // Language Tag

      // Variation Selectors
      "\xf3\xa0\x84\x80",  // VARIATION SELECTOR-17

      // Specials
      "\xef\xbb\xbf\xef\xbf\xbd\xef\xbf\xbf",

      // Left To Right/Right To Left
      // http://unicode.org/reports/tr9/

      // Hello! <RTL marker>!Merhaba<LTR marker>
      "Hello! \xE2\x80\x8F!\xd9\x85\xd8\xb1\xd8\xad\xd8\xa8\xd8\xa7\xE2\x80\x8E",

      "\xE2\x80\x8E LTR At Start",
      "LTR At End\xE2\x80\x8E",
      "\xE2\x80\x8F RTL At Start",
      "RTL At End\xE2\x80\x8F",
      "\xE2\x80\x8E\xE2\x80\x8E Double LTR \xE2\x80\x8E\xE2\x80\x8E",
      "\xE2\x80\x8F\xE2\x80\x8F Double RTL \xE2\x80\x8F\xE2\x80\x8F",
      "\xE2\x80\x8F\xE2\x80\x8E LTR-RTL LTR-RTL \xE2\x80\x8E\xE2\x80\x8F",
  };
  size_t stringsSize = GPBARRAYSIZE(strings);
  size_t numberUnicodeStrings = 17375;
  nsStrings_ = [[NSMutableArray alloc]
      initWithCapacity:stringsSize + numberUnicodeStrings];
  gpbStrings_ = [[NSMutableArray alloc]
      initWithCapacity:stringsSize + numberUnicodeStrings];
  for (size_t i = 0; i < stringsSize; ++i) {
    size_t length = strlen(strings[i]);
    NSString *nsString = [[NSString alloc] initWithBytes:strings[i]
                                                  length:length
                                                encoding:NSUTF8StringEncoding];
    [nsStrings_ addObject:nsString];
    [nsString release];
    GPBString *gpbString = GPBCreateGPBStringWithUTF8(strings[i], length);
    [gpbStrings_ addObject:gpbString];
    [gpbString release];
  }

  // Generate all UTF8 characters in a variety of strings
  // UTF8-1 - 1 Byte UTF 8 chars
  int length = 0x7F + 1;
  char *buffer = (char *)calloc(length, 1);
  for (int i = 0; i < length; ++i) {
    buffer[i] = (char)i;
  }
  NSString *nsString = [[NSString alloc] initWithBytes:buffer
                                                length:length
                                              encoding:NSUTF8StringEncoding];
  [nsStrings_ addObject:nsString];
  [nsString release];
  GPBString *gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
  [gpbStrings_ addObject:gpbString];
  [gpbString release];

  // UTF8-2 - 2 Byte UTF 8 chars
  int pointLength = 0xbf - 0x80 + 1;
  length = pointLength * 2;
  buffer = (char *)calloc(length, 1);
  for (int i = 0xc2; i <= 0xdf; ++i) {
    char *bufferPtr = buffer;
    for (int j = 0x80; j <= 0xbf; ++j) {
      (*bufferPtr++) = (char)i;
      (*bufferPtr++) = (char)j;
    }
    nsString = [[NSString alloc] initWithBytes:buffer
                                        length:length
                                      encoding:NSUTF8StringEncoding];
    [nsStrings_ addObject:nsString];
    [nsString release];
    gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
    [gpbStrings_ addObject:gpbString];
    [gpbString release];
  }
  free(buffer);

  // UTF8-3 - 3 Byte UTF 8 chars
  length = pointLength * 3;
  buffer = (char *)calloc(length, 1);
  for (int i = 0xa0; i <= 0xbf; ++i) {
    char *bufferPtr = buffer;
    for (int j = 0x80; j <= 0xbf; ++j) {
      (*bufferPtr++) = (char)0xE0;
      (*bufferPtr++) = (char)i;
      (*bufferPtr++) = (char)j;
    }
    nsString = [[NSString alloc] initWithBytes:buffer
                                        length:length
                                      encoding:NSUTF8StringEncoding];
    [nsStrings_ addObject:nsString];
    [nsString release];
    gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
    [gpbStrings_ addObject:gpbString];
    [gpbString release];
  }
  for (int i = 0xe1; i <= 0xec; ++i) {
    for (int j = 0x80; j <= 0xbf; ++j) {
      char *bufferPtr = buffer;
      for (int k = 0x80; k <= 0xbf; ++k) {
        (*bufferPtr++) = (char)i;
        (*bufferPtr++) = (char)j;
        (*bufferPtr++) = (char)k;
      }
      nsString = [[NSString alloc] initWithBytes:buffer
                                          length:length
                                        encoding:NSUTF8StringEncoding];
      [nsStrings_ addObject:nsString];
      [nsString release];
      gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
      [gpbStrings_ addObject:gpbString];
      [gpbString release];
    }
  }
  for (int i = 0x80; i <= 0x9f; ++i) {
    char *bufferPtr = buffer;
    for (int j = 0x80; j <= 0xbf; ++j) {
      (*bufferPtr++) = (char)0xED;
      (*bufferPtr++) = (char)i;
      (*bufferPtr++) = (char)j;
    }
    nsString = [[NSString alloc] initWithBytes:buffer
                                        length:length
                                      encoding:NSUTF8StringEncoding];
    [nsStrings_ addObject:nsString];
    [nsString release];
    gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
    [gpbStrings_ addObject:gpbString];
    [gpbString release];
  }
  for (int i = 0xee; i <= 0xef; ++i) {
    for (int j = 0x80; j <= 0xbf; ++j) {
      char *bufferPtr = buffer;
      for (int k = 0x80; k <= 0xbf; ++k) {
        (*bufferPtr++) = (char)i;
        (*bufferPtr++) = (char)j;
        (*bufferPtr++) = (char)k;
      }
      nsString = [[NSString alloc] initWithBytes:buffer
                                          length:length
                                        encoding:NSUTF8StringEncoding];
      [nsStrings_ addObject:nsString];
      [nsString release];
      gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
      [gpbStrings_ addObject:gpbString];
      [gpbString release];
    }
  }
  free(buffer);

  // UTF8-4 - 4 Byte UTF 8 chars
  length = pointLength * 4;
  buffer = (char *)calloc(length, 1);
  for (int i = 0x90; i <= 0xbf; ++i) {
    for (int j = 0x80; j <= 0xbf; ++j) {
      char *bufferPtr = buffer;
      for (int k = 0x80; k <= 0xbf; ++k) {
        (*bufferPtr++) = (char)0xF0;
        (*bufferPtr++) = (char)i;
        (*bufferPtr++) = (char)j;
        (*bufferPtr++) = (char)k;
      }
      nsString = [[NSString alloc] initWithBytes:buffer
                                          length:length
                                        encoding:NSUTF8StringEncoding];
      [nsStrings_ addObject:nsString];
      [nsString release];
      gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
      [gpbStrings_ addObject:gpbString];
      [gpbString release];
    }
  }
  for (int i = 0xf1; i <= 0xf3; ++i) {
    for (int j = 0x80; j <= 0xbf; ++j) {
      for (int k = 0x80; k <= 0xbf; ++k) {
        char *bufferPtr = buffer;
        for (int m = 0x80; m <= 0xbf; ++m) {
          (*bufferPtr++) = (char)i;
          (*bufferPtr++) = (char)j;
          (*bufferPtr++) = (char)k;
          (*bufferPtr++) = (char)m;
        }
        nsString = [[NSString alloc] initWithBytes:buffer
                                            length:length
                                          encoding:NSUTF8StringEncoding];
        [nsStrings_ addObject:nsString];
        [nsString release];
        gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
        [gpbStrings_ addObject:gpbString];
        [gpbString release];
      }
    }
  }
  for (int i = 0x80; i <= 0x8f; ++i) {
    for (int j = 0x80; j <= 0xbf; ++j) {
      char *bufferPtr = buffer;
      for (int k = 0x80; k <= 0xbf; ++k) {
        (*bufferPtr++) = (char)0xF4;
        (*bufferPtr++) = (char)i;
        (*bufferPtr++) = (char)j;
        (*bufferPtr++) = (char)k;
      }
      nsString = [[NSString alloc] initWithBytes:buffer
                                          length:length
                                        encoding:NSUTF8StringEncoding];
      [nsStrings_ addObject:nsString];
      [nsString release];
      gpbString = GPBCreateGPBStringWithUTF8(buffer, length);
      [gpbStrings_ addObject:gpbString];
      [gpbString release];
    }
  }
  free(buffer);
}

- (void)tearDown {
  [nsStrings_ release];
  [gpbStrings_ release];
  [super tearDown];
}

- (void)testLength {
  size_t i = 0;
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = gpbStrings_[i];
    XCTAssertEqual([nsString length], [gpbString length], @"%@ %@", nsString,
                   gpbString);
    ++i;
  }
}

- (void)testLengthOfBytesUsingEncoding {
  NSStringEncoding encodings[] = {
    NSUTF8StringEncoding,
    NSASCIIStringEncoding,
    NSISOLatin1StringEncoding,
    NSMacOSRomanStringEncoding,
    NSUTF16StringEncoding,
    NSUTF32StringEncoding,
  };

  for (size_t j = 0; j < GPBARRAYSIZE(encodings); ++j) {
    NSStringEncoding testEncoding = encodings[j];
    size_t i = 0;
    for (NSString *nsString in nsStrings_) {
      GPBString *gpbString = gpbStrings_[i];
      XCTAssertEqual([nsString lengthOfBytesUsingEncoding:testEncoding],
                     [gpbString lengthOfBytesUsingEncoding:testEncoding],
                     @"%@ %@", nsString, gpbString);
      ++i;
    }
  }
}

- (void)testHash {
  size_t i = 0;
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = gpbStrings_[i];
    XCTAssertEqual([nsString hash], [gpbString hash], @"%@ %@", nsString,
                   gpbString);
    ++i;
  }
}

- (void)testEquality {
  size_t i = 0;
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = gpbStrings_[i];
    XCTAssertEqualObjects(nsString, gpbString);
    ++i;
  }
}

- (void)testCharacterAtIndex {
  size_t i = 0;
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = gpbStrings_[i];
    NSUInteger length = [nsString length];
    for (size_t j = 0; j < length; ++j) {
      unichar nsChar = [nsString characterAtIndex:j];
      unichar pbChar = [gpbString characterAtIndex:j];
      XCTAssertEqual(nsChar, pbChar, @"%@ %@ %zu", nsString, gpbString, j);
    }
    ++i;
  }
}

- (void)testCopy {
  size_t i = 0;
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = [[gpbStrings_[i] copy] autorelease];
    XCTAssertEqualObjects(nsString, gpbString);
    ++i;
  }
}

- (void)testMutableCopy {
  size_t i = 0;
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = [[gpbStrings_[i] mutableCopy] autorelease];
    XCTAssertEqualObjects(nsString, gpbString);
    ++i;
  }
}

- (void)testGetBytes {
  // Do an attempt at a reasonably exhaustive test of get bytes.
  // Get bytes with options other than 0 should always fall through to Apple
  // code so we don't bother testing that path.
  size_t i = 0;
  char pbBuffer[256];
  char nsBuffer[256];
  int count = 0;
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = gpbStrings_[i];
    for (int j = 0; j < 100; ++j) {
      // [NSString getBytes:maxLength:usedLength:encoding:options:range:remainingRange]
      // does not return reliable results if the maxLength argument is 0,
      // or range is 0,0.
      // Radar 16385183
      NSUInteger length = [nsString length];
      NSUInteger maxBufferCount = (arc4random() % (length + 3)) + 1;
      NSUInteger rangeStart = arc4random() % length;
      NSUInteger rangeLength = arc4random() % (length - rangeStart);

      NSRange range = NSMakeRange(rangeStart, rangeLength);

      NSStringEncoding encodings[] = {
        NSASCIIStringEncoding,
        NSUTF8StringEncoding,
        NSUTF16StringEncoding,
      };

      for (size_t k = 0; k < GPBARRAYSIZE(encodings); ++k) {
        NSStringEncoding encoding = encodings[k];
        NSUInteger pbUsedBufferCount = 0;
        NSUInteger nsUsedBufferCount = 0;
        NSRange pbLeftOver = NSMakeRange(0, 0);
        NSRange nsLeftOver = NSMakeRange(0, 0);

        BOOL pbGotBytes = [gpbString getBytes:pbBuffer
                                    maxLength:maxBufferCount
                                   usedLength:&pbUsedBufferCount
                                     encoding:encoding
                                      options:0
                                        range:range
                               remainingRange:&pbLeftOver];
        BOOL nsGotBytes = [nsString getBytes:nsBuffer
                                   maxLength:maxBufferCount
                                  usedLength:&nsUsedBufferCount
                                    encoding:encoding
                                     options:0
                                       range:range
                              remainingRange:&nsLeftOver];
        XCTAssertEqual(
            (bool)pbGotBytes, (bool)nsGotBytes,
            @"PB %d '%@' vs '%@' Encoding:%tu MaxLength: %tu Range: %@ "
            @"Used: %tu, %tu LeftOver %@, %@)",
            count, gpbString, nsString, encoding, maxBufferCount,
            NSStringFromRange(range), pbUsedBufferCount, nsUsedBufferCount,
            NSStringFromRange(pbLeftOver), NSStringFromRange(nsLeftOver));
        XCTAssertEqual(
            pbUsedBufferCount, nsUsedBufferCount,
            @"PB %d '%@' vs '%@' Encoding:%tu MaxLength: %tu Range: %@ "
            @"Used: %tu, %tu LeftOver %@, %@)",
            count, gpbString, nsString, encoding, maxBufferCount,
            NSStringFromRange(range), pbUsedBufferCount, nsUsedBufferCount,
            NSStringFromRange(pbLeftOver), NSStringFromRange(nsLeftOver));
        XCTAssertEqual(
            pbLeftOver.location, nsLeftOver.location,
            @"PB %d '%@' vs '%@' Encoding:%tu MaxLength: %tu Range: %@ "
            @"Used: %tu, %tu LeftOver %@, %@)",
            count, gpbString, nsString, encoding, maxBufferCount,
            NSStringFromRange(range), pbUsedBufferCount, nsUsedBufferCount,
            NSStringFromRange(pbLeftOver), NSStringFromRange(nsLeftOver));
        XCTAssertEqual(
            pbLeftOver.length, nsLeftOver.length,
            @"PB %d '%@' vs '%@' Encoding:%tu MaxLength: %tu Range: %@ "
            @"Used: %tu, %tu LeftOver %@, %@)",
            count, gpbString, nsString, encoding, maxBufferCount,
            NSStringFromRange(range), pbUsedBufferCount, nsUsedBufferCount,
            NSStringFromRange(pbLeftOver), NSStringFromRange(nsLeftOver));
        ++count;
      }
    }
    ++i;
  }
}

- (void)testLengthAndGetBytes {
  // This test exists as an attempt to ferret out a bug.
  // http://b/13516532
  size_t i = 0;
  char pbBuffer[256];
  char nsBuffer[256];
  for (NSString *nsString in nsStrings_) {
    GPBString *gpbString = gpbStrings_[i++];
    NSUInteger nsLength =
        [nsString lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger pbLength =
        [gpbString lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    XCTAssertEqual(nsLength, pbLength, @"%@ %@", nsString, gpbString);
    NSUInteger pbUsedBufferCount = 0;
    NSUInteger nsUsedBufferCount = 0;
    NSRange pbLeftOver = NSMakeRange(0, 0);
    NSRange nsLeftOver = NSMakeRange(0, 0);
    NSRange range = NSMakeRange(0, [gpbString length]);
    BOOL pbGotBytes = [gpbString getBytes:pbBuffer
                                maxLength:sizeof(pbBuffer)
                               usedLength:&pbUsedBufferCount
                                 encoding:NSUTF8StringEncoding
                                  options:0
                                    range:range
                           remainingRange:&pbLeftOver];
    BOOL nsGotBytes = [nsString getBytes:nsBuffer
                               maxLength:sizeof(nsBuffer)
                              usedLength:&nsUsedBufferCount
                                encoding:NSUTF8StringEncoding
                                 options:0
                                   range:range
                          remainingRange:&nsLeftOver];
    XCTAssertTrue(pbGotBytes, @"%@", gpbString);
    XCTAssertTrue(nsGotBytes, @"%@", nsString);
    XCTAssertEqual(pbUsedBufferCount, pbLength, @"%@", gpbString);
    XCTAssertEqual(nsUsedBufferCount, nsLength, @"%@", nsString);
  }
}

@end
