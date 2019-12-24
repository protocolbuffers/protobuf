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

#import "GPBTestUtilities.h"

#import "google/protobuf/MapUnittest.pbobjc.h"
#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestImport.pbobjc.h"

const uint32_t kGPBDefaultRepeatCount = 2;

// Small category to easily turn a CString into an NSData.
@interface NSData (GPBTestCase)
+ (NSData *)gpbtu_dataWithCString:(char *)buffer;
+ (instancetype)gpbtu_dataWithEmbeddedNulls;
@end

@implementation NSData (GPBTestCase)
+ (NSData *)gpbtu_dataWithCString:(char *)buffer {
  return [NSData dataWithBytes:buffer length:strlen(buffer)];
}

+ (instancetype)gpbtu_dataWithUint32:(uint32_t)value {
  return [[[self alloc] initWithUint32_gpbtu:value] autorelease];
}

- (instancetype)initWithUint32_gpbtu:(uint32_t)value {
  value = CFSwapInt32HostToLittle(value);
  return [self initWithBytes:&value length:sizeof(value)];
}

+ (instancetype)gpbtu_dataWithEmbeddedNulls {
  char bytes[6] = "\1\0\2\3\0\5";
  return [self dataWithBytes:bytes length:sizeof(bytes)];
}
@end

@implementation GPBTestCase

// Return data for name. Optionally (based on #if setting) write out dataToWrite
// to replace that data. Useful for setting golden masters.
- (NSData *)getDataFileNamed:(NSString *)name
                 dataToWrite:(NSData *)dataToWrite {
  NSBundle *bundle = [NSBundle bundleForClass:[self class]];
  NSString *path = [bundle pathForResource:[name stringByDeletingPathExtension]
                                    ofType:[name pathExtension]];
  XCTAssertNotNil(path, @"Unable to find %@", name);
  NSData *data = [NSData dataWithContentsOfFile:path];
  XCTAssertNotNil(data, @"Unable to load from %@", path);
#if 0
  // Enable to write out golden master files.
  if (!path) {
    path = [[bundle resourcePath] stringByAppendingPathComponent:name];
  }
  NSError *error = nil;
  BOOL wrote = [dataToWrite writeToFile:path options:NSDataWritingAtomic error:&error];
  XCTAssertTrue(wrote, @"Unable to write %@ (%@)", path, error);
  NSLog(@"Wrote data file to %@", path);
#else
  // Kill off the unused variable warning.
  (void)dataToWrite;
#endif
  return data;
}

// -------------------------------------------------------------------

- (void)modifyRepeatedExtensions:(TestAllExtensions *)message {
  [message setExtension:[UnittestRoot repeatedInt32Extension]
                  index:1
                  value:@501];
  [message setExtension:[UnittestRoot repeatedInt64Extension]
                  index:1
                  value:@502];
  [message setExtension:[UnittestRoot repeatedUint32Extension]
                  index:1
                  value:@503];
  [message setExtension:[UnittestRoot repeatedUint64Extension]
                  index:1
                  value:@504];
  [message setExtension:[UnittestRoot repeatedSint32Extension]
                  index:1
                  value:@505];
  [message setExtension:[UnittestRoot repeatedSint64Extension]
                  index:1
                  value:@506];
  [message setExtension:[UnittestRoot repeatedFixed32Extension]
                  index:1
                  value:@507];
  [message setExtension:[UnittestRoot repeatedFixed64Extension]
                  index:1
                  value:@508];
  [message setExtension:[UnittestRoot repeatedSfixed32Extension]
                  index:1
                  value:@509];
  [message setExtension:[UnittestRoot repeatedSfixed64Extension]
                  index:1
                  value:@510];
  [message setExtension:[UnittestRoot repeatedFloatExtension]
                  index:1
                  value:@511.0f];
  [message setExtension:[UnittestRoot repeatedDoubleExtension]
                  index:1
                  value:@512.0];
  [message setExtension:[UnittestRoot repeatedBoolExtension]
                  index:1
                  value:@YES];
  [message setExtension:[UnittestRoot repeatedStringExtension]
                  index:1
                  value:@"515"];
  [message setExtension:[UnittestRoot repeatedBytesExtension]
                  index:1
                  value:[NSData gpbtu_dataWithUint32:516]];

  RepeatedGroup_extension *repeatedGroup = [RepeatedGroup_extension message];
  [repeatedGroup setA:517];
  [message setExtension:[UnittestRoot repeatedGroupExtension]
                  index:1
                  value:repeatedGroup];
  TestAllTypes_NestedMessage *nestedMessage =
      [TestAllTypes_NestedMessage message];
  [nestedMessage setBb:518];
  [message setExtension:[UnittestRoot repeatedNestedMessageExtension]
                  index:1
                  value:nestedMessage];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [foreignMessage setC:519];
  [message setExtension:[UnittestRoot repeatedForeignMessageExtension]
                  index:1
                  value:foreignMessage];
  ImportMessage *importMessage = [ImportMessage message];
  [importMessage setD:520];
  [message setExtension:[UnittestRoot repeatedImportMessageExtension]
                  index:1
                  value:importMessage];

  [message setExtension:[UnittestRoot repeatedNestedEnumExtension]
                  index:1
                  value:@(TestAllTypes_NestedEnum_Foo)];
  [message setExtension:[UnittestRoot repeatedForeignEnumExtension]
                  index:1
                  value:@(ForeignEnum_ForeignFoo)];
  [message setExtension:[UnittestRoot repeatedImportEnumExtension]
                  index:1
                  value:@(ImportEnum_ImportFoo)];

  [message setExtension:[UnittestRoot repeatedStringPieceExtension]
                  index:1
                  value:@"524"];
  [message setExtension:[UnittestRoot repeatedCordExtension]
                  index:1
                  value:@"525"];
}

- (void)assertAllExtensionsSet:(TestAllExtensions *)message
                 repeatedCount:(uint32_t)count {
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalInt32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalInt64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalUint32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalUint64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalSint32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalSint64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalFixed32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalFixed64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalSfixed32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalSfixed64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalFloatExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalDoubleExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalBoolExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalStringExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalBytesExtension]]);

  XCTAssertTrue([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalForeignMessageExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalImportMessageExtension]]);

  XCTAssertTrue([[message getExtension:[UnittestRoot optionalGroupExtension]] hasA]);
  XCTAssertTrue([[message getExtension:[UnittestRoot optionalNestedMessageExtension]] hasBb]);
  XCTAssertTrue([[message getExtension:[UnittestRoot optionalForeignMessageExtension]] hasC]);
  XCTAssertTrue([[message getExtension:[UnittestRoot optionalImportMessageExtension]] hasD]);

  XCTAssertTrue([message hasExtension:[UnittestRoot optionalNestedEnumExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalForeignEnumExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalImportEnumExtension]]);

  XCTAssertTrue([message hasExtension:[UnittestRoot optionalStringPieceExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot optionalCordExtension]]);

  XCTAssertTrue([message hasExtension:[UnittestRoot defaultInt32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultInt64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultUint32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultUint64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSint32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSint64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultFixed32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultFixed64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSfixed32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSfixed64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultFloatExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultDoubleExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultBoolExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultStringExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultBytesExtension]]);

  XCTAssertTrue([message hasExtension:[UnittestRoot defaultNestedEnumExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultForeignEnumExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultImportEnumExtension]]);

  XCTAssertTrue([message hasExtension:[UnittestRoot defaultStringPieceExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultCordExtension]]);

  XCTAssertEqual(101, [[message getExtension:[UnittestRoot optionalInt32Extension]] intValue]);
  XCTAssertEqual(102LL, [[message getExtension:[UnittestRoot optionalInt64Extension]] longLongValue]);
  XCTAssertEqual(103U, [[message getExtension:[UnittestRoot optionalUint32Extension]] unsignedIntValue]);
  XCTAssertEqual(104ULL, [[message getExtension:[UnittestRoot optionalUint64Extension]] unsignedLongLongValue]);
  XCTAssertEqual(105, [[message getExtension:[UnittestRoot optionalSint32Extension]] intValue]);
  XCTAssertEqual(106LL, [[message getExtension:[UnittestRoot optionalSint64Extension]] longLongValue]);
  XCTAssertEqual(107U, [[message getExtension:[UnittestRoot optionalFixed32Extension]] unsignedIntValue]);
  XCTAssertEqual(108ULL, [[message getExtension:[UnittestRoot optionalFixed64Extension]] unsignedLongLongValue]);
  XCTAssertEqual(109, [[message getExtension:[UnittestRoot optionalSfixed32Extension]] intValue]);
  XCTAssertEqual(110LL, [[message getExtension:[UnittestRoot optionalSfixed64Extension]] longLongValue]);
  XCTAssertEqualWithAccuracy(111.0f, [[message getExtension:[UnittestRoot optionalFloatExtension]] floatValue], 0.01);
  XCTAssertEqualWithAccuracy(112.0, [[message getExtension:[UnittestRoot optionalDoubleExtension]] doubleValue], 0.01);
  XCTAssertTrue([[message getExtension:[UnittestRoot optionalBoolExtension]] boolValue]);
  XCTAssertEqualObjects(@"115", [message getExtension:[UnittestRoot optionalStringExtension]]);
  XCTAssertEqualObjects([NSData gpbtu_dataWithEmbeddedNulls], [message getExtension:[UnittestRoot optionalBytesExtension]]);

  XCTAssertEqual(117, [(TestAllTypes_OptionalGroup*)[message getExtension:[UnittestRoot optionalGroupExtension]] a]);
  XCTAssertEqual(118, [(TestAllTypes_NestedMessage*)[message getExtension:[UnittestRoot optionalNestedMessageExtension]] bb]);
  XCTAssertEqual(119, [[message getExtension:[UnittestRoot optionalForeignMessageExtension]] c]);
  XCTAssertEqual(120, [[message getExtension:[UnittestRoot optionalImportMessageExtension]] d]);

  XCTAssertEqual(TestAllTypes_NestedEnum_Baz, [[message getExtension:[UnittestRoot optionalNestedEnumExtension]] intValue]);
  XCTAssertEqual(ForeignEnum_ForeignBaz, [[message getExtension:[UnittestRoot optionalForeignEnumExtension]] intValue]);
  XCTAssertEqual(ImportEnum_ImportBaz, [[message getExtension:[UnittestRoot optionalImportEnumExtension]] intValue]);

  XCTAssertEqualObjects(@"124", [message getExtension:[UnittestRoot optionalStringPieceExtension]]);
  XCTAssertEqualObjects(@"125", [message getExtension:[UnittestRoot optionalCordExtension]]);

  // -----------------------------------------------------------------

  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedInt32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedInt64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedUint32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedUint64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSint32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSint64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedFixed32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedFixed64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSfixed32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSfixed64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedFloatExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedDoubleExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedBoolExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedStringExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedBytesExtension]] count]);

  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedGroupExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedNestedMessageExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedForeignMessageExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedImportMessageExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedNestedEnumExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedForeignEnumExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedImportEnumExtension]] count]);

  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedStringPieceExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedCordExtension]] count]);

  for (uint32_t i = 0; i < count; ++i) {
    id extension = [message getExtension:[UnittestRoot repeatedInt32Extension]];
    XCTAssertEqual((int)(201 + i * 100), [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot repeatedInt64Extension]];
    XCTAssertEqual(202 + i * 100, [extension[i] longLongValue]);
    extension = [message getExtension:[UnittestRoot repeatedUint32Extension]];
    XCTAssertEqual(203 + i * 100, [extension[i] unsignedIntValue]);
    extension = [message getExtension:[UnittestRoot repeatedUint64Extension]];
    XCTAssertEqual(204 + i * 100, [extension[i] unsignedLongLongValue]);
    extension = [message getExtension:[UnittestRoot repeatedSint32Extension]];
    XCTAssertEqual((int)(205 + i * 100), [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot repeatedSint64Extension]];
    XCTAssertEqual(206 + i * 100, [extension[i] longLongValue]);
    extension = [message getExtension:[UnittestRoot repeatedFixed32Extension]];
    XCTAssertEqual(207 + i * 100, [extension[i] unsignedIntValue]);
    extension = [message getExtension:[UnittestRoot repeatedFixed64Extension]];
    XCTAssertEqual(208 + i * 100, [extension[i] unsignedLongLongValue]);
    extension = [message getExtension:[UnittestRoot repeatedSfixed32Extension]];
    XCTAssertEqual((int)(209 + i * 100), [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot repeatedSfixed64Extension]];
    XCTAssertEqual(210 + i * 100, [extension[i] longLongValue]);
    extension = [message getExtension:[UnittestRoot repeatedFloatExtension]];
    XCTAssertEqualWithAccuracy(211 + i * 100, [extension[i] floatValue], 0.01);
    extension = [message getExtension:[UnittestRoot repeatedDoubleExtension]];
    XCTAssertEqualWithAccuracy(212 + i * 100, [extension[i] doubleValue], 0.01);
    extension = [message getExtension:[UnittestRoot repeatedBoolExtension]];
    XCTAssertEqual((i % 2) ? YES : NO, [extension[i] boolValue]);

    NSString *string = [[NSString alloc] initWithFormat:@"%d", 215 + i * 100];
    extension = [message getExtension:[UnittestRoot repeatedStringExtension]];
    XCTAssertEqualObjects(string, extension[i]);
    [string release];

    NSData *data = [[NSData alloc] initWithUint32_gpbtu:216 + i * 100];
    extension = [message getExtension:[UnittestRoot repeatedBytesExtension]];
    XCTAssertEqualObjects(data, extension[i]);
    [data release];

    extension = [message getExtension:[UnittestRoot repeatedGroupExtension]];
    XCTAssertEqual((int)(217 + i * 100), [(TestAllTypes_OptionalGroup*)extension[i] a]);
    extension = [message getExtension:[UnittestRoot repeatedNestedMessageExtension]];
    XCTAssertEqual((int)(218 + i * 100), [(TestAllTypes_NestedMessage*)extension[i] bb]);
    extension = [message getExtension:[UnittestRoot repeatedForeignMessageExtension]];
    XCTAssertEqual((int)(219 + i * 100), [extension[i] c]);
    extension = [message getExtension:[UnittestRoot repeatedImportMessageExtension]];
    XCTAssertEqual((int)(220 + i * 100), [extension[i] d]);

    extension = [message getExtension:[UnittestRoot repeatedNestedEnumExtension]];
    XCTAssertEqual((i % 2) ? TestAllTypes_NestedEnum_Bar : TestAllTypes_NestedEnum_Baz, [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot repeatedForeignEnumExtension]];
    XCTAssertEqual((i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz, [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot repeatedImportEnumExtension]];
    XCTAssertEqual((i % 2) ? ImportEnum_ImportBar : ImportEnum_ImportBaz, [extension[i] intValue]);

    string = [[NSString alloc] initWithFormat:@"%d", 224 + i * 100];
    extension = [message getExtension:[UnittestRoot repeatedStringPieceExtension]];
    XCTAssertEqualObjects(string, extension[i]);
    [string release];

    string = [[NSString alloc] initWithFormat:@"%d", 225 + i * 100];
    extension = [message getExtension:[UnittestRoot repeatedCordExtension]];
    XCTAssertEqualObjects(string, extension[i]);
    [string release];
  }

  // -----------------------------------------------------------------

  XCTAssertTrue([message hasExtension:[UnittestRoot defaultInt32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultInt64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultUint32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultUint64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSint32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSint64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultFixed32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultFixed64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSfixed32Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultSfixed64Extension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultFloatExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultDoubleExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultBoolExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultStringExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultBytesExtension]]);

  XCTAssertTrue([message hasExtension:[UnittestRoot defaultNestedEnumExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultForeignEnumExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultImportEnumExtension]]);

  XCTAssertTrue([message hasExtension:[UnittestRoot defaultStringPieceExtension]]);
  XCTAssertTrue([message hasExtension:[UnittestRoot defaultCordExtension]]);

  XCTAssertEqual(401, [[message getExtension:[UnittestRoot defaultInt32Extension]] intValue]);
  XCTAssertEqual(402LL, [[message getExtension:[UnittestRoot defaultInt64Extension]] longLongValue]);
  XCTAssertEqual(403U, [[message getExtension:[UnittestRoot defaultUint32Extension]] unsignedIntValue]);
  XCTAssertEqual(404ULL, [[message getExtension:[UnittestRoot defaultUint64Extension]] unsignedLongLongValue]);
  XCTAssertEqual(405, [[message getExtension:[UnittestRoot defaultSint32Extension]] intValue]);
  XCTAssertEqual(406LL, [[message getExtension:[UnittestRoot defaultSint64Extension]] longLongValue]);
  XCTAssertEqual(407U, [[message getExtension:[UnittestRoot defaultFixed32Extension]] unsignedIntValue]);
  XCTAssertEqual(408ULL, [[message getExtension:[UnittestRoot defaultFixed64Extension]] unsignedLongLongValue]);
  XCTAssertEqual(409, [[message getExtension:[UnittestRoot defaultSfixed32Extension]] intValue]);
  XCTAssertEqual(410LL,[[message getExtension:[UnittestRoot defaultSfixed64Extension]] longLongValue]);
  XCTAssertEqualWithAccuracy(411.0f, [[message getExtension:[UnittestRoot defaultFloatExtension]] floatValue], 0.01);
  XCTAssertEqualWithAccuracy(412.0, [[message getExtension:[UnittestRoot defaultDoubleExtension]] doubleValue], 0.01);
  XCTAssertFalse([[message getExtension:[UnittestRoot defaultBoolExtension]] boolValue]);
  XCTAssertEqualObjects(@"415", [message getExtension:[UnittestRoot defaultStringExtension]]);
  XCTAssertEqualObjects([NSData gpbtu_dataWithUint32:416], [message getExtension:[UnittestRoot defaultBytesExtension]]);

  XCTAssertEqual(TestAllTypes_NestedEnum_Foo, [[message getExtension:[UnittestRoot defaultNestedEnumExtension]] intValue]);
  XCTAssertEqual(ForeignEnum_ForeignFoo, [[message getExtension:[UnittestRoot defaultForeignEnumExtension]] intValue]);
  XCTAssertEqual(ImportEnum_ImportFoo, [[message getExtension:[UnittestRoot defaultImportEnumExtension]] intValue]);

  XCTAssertEqualObjects(@"424", [message getExtension:[UnittestRoot defaultStringPieceExtension]]);
  XCTAssertEqualObjects(@"425", [message getExtension:[UnittestRoot defaultCordExtension]]);
}

