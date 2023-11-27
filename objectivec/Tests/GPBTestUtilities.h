// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
- (void)setAllExtensions:(TestAllExtensions *)message repeatedCount:(uint32_t)count;
- (void)setPackedFields:(TestPackedTypes *)message repeatedCount:(uint32_t)count;
- (void)setUnpackedFields:(TestUnpackedTypes *)message repeatedCount:(uint32_t)count;
- (void)setPackedExtensions:(TestPackedExtensions *)message repeatedCount:(uint32_t)count;
- (void)setUnpackedExtensions:(TestUnpackedExtensions *)message repeatedCount:(uint32_t)count;
- (void)setAllMapFields:(TestMap *)message numEntries:(uint32_t)count;

- (TestAllTypes *)allSetRepeatedCount:(uint32_t)count;
- (TestAllExtensions *)allExtensionsSetRepeatedCount:(uint32_t)count;
- (TestPackedTypes *)packedSetRepeatedCount:(uint32_t)count;
- (TestPackedExtensions *)packedExtensionsSetRepeatedCount:(uint32_t)count;

- (void)assertAllFieldsSet:(TestAllTypes *)message repeatedCount:(uint32_t)count;
- (void)assertAllExtensionsSet:(TestAllExtensions *)message repeatedCount:(uint32_t)count;
- (void)assertRepeatedFieldsModified:(TestAllTypes *)message repeatedCount:(uint32_t)count;
- (void)assertRepeatedExtensionsModified:(TestAllExtensions *)message repeatedCount:(uint32_t)count;
- (void)assertExtensionsClear:(TestAllExtensions *)message;
- (void)assertClear:(TestAllTypes *)message;
- (void)assertPackedFieldsSet:(TestPackedTypes *)message repeatedCount:(uint32_t)count;
- (void)assertPackedExtensionsSet:(TestPackedExtensions *)message repeatedCount:(uint32_t)count;

- (void)modifyRepeatedExtensions:(TestAllExtensions *)message;
- (void)modifyRepeatedFields:(TestAllTypes *)message;

- (GPBExtensionRegistry *)extensionRegistry;

- (NSData *)getDataFileNamed:(NSString *)name dataToWrite:(NSData *)dataToWrite;

- (void)assertAllFieldsKVCMatch:(TestAllTypes *)message;
- (void)setAllFieldsViaKVC:(TestAllTypes *)message repeatedCount:(uint32_t)count;
- (void)assertClearKVC:(TestAllTypes *)message;

@end
