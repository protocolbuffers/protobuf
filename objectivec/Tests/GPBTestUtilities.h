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

@class TestAllExtensions;
@class TestAllTypes;
@class TestMap;
@class TestPackedTypes;
@class TestPackedExtensions;
@class TestUnpackedTypes;
@class TestUnpackedExtensions;
@class GPBExtensionRegistry;

static inline NSData *DataFromCStr(const char *str) {
  return [NSData dataWithBytes:str length:strlen(str)];
}

// Helper for uses of C arrays in tests cases.
#ifndef GPBARRAYSIZE
#define GPBARRAYSIZE(a) ((sizeof(a) / sizeof((a[0]))))
#endif  // GPBARRAYSIZE


// The number of repetitions of any repeated objects inside of test messages.
extern const uint32_t kGPBDefaultRepeatCount;

@interface GPBTestCase : XCTestCase

- (void)setAllFields:(TestAllTypes *)message repeatedCount:(uint32_t)count;
- (void)clearAllFields:(TestAllTypes *)message;
- (void)setAllExtensions:(TestAllExtensions *)message
           repeatedCount:(uint32_t)count;
- (void)setPackedFields:(TestPackedTypes *)message
          repeatedCount:(uint32_t)count;
- (void)setUnpackedFields:(TestUnpackedTypes *)message
            repeatedCount:(uint32_t)count;
- (void)setPackedExtensions:(TestPackedExtensions *)message
              repeatedCount:(uint32_t)count;
- (void)setUnpackedExtensions:(TestUnpackedExtensions *)message
              repeatedCount:(uint32_t)count;
- (void)setAllMapFields:(TestMap *)message numEntries:(uint32_t)count;

- (TestAllTypes *)allSetRepeatedCount:(uint32_t)count;
- (TestAllExtensions *)allExtensionsSetRepeatedCount:(uint32_t)count;
- (TestPackedTypes *)packedSetRepeatedCount:(uint32_t)count;
- (TestPackedExtensions *)packedExtensionsSetRepeatedCount:(uint32_t)count;

- (void)assertAllFieldsSet:(TestAllTypes *)message
             repeatedCount:(uint32_t)count;
- (void)assertAllExtensionsSet:(TestAllExtensions *)message
                 repeatedCount:(uint32_t)count;
- (void)assertRepeatedFieldsModified:(TestAllTypes *)message
                       repeatedCount:(uint32_t)count;
- (void)assertRepeatedExtensionsModified:(TestAllExtensions *)message
                           repeatedCount:(uint32_t)count;
- (void)assertExtensionsClear:(TestAllExtensions *)message;
- (void)assertClear:(TestAllTypes *)message;
- (void)assertPackedFieldsSet:(TestPackedTypes *)message
                repeatedCount:(uint32_t)count;
- (void)assertPackedExtensionsSet:(TestPackedExtensions *)message
                    repeatedCount:(uint32_t)count;

- (void)modifyRepeatedExtensions:(TestAllExtensions *)message;
- (void)modifyRepeatedFields:(TestAllTypes *)message;

- (GPBExtensionRegistry *)extensionRegistry;

- (NSData *)getDataFileNamed:(NSString *)name dataToWrite:(NSData *)dataToWrite;

- (void)assertAllFieldsKVCMatch:(TestAllTypes *)message;
- (void)setAllFieldsViaKVC:(TestAllTypes *)message
             repeatedCount:(uint32_t)count;
- (void)assertClearKVC:(TestAllTypes *)message;

@end