- (void)assertRepeatedExtensionsModified:(TestAllExtensions *)message
                           repeatedCount:(uint32_t)count {
  // ModifyRepeatedFields only sets the second repeated element of each
  // field.  In addition to verifying this, we also verify that the first
  // element and size were *not* modified.
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedInt32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedInt64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedUint32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedUint64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSint32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSint64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedFixed32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedFixed64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSfixed32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedSfixed64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedFloatExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedDoubleExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedBoolExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedStringExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedBytesExtension]] count]);

  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedGroupExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedNestedMessageExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedForeignMessageExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedImportMessageExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedNestedEnumExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedForeignEnumExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedImportEnumExtension]] count]);

  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedStringPieceExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot repeatedCordExtension]] count]);

  XCTAssertEqual(201,[[message getExtension:[UnittestRoot repeatedInt32Extension]][0] intValue]);
  XCTAssertEqual(202LL, [[message getExtension:[UnittestRoot repeatedInt64Extension]][0] longLongValue]);
  XCTAssertEqual(203U, [[message getExtension:[UnittestRoot repeatedUint32Extension]][0] unsignedIntValue]);
  XCTAssertEqual(204ULL, [[message getExtension:[UnittestRoot repeatedUint64Extension]][0] unsignedLongLongValue]);
  XCTAssertEqual(205, [[message getExtension:[UnittestRoot repeatedSint32Extension]][0] intValue]);
  XCTAssertEqual(206LL, [[message getExtension:[UnittestRoot repeatedSint64Extension]][0] longLongValue]);
  XCTAssertEqual(207U, [[message getExtension:[UnittestRoot repeatedFixed32Extension]][0] unsignedIntValue]);
  XCTAssertEqual(208ULL, [[message getExtension:[UnittestRoot repeatedFixed64Extension]][0] unsignedLongLongValue]);
  XCTAssertEqual(209, [[message getExtension:[UnittestRoot repeatedSfixed32Extension]][0] intValue]);
  XCTAssertEqual(210LL, [[message getExtension:[UnittestRoot repeatedSfixed64Extension]][0] longLongValue]);
  XCTAssertEqualWithAccuracy(211.0f, [[message getExtension:[UnittestRoot repeatedFloatExtension]][0] floatValue], 0.01);
  XCTAssertEqualWithAccuracy(212.0, [[message getExtension:[UnittestRoot repeatedDoubleExtension]][0] doubleValue], 0.01);
  XCTAssertFalse([[message getExtension:[UnittestRoot repeatedBoolExtension]][0] boolValue]);
  XCTAssertEqualObjects(@"215", [message getExtension:[UnittestRoot repeatedStringExtension]][0]);
  XCTAssertEqualObjects([NSData gpbtu_dataWithUint32:216], [message getExtension:[UnittestRoot repeatedBytesExtension]][0]);

  XCTAssertEqual(217, [(TestAllTypes_OptionalGroup*)[message getExtension:[UnittestRoot repeatedGroupExtension]][0] a]);
  XCTAssertEqual(218, [(TestAllTypes_NestedMessage*)[message getExtension:[UnittestRoot repeatedNestedMessageExtension]][0] bb]);
  XCTAssertEqual(219, [[message getExtension:[UnittestRoot repeatedForeignMessageExtension]][0] c]);
  XCTAssertEqual(220, [[message getExtension:[UnittestRoot repeatedImportMessageExtension]][0] d]);

  XCTAssertEqual(TestAllTypes_NestedEnum_Baz,
                 [[message getExtension:[UnittestRoot repeatedNestedEnumExtension]][0] intValue]);
  XCTAssertEqual(ForeignEnum_ForeignBaz,
                 [[message getExtension:[UnittestRoot repeatedForeignEnumExtension]][0] intValue]);
  XCTAssertEqual(ImportEnum_ImportBaz,
                 [[message getExtension:[UnittestRoot repeatedImportEnumExtension]][0] intValue]);

  XCTAssertEqualObjects(@"224", [message getExtension:[UnittestRoot repeatedStringPieceExtension]][0]);
  XCTAssertEqualObjects(@"225", [message getExtension:[UnittestRoot repeatedCordExtension]][0]);

  // Actually verify the second (modified) elements now.
  XCTAssertEqual(501, [[message getExtension:[UnittestRoot repeatedInt32Extension]][1] intValue]);
  XCTAssertEqual(502LL, [[message getExtension:[UnittestRoot repeatedInt64Extension]][1] longLongValue]);
  XCTAssertEqual(503U, [[message getExtension:[UnittestRoot repeatedUint32Extension]][1] unsignedIntValue]);
  XCTAssertEqual(504ULL, [[message getExtension:[UnittestRoot repeatedUint64Extension]][1] unsignedLongLongValue]);
  XCTAssertEqual(505, [[message getExtension:[UnittestRoot repeatedSint32Extension]][1] intValue]);
  XCTAssertEqual(506LL, [[message getExtension:[UnittestRoot repeatedSint64Extension]][1] longLongValue]);
  XCTAssertEqual(507U, [[message getExtension:[UnittestRoot repeatedFixed32Extension]][1] unsignedIntValue]);
  XCTAssertEqual(508ULL, [[message getExtension:[UnittestRoot repeatedFixed64Extension]][1] unsignedLongLongValue]);
  XCTAssertEqual(509, [[message getExtension:[UnittestRoot repeatedSfixed32Extension]][1] intValue]);
  XCTAssertEqual(510LL, [[message getExtension:[UnittestRoot repeatedSfixed64Extension]][1] longLongValue]);
  XCTAssertEqualWithAccuracy(511.0f, [[message getExtension:[UnittestRoot repeatedFloatExtension]][1] floatValue], 0.01);
  XCTAssertEqualWithAccuracy(512.0, [[message getExtension:[UnittestRoot repeatedDoubleExtension]][1] doubleValue], 0.01);
  XCTAssertTrue([[message getExtension:[UnittestRoot repeatedBoolExtension]][1] boolValue]);
  XCTAssertEqualObjects(@"515", [message getExtension:[UnittestRoot repeatedStringExtension]][1]);
  XCTAssertEqualObjects([NSData gpbtu_dataWithUint32:516], [message getExtension:[UnittestRoot repeatedBytesExtension]][1]);

  XCTAssertEqual(517, [(TestAllTypes_OptionalGroup*)[message getExtension:[UnittestRoot repeatedGroupExtension]][1] a]);
  XCTAssertEqual(518, [(TestAllTypes_NestedMessage*)[message getExtension:[UnittestRoot repeatedNestedMessageExtension]][1] bb]);
  XCTAssertEqual(519, [[message getExtension:[UnittestRoot repeatedForeignMessageExtension]][1] c]);
  XCTAssertEqual(520, [[message getExtension:[UnittestRoot repeatedImportMessageExtension]][1] d]);

  XCTAssertEqual(TestAllTypes_NestedEnum_Foo,
                 [[message getExtension:[UnittestRoot repeatedNestedEnumExtension]][1] intValue]);
  XCTAssertEqual(ForeignEnum_ForeignFoo,
                 [[message getExtension:[UnittestRoot repeatedForeignEnumExtension]][1] intValue]);
  XCTAssertEqual(ImportEnum_ImportFoo,
                 [[message getExtension:[UnittestRoot repeatedImportEnumExtension]][1] intValue]);

  XCTAssertEqualObjects(@"524", [message getExtension:[UnittestRoot repeatedStringPieceExtension]][1]);
  XCTAssertEqualObjects(@"525", [message getExtension:[UnittestRoot repeatedCordExtension]][1]);
}

// -------------------------------------------------------------------

- (void)assertAllFieldsSet:(TestAllTypes *)message
             repeatedCount:(uint32_t)count {
  XCTAssertTrue(message.hasOptionalInt32);
  XCTAssertTrue(message.hasOptionalInt64);
  XCTAssertTrue(message.hasOptionalUint32);
  XCTAssertTrue(message.hasOptionalUint64);
  XCTAssertTrue(message.hasOptionalSint32);
  XCTAssertTrue(message.hasOptionalSint64);
  XCTAssertTrue(message.hasOptionalFixed32);
  XCTAssertTrue(message.hasOptionalFixed64);
  XCTAssertTrue(message.hasOptionalSfixed32);
  XCTAssertTrue(message.hasOptionalSfixed64);
  XCTAssertTrue(message.hasOptionalFloat);
  XCTAssertTrue(message.hasOptionalDouble);
  XCTAssertTrue(message.hasOptionalBool);
  XCTAssertTrue(message.hasOptionalString);
  XCTAssertTrue(message.hasOptionalBytes);

  XCTAssertTrue(message.hasOptionalGroup);
  XCTAssertTrue(message.hasOptionalNestedMessage);
  XCTAssertTrue(message.hasOptionalForeignMessage);
  XCTAssertTrue(message.hasOptionalImportMessage);

  XCTAssertTrue(message.optionalGroup.hasA);
  XCTAssertTrue(message.optionalNestedMessage.hasBb);
  XCTAssertTrue(message.optionalForeignMessage.hasC);
  XCTAssertTrue(message.optionalImportMessage.hasD);

  XCTAssertTrue(message.hasOptionalNestedEnum);
  XCTAssertTrue(message.hasOptionalForeignEnum);
  XCTAssertTrue(message.hasOptionalImportEnum);

  XCTAssertTrue(message.hasOptionalStringPiece);
  XCTAssertTrue(message.hasOptionalCord);

  XCTAssertEqual(101, message.optionalInt32);
  XCTAssertEqual(102LL, message.optionalInt64);
  XCTAssertEqual(103U, message.optionalUint32);
  XCTAssertEqual(104ULL, message.optionalUint64);
  XCTAssertEqual(105, message.optionalSint32);
  XCTAssertEqual(106LL, message.optionalSint64);
  XCTAssertEqual(107U, message.optionalFixed32);
  XCTAssertEqual(108ULL, message.optionalFixed64);
  XCTAssertEqual(109, message.optionalSfixed32);
  XCTAssertEqual(110LL, message.optionalSfixed64);
  XCTAssertEqualWithAccuracy(111.0f, message.optionalFloat, 0.1);
  XCTAssertEqualWithAccuracy(112.0, message.optionalDouble, 0.1);
  XCTAssertTrue(message.optionalBool);
  XCTAssertEqualObjects(@"115", message.optionalString);
  XCTAssertEqualObjects([NSData gpbtu_dataWithEmbeddedNulls],
                        message.optionalBytes);

  XCTAssertEqual(117, message.optionalGroup.a);
  XCTAssertEqual(118, message.optionalNestedMessage.bb);
  XCTAssertEqual(119, message.optionalForeignMessage.c);
  XCTAssertEqual(120, message.optionalImportMessage.d);

  XCTAssertEqual(TestAllTypes_NestedEnum_Baz, message.optionalNestedEnum);
  XCTAssertEqual(ForeignEnum_ForeignBaz, message.optionalForeignEnum);
  XCTAssertEqual(ImportEnum_ImportBaz, message.optionalImportEnum);

  XCTAssertEqualObjects(@"124", message.optionalStringPiece);
  XCTAssertEqualObjects(@"125", message.optionalCord);

  // -----------------------------------------------------------------

  XCTAssertEqual(count, message.repeatedInt32Array.count);
  XCTAssertEqual(count, message.repeatedInt64Array.count);
  XCTAssertEqual(count, message.repeatedUint32Array.count);
  XCTAssertEqual(count, message.repeatedUint64Array.count);
  XCTAssertEqual(count, message.repeatedSint32Array.count);
  XCTAssertEqual(count, message.repeatedSint64Array.count);
  XCTAssertEqual(count, message.repeatedFixed32Array.count);
  XCTAssertEqual(count, message.repeatedFixed64Array.count);
  XCTAssertEqual(count, message.repeatedSfixed32Array.count);
  XCTAssertEqual(count, message.repeatedSfixed64Array.count);
  XCTAssertEqual(count, message.repeatedFloatArray.count);
  XCTAssertEqual(count, message.repeatedDoubleArray.count);
  XCTAssertEqual(count, message.repeatedBoolArray.count);
  XCTAssertEqual(count, message.repeatedStringArray.count);
  XCTAssertEqual(count, message.repeatedBytesArray.count);

  XCTAssertEqual(count, message.repeatedGroupArray.count);
  XCTAssertEqual(count, message.repeatedNestedMessageArray.count);
  XCTAssertEqual(count, message.repeatedForeignMessageArray.count);
  XCTAssertEqual(count, message.repeatedImportMessageArray.count);
  XCTAssertEqual(count, message.repeatedNestedEnumArray.count);
  XCTAssertEqual(count, message.repeatedForeignEnumArray.count);
  XCTAssertEqual(count, message.repeatedImportEnumArray.count);

  XCTAssertEqual(count, message.repeatedStringPieceArray.count);
  XCTAssertEqual(count, message.repeatedCordArray.count);

  XCTAssertEqual(count, message.repeatedInt32Array_Count);
  XCTAssertEqual(count, message.repeatedInt64Array_Count);
  XCTAssertEqual(count, message.repeatedUint32Array_Count);
  XCTAssertEqual(count, message.repeatedUint64Array_Count);
  XCTAssertEqual(count, message.repeatedSint32Array_Count);
  XCTAssertEqual(count, message.repeatedSint64Array_Count);
  XCTAssertEqual(count, message.repeatedFixed32Array_Count);
  XCTAssertEqual(count, message.repeatedFixed64Array_Count);
  XCTAssertEqual(count, message.repeatedSfixed32Array_Count);
  XCTAssertEqual(count, message.repeatedSfixed64Array_Count);
  XCTAssertEqual(count, message.repeatedFloatArray_Count);
  XCTAssertEqual(count, message.repeatedDoubleArray_Count);
  XCTAssertEqual(count, message.repeatedBoolArray_Count);
  XCTAssertEqual(count, message.repeatedStringArray_Count);
  XCTAssertEqual(count, message.repeatedBytesArray_Count);

  XCTAssertEqual(count, message.repeatedGroupArray_Count);
  XCTAssertEqual(count, message.repeatedNestedMessageArray_Count);
  XCTAssertEqual(count, message.repeatedForeignMessageArray_Count);
  XCTAssertEqual(count, message.repeatedImportMessageArray_Count);
  XCTAssertEqual(count, message.repeatedNestedEnumArray_Count);
  XCTAssertEqual(count, message.repeatedForeignEnumArray_Count);
  XCTAssertEqual(count, message.repeatedImportEnumArray_Count);

  XCTAssertEqual(count, message.repeatedStringPieceArray_Count);
  XCTAssertEqual(count, message.repeatedCordArray_Count);

  for (uint32_t i = 0; i < count; ++i) {
    XCTAssertEqual((int)(201 + i * 100),
                   [message.repeatedInt32Array valueAtIndex:i]);
    XCTAssertEqual(202 + i * 100, [message.repeatedInt64Array valueAtIndex:i]);
    XCTAssertEqual(203 + i * 100, [message.repeatedUint32Array valueAtIndex:i]);
    XCTAssertEqual(204 + i * 100, [message.repeatedUint64Array valueAtIndex:i]);
    XCTAssertEqual((int)(205 + i * 100),
                   [message.repeatedSint32Array valueAtIndex:i]);
    XCTAssertEqual(206 + i * 100, [message.repeatedSint64Array valueAtIndex:i]);
    XCTAssertEqual(207 + i * 100,
                   [message.repeatedFixed32Array valueAtIndex:i]);
    XCTAssertEqual(208 + i * 100,
                   [message.repeatedFixed64Array valueAtIndex:i]);
    XCTAssertEqual((int)(209 + i * 100),
                   [message.repeatedSfixed32Array valueAtIndex:i]);
    XCTAssertEqual(210 + i * 100,
                   [message.repeatedSfixed64Array valueAtIndex:i]);
    XCTAssertEqualWithAccuracy(
        211 + i * 100, [message.repeatedFloatArray valueAtIndex:i], 0.1);
    XCTAssertEqualWithAccuracy(
        212 + i * 100, [message.repeatedDoubleArray valueAtIndex:i], 0.1);
    XCTAssertEqual((i % 2) ? YES : NO,
                   [message.repeatedBoolArray valueAtIndex:i]);

    NSString *string = [[NSString alloc] initWithFormat:@"%d", 215 + i * 100];
    XCTAssertEqualObjects(string, message.repeatedStringArray[i]);
    [string release];

    NSData *data = [[NSData alloc] initWithUint32_gpbtu:216 + i * 100];
    XCTAssertEqualObjects(data, message.repeatedBytesArray[i]);
    [data release];

    XCTAssertEqual((int)(217 + i * 100), ((TestAllTypes_RepeatedGroup*)message.repeatedGroupArray[i]).a);
    XCTAssertEqual((int)(218 + i * 100), ((TestAllTypes_NestedMessage*)message.repeatedNestedMessageArray[i]).bb);
    XCTAssertEqual((int)(219 + i * 100), ((ForeignMessage*)message.repeatedForeignMessageArray[i]).c);
    XCTAssertEqual((int)(220 + i * 100), ((ImportMessage*)message.repeatedImportMessageArray[i]).d);

    XCTAssertEqual((i % 2) ? TestAllTypes_NestedEnum_Bar : TestAllTypes_NestedEnum_Baz, [message.repeatedNestedEnumArray valueAtIndex:i]);
    XCTAssertEqual((i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz, [message.repeatedForeignEnumArray valueAtIndex:i]);
    XCTAssertEqual((i % 2) ? ImportEnum_ImportBar : ImportEnum_ImportBaz, [message.repeatedImportEnumArray valueAtIndex:i]);

    string = [[NSString alloc] initWithFormat:@"%d", 224 + i * 100];
    XCTAssertEqualObjects(string, message.repeatedStringPieceArray[i]);
    [string release];

    string = [[NSString alloc] initWithFormat:@"%d", 225 + i * 100];
    XCTAssertEqualObjects(string, message.repeatedCordArray[i]);
    [string release];
  }

  // -----------------------------------------------------------------

  XCTAssertTrue(message.hasDefaultInt32);
  XCTAssertTrue(message.hasDefaultInt64);
  XCTAssertTrue(message.hasDefaultUint32);
  XCTAssertTrue(message.hasDefaultUint64);
  XCTAssertTrue(message.hasDefaultSint32);
  XCTAssertTrue(message.hasDefaultSint64);
  XCTAssertTrue(message.hasDefaultFixed32);
  XCTAssertTrue(message.hasDefaultFixed64);
  XCTAssertTrue(message.hasDefaultSfixed32);
  XCTAssertTrue(message.hasDefaultSfixed64);
  XCTAssertTrue(message.hasDefaultFloat);
  XCTAssertTrue(message.hasDefaultDouble);
  XCTAssertTrue(message.hasDefaultBool);
  XCTAssertTrue(message.hasDefaultString);
  XCTAssertTrue(message.hasDefaultBytes);

  XCTAssertTrue(message.hasDefaultNestedEnum);
  XCTAssertTrue(message.hasDefaultForeignEnum);
  XCTAssertTrue(message.hasDefaultImportEnum);

  XCTAssertTrue(message.hasDefaultStringPiece);
  XCTAssertTrue(message.hasDefaultCord);

  XCTAssertEqual(401, message.defaultInt32);
  XCTAssertEqual(402LL, message.defaultInt64);
  XCTAssertEqual(403U, message.defaultUint32);
  XCTAssertEqual(404ULL, message.defaultUint64);
  XCTAssertEqual(405, message.defaultSint32);
  XCTAssertEqual(406LL, message.defaultSint64);
  XCTAssertEqual(407U, message.defaultFixed32);
  XCTAssertEqual(408ULL, message.defaultFixed64);
  XCTAssertEqual(409, message.defaultSfixed32);
  XCTAssertEqual(410LL, message.defaultSfixed64);
  XCTAssertEqualWithAccuracy(411.0f, message.defaultFloat, 0.1);
  XCTAssertEqualWithAccuracy(412.0, message.defaultDouble, 0.1);
  XCTAssertFalse(message.defaultBool);
  XCTAssertEqualObjects(@"415", message.defaultString);
  XCTAssertEqualObjects([NSData gpbtu_dataWithUint32:416],
                        message.defaultBytes);

  XCTAssertEqual(TestAllTypes_NestedEnum_Foo, message.defaultNestedEnum);
  XCTAssertEqual(ForeignEnum_ForeignFoo, message.defaultForeignEnum);
  XCTAssertEqual(ImportEnum_ImportFoo, message.defaultImportEnum);

  XCTAssertEqualObjects(@"424", message.defaultStringPiece);
  XCTAssertEqualObjects(@"425", message.defaultCord);
}

- (void)setAllFields:(TestAllTypes *)message repeatedCount:(uint32_t)count {
  [message setOptionalInt32:101];
  [message setOptionalInt64:102];
  [message setOptionalUint32:103];
  [message setOptionalUint64:104];
  [message setOptionalSint32:105];
  [message setOptionalSint64:106];
  [message setOptionalFixed32:107];
  [message setOptionalFixed64:108];
  [message setOptionalSfixed32:109];
  [message setOptionalSfixed64:110];
  [message setOptionalFloat:111];
  [message setOptionalDouble:112];
  [message setOptionalBool:YES];
  [message setOptionalString:@"115"];
  [message setOptionalBytes:[NSData gpbtu_dataWithEmbeddedNulls]];

  TestAllTypes_OptionalGroup *allTypes = [TestAllTypes_OptionalGroup message];
  [allTypes setA:117];
  [message setOptionalGroup:allTypes];
  TestAllTypes_NestedMessage *nestedMessage =
      [TestAllTypes_NestedMessage message];
  [nestedMessage setBb:118];
  [message setOptionalNestedMessage:nestedMessage];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [foreignMessage setC:119];
  [message setOptionalForeignMessage:foreignMessage];
  ImportMessage *importMessage = [ImportMessage message];
  [importMessage setD:120];
  [message setOptionalImportMessage:importMessage];

  [message setOptionalNestedEnum:TestAllTypes_NestedEnum_Baz];
  [message setOptionalForeignEnum:ForeignEnum_ForeignBaz];
  [message setOptionalImportEnum:ImportEnum_ImportBaz];

  [message setOptionalStringPiece:@"124"];
  [message setOptionalCord:@"125"];

  // -----------------------------------------------------------------

  for (uint32_t i = 0; i < count; i++) {
    [message.repeatedInt32Array addValue:201 + i * 100];
    [message.repeatedInt64Array addValue:202 + i * 100];
    [message.repeatedUint32Array addValue:203 + i * 100];
    [message.repeatedUint64Array addValue:204 + i * 100];
    [message.repeatedSint32Array addValue:205 + i * 100];
    [message.repeatedSint64Array addValue:206 + i * 100];
    [message.repeatedFixed32Array addValue:207 + i * 100];
    [message.repeatedFixed64Array addValue:208 + i * 100];
    [message.repeatedSfixed32Array addValue:209 + i * 100];
    [message.repeatedSfixed64Array addValue:210 + i * 100];
    [message.repeatedFloatArray addValue:211 + i * 100];
    [message.repeatedDoubleArray addValue:212 + i * 100];
    [message.repeatedBoolArray addValue:(i % 2)];
    NSString *string = [[NSString alloc] initWithFormat:@"%d", 215 + i * 100];
    [message.repeatedStringArray addObject:string];
    [string release];

    NSData *data = [[NSData alloc] initWithUint32_gpbtu:216 + i * 100];
    [message.repeatedBytesArray addObject:data];
    [data release];

    TestAllTypes_RepeatedGroup *testAll =
        [[TestAllTypes_RepeatedGroup alloc] init];
    [testAll setA:217 + i * 100];
    [message.repeatedGroupArray addObject:testAll];
    [testAll release];

    nestedMessage = [[TestAllTypes_NestedMessage alloc] init];
    [nestedMessage setBb:218 + i * 100];
    [message.repeatedNestedMessageArray addObject:nestedMessage];
    [nestedMessage release];

    foreignMessage = [[ForeignMessage alloc] init];
    [foreignMessage setC:219 + i * 100];
    [message.repeatedForeignMessageArray addObject:foreignMessage];
    [foreignMessage release];

    importMessage = [[ImportMessage alloc] init];
    [importMessage setD:220 + i * 100];
    [message.repeatedImportMessageArray addObject:importMessage];
    [importMessage release];

    [message.repeatedNestedEnumArray addValue:(i % 2) ? TestAllTypes_NestedEnum_Bar : TestAllTypes_NestedEnum_Baz];

    [message.repeatedForeignEnumArray addValue:(i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz];
    [message.repeatedImportEnumArray addValue:(i % 2) ? ImportEnum_ImportBar : ImportEnum_ImportBaz];

    string = [[NSString alloc] initWithFormat:@"%d", 224 + i * 100];
    [message.repeatedStringPieceArray addObject:string];
    [string release];

    string = [[NSString alloc] initWithFormat:@"%d", 225 + i * 100];
    [message.repeatedCordArray addObject:string];
    [string release];
  }
  // -----------------------------------------------------------------

  message.defaultInt32 = 401;
  message.defaultInt64 = 402;
  message.defaultUint32 = 403;
  message.defaultUint64 = 404;
  message.defaultSint32 = 405;
  message.defaultSint64 = 406;
  message.defaultFixed32 = 407;
  message.defaultFixed64 = 408;
  message.defaultSfixed32 = 409;
  message.defaultSfixed64 = 410;
  message.defaultFloat = 411;
  message.defaultDouble = 412;
  message.defaultBool = NO;
  message.defaultString = @"415";
  message.defaultBytes = [NSData gpbtu_dataWithUint32:416];

  message.defaultNestedEnum = TestAllTypes_NestedEnum_Foo;
  message.defaultForeignEnum = ForeignEnum_ForeignFoo;
  message.defaultImportEnum = ImportEnum_ImportFoo;

  message.defaultStringPiece = @"424";
  message.defaultCord = @"425";
}

- (void)clearAllFields:(TestAllTypes *)message {
  message.hasOptionalInt32 = NO;
  message.hasOptionalInt64 = NO;
  message.hasOptionalUint32 = NO;
  message.hasOptionalUint64 = NO;
  message.hasOptionalSint32 = NO;
  message.hasOptionalSint64 = NO;
  message.hasOptionalFixed32 = NO;
  message.hasOptionalFixed64 = NO;
  message.hasOptionalSfixed32 = NO;
  message.hasOptionalSfixed64 = NO;
  message.hasOptionalFloat = NO;
  message.hasOptionalDouble = NO;
  message.hasOptionalBool = NO;
  message.hasOptionalString = NO;
  message.hasOptionalBytes = NO;

  message.hasOptionalGroup = NO;
  message.hasOptionalNestedMessage = NO;
  message.hasOptionalForeignMessage = NO;
  message.hasOptionalImportMessage = NO;

  message.hasOptionalNestedEnum = NO;
  message.hasOptionalForeignEnum = NO;
  message.hasOptionalImportEnum = NO;

  message.hasOptionalStringPiece = NO;
  message.hasOptionalCord = NO;

  // -----------------------------------------------------------------

  [message.repeatedInt32Array removeAll];
  [message.repeatedInt64Array removeAll];
  [message.repeatedUint32Array removeAll];
  [message.repeatedUint64Array removeAll];
  [message.repeatedSint32Array removeAll];
  [message.repeatedSint64Array removeAll];
  [message.repeatedFixed32Array removeAll];
  [message.repeatedFixed64Array removeAll];
  [message.repeatedSfixed32Array removeAll];
  [message.repeatedSfixed64Array removeAll];
  [message.repeatedFloatArray removeAll];
  [message.repeatedDoubleArray removeAll];
  [message.repeatedBoolArray removeAll];
  [message.repeatedStringArray removeAllObjects];
  [message.repeatedBytesArray removeAllObjects];

  [message.repeatedGroupArray removeAllObjects];
  [message.repeatedNestedMessageArray removeAllObjects];
  [message.repeatedForeignMessageArray removeAllObjects];
  [message.repeatedImportMessageArray removeAllObjects];

  [message.repeatedNestedEnumArray removeAll];
  [message.repeatedForeignEnumArray removeAll];
  [message.repeatedImportEnumArray removeAll];

  [message.repeatedStringPieceArray removeAllObjects];
  [message.repeatedCordArray removeAllObjects];

  // -----------------------------------------------------------------

  message.hasDefaultInt32 = NO;
  message.hasDefaultInt64 = NO;
  message.hasDefaultUint32 = NO;
  message.hasDefaultUint64 = NO;
  message.hasDefaultSint32 = NO;
  message.hasDefaultSint64 = NO;
  message.hasDefaultFixed32 = NO;
  message.hasDefaultFixed64 = NO;
  message.hasDefaultSfixed32 = NO;
  message.hasDefaultSfixed64 = NO;
  message.hasDefaultFloat = NO;
  message.hasDefaultDouble = NO;
  message.hasDefaultBool = NO;
  message.hasDefaultString = NO;
  message.hasDefaultBytes = NO;

  message.hasDefaultNestedEnum = NO;
  message.hasDefaultForeignEnum = NO;
  message.hasDefaultImportEnum = NO;

  message.hasDefaultStringPiece = NO;
  message.hasDefaultCord = NO;
}

- (void)setAllExtensions:(TestAllExtensions *)message
           repeatedCount:(uint32_t)count {
  [message setExtension:[UnittestRoot optionalInt32Extension] value:@101];
  [message setExtension:[UnittestRoot optionalInt64Extension] value:@102L];
  [message setExtension:[UnittestRoot optionalUint32Extension] value:@103];
  [message setExtension:[UnittestRoot optionalUint64Extension] value:@104L];
  [message setExtension:[UnittestRoot optionalSint32Extension] value:@105];
  [message setExtension:[UnittestRoot optionalSint64Extension] value:@106L];
  [message setExtension:[UnittestRoot optionalFixed32Extension] value:@107];
  [message setExtension:[UnittestRoot optionalFixed64Extension] value:@108L];
  [message setExtension:[UnittestRoot optionalSfixed32Extension] value:@109];
  [message setExtension:[UnittestRoot optionalSfixed64Extension] value:@110L];
  [message setExtension:[UnittestRoot optionalFloatExtension] value:@111.0f];
  [message setExtension:[UnittestRoot optionalDoubleExtension] value:@112.0];
  [message setExtension:[UnittestRoot optionalBoolExtension] value:@YES];
  [message setExtension:[UnittestRoot optionalStringExtension] value:@"115"];
  [message setExtension:[UnittestRoot optionalBytesExtension]
                  value:[NSData gpbtu_dataWithEmbeddedNulls]];

  OptionalGroup_extension *optionalGroup = [OptionalGroup_extension message];
  [optionalGroup setA:117];
  [message setExtension:[UnittestRoot optionalGroupExtension]
                  value:optionalGroup];
  TestAllTypes_NestedMessage *nestedMessage =
      [TestAllTypes_NestedMessage message];
  [nestedMessage setBb:118];
  [message setExtension:[UnittestRoot optionalNestedMessageExtension]
                  value:nestedMessage];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [foreignMessage setC:119];
  [message setExtension:[UnittestRoot optionalForeignMessageExtension]
                  value:foreignMessage];
  ImportMessage *importMessage = [ImportMessage message];
  [importMessage setD:120];
  [message setExtension:[UnittestRoot optionalImportMessageExtension]
                  value:importMessage];

  [message setExtension:[UnittestRoot optionalNestedEnumExtension]
                  value:@(TestAllTypes_NestedEnum_Baz)];
  [message setExtension:[UnittestRoot optionalForeignEnumExtension]
                  value:@(ForeignEnum_ForeignBaz)];
  [message setExtension:[UnittestRoot optionalImportEnumExtension]
                  value:@(ImportEnum_ImportBaz)];

  [message setExtension:[UnittestRoot optionalStringPieceExtension]
                  value:@"124"];
  [message setExtension:[UnittestRoot optionalCordExtension] value:@"125"];

  for (uint32_t i = 0; i < count; ++i) {
    [message addExtension:[UnittestRoot repeatedInt32Extension]
                    value:@(201 + i * 100)];
    [message addExtension:[UnittestRoot repeatedInt64Extension]
                    value:@(202 + i * 100)];
    [message addExtension:[UnittestRoot repeatedUint32Extension]
                    value:@(203 + i * 100)];
    [message addExtension:[UnittestRoot repeatedUint64Extension]
                    value:@(204 + i * 100)];
    [message addExtension:[UnittestRoot repeatedSint32Extension]
                    value:@(205 + i * 100)];
    [message addExtension:[UnittestRoot repeatedSint64Extension]
                    value:@(206 + i * 100)];
    [message addExtension:[UnittestRoot repeatedFixed32Extension]
                    value:@(207 + i * 100)];
    [message addExtension:[UnittestRoot repeatedFixed64Extension]
                    value:@(208 + i * 100)];
    [message addExtension:[UnittestRoot repeatedSfixed32Extension]
                    value:@(209 + i * 100)];
    [message addExtension:[UnittestRoot repeatedSfixed64Extension]
                    value:@(210 + i * 100)];
    [message addExtension:[UnittestRoot repeatedFloatExtension]
                    value:@(211 + i * 100)];
    [message addExtension:[UnittestRoot repeatedDoubleExtension]
                    value:@(212 + i * 100)];
    [message addExtension:[UnittestRoot repeatedBoolExtension]
                    value:@((i % 2) ? YES : NO)];
    NSString *string = [[NSString alloc] initWithFormat:@"%d", 215 + i * 100];
    [message addExtension:[UnittestRoot repeatedStringExtension] value:string];
    [string release];
    NSData *data = [[NSData alloc] initWithUint32_gpbtu:216 + i * 100];
    [message addExtension:[UnittestRoot repeatedBytesExtension] value:data];
    [data release];

    RepeatedGroup_extension *repeatedGroup =
        [[RepeatedGroup_extension alloc] init];
    [repeatedGroup setA:217 + i * 100];
    [message addExtension:[UnittestRoot repeatedGroupExtension]
                    value:repeatedGroup];
    [repeatedGroup release];
    nestedMessage = [[TestAllTypes_NestedMessage alloc] init];
    [nestedMessage setBb:218 + i * 100];
    [message addExtension:[UnittestRoot repeatedNestedMessageExtension]
                    value:nestedMessage];
    [nestedMessage release];
    foreignMessage = [[ForeignMessage alloc] init];
    [foreignMessage setC:219 + i * 100];
    [message addExtension:[UnittestRoot repeatedForeignMessageExtension]
                    value:foreignMessage];
    [foreignMessage release];
    importMessage = [[ImportMessage alloc] init];
    [importMessage setD:220 + i * 100];
    [message addExtension:[UnittestRoot repeatedImportMessageExtension]
                    value:importMessage];
    [importMessage release];
    [message addExtension:[UnittestRoot repeatedNestedEnumExtension]
                    value:@((i % 2) ? TestAllTypes_NestedEnum_Bar
                                    : TestAllTypes_NestedEnum_Baz)];
    [message addExtension:[UnittestRoot repeatedForeignEnumExtension]
                    value:@((i % 2) ? ForeignEnum_ForeignBar
                                    : ForeignEnum_ForeignBaz)];
    [message
        addExtension:[UnittestRoot repeatedImportEnumExtension]
               value:@((i % 2) ? ImportEnum_ImportBar : ImportEnum_ImportBaz)];

    string = [[NSString alloc] initWithFormat:@"%d", 224 + i * 100];
    [message addExtension:[UnittestRoot repeatedStringPieceExtension]
                    value:string];
    [string release];

    string = [[NSString alloc] initWithFormat:@"%d", 225 + i * 100];
    [message addExtension:[UnittestRoot repeatedCordExtension] value:string];
    [string release];
  }

  // -----------------------------------------------------------------

  [message setExtension:[UnittestRoot defaultInt32Extension] value:@401];
  [message setExtension:[UnittestRoot defaultInt64Extension] value:@402L];
  [message setExtension:[UnittestRoot defaultUint32Extension] value:@403];
  [message setExtension:[UnittestRoot defaultUint64Extension] value:@404L];
  [message setExtension:[UnittestRoot defaultSint32Extension] value:@405];
  [message setExtension:[UnittestRoot defaultSint64Extension] value:@406L];
  [message setExtension:[UnittestRoot defaultFixed32Extension] value:@407];
  [message setExtension:[UnittestRoot defaultFixed64Extension] value:@408L];
  [message setExtension:[UnittestRoot defaultSfixed32Extension] value:@409];
  [message setExtension:[UnittestRoot defaultSfixed64Extension] value:@410L];
  [message setExtension:[UnittestRoot defaultFloatExtension] value:@411.0f];
  [message setExtension:[UnittestRoot defaultDoubleExtension] value:@412.0];
  [message setExtension:[UnittestRoot defaultBoolExtension] value:@NO];
  [message setExtension:[UnittestRoot defaultStringExtension] value:@"415"];
  [message setExtension:[UnittestRoot defaultBytesExtension]
                  value:[NSData gpbtu_dataWithUint32:416]];

  [message setExtension:[UnittestRoot defaultNestedEnumExtension]
                  value:@(TestAllTypes_NestedEnum_Foo)];
  [message setExtension:[UnittestRoot defaultForeignEnumExtension]
                  value:@(ForeignEnum_ForeignFoo)];
  [message setExtension:[UnittestRoot defaultImportEnumExtension]
                  value:@(ImportEnum_ImportFoo)];

  [message setExtension:[UnittestRoot defaultStringPieceExtension]
                  value:@"424"];
  [message setExtension:[UnittestRoot defaultCordExtension] value:@"425"];
}

- (void)setAllMapFields:(TestMap *)message numEntries:(uint32_t)count {
  for (uint32_t i = 0; i < count; i++) {
    [message.mapInt32Int32 setInt32:(i + 1) forKey:100 + i * 100];
    [message.mapInt64Int64 setInt64:(i + 1) forKey:101 + i * 100];
    [message.mapUint32Uint32 setUInt32:(i + 1) forKey:102 + i * 100];
    [message.mapUint64Uint64 setUInt64:(i + 1) forKey:103 + i * 100];
    [message.mapSint32Sint32 setInt32:(i + 1) forKey:104 + i * 100];
    [message.mapSint64Sint64 setInt64:(i + 1) forKey:105 + i * 100];
    [message.mapFixed32Fixed32 setUInt32:(i + 1) forKey:106 + i * 100];
    [message.mapFixed64Fixed64 setUInt64:(i + 1) forKey:107 + i * 100];
    [message.mapSfixed32Sfixed32 setInt32:(i + 1) forKey:108 + i * 100];
    [message.mapSfixed64Sfixed64 setInt64:(i + 1) forKey:109 + i * 100];
    [message.mapInt32Float setFloat:(i + 1) forKey:110 + i * 100];
    [message.mapInt32Double setDouble:(i + 1) forKey:111 + i * 100];
    [message.mapBoolBool setBool:((i % 2) == 1) forKey:((i % 2) == 0)];

    NSString *keyStr = [[NSString alloc] initWithFormat:@"%d", 112 + i * 100];
    NSString *dataStr = [[NSString alloc] initWithFormat:@"%d", i + 1];
    [message.mapStringString setObject:dataStr forKey:keyStr];
    [keyStr release];
    [dataStr release];

    NSData *data = [[NSData alloc] initWithUint32_gpbtu:i + 1];
    [message.mapInt32Bytes setObject:data forKey:113 + i * 100];
    [data release];

    [message.mapInt32Enum
        setEnum:(i % 2) ? MapEnum_MapEnumBar : MapEnum_MapEnumBaz
         forKey:114 + i * 100];

    ForeignMessage *subMsg = [[ForeignMessage alloc] init];
    subMsg.c = i + 1;
    [message.mapInt32ForeignMessage setObject:subMsg forKey:115 + i * 100];
    [subMsg release];
  }
}

- (void)setAllTestPackedFields:(TestPackedTypes *)message {
  // Must match -setAllTestUnpackedFields:
  [message.packedInt32Array addValue:101];
  [message.packedInt64Array addValue:102];
  [message.packedUint32Array addValue:103];
  [message.packedUint64Array addValue:104];
  [message.packedSint32Array addValue:105];
  [message.packedSint64Array addValue:106];
  [message.packedFixed32Array addValue:107];
  [message.packedFixed64Array addValue:108];
  [message.packedSfixed32Array addValue:109];
  [message.packedSfixed64Array addValue:110];
  [message.packedFloatArray addValue:111.f];
  [message.packedDoubleArray addValue:112.];
  [message.packedBoolArray addValue:YES];
  [message.packedEnumArray addValue:ForeignEnum_ForeignBar];

  [message.packedInt32Array addValue:201];
  [message.packedInt64Array addValue:302];
  [message.packedUint32Array addValue:203];
  [message.packedUint64Array addValue:204];
  [message.packedSint32Array addValue:205];
  [message.packedSint64Array addValue:206];
  [message.packedFixed32Array addValue:207];
  [message.packedFixed64Array addValue:208];
  [message.packedSfixed32Array addValue:209];
  [message.packedSfixed64Array addValue:210];
  [message.packedFloatArray addValue:211.f];
  [message.packedDoubleArray addValue:212.];
  [message.packedBoolArray addValue:NO];
  [message.packedEnumArray addValue:ForeignEnum_ForeignBaz];
}

- (void)setAllTestUnpackedFields:(TestUnpackedTypes *)message {
  // Must match -setAllTestPackedFields:
  [message.unpackedInt32Array addValue:101];
  [message.unpackedInt64Array addValue:102];
  [message.unpackedUint32Array addValue:103];
  [message.unpackedUint64Array addValue:104];
  [message.unpackedSint32Array addValue:105];
  [message.unpackedSint64Array addValue:106];
  [message.unpackedFixed32Array addValue:107];
  [message.unpackedFixed64Array addValue:108];
  [message.unpackedSfixed32Array addValue:109];
  [message.unpackedSfixed64Array addValue:110];
  [message.unpackedFloatArray addValue:111.f];
  [message.unpackedDoubleArray addValue:112.];
  [message.unpackedBoolArray addValue:YES];
  [message.unpackedEnumArray addValue:ForeignEnum_ForeignBar];

  [message.unpackedInt32Array addValue:201];
  [message.unpackedInt64Array addValue:302];
  [message.unpackedUint32Array addValue:203];
  [message.unpackedUint64Array addValue:204];
  [message.unpackedSint32Array addValue:205];
  [message.unpackedSint64Array addValue:206];
  [message.unpackedFixed32Array addValue:207];
  [message.unpackedFixed64Array addValue:208];
  [message.unpackedSfixed32Array addValue:209];
  [message.unpackedSfixed64Array addValue:210];
  [message.unpackedFloatArray addValue:211.f];
  [message.unpackedDoubleArray addValue:212.];
  [message.unpackedBoolArray addValue:NO];
  [message.unpackedEnumArray addValue:ForeignEnum_ForeignBaz];
}

- (GPBExtensionRegistry *)extensionRegistry {
  return [UnittestRoot extensionRegistry];
}

- (TestAllTypes *)allSetRepeatedCount:(uint32_t)count {
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:count];
  return message;
}

- (TestAllExtensions *)allExtensionsSetRepeatedCount:(uint32_t)count {
  TestAllExtensions *message = [TestAllExtensions message];
  [self setAllExtensions:message repeatedCount:count];
  return message;
}

- (TestPackedTypes *)packedSetRepeatedCount:(uint32_t)count {
  TestPackedTypes *message = [TestPackedTypes message];
  [self setPackedFields:message repeatedCount:count];
  return message;
}

- (TestPackedExtensions *)packedExtensionsSetRepeatedCount:(uint32_t)count {
  TestPackedExtensions *message = [TestPackedExtensions message];
  [self setPackedExtensions:message repeatedCount:count];
  return message;
}

// -------------------------------------------------------------------

- (void)assertClear:(TestAllTypes *)message {
  // hasBlah() should initially be NO for all optional fields.
  XCTAssertFalse(message.hasOptionalInt32);
  XCTAssertFalse(message.hasOptionalInt64);
  XCTAssertFalse(message.hasOptionalUint32);
  XCTAssertFalse(message.hasOptionalUint64);
  XCTAssertFalse(message.hasOptionalSint32);
  XCTAssertFalse(message.hasOptionalSint64);
  XCTAssertFalse(message.hasOptionalFixed32);
  XCTAssertFalse(message.hasOptionalFixed64);
  XCTAssertFalse(message.hasOptionalSfixed32);
  XCTAssertFalse(message.hasOptionalSfixed64);
  XCTAssertFalse(message.hasOptionalFloat);
  XCTAssertFalse(message.hasOptionalDouble);
  XCTAssertFalse(message.hasOptionalBool);
  XCTAssertFalse(message.hasOptionalString);
  XCTAssertFalse(message.hasOptionalBytes);

  XCTAssertFalse(message.hasOptionalGroup);
  XCTAssertFalse(message.hasOptionalNestedMessage);
  XCTAssertFalse(message.hasOptionalForeignMessage);
  XCTAssertFalse(message.hasOptionalImportMessage);

  XCTAssertFalse(message.hasOptionalNestedEnum);
  XCTAssertFalse(message.hasOptionalForeignEnum);
  XCTAssertFalse(message.hasOptionalImportEnum);

  XCTAssertFalse(message.hasOptionalStringPiece);
  XCTAssertFalse(message.hasOptionalCord);

  // Optional fields without defaults are set to zero or something like it.
  XCTAssertEqual(0, message.optionalInt32);
  XCTAssertEqual(0LL, message.optionalInt64);
  XCTAssertEqual(0U, message.optionalUint32);
  XCTAssertEqual(0ULL, message.optionalUint64);
  XCTAssertEqual(0, message.optionalSint32);
  XCTAssertEqual(0LL, message.optionalSint64);
  XCTAssertEqual(0U, message.optionalFixed32);
  XCTAssertEqual(0ULL, message.optionalFixed64);
  XCTAssertEqual(0, message.optionalSfixed32);
  XCTAssertEqual(0LL, message.optionalSfixed64);
  XCTAssertEqual(0.0f, message.optionalFloat);
  XCTAssertEqual(0.0, message.optionalDouble);
  XCTAssertFalse(message.optionalBool);
  XCTAssertEqualObjects(message.optionalString, @"");
  XCTAssertEqualObjects(message.optionalBytes, GPBEmptyNSData());

  // Embedded messages should also be clear.
  XCTAssertFalse(message.hasOptionalGroup);
  XCTAssertFalse(message.hasOptionalNestedMessage);
  XCTAssertFalse(message.hasOptionalForeignMessage);
  XCTAssertFalse(message.hasOptionalImportMessage);

  // Enums without defaults are set to the first value in the enum.
  XCTAssertEqual(TestAllTypes_NestedEnum_Foo, message.optionalNestedEnum);
  XCTAssertEqual(ForeignEnum_ForeignFoo, message.optionalForeignEnum);
  XCTAssertEqual(ImportEnum_ImportFoo, message.optionalImportEnum);

  XCTAssertEqualObjects(message.optionalStringPiece, @"");
  XCTAssertEqualObjects(message.optionalCord, @"");

  // Repeated fields are empty.

  XCTAssertEqual(0U, message.repeatedInt32Array.count);
  XCTAssertEqual(0U, message.repeatedInt64Array.count);
  XCTAssertEqual(0U, message.repeatedUint32Array.count);
  XCTAssertEqual(0U, message.repeatedUint64Array.count);
  XCTAssertEqual(0U, message.repeatedSint32Array.count);
  XCTAssertEqual(0U, message.repeatedSint64Array.count);
  XCTAssertEqual(0U, message.repeatedFixed32Array.count);
  XCTAssertEqual(0U, message.repeatedFixed64Array.count);
  XCTAssertEqual(0U, message.repeatedSfixed32Array.count);
  XCTAssertEqual(0U, message.repeatedSfixed64Array.count);
  XCTAssertEqual(0U, message.repeatedFloatArray.count);
  XCTAssertEqual(0U, message.repeatedDoubleArray.count);
  XCTAssertEqual(0U, message.repeatedBoolArray.count);
  XCTAssertEqual(0U, message.repeatedStringArray.count);
  XCTAssertEqual(0U, message.repeatedBytesArray.count);

  XCTAssertEqual(0U, message.repeatedGroupArray.count);
  XCTAssertEqual(0U, message.repeatedNestedMessageArray.count);
  XCTAssertEqual(0U, message.repeatedForeignMessageArray.count);
  XCTAssertEqual(0U, message.repeatedImportMessageArray.count);
  XCTAssertEqual(0U, message.repeatedNestedEnumArray.count);
  XCTAssertEqual(0U, message.repeatedForeignEnumArray.count);
  XCTAssertEqual(0U, message.repeatedImportEnumArray.count);

  XCTAssertEqual(0U, message.repeatedStringPieceArray.count);
  XCTAssertEqual(0U, message.repeatedCordArray.count);

  XCTAssertEqual(0U, message.repeatedInt32Array_Count);
  XCTAssertEqual(0U, message.repeatedInt64Array_Count);
  XCTAssertEqual(0U, message.repeatedUint32Array_Count);
  XCTAssertEqual(0U, message.repeatedUint64Array_Count);
  XCTAssertEqual(0U, message.repeatedSint32Array_Count);
  XCTAssertEqual(0U, message.repeatedSint64Array_Count);
  XCTAssertEqual(0U, message.repeatedFixed32Array_Count);
  XCTAssertEqual(0U, message.repeatedFixed64Array_Count);
  XCTAssertEqual(0U, message.repeatedSfixed32Array_Count);
  XCTAssertEqual(0U, message.repeatedSfixed64Array_Count);
  XCTAssertEqual(0U, message.repeatedFloatArray_Count);
  XCTAssertEqual(0U, message.repeatedDoubleArray_Count);
  XCTAssertEqual(0U, message.repeatedBoolArray_Count);
  XCTAssertEqual(0U, message.repeatedStringArray_Count);
  XCTAssertEqual(0U, message.repeatedBytesArray_Count);

  XCTAssertEqual(0U, message.repeatedGroupArray_Count);
  XCTAssertEqual(0U, message.repeatedNestedMessageArray_Count);
  XCTAssertEqual(0U, message.repeatedForeignMessageArray_Count);
  XCTAssertEqual(0U, message.repeatedImportMessageArray_Count);
  XCTAssertEqual(0U, message.repeatedNestedEnumArray_Count);
  XCTAssertEqual(0U, message.repeatedForeignEnumArray_Count);
  XCTAssertEqual(0U, message.repeatedImportEnumArray_Count);

  XCTAssertEqual(0U, message.repeatedStringPieceArray_Count);
  XCTAssertEqual(0U, message.repeatedCordArray_Count);

  // hasBlah() should also be NO for all default fields.
  XCTAssertFalse(message.hasDefaultInt32);
  XCTAssertFalse(message.hasDefaultInt64);
  XCTAssertFalse(message.hasDefaultUint32);
  XCTAssertFalse(message.hasDefaultUint64);
  XCTAssertFalse(message.hasDefaultSint32);
  XCTAssertFalse(message.hasDefaultSint64);
  XCTAssertFalse(message.hasDefaultFixed32);
  XCTAssertFalse(message.hasDefaultFixed64);
  XCTAssertFalse(message.hasDefaultSfixed32);
  XCTAssertFalse(message.hasDefaultSfixed64);
  XCTAssertFalse(message.hasDefaultFloat);
  XCTAssertFalse(message.hasDefaultDouble);
  XCTAssertFalse(message.hasDefaultBool);
  XCTAssertFalse(message.hasDefaultString);
  XCTAssertFalse(message.hasDefaultBytes);

  XCTAssertFalse(message.hasDefaultNestedEnum);
  XCTAssertFalse(message.hasDefaultForeignEnum);
  XCTAssertFalse(message.hasDefaultImportEnum);

  XCTAssertFalse(message.hasDefaultStringPiece);
  XCTAssertFalse(message.hasDefaultCord);

  // Fields with defaults have their default values (duh).
  XCTAssertEqual(41, message.defaultInt32);
  XCTAssertEqual(42LL, message.defaultInt64);
  XCTAssertEqual(43U, message.defaultUint32);
  XCTAssertEqual(44ULL, message.defaultUint64);
  XCTAssertEqual(-45, message.defaultSint32);
  XCTAssertEqual(46LL, message.defaultSint64);
  XCTAssertEqual(47U, message.defaultFixed32);
  XCTAssertEqual(48ULL, message.defaultFixed64);
  XCTAssertEqual(49, message.defaultSfixed32);
  XCTAssertEqual(-50LL, message.defaultSfixed64);
  XCTAssertEqualWithAccuracy(51.5f, message.defaultFloat, 0.1);
  XCTAssertEqualWithAccuracy(52e3, message.defaultDouble, 0.1);
  XCTAssertTrue(message.defaultBool);
  XCTAssertEqualObjects(@"hello", message.defaultString);
  XCTAssertEqualObjects([NSData gpbtu_dataWithCString:"world"],
                        message.defaultBytes);

  XCTAssertEqual(TestAllTypes_NestedEnum_Bar, message.defaultNestedEnum);
  XCTAssertEqual(ForeignEnum_ForeignBar, message.defaultForeignEnum);
  XCTAssertEqual(ImportEnum_ImportBar, message.defaultImportEnum);

  XCTAssertEqualObjects(@"abc", message.defaultStringPiece);
  XCTAssertEqualObjects(@"123", message.defaultCord);
}

- (void)assertExtensionsClear:(TestAllExtensions *)message {
  // hasBlah() should initially be NO for all optional fields.
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalInt32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalInt64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalUint32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalUint64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalSint32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalSint64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalFixed32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalFixed64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalSfixed32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalSfixed64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalFloatExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalDoubleExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalBoolExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalStringExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalBytesExtension]]);

  XCTAssertFalse([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalForeignMessageExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalImportMessageExtension]]);

  XCTAssertFalse([message hasExtension:[UnittestRoot optionalNestedEnumExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalForeignEnumExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalImportEnumExtension]]);

  XCTAssertFalse([message hasExtension:[UnittestRoot optionalStringPieceExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalCordExtension]]);

  // Optional fields without defaults are set to zero or something like it.
  XCTAssertEqual(0, [[message getExtension:[UnittestRoot optionalInt32Extension]] intValue]);
  XCTAssertEqual(0LL,[[message getExtension:[UnittestRoot optionalInt64Extension]] longLongValue]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot optionalUint32Extension]] unsignedIntValue]);
  XCTAssertEqual(0ULL, [[message getExtension:[UnittestRoot optionalUint64Extension]] unsignedLongLongValue]);
  XCTAssertEqual(0, [[message getExtension:[UnittestRoot optionalSint32Extension]] intValue]);
  XCTAssertEqual(0LL, [[message getExtension:[UnittestRoot optionalSint64Extension]] longLongValue]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot optionalFixed32Extension]] unsignedIntValue]);
  XCTAssertEqual(0ULL, [[message getExtension:[UnittestRoot optionalFixed64Extension]] unsignedLongLongValue]);
  XCTAssertEqual(0, [[message getExtension:[UnittestRoot optionalSfixed32Extension]] intValue]);
  XCTAssertEqual(0LL, [[message getExtension:[UnittestRoot optionalSfixed64Extension]] longLongValue]);
  XCTAssertEqualWithAccuracy(0.0f, [[message getExtension:[UnittestRoot optionalFloatExtension]] floatValue], 0.01);
  XCTAssertEqualWithAccuracy(0.0, [[message getExtension:[UnittestRoot optionalDoubleExtension]] doubleValue], 0.01);
  XCTAssertFalse([[message getExtension:[UnittestRoot optionalBoolExtension]] boolValue]);
  XCTAssertEqualObjects(@"", [message getExtension:[UnittestRoot optionalStringExtension]]);
  XCTAssertEqualObjects(GPBEmptyNSData(), [message getExtension:[UnittestRoot optionalBytesExtension]]);

  // Embedded messages should also be clear.

  XCTAssertFalse([[message getExtension:[UnittestRoot optionalGroupExtension]] hasA]);
  XCTAssertFalse([[message getExtension:[UnittestRoot optionalNestedMessageExtension]] hasBb]);
  XCTAssertFalse([[message getExtension:[UnittestRoot optionalForeignMessageExtension]] hasC]);
  XCTAssertFalse([[message getExtension:[UnittestRoot optionalImportMessageExtension]] hasD]);

  XCTAssertEqual(0, [(TestAllTypes_OptionalGroup*)[message getExtension:[UnittestRoot optionalGroupExtension]] a]);
  XCTAssertEqual(0, [(TestAllTypes_NestedMessage*)[message getExtension:[UnittestRoot optionalNestedMessageExtension]] bb]);
  XCTAssertEqual(0, [[message getExtension:[UnittestRoot optionalForeignMessageExtension]] c]);
  XCTAssertEqual(0, [[message getExtension:[UnittestRoot optionalImportMessageExtension]] d]);

  // Enums without defaults are set to the first value in the enum.
  XCTAssertEqual(TestAllTypes_NestedEnum_Foo,
               [[message getExtension:[UnittestRoot optionalNestedEnumExtension]] intValue]);
  XCTAssertEqual(ForeignEnum_ForeignFoo,
               [[message getExtension:[UnittestRoot optionalForeignEnumExtension]] intValue]);
  XCTAssertEqual(ImportEnum_ImportFoo,
               [[message getExtension:[UnittestRoot optionalImportEnumExtension]] intValue]);

  XCTAssertEqualObjects(@"", [message getExtension:[UnittestRoot optionalStringPieceExtension]]);
  XCTAssertEqualObjects(@"", [message getExtension:[UnittestRoot optionalCordExtension]]);

  // Repeated fields are empty.
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedInt32Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedInt64Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedUint32Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedUint64Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedSint32Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedSint64Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedFixed32Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedFixed64Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedSfixed32Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedSfixed64Extension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedFloatExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedDoubleExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedBoolExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedStringExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedBytesExtension]] count]);

  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedGroupExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedNestedMessageExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedForeignMessageExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedImportMessageExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedNestedEnumExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedForeignEnumExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedImportEnumExtension]] count]);

  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedStringPieceExtension]] count]);
  XCTAssertEqual(0U, [[message getExtension:[UnittestRoot repeatedCordExtension]] count]);

  // hasBlah() should also be NO for all default fields.
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultInt32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultInt64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultUint32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultUint64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultSint32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultSint64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultFixed32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultFixed64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultSfixed32Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultSfixed64Extension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultFloatExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultDoubleExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultBoolExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultStringExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultBytesExtension]]);

  XCTAssertFalse([message hasExtension:[UnittestRoot defaultNestedEnumExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultForeignEnumExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultImportEnumExtension]]);

  XCTAssertFalse([message hasExtension:[UnittestRoot defaultStringPieceExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot defaultCordExtension]]);

  // Fields with defaults have their default values (duh).
  XCTAssertEqual( 41, [[message getExtension:[UnittestRoot defaultInt32Extension]] intValue]);
  XCTAssertEqual( 42LL, [[message getExtension:[UnittestRoot defaultInt64Extension]] longLongValue]);
  XCTAssertEqual( 43U, [[message getExtension:[UnittestRoot defaultUint32Extension]] unsignedIntValue]);
  XCTAssertEqual( 44ULL, [[message getExtension:[UnittestRoot defaultUint64Extension]] unsignedLongLongValue]);
  XCTAssertEqual(-45, [[message getExtension:[UnittestRoot defaultSint32Extension]] intValue]);
  XCTAssertEqual( 46LL, [[message getExtension:[UnittestRoot defaultSint64Extension]] longLongValue]);
  XCTAssertEqual( 47, [[message getExtension:[UnittestRoot defaultFixed32Extension]] intValue]);
  XCTAssertEqual( 48ULL, [[message getExtension:[UnittestRoot defaultFixed64Extension]] unsignedLongLongValue]);
  XCTAssertEqual( 49, [[message getExtension:[UnittestRoot defaultSfixed32Extension]] intValue]);
  XCTAssertEqual(-50LL, [[message getExtension:[UnittestRoot defaultSfixed64Extension]] longLongValue]);
  XCTAssertEqualWithAccuracy( 51.5f, [[message getExtension:[UnittestRoot defaultFloatExtension]] floatValue], 0.01);
  XCTAssertEqualWithAccuracy( 52e3, [[message getExtension:[UnittestRoot defaultDoubleExtension]] doubleValue], 0.01);
  XCTAssertTrue([[message getExtension:[UnittestRoot defaultBoolExtension]] boolValue]);
  XCTAssertEqualObjects(@"hello", [message getExtension:[UnittestRoot defaultStringExtension]]);
  XCTAssertEqualObjects([NSData gpbtu_dataWithCString:"world"], [message getExtension:[UnittestRoot defaultBytesExtension]]);

  XCTAssertEqual(TestAllTypes_NestedEnum_Bar,
               [[message getExtension:[UnittestRoot defaultNestedEnumExtension]] intValue]);
  XCTAssertEqual(ForeignEnum_ForeignBar,
               [[message getExtension:[UnittestRoot defaultForeignEnumExtension]] intValue]);
  XCTAssertEqual(ImportEnum_ImportBar,
               [[message getExtension:[UnittestRoot defaultImportEnumExtension]] intValue]);

  XCTAssertEqualObjects(@"abc", [message getExtension:[UnittestRoot defaultStringPieceExtension]]);
  XCTAssertEqualObjects(@"123", [message getExtension:[UnittestRoot defaultCordExtension]]);
}

- (void)modifyRepeatedFields:(TestAllTypes *)message {
  [message.repeatedInt32Array replaceValueAtIndex:1 withValue:501];
  [message.repeatedInt64Array replaceValueAtIndex:1 withValue:502];
  [message.repeatedUint32Array replaceValueAtIndex:1 withValue:503];
  [message.repeatedUint64Array replaceValueAtIndex:1 withValue:504];
  [message.repeatedSint32Array replaceValueAtIndex:1 withValue:505];
  [message.repeatedSint64Array replaceValueAtIndex:1 withValue:506];
  [message.repeatedFixed32Array replaceValueAtIndex:1 withValue:507];
  [message.repeatedFixed64Array replaceValueAtIndex:1 withValue:508];
  [message.repeatedSfixed32Array replaceValueAtIndex:1 withValue:509];
  [message.repeatedSfixed64Array replaceValueAtIndex:1 withValue:510];
  [message.repeatedFloatArray replaceValueAtIndex:1 withValue:511];
  [message.repeatedDoubleArray replaceValueAtIndex:1 withValue:512];
  [message.repeatedBoolArray replaceValueAtIndex:1 withValue:YES];
  [message.repeatedStringArray replaceObjectAtIndex:1 withObject:@"515"];

  NSData *data = [[NSData alloc] initWithUint32_gpbtu:516];
  [message.repeatedBytesArray replaceObjectAtIndex:1 withObject:data];
  [data release];

  TestAllTypes_RepeatedGroup *testAll =
      [[TestAllTypes_RepeatedGroup alloc] init];
  [testAll setA:517];
  [message.repeatedGroupArray replaceObjectAtIndex:1 withObject:testAll];
  [testAll release];

  TestAllTypes_NestedMessage *nestedMessage =
      [[TestAllTypes_NestedMessage alloc] init];
  [nestedMessage setBb:518];
  [message.repeatedNestedMessageArray replaceObjectAtIndex:1
                                                withObject:nestedMessage];
  [nestedMessage release];

  ForeignMessage *foreignMessage = [[ForeignMessage alloc] init];
  [foreignMessage setC:519];
  [message.repeatedForeignMessageArray replaceObjectAtIndex:1
                                                 withObject:foreignMessage];
  [foreignMessage release];

  ImportMessage *importMessage = [[ImportMessage alloc] init];
  [importMessage setD:520];
  [message.repeatedImportMessageArray replaceObjectAtIndex:1
                                                withObject:importMessage];
  [importMessage release];

  [message.repeatedNestedEnumArray replaceValueAtIndex:1 withValue:TestAllTypes_NestedEnum_Foo];
  [message.repeatedForeignEnumArray replaceValueAtIndex:1 withValue:ForeignEnum_ForeignFoo];
  [message.repeatedImportEnumArray replaceValueAtIndex:1 withValue:ImportEnum_ImportFoo];

  [message.repeatedStringPieceArray replaceObjectAtIndex:1 withObject:@"524"];
  [message.repeatedCordArray replaceObjectAtIndex:1 withObject:@"525"];
}

- (void)assertRepeatedFieldsModified:(TestAllTypes *)message
                       repeatedCount:(uint32_t)count {
  // ModifyRepeatedFields only sets the second repeated element of each
  // field.  In addition to verifying this, we also verify that the first
  // element and size were *not* modified.

  XCTAssertEqual(count, message.repeatedInt32Array.count);
  XCTAssertEqual(count, message.repeatedInt64Array.count);
  XCTAssertEqual(count, message.repeatedUint32Array.count);
  XCTAssertEqual(count, message.repeatedUint64Array.count);
  XCTAssertEqual(count, message.repeatedSint32Array.count);
  XCTAssertEqual(count, message.repeatedSint64Array.count);
  XCTAssertEqual(count, message.repeatedFixed32Array.count);
  XCTAssertEqual(count, message.repeatedFixed64Array.count);
  XCTAssertEqual(count, message.repeatedSfixed32Array.count);
  XCTAssertEqual(count, message.repeatedSfixed64Array.count);
  XCTAssertEqual(count, message.repeatedFloatArray.count);
  XCTAssertEqual(count, message.repeatedDoubleArray.count);
  XCTAssertEqual(count, message.repeatedBoolArray.count);
  XCTAssertEqual(count, message.repeatedStringArray.count);
  XCTAssertEqual(count, message.repeatedBytesArray.count);

  XCTAssertEqual(count, message.repeatedGroupArray.count);
  XCTAssertEqual(count, message.repeatedNestedMessageArray.count);
  XCTAssertEqual(count, message.repeatedForeignMessageArray.count);
  XCTAssertEqual(count, message.repeatedImportMessageArray.count);
  XCTAssertEqual(count, message.repeatedNestedEnumArray.count);
  XCTAssertEqual(count, message.repeatedForeignEnumArray.count);
  XCTAssertEqual(count, message.repeatedImportEnumArray.count);

  XCTAssertEqual(count, message.repeatedStringPieceArray.count);
  XCTAssertEqual(count, message.repeatedCordArray.count);

  XCTAssertEqual(count, message.repeatedInt32Array_Count);
  XCTAssertEqual(count, message.repeatedInt64Array_Count);
  XCTAssertEqual(count, message.repeatedUint32Array_Count);
  XCTAssertEqual(count, message.repeatedUint64Array_Count);
  XCTAssertEqual(count, message.repeatedSint32Array_Count);
  XCTAssertEqual(count, message.repeatedSint64Array_Count);
  XCTAssertEqual(count, message.repeatedFixed32Array_Count);
  XCTAssertEqual(count, message.repeatedFixed64Array_Count);
  XCTAssertEqual(count, message.repeatedSfixed32Array_Count);
  XCTAssertEqual(count, message.repeatedSfixed64Array_Count);
  XCTAssertEqual(count, message.repeatedFloatArray_Count);
  XCTAssertEqual(count, message.repeatedDoubleArray_Count);
  XCTAssertEqual(count, message.repeatedBoolArray_Count);
  XCTAssertEqual(count, message.repeatedStringArray_Count);
  XCTAssertEqual(count, message.repeatedBytesArray_Count);

  XCTAssertEqual(count, message.repeatedGroupArray_Count);
  XCTAssertEqual(count, message.repeatedNestedMessageArray_Count);
  XCTAssertEqual(count, message.repeatedForeignMessageArray_Count);
  XCTAssertEqual(count, message.repeatedImportMessageArray_Count);
  XCTAssertEqual(count, message.repeatedNestedEnumArray_Count);
  XCTAssertEqual(count, message.repeatedForeignEnumArray_Count);
  XCTAssertEqual(count, message.repeatedImportEnumArray_Count);

  XCTAssertEqual(count, message.repeatedStringPieceArray_Count);
  XCTAssertEqual(count, message.repeatedCordArray_Count);

  XCTAssertEqual(201, [message.repeatedInt32Array valueAtIndex:0]);
  XCTAssertEqual(202LL, [message.repeatedInt64Array valueAtIndex:0]);
  XCTAssertEqual(203U, [message.repeatedUint32Array valueAtIndex:0]);
  XCTAssertEqual(204ULL, [message.repeatedUint64Array valueAtIndex:0]);
  XCTAssertEqual(205, [message.repeatedSint32Array valueAtIndex:0]);
  XCTAssertEqual(206LL, [message.repeatedSint64Array valueAtIndex:0]);
  XCTAssertEqual(207U, [message.repeatedFixed32Array valueAtIndex:0]);
  XCTAssertEqual(208ULL, [message.repeatedFixed64Array valueAtIndex:0]);
  XCTAssertEqual(209, [message.repeatedSfixed32Array valueAtIndex:0]);
  XCTAssertEqual(210LL, [message.repeatedSfixed64Array valueAtIndex:0]);
  XCTAssertEqualWithAccuracy(211.0f, [message.repeatedFloatArray valueAtIndex:0], 0.01);
  XCTAssertEqualWithAccuracy(212.0, [message.repeatedDoubleArray valueAtIndex:0], 0.01);
  XCTAssertFalse([message.repeatedBoolArray valueAtIndex:0]);
  XCTAssertEqualObjects(@"215", message.repeatedStringArray[0]);
  XCTAssertEqualObjects([NSData gpbtu_dataWithUint32:216],
                        message.repeatedBytesArray[0]);

  XCTAssertEqual(217, ((TestAllTypes_RepeatedGroup*)message.repeatedGroupArray[0]).a);
  XCTAssertEqual(218, ((TestAllTypes_NestedMessage*)message.repeatedNestedMessageArray[0]).bb);
  XCTAssertEqual(219, ((ForeignMessage*)message.repeatedForeignMessageArray[0]).c);
  XCTAssertEqual(220, ((ImportMessage*)message.repeatedImportMessageArray[0]).d);

  XCTAssertEqual(TestAllTypes_NestedEnum_Baz, [message.repeatedNestedEnumArray valueAtIndex:0]);
  XCTAssertEqual(ForeignEnum_ForeignBaz, [message.repeatedForeignEnumArray valueAtIndex:0]);
  XCTAssertEqual(ImportEnum_ImportBaz, [message.repeatedImportEnumArray valueAtIndex:0]);

  XCTAssertEqualObjects(@"224", message.repeatedStringPieceArray[0]);
  XCTAssertEqualObjects(@"225", message.repeatedCordArray[0]);

  // Actually verify the second (modified) elements now.
  XCTAssertEqual(501, [message.repeatedInt32Array valueAtIndex:1]);
  XCTAssertEqual(502LL, [message.repeatedInt64Array valueAtIndex:1]);
  XCTAssertEqual(503U, [message.repeatedUint32Array valueAtIndex:1]);
  XCTAssertEqual(504ULL, [message.repeatedUint64Array valueAtIndex:1]);
  XCTAssertEqual(505, [message.repeatedSint32Array valueAtIndex:1]);
  XCTAssertEqual(506LL, [message.repeatedSint64Array valueAtIndex:1]);
  XCTAssertEqual(507U, [message.repeatedFixed32Array valueAtIndex:1]);
  XCTAssertEqual(508ULL, [message.repeatedFixed64Array valueAtIndex:1]);
  XCTAssertEqual(509, [message.repeatedSfixed32Array valueAtIndex:1]);
  XCTAssertEqual(510LL, [message.repeatedSfixed64Array valueAtIndex:1]);
  XCTAssertEqualWithAccuracy(511.0f, [message.repeatedFloatArray valueAtIndex:1], 0.01);
  XCTAssertEqualWithAccuracy(512.0, [message.repeatedDoubleArray valueAtIndex:1], 0.01);
  XCTAssertTrue([message.repeatedBoolArray valueAtIndex:1]);
  XCTAssertEqualObjects(@"515", message.repeatedStringArray[1]);
  XCTAssertEqualObjects([NSData gpbtu_dataWithUint32:516],
                        message.repeatedBytesArray[1]);

  XCTAssertEqual(517, ((TestAllTypes_RepeatedGroup*)message.repeatedGroupArray[1]).a);
  XCTAssertEqual(518, ((TestAllTypes_NestedMessage*)message.repeatedNestedMessageArray[1]).bb);
  XCTAssertEqual(519, ((ForeignMessage*)message.repeatedForeignMessageArray[1]).c);
  XCTAssertEqual(520, ((ImportMessage*)message.repeatedImportMessageArray[1]).d);

  XCTAssertEqual(TestAllTypes_NestedEnum_Foo, [message.repeatedNestedEnumArray valueAtIndex:1]);
  XCTAssertEqual(ForeignEnum_ForeignFoo, [message.repeatedForeignEnumArray valueAtIndex:1]);
  XCTAssertEqual(ImportEnum_ImportFoo, [message.repeatedImportEnumArray valueAtIndex:1]);

  XCTAssertEqualObjects(@"524", message.repeatedStringPieceArray[1]);
  XCTAssertEqualObjects(@"525", message.repeatedCordArray[1]);
}

- (void)setPackedFields:(TestPackedTypes *)message
          repeatedCount:(uint32_t)count {
  // Must match -setUnpackedFields:repeatedCount:
  // Must match -setPackedExtensions:repeatedCount:
  // Must match -setUnpackedExtensions:repeatedCount:
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedInt32Array addValue:601 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedInt64Array addValue:602 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedUint32Array addValue:603 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedUint64Array addValue:604 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedSint32Array addValue:605 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedSint64Array addValue:606 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedFixed32Array addValue:607 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedFixed64Array addValue:608 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedSfixed32Array addValue:609 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedSfixed64Array addValue:610 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedFloatArray addValue:611 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedDoubleArray addValue:612 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedBoolArray addValue:(i % 2) ? YES : NO];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.packedEnumArray
        addValue:(i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz];
  }
}

- (void)setUnpackedFields:(TestUnpackedTypes *)message
            repeatedCount:(uint32_t)count {
  // Must match -setPackedFields:repeatedCount:
  // Must match -setPackedExtensions:repeatedCount:
  // Must match -setUnpackedExtensions:repeatedCount:
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedInt32Array addValue:601 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedInt64Array addValue:602 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedUint32Array addValue:603 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedUint64Array addValue:604 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedSint32Array addValue:605 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedSint64Array addValue:606 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedFixed32Array addValue:607 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedFixed64Array addValue:608 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedSfixed32Array addValue:609 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedSfixed64Array addValue:610 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedFloatArray addValue:611 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedDoubleArray addValue:612 + i * 100];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedBoolArray addValue:(i % 2) ? YES : NO];
  }
  for (uint32_t i = 0; i < count; ++i) {
    [message.unpackedEnumArray
        addValue:(i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz];
  }
}

- (void)assertPackedFieldsSet:(TestPackedTypes *)message
                repeatedCount:(uint32_t)count {
  XCTAssertEqual(count, message.packedInt32Array.count);
  XCTAssertEqual(count, message.packedInt64Array.count);
  XCTAssertEqual(count, message.packedUint32Array.count);
  XCTAssertEqual(count, message.packedUint64Array.count);
  XCTAssertEqual(count, message.packedSint32Array.count);
  XCTAssertEqual(count, message.packedSint64Array.count);
  XCTAssertEqual(count, message.packedFixed32Array.count);
  XCTAssertEqual(count, message.packedFixed64Array.count);
  XCTAssertEqual(count, message.packedSfixed32Array.count);
  XCTAssertEqual(count, message.packedSfixed64Array.count);
  XCTAssertEqual(count, message.packedFloatArray.count);
  XCTAssertEqual(count, message.packedDoubleArray.count);
  XCTAssertEqual(count, message.packedBoolArray.count);
  XCTAssertEqual(count, message.packedEnumArray.count);
  for (uint32_t i = 0; i < count; ++i) {
    XCTAssertEqual((int)(601 + i * 100),
                   [message.packedInt32Array valueAtIndex:i]);
    XCTAssertEqual(602 + i * 100, [message.packedInt64Array valueAtIndex:i]);
    XCTAssertEqual(603 + i * 100, [message.packedUint32Array valueAtIndex:i]);
    XCTAssertEqual(604 + i * 100, [message.packedUint64Array valueAtIndex:i]);
    XCTAssertEqual((int)(605 + i * 100),
                   [message.packedSint32Array valueAtIndex:i]);
    XCTAssertEqual(606 + i * 100, [message.packedSint64Array valueAtIndex:i]);
    XCTAssertEqual(607 + i * 100, [message.packedFixed32Array valueAtIndex:i]);
    XCTAssertEqual(608 + i * 100, [message.packedFixed64Array valueAtIndex:i]);
    XCTAssertEqual((int)(609 + i * 100),
                   [message.packedSfixed32Array valueAtIndex:i]);
    XCTAssertEqual(610 + i * 100, [message.packedSfixed64Array valueAtIndex:i]);
    XCTAssertEqualWithAccuracy(611 + i * 100,
                               [message.packedFloatArray valueAtIndex:i], 0.01);
    XCTAssertEqualWithAccuracy(
        612 + i * 100, [message.packedDoubleArray valueAtIndex:i], 0.01);
    XCTAssertEqual((i % 2) ? YES : NO,
                   [message.packedBoolArray valueAtIndex:i]);
    XCTAssertEqual((i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz,
                   [message.packedEnumArray valueAtIndex:i]);
  }
}

- (void)setPackedExtensions:(TestPackedExtensions *)message
              repeatedCount:(uint32_t)count {
  // Must match -setPackedFields:repeatedCount:
  // Must match -setUnpackedFields:repeatedCount:
  // Must match -setUnpackedExtensions:repeatedCount:
  for (uint32_t i = 0; i < count; i++) {
    [message addExtension:[UnittestRoot packedInt32Extension]
                    value:@(601 + i * 100)];
    [message addExtension:[UnittestRoot packedInt64Extension]
                    value:@(602 + i * 100)];
    [message addExtension:[UnittestRoot packedUint32Extension]
                    value:@(603 + i * 100)];
    [message addExtension:[UnittestRoot packedUint64Extension]
                    value:@(604 + i * 100)];
    [message addExtension:[UnittestRoot packedSint32Extension]
                    value:@(605 + i * 100)];
    [message addExtension:[UnittestRoot packedSint64Extension]
                    value:@(606 + i * 100)];
    [message addExtension:[UnittestRoot packedFixed32Extension]
                    value:@(607 + i * 100)];
    [message addExtension:[UnittestRoot packedFixed64Extension]
                    value:@(608 + i * 100)];
    [message addExtension:[UnittestRoot packedSfixed32Extension]
                    value:@(609 + i * 100)];
    [message addExtension:[UnittestRoot packedSfixed64Extension]
                    value:@(610 + i * 100)];
    [message addExtension:[UnittestRoot packedFloatExtension]
                    value:@(611 + i * 100)];
    [message addExtension:[UnittestRoot packedDoubleExtension]
                    value:@(612 + i * 100)];
    [message addExtension:[UnittestRoot packedBoolExtension]
                    value:@((i % 2) ? YES : NO)];
    [message addExtension:[UnittestRoot packedEnumExtension]
                    value:@((i % 2) ? ForeignEnum_ForeignBar
                                    : ForeignEnum_ForeignBaz)];
  }
}

- (void)setUnpackedExtensions:(TestUnpackedExtensions *)message
                repeatedCount:(uint32_t)count {
  // Must match -setPackedFields:repeatedCount:
  // Must match -setUnpackedFields:repeatedCount:
  // Must match -setPackedExtensions:repeatedCount:
  for (uint32_t i = 0; i < count; i++) {
    [message addExtension:[UnittestRoot unpackedInt32Extension]
                    value:@(601 + i * 100)];
    [message addExtension:[UnittestRoot unpackedInt64Extension]
                    value:@(602 + i * 100)];
    [message addExtension:[UnittestRoot unpackedUint32Extension]
                    value:@(603 + i * 100)];
    [message addExtension:[UnittestRoot unpackedUint64Extension]
                    value:@(604 + i * 100)];
    [message addExtension:[UnittestRoot unpackedSint32Extension]
                    value:@(605 + i * 100)];
    [message addExtension:[UnittestRoot unpackedSint64Extension]
                    value:@(606 + i * 100)];
    [message addExtension:[UnittestRoot unpackedFixed32Extension]
                    value:@(607 + i * 100)];
    [message addExtension:[UnittestRoot unpackedFixed64Extension]
                    value:@(608 + i * 100)];
    [message addExtension:[UnittestRoot unpackedSfixed32Extension]
                    value:@(609 + i * 100)];
    [message addExtension:[UnittestRoot unpackedSfixed64Extension]
                    value:@(610 + i * 100)];
    [message addExtension:[UnittestRoot unpackedFloatExtension]
                    value:@(611 + i * 100)];
    [message addExtension:[UnittestRoot unpackedDoubleExtension]
                    value:@(612 + i * 100)];
    [message addExtension:[UnittestRoot unpackedBoolExtension]
                    value:@((i % 2) ? YES : NO)];
    [message addExtension:[UnittestRoot unpackedEnumExtension]
                    value:@((i % 2) ? ForeignEnum_ForeignBar
                         : ForeignEnum_ForeignBaz)];
  }
}

- (void)assertPackedExtensionsSet:(TestPackedExtensions *)message
                    repeatedCount:(uint32_t)count{
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedInt32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedInt64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedUint32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedUint64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedSint32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedSint64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedFixed32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedFixed64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedSfixed32Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedSfixed64Extension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedFloatExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedDoubleExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedBoolExtension]] count]);
  XCTAssertEqual(count, [[message getExtension:[UnittestRoot packedEnumExtension]] count]);

  for (uint32_t i = 0; i < count; ++i) {
    id extension = [message getExtension:[UnittestRoot packedInt32Extension]];
    XCTAssertEqual((int)(601 + i * 100), [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot packedInt64Extension]];
    XCTAssertEqual(602 + i * 100, [extension[i] longLongValue]);
    extension = [message getExtension:[UnittestRoot packedUint32Extension]];
    XCTAssertEqual(603 + i * 100, [extension[i] unsignedIntValue]);
    extension = [message getExtension:[UnittestRoot packedUint64Extension]];
    XCTAssertEqual(604 + i * 100, [extension[i] unsignedLongLongValue]);
    extension = [message getExtension:[UnittestRoot packedSint32Extension]];
    XCTAssertEqual((int)(605 + i * 100), [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot packedSint64Extension]];
    XCTAssertEqual(606 + i * 100, [extension[i] longLongValue]);
    extension = [message getExtension:[UnittestRoot packedFixed32Extension]];
    XCTAssertEqual(607 + i * 100, [extension[i] unsignedIntValue]);
    extension = [message getExtension:[UnittestRoot packedFixed64Extension]];
    XCTAssertEqual(608 + i * 100, [extension[i] unsignedLongLongValue]);
    extension = [message getExtension:[UnittestRoot packedSfixed32Extension]];
    XCTAssertEqual((int)(609 + i * 100), [extension[i] intValue]);
    extension = [message getExtension:[UnittestRoot packedSfixed64Extension]];
    XCTAssertEqual(610 + i * 100, [extension[i] longLongValue]);
    extension = [message getExtension:[UnittestRoot packedFloatExtension]];
    XCTAssertEqualWithAccuracy(611 + i * 100, [extension[i] floatValue], 0.01);
    extension = [message getExtension:[UnittestRoot packedDoubleExtension]];
    XCTAssertEqualWithAccuracy(612 + i * 100, [extension[i] doubleValue], 0.01);
    extension = [message getExtension:[UnittestRoot packedBoolExtension]];
    XCTAssertEqual((i % 2) ? YES : NO, [extension[i] boolValue]);
    extension = [message getExtension:[UnittestRoot packedEnumExtension]];
    XCTAssertEqual((i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz,
                   [extension[i] intValue]);
  }
}

- (void)assertAllFieldsKVCMatch:(TestAllTypes *)message {
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalInt32"], @YES);
  XCTAssertEqualObjects(@(message.optionalInt32), [message valueForKey:@"optionalInt32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalInt64"], @YES);
  XCTAssertEqualObjects(@(message.optionalInt64), [message valueForKey:@"optionalInt64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalUint32"], @YES);
  XCTAssertEqualObjects(@(message.optionalUint32), [message valueForKey:@"optionalUint32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalUint64"], @YES);
  XCTAssertEqualObjects(@(message.optionalUint64), [message valueForKey:@"optionalUint64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSint32"], @YES);
  XCTAssertEqualObjects(@(message.optionalSint32), [message valueForKey:@"optionalSint32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSint64"], @YES);
  XCTAssertEqualObjects(@(message.optionalSint64), [message valueForKey:@"optionalSint64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalFixed32"], @YES);
  XCTAssertEqualObjects(@(message.optionalFixed32), [message valueForKey:@"optionalFixed32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalFixed64"], @YES);
  XCTAssertEqualObjects(@(message.optionalFixed64), [message valueForKey:@"optionalFixed64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSfixed32"], @YES);
  XCTAssertEqualObjects(@(message.optionalSfixed32), [message valueForKey:@"optionalSfixed32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSfixed64"], @YES);
  XCTAssertEqualObjects(@(message.optionalSfixed64), [message valueForKey:@"optionalSfixed64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalFloat"], @YES);
  XCTAssertEqualObjects(@(message.optionalFloat), [message valueForKey:@"optionalFloat"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalDouble"], @YES);
  XCTAssertEqualObjects(@(message.optionalDouble), [message valueForKey:@"optionalDouble"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalBool"], @YES);
  XCTAssertEqualObjects(@(message.optionalBool), [message valueForKey:@"optionalBool"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalString"], @YES);
  XCTAssertEqualObjects(message.optionalString, [message valueForKey:@"optionalString"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalBytes"], @YES);
  XCTAssertEqualObjects(message.optionalBytes, [message valueForKey:@"optionalBytes"]);

  XCTAssertEqualObjects([message valueForKey:@"hasOptionalGroup"], @YES);
  XCTAssertNotNil(message.optionalGroup);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalGroup.hasA"], @YES);
  XCTAssertEqualObjects(@(message.optionalGroup.a), [message valueForKeyPath:@"optionalGroup.a"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalNestedMessage"], @YES);
  XCTAssertNotNil(message.optionalNestedMessage);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalNestedMessage.hasBb"], @YES);
  XCTAssertEqualObjects(@(message.optionalNestedMessage.bb), [message valueForKeyPath:@"optionalNestedMessage.bb"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalForeignMessage"], @YES);
  XCTAssertNotNil(message.optionalForeignMessage);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalForeignMessage.hasC"], @YES);
  XCTAssertEqualObjects(@(message.optionalForeignMessage.c), [message valueForKeyPath:@"optionalForeignMessage.c"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalImportMessage"], @YES);
  XCTAssertNotNil(message.optionalForeignMessage);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalImportMessage.hasD"], @YES);
  XCTAssertEqualObjects(@(message.optionalImportMessage.d), [message valueForKeyPath:@"optionalImportMessage.d"]);

  XCTAssertEqualObjects([message valueForKey:@"hasOptionalNestedEnum"], @YES);
  XCTAssertEqualObjects(@(message.optionalNestedEnum), [message valueForKey:@"optionalNestedEnum"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalForeignEnum"], @YES);
  XCTAssertEqualObjects(@(message.optionalForeignEnum), [message valueForKey:@"optionalForeignEnum"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalImportEnum"], @YES);
  XCTAssertEqualObjects(@(message.optionalImportEnum), [message valueForKey:@"optionalImportEnum"]);

  XCTAssertEqualObjects([message valueForKey:@"hasOptionalStringPiece"], @YES);
  XCTAssertEqualObjects(message.optionalStringPiece, [message valueForKey:@"optionalStringPiece"]);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalCord"], @YES);
  XCTAssertEqualObjects(message.optionalCord, [message valueForKey:@"optionalCord"]);

  // -----------------------------------------------------------------

  // GPBArray interface for repeated

  XCTAssertEqualObjects(message.repeatedInt32Array, [message valueForKey:@"repeatedInt32Array"]);
  XCTAssertEqualObjects(message.repeatedInt64Array, [message valueForKey:@"repeatedInt64Array"]);
  XCTAssertEqualObjects(message.repeatedUint32Array, [message valueForKey:@"repeatedUint32Array"]);
  XCTAssertEqualObjects(message.repeatedUint64Array, [message valueForKey:@"repeatedUint64Array"]);
  XCTAssertEqualObjects(message.repeatedSint32Array, [message valueForKey:@"repeatedSint32Array"]);
  XCTAssertEqualObjects(message.repeatedSint64Array, [message valueForKey:@"repeatedSint64Array"]);
  XCTAssertEqualObjects(message.repeatedFixed32Array, [message valueForKey:@"repeatedFixed32Array"]);
  XCTAssertEqualObjects(message.repeatedFixed64Array, [message valueForKey:@"repeatedFixed64Array"]);
  XCTAssertEqualObjects(message.repeatedSfixed32Array, [message valueForKey:@"repeatedSfixed32Array"]);
  XCTAssertEqualObjects(message.repeatedSfixed64Array, [message valueForKey:@"repeatedSfixed64Array"]);
  XCTAssertEqualObjects(message.repeatedFloatArray, [message valueForKey:@"repeatedFloatArray"]);
  XCTAssertEqualObjects(message.repeatedDoubleArray, [message valueForKey:@"repeatedDoubleArray"]);
  XCTAssertEqualObjects(message.repeatedBoolArray, [message valueForKey:@"repeatedBoolArray"]);
  XCTAssertEqualObjects(message.repeatedStringArray, [message valueForKey:@"repeatedStringArray"]);
  XCTAssertEqualObjects(message.repeatedBytesArray, [message valueForKey:@"repeatedBytesArray"]);

  XCTAssertEqualObjects(message.repeatedGroupArray, [message valueForKey:@"repeatedGroupArray"]);
  XCTAssertEqualObjects(message.repeatedNestedMessageArray, [message valueForKey:@"repeatedNestedMessageArray"]);
  XCTAssertEqualObjects(message.repeatedForeignMessageArray, [message valueForKey:@"repeatedForeignMessageArray"]);
  XCTAssertEqualObjects(message.repeatedImportMessageArray, [message valueForKey:@"repeatedImportMessageArray"]);

  XCTAssertEqualObjects(message.repeatedNestedEnumArray, [message valueForKey:@"repeatedNestedEnumArray"]);
  XCTAssertEqualObjects(message.repeatedForeignEnumArray, [message valueForKey:@"repeatedForeignEnumArray"]);
  XCTAssertEqualObjects(message.repeatedImportEnumArray, [message valueForKey:@"repeatedImportEnumArray"]);

  XCTAssertEqualObjects(message.repeatedStringPieceArray, [message valueForKey:@"repeatedStringPieceArray"]);
  XCTAssertEqualObjects(message.repeatedCordArray, [message valueForKey:@"repeatedCordArray"]);

  XCTAssertEqualObjects(@(message.repeatedInt32Array_Count), [message valueForKey:@"repeatedInt32Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedInt64Array_Count), [message valueForKey:@"repeatedInt64Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedUint32Array_Count), [message valueForKey:@"repeatedUint32Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedUint64Array_Count), [message valueForKey:@"repeatedUint64Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedSint32Array_Count), [message valueForKey:@"repeatedSint32Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedSint64Array_Count), [message valueForKey:@"repeatedSint64Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedFixed32Array_Count), [message valueForKey:@"repeatedFixed32Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedFixed64Array_Count), [message valueForKey:@"repeatedFixed64Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedSfixed32Array_Count), [message valueForKey:@"repeatedSfixed32Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedSfixed64Array_Count), [message valueForKey:@"repeatedSfixed64Array_Count"]);
  XCTAssertEqualObjects(@(message.repeatedFloatArray_Count), [message valueForKey:@"repeatedFloatArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedDoubleArray_Count), [message valueForKey:@"repeatedDoubleArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedBoolArray_Count), [message valueForKey:@"repeatedBoolArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedStringArray_Count), [message valueForKey:@"repeatedStringArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedBytesArray_Count), [message valueForKey:@"repeatedBytesArray_Count"]);

  XCTAssertEqualObjects(@(message.repeatedGroupArray_Count), [message valueForKey:@"repeatedGroupArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedNestedMessageArray_Count), [message valueForKey:@"repeatedNestedMessageArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedForeignMessageArray_Count), [message valueForKey:@"repeatedForeignMessageArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedImportMessageArray_Count), [message valueForKey:@"repeatedImportMessageArray_Count"]);

  XCTAssertEqualObjects(@(message.repeatedNestedEnumArray_Count), [message valueForKey:@"repeatedNestedEnumArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedForeignEnumArray_Count), [message valueForKey:@"repeatedForeignEnumArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedImportEnumArray_Count), [message valueForKey:@"repeatedImportEnumArray_Count"]);

  XCTAssertEqualObjects(@(message.repeatedStringPieceArray_Count), [message valueForKey:@"repeatedStringPieceArray_Count"]);
  XCTAssertEqualObjects(@(message.repeatedCordArray_Count), [message valueForKey:@"repeatedCordArray_Count"]);

  // -----------------------------------------------------------------

  XCTAssertEqualObjects([message valueForKey:@"hasDefaultInt32"], @YES);
  XCTAssertEqualObjects(@(message.defaultInt32), [message valueForKey:@"defaultInt32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultInt64"], @YES);
  XCTAssertEqualObjects(@(message.defaultInt64), [message valueForKey:@"defaultInt64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultUint32"], @YES);
  XCTAssertEqualObjects(@(message.defaultUint32), [message valueForKey:@"defaultUint32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultUint64"], @YES);
  XCTAssertEqualObjects(@(message.defaultUint64), [message valueForKey:@"defaultUint64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSint32"], @YES);
  XCTAssertEqualObjects(@(message.defaultSint32), [message valueForKey:@"defaultSint32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSint64"], @YES);
  XCTAssertEqualObjects(@(message.defaultSint64), [message valueForKey:@"defaultSint64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultFixed32"], @YES);
  XCTAssertEqualObjects(@(message.defaultFixed32), [message valueForKey:@"defaultFixed32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultFixed64"], @YES);
  XCTAssertEqualObjects(@(message.defaultFixed64), [message valueForKey:@"defaultFixed64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSfixed32"], @YES);
  XCTAssertEqualObjects(@(message.defaultSfixed32), [message valueForKey:@"defaultSfixed32"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSfixed64"], @YES);
  XCTAssertEqualObjects(@(message.defaultSfixed64), [message valueForKey:@"defaultSfixed64"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultFloat"], @YES);
  XCTAssertEqualObjects(@(message.defaultFloat), [message valueForKey:@"defaultFloat"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultDouble"], @YES);
  XCTAssertEqualObjects(@(message.defaultDouble), [message valueForKey:@"defaultDouble"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultBool"], @YES);
  XCTAssertEqualObjects(@(message.defaultBool), [message valueForKey:@"defaultBool"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultString"], @YES);
  XCTAssertEqualObjects(message.defaultString, [message valueForKey:@"defaultString"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultBytes"], @YES);
  XCTAssertEqualObjects(message.defaultBytes, [message valueForKey:@"defaultBytes"]);

  XCTAssertEqualObjects([message valueForKey:@"hasDefaultNestedEnum"], @YES);
  XCTAssertEqualObjects(@(message.defaultNestedEnum), [message valueForKey:@"defaultNestedEnum"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultForeignEnum"], @YES);
  XCTAssertEqualObjects(@(message.defaultForeignEnum), [message valueForKey:@"defaultForeignEnum"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultImportEnum"], @YES);
  XCTAssertEqualObjects(@(message.defaultImportEnum), [message valueForKey:@"defaultImportEnum"]);

  XCTAssertEqualObjects([message valueForKey:@"hasDefaultStringPiece"], @YES);
  XCTAssertEqualObjects(message.defaultStringPiece, [message valueForKey:@"defaultStringPiece"]);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultCord"], @YES);
  XCTAssertEqualObjects(message.defaultCord, [message valueForKey:@"defaultCord"]);
}

- (void)setAllFieldsViaKVC:(TestAllTypes *)message
             repeatedCount:(uint32_t)count {
  [message setValue:@101 forKey:@"optionalInt32"];
  [message setValue:@102 forKey:@"optionalInt64"];
  [message setValue:@103 forKey:@"optionalUint32"];
  [message setValue:@104 forKey:@"optionalUint64"];
  [message setValue:@105 forKey:@"optionalSint32"];
  [message setValue:@106 forKey:@"optionalSint64"];
  [message setValue:@107 forKey:@"optionalFixed32"];
  [message setValue:@108 forKey:@"optionalFixed64"];
  [message setValue:@109 forKey:@"optionalSfixed32"];
  [message setValue:@110 forKey:@"optionalSfixed64"];
  [message setValue:@111 forKey:@"optionalFloat"];
  [message setValue:@112 forKey:@"optionalDouble"];
  [message setValue:@YES forKey:@"optionalBool"];
  [message setValue:@"115" forKey:@"optionalString"];
  [message setValue:[NSData gpbtu_dataWithEmbeddedNulls]
             forKey:@"optionalBytes"];

  TestAllTypes_OptionalGroup *allTypes = [TestAllTypes_OptionalGroup message];
  [allTypes setValue:@117 forKey:@"a"];
  [message setValue:allTypes forKey:@"optionalGroup"];
  TestAllTypes_NestedMessage *nestedMessage =
      [TestAllTypes_NestedMessage message];
  [nestedMessage setValue:@118 forKey:@"bb"];
  [message setValue:nestedMessage forKey:@"optionalNestedMessage"];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [foreignMessage setValue:@119 forKey:@"c"];
  [message setValue:foreignMessage forKey:@"optionalForeignMessage"];
  ImportMessage *importMessage = [ImportMessage message];
  [importMessage setValue:@120 forKey:@"d"];
  [message setValue:importMessage forKey:@"optionalImportMessage"];

  [message setValue:@(TestAllTypes_NestedEnum_Baz)
             forKey:@"optionalNestedEnum"];
  [message setValue:@(ForeignEnum_ForeignBaz) forKey:@"optionalForeignEnum"];
  [message setValue:@(ImportEnum_ImportBaz) forKey:@"optionalImportEnum"];

  [message setValue:@"124" forKey:@"optionalStringPiece"];
  [message setValue:@"125" forKey:@"optionalCord"];

  // -----------------------------------------------------------------

  {
    GPBInt32Array *scratch = [GPBInt32Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:201 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedInt32Array"];
  }
  {
    GPBInt64Array *scratch = [GPBInt64Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:202 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedInt64Array"];
  }
  {
    GPBUInt32Array *scratch = [GPBUInt32Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:203 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedUint32Array"];
  }
  {
    GPBUInt64Array *scratch = [GPBUInt64Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:204 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedUint64Array"];
  }
  {
    GPBInt32Array *scratch = [GPBInt32Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:205 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedSint32Array"];
  }
  {
    GPBInt64Array *scratch = [GPBInt64Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:206 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedSint64Array"];
  }
  {
    GPBUInt32Array *scratch = [GPBUInt32Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:207 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedFixed32Array"];
  }
  {
    GPBUInt64Array *scratch = [GPBUInt64Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:208 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedFixed64Array"];
  }
  {
    GPBInt32Array *scratch = [GPBInt32Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:209 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedSfixed32Array"];
  }
  {
    GPBInt64Array *scratch = [GPBInt64Array array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:210 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedSfixed64Array"];
  }
  {
    GPBFloatArray *scratch = [GPBFloatArray array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:211 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedFloatArray"];
  }
  {
    GPBDoubleArray *scratch = [GPBDoubleArray array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:212 + i * 100];
    }
    [message setValue:scratch forKey:@"repeatedDoubleArray"];
  }
  {
    GPBBoolArray *scratch = [GPBBoolArray array];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:(i % 2) ? YES : NO];
    }
    [message setValue:scratch forKey:@"repeatedBoolArray"];
  }

  NSMutableArray *array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    NSString *string = [[NSString alloc] initWithFormat:@"%d", 215 + i * 100];
    [array addObject:string];
    [string release];
  }
  [message setValue:array forKey:@"repeatedStringArray"];
  [array release];

  array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    NSData *data = [[NSData alloc] initWithUint32_gpbtu:216 + i * 100];
    [array addObject:data];
    [data release];
  }
  [message setValue:array forKey:@"repeatedBytesArray"];
  [array release];

  array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    TestAllTypes_RepeatedGroup *testAll =
        [[TestAllTypes_RepeatedGroup alloc] init];
    [testAll setA:217 + i * 100];
    [array addObject:testAll];
    [testAll release];
  }
  [message setValue:array forKey:@"repeatedGroupArray"];
  [array release];

  array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    nestedMessage = [[TestAllTypes_NestedMessage alloc] init];
    [nestedMessage setBb:218 + i * 100];
    [array addObject:nestedMessage];
    [nestedMessage release];
  }
  [message setValue:array forKey:@"repeatedNestedMessageArray"];
  [array release];

  array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    foreignMessage = [[ForeignMessage alloc] init];
    [foreignMessage setC:219 + i * 100];
    [array addObject:foreignMessage];
    [foreignMessage release];
  }
  [message setValue:array forKey:@"repeatedForeignMessageArray"];
  [array release];

  array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    importMessage = [[ImportMessage alloc] init];
    [importMessage setD:220 + i * 100];
    [array addObject:importMessage];
    [importMessage release];
  }
  [message setValue:array forKey:@"repeatedImportMessageArray"];
  [array release];

  {
    GPBEnumArray *scratch = [GPBEnumArray
        arrayWithValidationFunction:TestAllTypes_NestedEnum_IsValidValue];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:(i % 2) ? TestAllTypes_NestedEnum_Bar
                                : TestAllTypes_NestedEnum_Baz];
    }
    [message setValue:scratch forKey:@"repeatedNestedEnumArray"];
  }
  {
    GPBEnumArray *scratch =
        [GPBEnumArray arrayWithValidationFunction:ForeignEnum_IsValidValue];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch
          addValue:(i % 2) ? ForeignEnum_ForeignBar : ForeignEnum_ForeignBaz];
    }
    [message setValue:scratch forKey:@"repeatedForeignEnumArray"];
  }
  {
    GPBEnumArray *scratch =
        [GPBEnumArray arrayWithValidationFunction:ImportEnum_IsValidValue];
    for (uint32_t i = 0; i < count; ++i) {
      [scratch addValue:(i % 2) ? ImportEnum_ImportBar : ImportEnum_ImportBaz];
    }
    [message setValue:scratch forKey:@"repeatedImportEnumArray"];
  }

  array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    NSString *string = [[NSString alloc] initWithFormat:@"%d", 224 + i * 100];
    [array addObject:string];
    [string release];
  }
  [message setValue:array forKey:@"repeatedStringPieceArray"];
  [array release];

  array = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0; i < count; ++i) {
    NSString *string = [[NSString alloc] initWithFormat:@"%d", 225 + i * 100];
    [array addObject:string];
    [string release];
  }
  [message setValue:array forKey:@"repeatedCordArray"];
  [array release];

  // -----------------------------------------------------------------

  [message setValue:@401 forKey:@"defaultInt32"];
  [message setValue:@402 forKey:@"defaultInt64"];
  [message setValue:@403 forKey:@"defaultUint32"];
  [message setValue:@404 forKey:@"defaultUint64"];
  [message setValue:@405 forKey:@"defaultSint32"];
  [message setValue:@406 forKey:@"defaultSint64"];
  [message setValue:@407 forKey:@"defaultFixed32"];
  [message setValue:@408 forKey:@"defaultFixed64"];
  [message setValue:@409 forKey:@"defaultSfixed32"];
  [message setValue:@410 forKey:@"defaultSfixed64"];
  [message setValue:@411 forKey:@"defaultFloat"];
  [message setValue:@412 forKey:@"defaultDouble"];
  [message setValue:@NO forKey:@"defaultBool"];
  [message setValue:@"415" forKey:@"defaultString"];
  [message setValue:[NSData gpbtu_dataWithUint32:416] forKey:@"defaultBytes"];

  [message setValue:@(TestAllTypes_NestedEnum_Foo) forKey:@"defaultNestedEnum"];
  [message setValue:@(ForeignEnum_ForeignFoo) forKey:@"defaultForeignEnum"];
  [message setValue:@(ImportEnum_ImportFoo) forKey:@"defaultImportEnum"];

  [message setValue:@"424" forKey:@"defaultStringPiece"];
  [message setValue:@"425" forKey:@"defaultCord"];
}

- (void)assertClearKVC:(TestAllTypes *)message {
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalInt32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalInt64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalUint32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalUint64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSint32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSint64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalFixed32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalFixed64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSfixed32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalSfixed64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalFloat"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalDouble"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalBool"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalString"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalBytes"], @NO);

  XCTAssertEqualObjects([message valueForKey:@"hasOptionalGroup"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalNestedMessage"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalForeignMessage"],
                        @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalImportMessage"], @NO);

  XCTAssertEqualObjects([message valueForKey:@"hasOptionalNestedEnum"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalForeignEnum"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalImportEnum"], @NO);

  XCTAssertEqualObjects([message valueForKey:@"hasOptionalStringPiece"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasOptionalCord"], @NO);

  // Optional fields without defaults are set to zero or something like it.
  XCTAssertEqualObjects([message valueForKey:@"optionalInt32"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalInt64"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalUint32"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalUint64"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalSint32"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalSint64"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalFixed32"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalFixed64"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalSfixed32"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalSfixed64"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalFloat"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalDouble"], @0);
  XCTAssertEqualObjects([message valueForKey:@"optionalBool"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"optionalString"], @"");
  XCTAssertEqualObjects([message valueForKey:@"optionalBytes"],
                        GPBEmptyNSData());

  // Embedded messages should also be exist, but be clear.
  XCTAssertNotNil([message valueForKeyPath:@"optionalGroup"]);
  XCTAssertNotNil([message valueForKeyPath:@"optionalNestedMessage"]);
  XCTAssertNotNil([message valueForKeyPath:@"optionalForeignMessage"]);
  XCTAssertNotNil([message valueForKeyPath:@"optionalImportMessage"]);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalGroup.hasA"], @NO);
  XCTAssertEqualObjects(
      [message valueForKeyPath:@"optionalNestedMessage.hasBb"], @NO);
  XCTAssertEqualObjects(
      [message valueForKeyPath:@"optionalForeignMessage.hasC"], @NO);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalImportMessage.hasD"],
                        @NO);

  XCTAssertEqualObjects([message valueForKeyPath:@"optionalGroup.a"], @0);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalNestedMessage.bb"],
                        @0);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalForeignMessage.c"],
                        @0);
  XCTAssertEqualObjects([message valueForKeyPath:@"optionalImportMessage.d"],
                        @0);

  // Enums without defaults are set to the first value in the enum.
  XCTAssertEqualObjects([message valueForKey:@"optionalNestedEnum"],
                        @(TestAllTypes_NestedEnum_Foo));
  XCTAssertEqualObjects([message valueForKey:@"optionalForeignEnum"],
                        @(ForeignEnum_ForeignFoo));
  XCTAssertEqualObjects([message valueForKey:@"optionalImportEnum"],
                        @(ImportEnum_ImportFoo));

  XCTAssertEqualObjects([message valueForKey:@"optionalStringPiece"], @"");
  XCTAssertEqualObjects([message valueForKey:@"optionalCord"], @"");

  // NSArray interface for repeated doesn't have has*, nil means no value.

  XCTAssertEqualObjects([message valueForKey:@"hasDefaultInt32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultInt64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultUint32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultUint64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSint32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSint64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultFixed32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultFixed64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSfixed32"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultSfixed64"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultFloat"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultDouble"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultBool"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultString"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultBytes"], @NO);

  XCTAssertEqualObjects([message valueForKey:@"hasDefaultNestedEnum"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultForeignEnum"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultImportEnum"], @NO);

  XCTAssertEqualObjects([message valueForKey:@"hasDefaultStringPiece"], @NO);
  XCTAssertEqualObjects([message valueForKey:@"hasDefaultCord"], @NO);

  // Fields with defaults have their default values (duh).
  XCTAssertEqualObjects([message valueForKey:@"defaultInt32"], @41);
  XCTAssertEqualObjects([message valueForKey:@"defaultInt64"], @42);
  XCTAssertEqualObjects([message valueForKey:@"defaultUint32"], @43);
  XCTAssertEqualObjects([message valueForKey:@"defaultUint64"], @44);
  XCTAssertEqualObjects([message valueForKey:@"defaultSint32"], @-45);
  XCTAssertEqualObjects([message valueForKey:@"defaultSint64"], @46);
  XCTAssertEqualObjects([message valueForKey:@"defaultFixed32"], @47);
  XCTAssertEqualObjects([message valueForKey:@"defaultFixed64"], @48);
  XCTAssertEqualObjects([message valueForKey:@"defaultSfixed32"], @49);
  XCTAssertEqualObjects([message valueForKey:@"defaultSfixed64"], @-50);
  XCTAssertEqualObjects([message valueForKey:@"defaultFloat"], @51.5);
  XCTAssertEqualObjects([message valueForKey:@"defaultDouble"], @52e3);
  XCTAssertEqualObjects([message valueForKey:@"defaultBool"], @YES);
  XCTAssertEqualObjects([message valueForKey:@"defaultString"], @"hello");
  XCTAssertEqualObjects([message valueForKey:@"defaultBytes"],
                        [NSData gpbtu_dataWithCString:"world"]);

  XCTAssertEqualObjects([message valueForKey:@"defaultNestedEnum"],
                        @(TestAllTypes_NestedEnum_Bar));
  XCTAssertEqualObjects([message valueForKey:@"defaultForeignEnum"],
                        @(ForeignEnum_ForeignBar));
  XCTAssertEqualObjects([message valueForKey:@"defaultImportEnum"],
                        @(ImportEnum_ImportBar));

  XCTAssertEqualObjects([message valueForKey:@"defaultStringPiece"], @"abc");
  XCTAssertEqualObjects([message valueForKey:@"defaultCord"], @"123");
}

@end
