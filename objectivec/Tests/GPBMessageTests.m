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

#import <objc/runtime.h>

#import "GPBArray_PackagePrivate.h"
#import "GPBDescriptor.h"
#import "GPBDictionary_PackagePrivate.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUnknownField_PackagePrivate.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestObjc.pbobjc.h"
#import "google/protobuf/UnittestObjcOptions.pbobjc.h"

@interface MessageTests : GPBTestCase
@end

@implementation MessageTests

// TODO(thomasvl): this should get split into a few files of logic junks, it is
// a jumble of things at the moment (and the testutils have a bunch of the real
// assertions).

- (TestAllTypes *)mergeSource {
  TestAllTypes *message = [TestAllTypes message];
  [message setOptionalInt32:1];
  [message setOptionalString:@"foo"];
  [message setOptionalForeignMessage:[ForeignMessage message]];
  [message.repeatedStringArray addObject:@"bar"];
  return message;
}

- (TestAllTypes *)mergeDestination {
  TestAllTypes *message = [TestAllTypes message];
  [message setOptionalInt64:2];
  [message setOptionalString:@"baz"];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [foreignMessage setC:3];
  [message setOptionalForeignMessage:foreignMessage];
  [message.repeatedStringArray addObject:@"qux"];
  return message;
}

- (TestAllTypes *)mergeDestinationWithoutForeignMessageIvar {
  TestAllTypes *message = [TestAllTypes message];
  [message setOptionalInt64:2];
  [message setOptionalString:@"baz"];
  [message.repeatedStringArray addObject:@"qux"];
  return message;
}

- (TestAllTypes *)mergeResult {
  TestAllTypes *message = [TestAllTypes message];
  [message setOptionalInt32:1];
  [message setOptionalInt64:2];
  [message setOptionalString:@"foo"];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [foreignMessage setC:3];
  [message setOptionalForeignMessage:foreignMessage];
  [message.repeatedStringArray addObject:@"qux"];
  [message.repeatedStringArray addObject:@"bar"];
  return message;
}

- (TestAllTypes *)mergeResultForDestinationWithoutForeignMessageIvar {
  TestAllTypes *message = [TestAllTypes message];
  [message setOptionalInt32:1];
  [message setOptionalInt64:2];
  [message setOptionalString:@"foo"];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [message setOptionalForeignMessage:foreignMessage];
  [message.repeatedStringArray addObject:@"qux"];
  [message.repeatedStringArray addObject:@"bar"];
  return message;
}

- (TestAllExtensions *)mergeExtensionsDestination {
  TestAllExtensions *message = [TestAllExtensions message];
  [message setExtension:[UnittestRoot optionalInt32Extension] value:@5];
  [message setExtension:[UnittestRoot optionalStringExtension] value:@"foo"];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  foreignMessage.c = 4;
  [message setExtension:[UnittestRoot optionalForeignMessageExtension]
                  value:foreignMessage];
  TestAllTypes_NestedMessage *nestedMessage =
      [TestAllTypes_NestedMessage message];
  [message setExtension:[UnittestRoot optionalNestedMessageExtension]
                  value:nestedMessage];
  return message;
}

- (TestAllExtensions *)mergeExtensionsSource {
  TestAllExtensions *message = [TestAllExtensions message];
  [message setExtension:[UnittestRoot optionalInt64Extension] value:@6];
  [message setExtension:[UnittestRoot optionalStringExtension] value:@"bar"];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  [message setExtension:[UnittestRoot optionalForeignMessageExtension]
                  value:foreignMessage];
  TestAllTypes_NestedMessage *nestedMessage =
      [TestAllTypes_NestedMessage message];
  nestedMessage.bb = 7;
  [message setExtension:[UnittestRoot optionalNestedMessageExtension]
                  value:nestedMessage];
  return message;
}

- (TestAllExtensions *)mergeExtensionsResult {
  TestAllExtensions *message = [TestAllExtensions message];
  [message setExtension:[UnittestRoot optionalInt32Extension] value:@5];
  [message setExtension:[UnittestRoot optionalInt64Extension] value:@6];
  [message setExtension:[UnittestRoot optionalStringExtension] value:@"bar"];
  ForeignMessage *foreignMessage = [ForeignMessage message];
  foreignMessage.c = 4;
  [message setExtension:[UnittestRoot optionalForeignMessageExtension]
                  value:foreignMessage];
  TestAllTypes_NestedMessage *nestedMessage =
      [TestAllTypes_NestedMessage message];
  nestedMessage.bb = 7;
  [message setExtension:[UnittestRoot optionalNestedMessageExtension]
                  value:nestedMessage];
  return message;
}

- (void)testMergeFrom {
  TestAllTypes *result = [[self.mergeDestination copy] autorelease];
  [result mergeFrom:self.mergeSource];
  NSData *resultData = [result data];
  NSData *mergeResultData = [self.mergeResult data];
  XCTAssertEqualObjects(resultData, mergeResultData);
  XCTAssertEqualObjects(result, self.mergeResult);

  // Test when destination does not have an Ivar (type is an object) but source
  // has such Ivar.
  // The result must has the Ivar which is same as the one in source.
  result = [[self.mergeDestinationWithoutForeignMessageIvar copy] autorelease];
  [result mergeFrom:self.mergeSource];
  resultData = [result data];
  mergeResultData =
      [self.mergeResultForDestinationWithoutForeignMessageIvar data];
  XCTAssertEqualObjects(resultData, mergeResultData);
  XCTAssertEqualObjects(
      result, self.mergeResultForDestinationWithoutForeignMessageIvar);

  // Test when destination is empty.
  // The result must is same as the source.
  result = [TestAllTypes message];
  [result mergeFrom:self.mergeSource];
  resultData = [result data];
  mergeResultData = [self.mergeSource data];
  XCTAssertEqualObjects(resultData, mergeResultData);
  XCTAssertEqualObjects(result, self.mergeSource);
}

- (void)testMergeFromWithExtensions {
  TestAllExtensions *result = [self mergeExtensionsDestination];
  [result mergeFrom:[self mergeExtensionsSource]];
  NSData *resultData = [result data];
  NSData *mergeResultData = [[self mergeExtensionsResult] data];
  XCTAssertEqualObjects(resultData, mergeResultData);
  XCTAssertEqualObjects(result, [self mergeExtensionsResult]);

  // Test merging from data.
  result = [self mergeExtensionsDestination];
  NSData *data = [[self mergeExtensionsSource] data];
  XCTAssertNotNil(data);
  [result mergeFromData:data
      extensionRegistry:[UnittestRoot extensionRegistry]];
  resultData = [result data];
  XCTAssertEqualObjects(resultData, mergeResultData);
  XCTAssertEqualObjects(result, [self mergeExtensionsResult]);
}

- (void)testIsEquals {
  TestAllTypes *result = [[self.mergeDestination copy] autorelease];
  [result mergeFrom:self.mergeSource];
  XCTAssertEqualObjects(result.data, self.mergeResult.data);
  XCTAssertEqualObjects(result, self.mergeResult);
  TestAllTypes *result2 = [[self.mergeDestination copy] autorelease];
  XCTAssertNotEqualObjects(result2.data, self.mergeResult.data);
  XCTAssertNotEqualObjects(result2, self.mergeResult);
}

// =================================================================
// Required-field-related tests.

- (TestRequired *)testRequiredInitialized {
  TestRequired *message = [TestRequired message];
  [message setA:1];
  [message setB:2];
  [message setC:3];
  return message;
}

- (void)testRequired {
  TestRequired *message = [TestRequired message];

  XCTAssertFalse(message.initialized);
  [message setA:1];
  XCTAssertFalse(message.initialized);
  [message setB:1];
  XCTAssertFalse(message.initialized);
  [message setC:1];
  XCTAssertTrue(message.initialized);
}

- (void)testRequiredForeign {
  TestRequiredForeign *message = [TestRequiredForeign message];

  XCTAssertTrue(message.initialized);

  [message setOptionalMessage:[TestRequired message]];
  XCTAssertFalse(message.initialized);

  [message setOptionalMessage:self.testRequiredInitialized];
  XCTAssertTrue(message.initialized);

  [message.repeatedMessageArray addObject:[TestRequired message]];
  XCTAssertFalse(message.initialized);

  [message.repeatedMessageArray removeAllObjects];
  [message.repeatedMessageArray addObject:self.testRequiredInitialized];
  XCTAssertTrue(message.initialized);
}

- (void)testRequiredExtension {
  TestAllExtensions *message = [TestAllExtensions message];

  XCTAssertTrue(message.initialized);

  [message setExtension:[TestRequired single] value:[TestRequired message]];
  XCTAssertFalse(message.initialized);

  [message setExtension:[TestRequired single]
                  value:self.testRequiredInitialized];
  XCTAssertTrue(message.initialized);

  [message addExtension:[TestRequired multi] value:[TestRequired message]];
  XCTAssertFalse(message.initialized);

  [message setExtension:[TestRequired multi]
                  index:0
                  value:self.testRequiredInitialized];
  XCTAssertTrue(message.initialized);
}

- (void)testDataFromUninitialized {
  TestRequired *message = [TestRequired message];
  NSData *data = [message data];
  // In DEBUG, the data generation will fail, but in non DEBUG, it passes
  // because the check isn't done (for speed).
#ifdef DEBUG
  XCTAssertNil(data);
#else
  XCTAssertNotNil(data);
  XCTAssertFalse(message.initialized);
#endif  // DEBUG
}

- (void)testInitialized {
  // We're mostly testing that no exception is thrown.
  TestRequired *message = [TestRequired message];
  XCTAssertFalse(message.initialized);
}

- (void)testDataFromNestedUninitialized {
  TestRequiredForeign *message = [TestRequiredForeign message];
  [message setOptionalMessage:[TestRequired message]];
  [message.repeatedMessageArray addObject:[TestRequired message]];
  [message.repeatedMessageArray addObject:[TestRequired message]];
  NSData *data = [message data];
  // In DEBUG, the data generation will fail, but in non DEBUG, it passes
  // because the check isn't done (for speed).
#ifdef DEBUG
  XCTAssertNil(data);
#else
  XCTAssertNotNil(data);
  XCTAssertFalse(message.initialized);
#endif  // DEBUG
}

- (void)testNestedInitialized {
  // We're mostly testing that no exception is thrown.

  TestRequiredForeign *message = [TestRequiredForeign message];
  [message setOptionalMessage:[TestRequired message]];
  [message.repeatedMessageArray addObject:[TestRequired message]];
  [message.repeatedMessageArray addObject:[TestRequired message]];

  XCTAssertFalse(message.initialized);
}

- (void)testParseUninitialized {
  NSError *error = nil;
  TestRequired *msg =
      [TestRequired parseFromData:GPBEmptyNSData() error:&error];
  // In DEBUG, the parse will fail, but in non DEBUG, it passes because
  // the check isn't done (for speed).
#ifdef DEBUG
  XCTAssertNil(msg);
  XCTAssertNotNil(error);
  XCTAssertEqualObjects(error.domain, GPBMessageErrorDomain);
  XCTAssertEqual(error.code, GPBMessageErrorCodeMissingRequiredField);
#else
  XCTAssertNotNil(msg);
  XCTAssertNil(error);
  XCTAssertFalse(msg.initialized);
#endif  // DEBUG
}

- (void)testCoding {
  NSData *data =
      [NSKeyedArchiver archivedDataWithRootObject:[self mergeResult]];
  id unarchivedObject = [NSKeyedUnarchiver unarchiveObjectWithData:data];

  XCTAssertEqualObjects(unarchivedObject, [self mergeResult]);

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(unarchivedObject, [self mergeResult]);
}

- (void)testObjectReset {
  // Tests a failure where clearing out defaults values caused an over release.
  TestAllTypes *message = [TestAllTypes message];
  message.hasOptionalNestedMessage = NO;
  [message setOptionalNestedMessage:[TestAllTypes_NestedMessage message]];
  message.hasOptionalNestedMessage = NO;
  [message setOptionalNestedMessage:[TestAllTypes_NestedMessage message]];
  [message setOptionalNestedMessage:nil];
  message.hasOptionalNestedMessage = NO;
}

- (void)testSettingHasToYes {
  TestAllTypes *message = [TestAllTypes message];
  XCTAssertThrows([message setHasOptionalNestedMessage:YES]);
}

- (void)testRoot {
  XCTAssertNotNil([UnittestRoot extensionRegistry]);
}

- (void)testGPBMessageSize {
  // See the note in GPBMessage_PackagePrivate.h about why we want to keep the
  // base instance size pointer size aligned.
  size_t messageSize = class_getInstanceSize([GPBMessage class]);
  XCTAssertEqual((messageSize % sizeof(void *)), (size_t)0,
                 @"Base size isn't pointer size aligned");

  // Since we add storage ourselves (see +allocWithZone: in GPBMessage), confirm
  // that the size of some generated classes is still the same as the base for
  // that logic to work as desired.
  size_t testMessageSize = class_getInstanceSize([TestAllTypes class]);
  XCTAssertEqual(testMessageSize, messageSize);
}

- (void)testInit {
  TestAllTypes *message = [TestAllTypes message];
  [self assertClear:message];
}

- (void)testAccessors {
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:kGPBDefaultRepeatCount];
  [self assertAllFieldsSet:message repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testKVC_ValueForKey {
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:kGPBDefaultRepeatCount];
  [self assertAllFieldsKVCMatch:message];
}

- (void)testKVC_SetValue_ForKey {
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFieldsViaKVC:message repeatedCount:kGPBDefaultRepeatCount];
  [self assertAllFieldsKVCMatch:message];
  [self assertAllFieldsSet:message repeatedCount:kGPBDefaultRepeatCount];
  [self assertAllFieldsKVCMatch:message];
}

- (void)testDescription {
  // No real test, just exercise code
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:kGPBDefaultRepeatCount];

  GPBUnknownFieldSet *unknownFields =
      [[[GPBUnknownFieldSet alloc] init] autorelease];
  GPBUnknownField *field =
      [[[GPBUnknownField alloc] initWithNumber:2] autorelease];
  [field addVarint:2];
  [unknownFields addField:field];
  field = [[[GPBUnknownField alloc] initWithNumber:3] autorelease];
  [field addVarint:4];
  [unknownFields addField:field];

  [message setUnknownFields:unknownFields];

  NSString *description = [message description];
  XCTAssertGreaterThan([description length], 0U);

  GPBMessage *message2 = [TestAllExtensions message];
  [message2 setExtension:[UnittestRoot optionalInt32Extension] value:@1];

  [message2 addExtension:[UnittestRoot repeatedInt32Extension] value:@2];

  description = [message2 description];
  XCTAssertGreaterThan([description length], 0U);
}

- (void)testSetter {
  // Test to make sure that if we set a value that has a default value
  // with the default, that the has is set, and the value gets put into the
  // message correctly.
  TestAllTypes *message = [TestAllTypes message];
  GPBDescriptor *descriptor = [[message class] descriptor];
  XCTAssertNotNil(descriptor);
  GPBFieldDescriptor *fieldDescriptor =
      [descriptor fieldWithName:@"defaultInt32"];
  XCTAssertNotNil(fieldDescriptor);
  GPBGenericValue defaultValue = [fieldDescriptor defaultValue];
  [message setDefaultInt32:defaultValue.valueInt32];
  XCTAssertTrue(message.hasDefaultInt32);
  XCTAssertEqual(message.defaultInt32, defaultValue.valueInt32);

  // Do the same thing with an object type.
  message = [TestAllTypes message];
  fieldDescriptor = [descriptor fieldWithName:@"defaultString"];
  XCTAssertNotNil(fieldDescriptor);
  defaultValue = [fieldDescriptor defaultValue];
  [message setDefaultString:defaultValue.valueString];
  XCTAssertTrue(message.hasDefaultString);
  XCTAssertEqualObjects(message.defaultString, defaultValue.valueString);

  // Test default string type.
  message = [TestAllTypes message];
  XCTAssertEqualObjects(message.defaultString, @"hello");
  XCTAssertFalse(message.hasDefaultString);
  fieldDescriptor = [descriptor fieldWithName:@"defaultString"];
  XCTAssertNotNil(fieldDescriptor);
  defaultValue = [fieldDescriptor defaultValue];
  [message setDefaultString:defaultValue.valueString];
  XCTAssertEqualObjects(message.defaultString, @"hello");
  XCTAssertTrue(message.hasDefaultString);
  [message setDefaultString:nil];
  XCTAssertEqualObjects(message.defaultString, @"hello");
  XCTAssertFalse(message.hasDefaultString);
  message.hasDefaultString = NO;
  XCTAssertFalse(message.hasDefaultString);
  XCTAssertEqualObjects(message.defaultString, @"hello");

  // Test default bytes type.
  NSData *defaultBytes = [@"world" dataUsingEncoding:NSUTF8StringEncoding];
  XCTAssertEqualObjects(message.defaultBytes, defaultBytes);
  XCTAssertFalse(message.hasDefaultString);
  fieldDescriptor = [descriptor fieldWithName:@"defaultBytes"];
  XCTAssertNotNil(fieldDescriptor);
  defaultValue = [fieldDescriptor defaultValue];
  [message setDefaultBytes:defaultValue.valueData];
  XCTAssertEqualObjects(message.defaultBytes, defaultBytes);
  XCTAssertTrue(message.hasDefaultBytes);
  [message setDefaultBytes:nil];
  XCTAssertEqualObjects(message.defaultBytes, defaultBytes);
  XCTAssertFalse(message.hasDefaultBytes);
  message.hasDefaultBytes = NO;
  XCTAssertFalse(message.hasDefaultBytes);
  XCTAssertEqualObjects(message.defaultBytes, defaultBytes);

  // Test optional string.
  XCTAssertFalse(message.hasOptionalString);
  XCTAssertEqualObjects(message.optionalString, @"");
  XCTAssertFalse(message.hasOptionalString);
  message.optionalString = nil;
  XCTAssertFalse(message.hasOptionalString);
  XCTAssertEqualObjects(message.optionalString, @"");
  NSString *string = @"string";
  message.optionalString = string;
  XCTAssertEqualObjects(message.optionalString, string);
  XCTAssertTrue(message.hasOptionalString);
  message.optionalString = nil;
  XCTAssertFalse(message.hasOptionalString);
  XCTAssertEqualObjects(message.optionalString, @"");

  // Test optional data.
  XCTAssertFalse(message.hasOptionalBytes);
  XCTAssertEqualObjects(message.optionalBytes, GPBEmptyNSData());
  XCTAssertFalse(message.hasOptionalBytes);
  message.optionalBytes = nil;
  XCTAssertFalse(message.hasOptionalBytes);
  XCTAssertEqualObjects(message.optionalBytes, GPBEmptyNSData());
  NSData *data = [@"bytes" dataUsingEncoding:NSUTF8StringEncoding];
  message.optionalBytes = data;
  XCTAssertEqualObjects(message.optionalBytes, data);
  XCTAssertTrue(message.hasOptionalBytes);
  message.optionalBytes = nil;
  XCTAssertFalse(message.hasOptionalBytes);
  XCTAssertEqualObjects(message.optionalBytes, GPBEmptyNSData());

  // Test lazy message setting
  XCTAssertFalse(message.hasOptionalLazyMessage);
  XCTAssertNotNil(message.optionalLazyMessage);
  XCTAssertFalse(message.hasOptionalLazyMessage);
  message.hasOptionalLazyMessage = NO;
  XCTAssertFalse(message.hasOptionalLazyMessage);
  XCTAssertNotNil(message.optionalLazyMessage);
  XCTAssertFalse(message.hasOptionalLazyMessage);
  message.optionalLazyMessage = nil;
  XCTAssertFalse(message.hasOptionalLazyMessage);

  // Test nested messages
  XCTAssertFalse(message.hasOptionalLazyMessage);
  message.optionalLazyMessage.bb = 1;
  XCTAssertTrue(message.hasOptionalLazyMessage);
  XCTAssertEqual(message.optionalLazyMessage.bb, 1);
  XCTAssertNotNil(message.optionalLazyMessage);
  message.optionalLazyMessage = nil;
  XCTAssertFalse(message.hasOptionalLazyMessage);
  XCTAssertEqual(message.optionalLazyMessage.bb, 0);
  XCTAssertFalse(message.hasOptionalLazyMessage);
  XCTAssertNotNil(message.optionalLazyMessage);

  // -testDefaultSubMessages tests the "defaulting" handling of fields
  // containing messages.
}

- (void)testRepeatedSetters {
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:kGPBDefaultRepeatCount];
  [self modifyRepeatedFields:message];
  [self assertRepeatedFieldsModified:message
                       repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testClear {
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:kGPBDefaultRepeatCount];
  [self clearAllFields:message];
  [self assertClear:message];
  TestAllTypes *message2 = [TestAllTypes message];
  XCTAssertEqualObjects(message, message2);
}

- (void)testClearKVC {
  TestAllTypes *message = [TestAllTypes message];
  [self setAllFields:message repeatedCount:kGPBDefaultRepeatCount];
  [self clearAllFields:message];
  [self assertClear:message];
  [self assertClearKVC:message];
}

- (void)testClearExtension {
  // clearExtension() is not actually used in TestUtil, so try it manually.
  GPBMessage *message1 = [TestAllExtensions message];
  [message1 setExtension:[UnittestRoot optionalInt32Extension] value:@1];

  XCTAssertTrue([message1 hasExtension:[UnittestRoot optionalInt32Extension]]);
  [message1 clearExtension:[UnittestRoot optionalInt32Extension]];
  XCTAssertFalse([message1 hasExtension:[UnittestRoot optionalInt32Extension]]);

  GPBMessage *message2 = [TestAllExtensions message];
  [message2 addExtension:[UnittestRoot repeatedInt32Extension] value:@1];

  XCTAssertEqual(
      [[message2 getExtension:[UnittestRoot repeatedInt32Extension]] count],
      (NSUInteger)1);
  [message2 clearExtension:[UnittestRoot repeatedInt32Extension]];
  XCTAssertEqual(
      [[message2 getExtension:[UnittestRoot repeatedInt32Extension]] count],
      (NSUInteger)0);

  // Clearing an unset extension field shouldn't make the target message
  // visible.
  GPBMessage *message3 = [TestAllExtensions message];
  GPBMessage *extension_msg =
      [message3 getExtension:[UnittestObjcRoot recursiveExtension]];
  XCTAssertFalse([message3 hasExtension:[UnittestObjcRoot recursiveExtension]]);
  [extension_msg clearExtension:[UnittestRoot optionalInt32Extension]];
  XCTAssertFalse([message3 hasExtension:[UnittestObjcRoot recursiveExtension]]);
}

- (void)testDefaultingSubMessages {
  TestAllTypes *message = [TestAllTypes message];

  // Initially they should all not have values.

  XCTAssertFalse(message.hasOptionalGroup);
  XCTAssertFalse(message.hasOptionalNestedMessage);
  XCTAssertFalse(message.hasOptionalForeignMessage);
  XCTAssertFalse(message.hasOptionalImportMessage);
  XCTAssertFalse(message.hasOptionalPublicImportMessage);
  XCTAssertFalse(message.hasOptionalLazyMessage);

  // They should auto create something when fetched.

  TestAllTypes_OptionalGroup *optionalGroup = [message.optionalGroup retain];
  TestAllTypes_NestedMessage *optionalNestedMessage =
      [message.optionalNestedMessage retain];
  ForeignMessage *optionalForeignMessage =
      [message.optionalForeignMessage retain];
  ImportMessage *optionalImportMessage = [message.optionalImportMessage retain];
  PublicImportMessage *optionalPublicImportMessage =
      [message.optionalPublicImportMessage retain];
  TestAllTypes_NestedMessage *optionalLazyMessage =
      [message.optionalLazyMessage retain];

  XCTAssertNotNil(optionalGroup);
  XCTAssertNotNil(optionalNestedMessage);
  XCTAssertNotNil(optionalForeignMessage);
  XCTAssertNotNil(optionalImportMessage);
  XCTAssertNotNil(optionalPublicImportMessage);
  XCTAssertNotNil(optionalLazyMessage);

  // Although they were created, they should not respond to hasValue until that
  // submessage is mutated.

  XCTAssertFalse(message.hasOptionalGroup);
  XCTAssertFalse(message.hasOptionalNestedMessage);
  XCTAssertFalse(message.hasOptionalForeignMessage);
  XCTAssertFalse(message.hasOptionalImportMessage);
  XCTAssertFalse(message.hasOptionalPublicImportMessage);
  XCTAssertFalse(message.hasOptionalLazyMessage);

  // And they set that value back in to the message since the value created was
  // mutable (so a second fetch should give the same object).

  XCTAssertEqual(message.optionalGroup, optionalGroup);
  XCTAssertEqual(message.optionalNestedMessage, optionalNestedMessage);
  XCTAssertEqual(message.optionalForeignMessage, optionalForeignMessage);
  XCTAssertEqual(message.optionalImportMessage, optionalImportMessage);
  XCTAssertEqual(message.optionalPublicImportMessage,
                 optionalPublicImportMessage);
  XCTAssertEqual(message.optionalLazyMessage, optionalLazyMessage);

  // And the default objects for a second message should be distinct (again,
  // since they are mutable, each needs their own copy).

  TestAllTypes *message2 = [TestAllTypes message];

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(message2.optionalGroup, optionalGroup);
  XCTAssertNotEqual(message2.optionalNestedMessage, optionalNestedMessage);
  XCTAssertNotEqual(message2.optionalForeignMessage, optionalForeignMessage);
  XCTAssertNotEqual(message2.optionalImportMessage, optionalImportMessage);
  XCTAssertNotEqual(message2.optionalPublicImportMessage,
                    optionalPublicImportMessage);
  XCTAssertNotEqual(message2.optionalLazyMessage, optionalLazyMessage);

  // Setting the values to nil will clear the has flag, and on next access you
  // get back new submessages.

  message.optionalGroup = nil;
  message.optionalNestedMessage = nil;
  message.optionalForeignMessage = nil;
  message.optionalImportMessage = nil;
  message.optionalPublicImportMessage = nil;
  message.optionalLazyMessage = nil;

  XCTAssertFalse(message.hasOptionalGroup);
  XCTAssertFalse(message.hasOptionalNestedMessage);
  XCTAssertFalse(message.hasOptionalForeignMessage);
  XCTAssertFalse(message.hasOptionalImportMessage);
  XCTAssertFalse(message.hasOptionalPublicImportMessage);
  XCTAssertFalse(message.hasOptionalLazyMessage);

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(message.optionalGroup, optionalGroup);
  XCTAssertNotEqual(message.optionalNestedMessage, optionalNestedMessage);
  XCTAssertNotEqual(message.optionalForeignMessage, optionalForeignMessage);
  XCTAssertNotEqual(message.optionalImportMessage, optionalImportMessage);
  XCTAssertNotEqual(message.optionalPublicImportMessage,
                    optionalPublicImportMessage);
  XCTAssertNotEqual(message.optionalLazyMessage, optionalLazyMessage);

  [optionalGroup release];
  [optionalNestedMessage release];
  [optionalForeignMessage release];
  [optionalImportMessage release];
  [optionalPublicImportMessage release];
  [optionalLazyMessage release];
}

- (void)testMultiplePointersToAutocreatedMessage {
  // Multiple objects pointing to the same autocreated message.
  TestAllTypes *message = [TestAllTypes message];
  TestAllTypes *message2 = [TestAllTypes message];
  message2.optionalGroup = message.optionalGroup;
  XCTAssertTrue([message2 hasOptionalGroup]);
  XCTAssertFalse([message hasOptionalGroup]);
  message2.optionalGroup.a = 42;
  XCTAssertTrue([message hasOptionalGroup]);
  XCTAssertTrue([message2 hasOptionalGroup]);
}

- (void)testCopyWithAutocreatedMessage {
  // Mutable copy should not copy autocreated messages.
  TestAllTypes *message = [TestAllTypes message];
  message.optionalGroup.a = 42;
  XCTAssertNotNil(message.optionalNestedMessage);
  TestAllTypes *message2 = [[message copy] autorelease];
  XCTAssertTrue([message2 hasOptionalGroup]);
  XCTAssertFalse([message2 hasOptionalNestedMessage]);

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(message.optionalNestedMessage,
                    message2.optionalNestedMessage);
}

- (void)testClearAutocreatedSubmessage {
  // Call clear on an intermediate submessage should cause it to get recreated
  // on the next call.
  TestRecursiveMessage *message = [TestRecursiveMessage message];
  TestRecursiveMessage *message_inner = [message.a.a.a retain];
  XCTAssertNotNil(message_inner);
  XCTAssertTrue(GPBWasMessageAutocreatedBy(message_inner, message.a.a));
  [message.a.a clear];
  XCTAssertFalse(GPBWasMessageAutocreatedBy(message_inner, message.a.a));

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(message.a.a.a, message_inner);
  [message_inner release];
}

- (void)testRetainAutocreatedSubmessage {
  // Should be able to retain autocreated submessage while the creator is
  // dealloced.
  TestAllTypes *message = [TestAllTypes message];

  ForeignMessage *subMessage;
  @autoreleasepool {
    TestAllTypes *message2 = [TestAllTypes message];
    subMessage = message2.optionalForeignMessage; // Autocreated
    message.optionalForeignMessage = subMessage;
    XCTAssertTrue(GPBWasMessageAutocreatedBy(message.optionalForeignMessage,
                                             message2));
  }

  // Should be the same object, and should still be live.
  XCTAssertEqual(message.optionalForeignMessage, subMessage);
  XCTAssertNotNil([subMessage description]);
}

- (void)testSetNilAutocreatedSubmessage {
  TestRecursiveMessage *message = [TestRecursiveMessage message];
  TestRecursiveMessage *message_inner = [message.a.a retain];
  XCTAssertFalse([message hasA]);
  XCTAssertFalse([message.a hasA]);
  message.a.a = nil;

  // |message.a| has to be made visible, but |message.a.a| was set to nil so
  // shouldn't be.
  XCTAssertTrue([message hasA]);
  XCTAssertFalse([message.a hasA]);

  // Setting submessage to nil should cause it to lose its creator.
  XCTAssertFalse(GPBWasMessageAutocreatedBy(message_inner, message.a));

  // After setting to nil, getting it again should create a new autocreated
  // message.
  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(message.a.a, message_inner);

  [message_inner release];
}

- (void)testSetDoesntHaveAutocreatedSubmessage {
  // Clearing submessage (set has == NO) should NOT cause it to lose its
  // creator.
  TestAllTypes *message = [TestAllTypes message];
  TestAllTypes_NestedMessage *nestedMessage = message.optionalNestedMessage;
  XCTAssertFalse([message hasOptionalNestedMessage]);
  [message setHasOptionalNestedMessage:NO];
  XCTAssertFalse([message hasOptionalNestedMessage]);
  XCTAssertEqual(message.optionalNestedMessage, nestedMessage);
}

- (void)testSetAutocreatedMessageBecomesVisible {
  // Setting a value should cause the submessage to appear to its creator.
  // Test this several levels deep.
  TestRecursiveMessage *message = [TestRecursiveMessage message];
  message.a.a.a.a.i = 42;
  XCTAssertTrue([message hasA]);
  XCTAssertTrue([message.a hasA]);
  XCTAssertTrue([message.a.a hasA]);
  XCTAssertTrue([message.a.a.a hasA]);
  XCTAssertFalse([message.a.a.a.a hasA]);
  XCTAssertEqual(message.a.a.a.a.i, 42);
}

- (void)testClearUnsetFieldOfAutocreatedMessage {
  // Clearing an unset field should not cause the submessage to appear to its
  // creator.
  TestRecursiveMessage *message = [TestRecursiveMessage message];
  message.a.a.a.a.hasI = NO;
  XCTAssertFalse([message hasA]);
  XCTAssertFalse([message.a hasA]);
  XCTAssertFalse([message.a.a hasA]);
  XCTAssertFalse([message.a.a.a hasA]);
}

- (void)testAutocreatedSubmessageAssignSkip {
  TestRecursiveMessage *message = [TestRecursiveMessage message];
  TestRecursiveMessage *messageLevel1 = [message.a retain];
  TestRecursiveMessage *messageLevel2 = [message.a.a retain];
  TestRecursiveMessage *messageLevel3 = [message.a.a.a retain];
  TestRecursiveMessage *messageLevel4 = [message.a.a.a.a retain];
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel1, message));
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel2, messageLevel1));
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel3, messageLevel2));
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel4, messageLevel3));

  // Test skipping over an autocreated submessage and ensure it gets unset.
  message.a = message.a.a;
  XCTAssertEqual(message.a, messageLevel2);
  XCTAssertTrue([message hasA]);
  XCTAssertEqual(message.a.a, messageLevel3);
  XCTAssertFalse([message.a hasA]);
  XCTAssertEqual(message.a.a.a, messageLevel4);
  XCTAssertFalse(GPBWasMessageAutocreatedBy(messageLevel1,
                                            message));  // Because it was orphaned.
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel2, messageLevel1));
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel3, messageLevel2));

  [messageLevel1 release];
  [messageLevel2 release];
  [messageLevel3 release];
  [messageLevel4 release];
}

- (void)testAutocreatedSubmessageAssignLoop {
  TestRecursiveMessage *message = [TestRecursiveMessage message];
  TestRecursiveMessage *messageLevel1 = [message.a retain];
  TestRecursiveMessage *messageLevel2 = [message.a.a retain];
  TestRecursiveMessage *messageLevel3 = [message.a.a.a retain];
  TestRecursiveMessage *messageLevel4 = [message.a.a.a.a retain];
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel1, message));
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel2, messageLevel1));
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel3, messageLevel2));
  XCTAssertTrue(GPBWasMessageAutocreatedBy(messageLevel4, messageLevel3));

  // Test a property with a loop. You'd never do this but at least ensure the
  // autocreated submessages behave sanely.
  message.a.a = message.a;
  XCTAssertTrue([message hasA]);
  XCTAssertEqual(message.a, messageLevel1);
  XCTAssertTrue([message.a hasA]);
  XCTAssertEqual(message.a.a, messageLevel1);
  XCTAssertTrue([message.a.a hasA]);
  XCTAssertEqual(message.a.a.a, messageLevel1);
  XCTAssertFalse(GPBWasMessageAutocreatedBy(messageLevel1,
                                            message));  // Because it was assigned.
  XCTAssertFalse(GPBWasMessageAutocreatedBy(messageLevel2,
                                            messageLevel1));  // Because it was orphaned.
  XCTAssertFalse([messageLevel2 hasA]);

  // Break the retain loop.
  message.a.a = nil;
  XCTAssertTrue([message hasA]);
  XCTAssertFalse([message.a hasA]);

  [messageLevel1 release];
  [messageLevel2 release];
  [messageLevel3 release];
  [messageLevel4 release];
}

- (void)testSetAutocreatedSubmessage {
  // Setting autocreated submessage to another value should cause the old one to
  // lose its creator.
  TestAllTypes *message = [TestAllTypes message];
  TestAllTypes_NestedMessage *nestedMessage =
      [message.optionalNestedMessage retain];

  message.optionalNestedMessage = [TestAllTypes_NestedMessage message];
  XCTAssertTrue([message hasOptionalNestedMessage]);
  XCTAssertTrue(message.optionalNestedMessage != nestedMessage);
  XCTAssertFalse(GPBWasMessageAutocreatedBy(nestedMessage, message));

  [nestedMessage release];
}

- (void)testAutocreatedUnknownFields {
  // Doing anything with (except reading) unknown fields should cause the
  // submessage to become visible.
  TestAllTypes *message = [TestAllTypes message];
  XCTAssertNotNil(message.optionalNestedMessage);
  XCTAssertFalse([message hasOptionalNestedMessage]);
  XCTAssertNil(message.optionalNestedMessage.unknownFields);
  XCTAssertFalse([message hasOptionalNestedMessage]);

  GPBUnknownFieldSet *unknownFields =
      [[[GPBUnknownFieldSet alloc] init] autorelease];
  message.optionalNestedMessage.unknownFields = unknownFields;
  XCTAssertTrue([message hasOptionalNestedMessage]);

  message.optionalNestedMessage = nil;
  XCTAssertFalse([message hasOptionalNestedMessage]);
  [message.optionalNestedMessage setUnknownFields:unknownFields];
  XCTAssertTrue([message hasOptionalNestedMessage]);
}

- (void)testSetAutocreatedSubmessageToSelf {
  // Setting submessage to itself should cause it to become visible.
  TestAllTypes *message = [TestAllTypes message];
  XCTAssertNotNil(message.optionalNestedMessage);
  XCTAssertFalse([message hasOptionalNestedMessage]);
  message.optionalNestedMessage = message.optionalNestedMessage;
  XCTAssertTrue([message hasOptionalNestedMessage]);
}

- (void)testAutocreatedSubmessageMemoryLeaks {
  // Test for memory leaks with autocreated submessages.
  TestRecursiveMessage *message;
  TestRecursiveMessage *messageLevel1;
  TestRecursiveMessage *messageLevel2;
  TestRecursiveMessage *messageLevel3;
  TestRecursiveMessage *messageLevel4;
  @autoreleasepool {
    message = [[TestRecursiveMessage alloc] init];
    messageLevel1 = [message.a retain];
    messageLevel2 = [message.a.a retain];
    messageLevel3 = [message.a.a.a retain];
    messageLevel4 = [message.a.a.a.a retain];
    message.a.i = 1;
  }

  XCTAssertEqual(message.retainCount, (NSUInteger)1);
  [message release];
  XCTAssertEqual(messageLevel1.retainCount, (NSUInteger)1);
  [messageLevel1 release];
  XCTAssertEqual(messageLevel2.retainCount, (NSUInteger)1);
  [messageLevel2 release];
  XCTAssertEqual(messageLevel3.retainCount, (NSUInteger)1);
  [messageLevel3 release];
  XCTAssertEqual(messageLevel4.retainCount, (NSUInteger)1);
  [messageLevel4 release];
}

- (void)testDefaultingArrays {
  // Basic tests for default creation of arrays in a message.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];
  TestRecursiveMessageWithRepeatedField *message2 =
      [TestRecursiveMessageWithRepeatedField message];

  // Simply accessing the array should not make any fields visible.
  XCTAssertNotNil(message.a.a.iArray);
  XCTAssertFalse([message hasA]);
  XCTAssertFalse([message.a hasA]);
  XCTAssertNotNil(message2.a.a.strArray);
  XCTAssertFalse([message2 hasA]);
  XCTAssertFalse([message2.a hasA]);

  // But adding an element to the array should.
  [message.a.a.iArray addValue:42];
  XCTAssertTrue([message hasA]);
  XCTAssertTrue([message.a hasA]);
  XCTAssertEqual([message.a.a.iArray count], (NSUInteger)1);
  [message2.a.a.strArray addObject:@"foo"];
  XCTAssertTrue([message2 hasA]);
  XCTAssertTrue([message2.a hasA]);
  XCTAssertEqual([message2.a.a.strArray count], (NSUInteger)1);
}

- (void)testAutocreatedArrayShared {
  // Multiple objects pointing to the same array.
  TestRecursiveMessageWithRepeatedField *message1a =
      [TestRecursiveMessageWithRepeatedField message];
  TestRecursiveMessageWithRepeatedField *message1b =
      [TestRecursiveMessageWithRepeatedField message];
  message1a.a.iArray = message1b.a.iArray;
  XCTAssertTrue([message1a hasA]);
  XCTAssertFalse([message1b hasA]);
  [message1a.a.iArray addValue:1];
  XCTAssertTrue([message1a hasA]);
  XCTAssertTrue([message1b hasA]);
  XCTAssertEqual(message1a.a.iArray, message1b.a.iArray);

  TestRecursiveMessageWithRepeatedField *message2a =
      [TestRecursiveMessageWithRepeatedField message];
  TestRecursiveMessageWithRepeatedField *message2b =
      [TestRecursiveMessageWithRepeatedField message];
  message2a.a.strArray = message2b.a.strArray;
  XCTAssertTrue([message2a hasA]);
  XCTAssertFalse([message2b hasA]);
  [message2a.a.strArray addObject:@"bar"];
  XCTAssertTrue([message2a hasA]);
  XCTAssertTrue([message2b hasA]);
  XCTAssertEqual(message2a.a.strArray, message2b.a.strArray);
}

- (void)testAutocreatedArrayCopy {
  // Copy should not copy autocreated arrays.
  TestAllTypes *message = [TestAllTypes message];
  XCTAssertNotNil(message.repeatedStringArray);
  XCTAssertNotNil(message.repeatedInt32Array);
  TestAllTypes *message2 = [[message copy] autorelease];
  // Pointer conparisions.
  XCTAssertNotEqual(message.repeatedStringArray, message2.repeatedStringArray);
  XCTAssertNotEqual(message.repeatedInt32Array, message2.repeatedInt32Array);

  // Mutable copy should copy empty arrays that were explicitly set (end up
  // with different objects that are equal).
  TestAllTypes *message3 = [TestAllTypes message];
  message3.repeatedInt32Array = [GPBInt32Array arrayWithValue:42];
  message3.repeatedStringArray = [NSMutableArray arrayWithObject:@"wee"];
  XCTAssertNotNil(message.repeatedInt32Array);
  XCTAssertNotNil(message.repeatedStringArray);
  TestAllTypes *message4 = [[message3 copy] autorelease];
  XCTAssertNotEqual(message3.repeatedInt32Array, message4.repeatedInt32Array);
  XCTAssertEqualObjects(message3.repeatedInt32Array,
                        message4.repeatedInt32Array);
  XCTAssertNotEqual(message3.repeatedStringArray, message4.repeatedStringArray);
  XCTAssertEqualObjects(message3.repeatedStringArray,
                        message4.repeatedStringArray);
}

- (void)testAutocreatedArrayRetain {
  // Should be able to retain autocreated array while the creator is dealloced.
  TestAllTypes *message = [TestAllTypes message];

  @autoreleasepool {
    TestAllTypes *message2 = [TestAllTypes message];
    message.repeatedInt32Array = message2.repeatedInt32Array;
    message.repeatedStringArray = message2.repeatedStringArray;
    // Pointer conparision
    XCTAssertEqual(message.repeatedInt32Array->_autocreator, message2);
    XCTAssertTrue([message.repeatedStringArray
        isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(
        ((GPBAutocreatedArray *)message.repeatedStringArray)->_autocreator,
        message2);
  }

  XCTAssertNil(message.repeatedInt32Array->_autocreator);
  XCTAssertTrue(
      [message.repeatedStringArray isKindOfClass:[GPBAutocreatedArray class]]);
  XCTAssertNil(
      ((GPBAutocreatedArray *)message.repeatedStringArray)->_autocreator);
}

- (void)testSetNilAutocreatedArray {
  // Setting array to nil should cause it to lose its delegate.
  TestAllTypes *message = [TestAllTypes message];
  GPBInt32Array *repeatedInt32Array = [message.repeatedInt32Array retain];
  GPBAutocreatedArray *repeatedStringArray =
      (GPBAutocreatedArray *)[message.repeatedStringArray retain];
  XCTAssertTrue([repeatedStringArray isKindOfClass:[GPBAutocreatedArray class]]);
  XCTAssertEqual(repeatedInt32Array->_autocreator, message);
  XCTAssertEqual(repeatedStringArray->_autocreator, message);
  message.repeatedInt32Array = nil;
  message.repeatedStringArray = nil;
  XCTAssertNil(repeatedInt32Array->_autocreator);
  XCTAssertNil(repeatedStringArray->_autocreator);
  [repeatedInt32Array release];
  [repeatedStringArray release];
}

- (void)testSetOverAutocreatedArrayAndSetAgain {
  // Ensure when dealing with replacing an array it is handled being either
  // an autocreated one or a straight NSArray.

  // The real test here is that nothing crashes while doing the work.
  TestAllTypes *message = [TestAllTypes message];
  [message.repeatedStringArray addObject:@"foo"];
  XCTAssertEqual(message.repeatedStringArray_Count, (NSUInteger)1);
  message.repeatedStringArray = [NSMutableArray arrayWithObjects:@"bar", @"bar2", nil];
  XCTAssertEqual(message.repeatedStringArray_Count, (NSUInteger)2);
  message.repeatedStringArray = [NSMutableArray arrayWithObject:@"baz"];
  XCTAssertEqual(message.repeatedStringArray_Count, (NSUInteger)1);
}

- (void)testReplaceAutocreatedArray {
  // Replacing array should orphan the old one and cause its creator to become
  // visible.
  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.iArray);
    XCTAssertFalse([message hasA]);
    GPBInt32Array *iArray = [message.a.iArray retain];
    XCTAssertEqual(iArray->_autocreator, message.a);  // Pointer comparision
    message.a.iArray = [GPBInt32Array arrayWithValue:1];
    XCTAssertTrue([message hasA]);
    XCTAssertNotEqual(message.a.iArray, iArray);  // Pointer comparision
    XCTAssertNil(iArray->_autocreator);
    [iArray release];
  }

  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.strArray);
    XCTAssertFalse([message hasA]);
    GPBAutocreatedArray *strArray =
        (GPBAutocreatedArray *)[message.a.strArray retain];
    XCTAssertTrue([strArray isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertEqual(strArray->_autocreator, message.a);  // Pointer comparision
    message.a.strArray = [NSMutableArray arrayWithObject:@"foo"];
    XCTAssertTrue([message hasA]);
    XCTAssertNotEqual(message.a.strArray, strArray);  // Pointer comparision
    XCTAssertNil(strArray->_autocreator);
    [strArray release];
  }
}

- (void)testSetAutocreatedArrayToSelf {
  // Setting array to itself should cause it to become visible.
  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.iArray);
    XCTAssertFalse([message hasA]);
    message.a.iArray = message.a.iArray;
    XCTAssertTrue([message hasA]);
    XCTAssertNil(message.a.iArray->_autocreator);
  }

  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.strArray);
    XCTAssertFalse([message hasA]);
    message.a.strArray = message.a.strArray;
    XCTAssertTrue([message hasA]);
    XCTAssertTrue([message.a.strArray isKindOfClass:[GPBAutocreatedArray class]]);
    XCTAssertNil(((GPBAutocreatedArray *)message.a.strArray)->_autocreator);
  }
}

- (void)testAutocreatedArrayRemoveAllValues {
  // Calling removeAllValues on autocreated array should not cause it to be
  // visible.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];
  [message.a.iArray removeAll];
  XCTAssertFalse([message hasA]);
  [message.a.strArray removeAllObjects];
  XCTAssertFalse([message hasA]);
}

- (void)testDefaultingMaps {
  // Basic tests for default creation of maps in a message.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];
  TestRecursiveMessageWithRepeatedField *message2 =
      [TestRecursiveMessageWithRepeatedField message];

  // Simply accessing the map should not make any fields visible.
  XCTAssertNotNil(message.a.a.iToI);
  XCTAssertFalse([message hasA]);
  XCTAssertFalse([message.a hasA]);
  XCTAssertNotNil(message2.a.a.strToStr);
  XCTAssertFalse([message2 hasA]);
  XCTAssertFalse([message2.a hasA]);

  // But adding an element to the map should.
  [message.a.a.iToI setInt32:100 forKey:200];
  XCTAssertTrue([message hasA]);
  XCTAssertTrue([message.a hasA]);
  XCTAssertEqual([message.a.a.iToI count], (NSUInteger)1);
  [message2.a.a.strToStr setObject:@"foo" forKey:@"bar"];
  XCTAssertTrue([message2 hasA]);
  XCTAssertTrue([message2.a hasA]);
  XCTAssertEqual([message2.a.a.strToStr count], (NSUInteger)1);
}

- (void)testAutocreatedMapShared {
  // Multiple objects pointing to the same map.
  TestRecursiveMessageWithRepeatedField *message1a =
      [TestRecursiveMessageWithRepeatedField message];
  TestRecursiveMessageWithRepeatedField *message1b =
      [TestRecursiveMessageWithRepeatedField message];
  message1a.a.iToI = message1b.a.iToI;
  XCTAssertTrue([message1a hasA]);
  XCTAssertFalse([message1b hasA]);
  [message1a.a.iToI setInt32:1 forKey:2];
  XCTAssertTrue([message1a hasA]);
  XCTAssertTrue([message1b hasA]);
  XCTAssertEqual(message1a.a.iToI, message1b.a.iToI);

  TestRecursiveMessageWithRepeatedField *message2a =
      [TestRecursiveMessageWithRepeatedField message];
  TestRecursiveMessageWithRepeatedField *message2b =
      [TestRecursiveMessageWithRepeatedField message];
  message2a.a.strToStr = message2b.a.strToStr;
  XCTAssertTrue([message2a hasA]);
  XCTAssertFalse([message2b hasA]);
  [message2a.a.strToStr setObject:@"bar" forKey:@"foo"];
  XCTAssertTrue([message2a hasA]);
  XCTAssertTrue([message2b hasA]);
  XCTAssertEqual(message2a.a.strToStr, message2b.a.strToStr);
}

- (void)testAutocreatedMapCopy {
  // Copy should not copy autocreated maps.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];
  XCTAssertNotNil(message.strToStr);
  XCTAssertNotNil(message.iToI);
  TestRecursiveMessageWithRepeatedField *message2 =
      [[message copy] autorelease];
  // Pointer conparisions.
  XCTAssertNotEqual(message.strToStr, message2.strToStr);
  XCTAssertNotEqual(message.iToI, message2.iToI);

  // Mutable copy should copy empty arrays that were explicitly set (end up
  // with different objects that are equal).
  TestRecursiveMessageWithRepeatedField *message3 =
      [TestRecursiveMessageWithRepeatedField message];
  message3.iToI = [[[GPBInt32Int32Dictionary alloc] init] autorelease];
  [message3.iToI setInt32:10 forKey:20];
  message3.strToStr =
      [NSMutableDictionary dictionaryWithObject:@"abc" forKey:@"123"];
  XCTAssertNotNil(message.iToI);
  XCTAssertNotNil(message.iToI);
  TestRecursiveMessageWithRepeatedField *message4 =
      [[message3 copy] autorelease];
  XCTAssertNotEqual(message3.iToI, message4.iToI);
  XCTAssertEqualObjects(message3.iToI, message4.iToI);
  XCTAssertNotEqual(message3.strToStr, message4.strToStr);
  XCTAssertEqualObjects(message3.strToStr, message4.strToStr);
}

- (void)testAutocreatedMapRetain {
  // Should be able to retain autocreated map while the creator is dealloced.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];

  @autoreleasepool {
    TestRecursiveMessageWithRepeatedField *message2 =
        [TestRecursiveMessageWithRepeatedField message];
    message.iToI = message2.iToI;
    message.strToStr = message2.strToStr;
    // Pointer conparision
    XCTAssertEqual(message.iToI->_autocreator, message2);
    XCTAssertTrue([message.strToStr
        isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(
        ((GPBAutocreatedDictionary *)message.strToStr)->_autocreator,
        message2);
  }

  XCTAssertNil(message.iToI->_autocreator);
  XCTAssertTrue(
      [message.strToStr isKindOfClass:[GPBAutocreatedDictionary class]]);
  XCTAssertNil(
      ((GPBAutocreatedDictionary *)message.strToStr)->_autocreator);
}

- (void)testSetNilAutocreatedMap {
  // Setting map to nil should cause it to lose its delegate.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];
  GPBInt32Int32Dictionary *iToI = [message.iToI retain];
  GPBAutocreatedDictionary *strToStr =
      (GPBAutocreatedDictionary *)[message.strToStr retain];
  XCTAssertTrue([strToStr isKindOfClass:[GPBAutocreatedDictionary class]]);
  XCTAssertEqual(iToI->_autocreator, message);
  XCTAssertEqual(strToStr->_autocreator, message);
  message.iToI = nil;
  message.strToStr = nil;
  XCTAssertNil(iToI->_autocreator);
  XCTAssertNil(strToStr->_autocreator);
  [iToI release];
  [strToStr release];
}

- (void)testSetOverAutocreatedMapAndSetAgain {
  // Ensure when dealing with replacing a map it is handled being either
  // an autocreated one or a straight NSDictionary.

  // The real test here is that nothing crashes while doing the work.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];
  message.strToStr[@"foo"] = @"bar";
  XCTAssertEqual(message.strToStr_Count, (NSUInteger)1);
  message.strToStr =
      [NSMutableDictionary dictionaryWithObjectsAndKeys:@"bar", @"key1", @"baz", @"key2", nil];
  XCTAssertEqual(message.strToStr_Count, (NSUInteger)2);
  message.strToStr =
      [NSMutableDictionary dictionaryWithObject:@"baz" forKey:@"mumble"];
  XCTAssertEqual(message.strToStr_Count, (NSUInteger)1);
}

- (void)testReplaceAutocreatedMap {
  // Replacing map should orphan the old one and cause its creator to become
  // visible.
  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.iToI);
    XCTAssertFalse([message hasA]);
    GPBInt32Int32Dictionary *iToI = [message.a.iToI retain];
    XCTAssertEqual(iToI->_autocreator, message.a);  // Pointer comparision
    message.a.iToI = [[[GPBInt32Int32Dictionary alloc] init] autorelease];
    [message.a.iToI setInt32:6 forKey:7];
    XCTAssertTrue([message hasA]);
    XCTAssertNotEqual(message.a.iToI, iToI);  // Pointer comparision
    XCTAssertNil(iToI->_autocreator);
    [iToI release];
  }

  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.strToStr);
    XCTAssertFalse([message hasA]);
    GPBAutocreatedDictionary *strToStr =
        (GPBAutocreatedDictionary *)[message.a.strToStr retain];
    XCTAssertTrue([strToStr isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertEqual(strToStr->_autocreator, message.a);  // Pointer comparision
    message.a.strToStr =
        [NSMutableDictionary dictionaryWithObject:@"abc" forKey:@"def"];
    XCTAssertTrue([message hasA]);
    XCTAssertNotEqual(message.a.strToStr, strToStr);  // Pointer comparision
    XCTAssertNil(strToStr->_autocreator);
    [strToStr release];
  }
}

- (void)testSetAutocreatedMapToSelf {
  // Setting map to itself should cause it to become visible.
  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.iToI);
    XCTAssertFalse([message hasA]);
    message.a.iToI = message.a.iToI;
    XCTAssertTrue([message hasA]);
    XCTAssertNil(message.a.iToI->_autocreator);
  }

  {
    TestRecursiveMessageWithRepeatedField *message =
        [TestRecursiveMessageWithRepeatedField message];
    XCTAssertNotNil(message.a);
    XCTAssertNotNil(message.a.strToStr);
    XCTAssertFalse([message hasA]);
    message.a.strToStr = message.a.strToStr;
    XCTAssertTrue([message hasA]);
    XCTAssertTrue([message.a.strToStr isKindOfClass:[GPBAutocreatedDictionary class]]);
    XCTAssertNil(((GPBAutocreatedDictionary *)message.a.strToStr)->_autocreator);
  }
}

- (void)testAutocreatedMapRemoveAllValues {
  // Calling removeAll on autocreated map should not cause it to be visible.
  TestRecursiveMessageWithRepeatedField *message =
      [TestRecursiveMessageWithRepeatedField message];
  [message.a.iToI removeAll];
  XCTAssertFalse([message hasA]);
  [message.a.strToStr removeAllObjects];
  XCTAssertFalse([message hasA]);
}

- (void)testExtensionAccessors {
  TestAllExtensions *message = [TestAllExtensions message];
  [self setAllExtensions:message repeatedCount:kGPBDefaultRepeatCount];
  [self assertAllExtensionsSet:message repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testExtensionRepeatedSetters {
  TestAllExtensions *message = [TestAllExtensions message];
  [self setAllExtensions:message repeatedCount:kGPBDefaultRepeatCount];
  [self modifyRepeatedExtensions:message];
  [self assertRepeatedExtensionsModified:message
                           repeatedCount:kGPBDefaultRepeatCount];
}

- (void)testExtensionDefaults {
  [self assertExtensionsClear:[TestAllExtensions message]];
}

- (void)testExtensionIsEquals {
  TestAllExtensions *message = [TestAllExtensions message];
  [self setAllExtensions:message repeatedCount:kGPBDefaultRepeatCount];
  [self modifyRepeatedExtensions:message];
  TestAllExtensions *message2 = [TestAllExtensions message];
  [self setAllExtensions:message2 repeatedCount:kGPBDefaultRepeatCount];
  XCTAssertFalse([message isEqual:message2]);
  message2 = [TestAllExtensions message];
  [self setAllExtensions:message2 repeatedCount:kGPBDefaultRepeatCount];
  [self modifyRepeatedExtensions:message2];
  XCTAssertEqualObjects(message, message2);
}

- (void)testExtensionsMergeFrom {
  TestAllExtensions *message = [TestAllExtensions message];
  [self setAllExtensions:message repeatedCount:kGPBDefaultRepeatCount];
  [self modifyRepeatedExtensions:message];

  message = [TestAllExtensions message];
  [self setAllExtensions:message repeatedCount:kGPBDefaultRepeatCount];
  TestAllExtensions *message2 = [TestAllExtensions message];
  [self modifyRepeatedExtensions:message2];
  [message2 mergeFrom:message];

  XCTAssertEqualObjects(message, message2);
}

- (void)testDefaultingExtensionMessages {
  TestAllExtensions *message = [TestAllExtensions message];

  // Initially they should all not have values.

  XCTAssertFalse([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalForeignMessageExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalImportMessageExtension]]);
  XCTAssertFalse([message
      hasExtension:[UnittestRoot optionalPublicImportMessageExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalLazyMessageExtension]]);

  // They should auto create something when fetched.

  TestAllTypes_OptionalGroup *optionalGroup =
      [message getExtension:[UnittestRoot optionalGroupExtension]];
  TestAllTypes_NestedMessage *optionalNestedMessage =
      [message getExtension:[UnittestRoot optionalNestedMessageExtension]];
  ForeignMessage *optionalForeignMessage =
      [message getExtension:[UnittestRoot optionalForeignMessageExtension]];
  ImportMessage *optionalImportMessage =
      [message getExtension:[UnittestRoot optionalImportMessageExtension]];
  PublicImportMessage *optionalPublicImportMessage = [message
      getExtension:[UnittestRoot optionalPublicImportMessageExtension]];
  TestAllTypes_NestedMessage *optionalLazyMessage =
      [message getExtension:[UnittestRoot optionalLazyMessageExtension]];

  XCTAssertNotNil(optionalGroup);
  XCTAssertNotNil(optionalNestedMessage);
  XCTAssertNotNil(optionalForeignMessage);
  XCTAssertNotNil(optionalImportMessage);
  XCTAssertNotNil(optionalPublicImportMessage);
  XCTAssertNotNil(optionalLazyMessage);

  // Although it auto-created empty messages, it should not show that it has
  // them.

  XCTAssertFalse([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalForeignMessageExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalImportMessageExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalPublicImportMessageExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalLazyMessageExtension]]);

  // And they set that value back in to the message since the value created was
  // mutable (so a second fetch should give the same object).

  XCTAssertEqual([message getExtension:[UnittestRoot optionalGroupExtension]],
                 optionalGroup);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalNestedMessageExtension]],
      optionalNestedMessage);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalForeignMessageExtension]],
      optionalForeignMessage);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalImportMessageExtension]],
      optionalImportMessage);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalPublicImportMessageExtension]],
      optionalPublicImportMessage);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalLazyMessageExtension]],
      optionalLazyMessage);

  // And the default objects for a second message should be distinct (again,
  // since they are mutable, each needs their own copy).

  TestAllExtensions *message2 = [TestAllExtensions message];

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(
      [message2 getExtension:[UnittestRoot optionalGroupExtension]],
      optionalGroup);
  XCTAssertNotEqual(
      [message2 getExtension:[UnittestRoot optionalNestedMessageExtension]],
      optionalNestedMessage);
  XCTAssertNotEqual(
      [message2 getExtension:[UnittestRoot optionalForeignMessageExtension]],
      optionalForeignMessage);
  XCTAssertNotEqual(
      [message2 getExtension:[UnittestRoot optionalImportMessageExtension]],
      optionalImportMessage);
  XCTAssertNotEqual(
      [message2 getExtension:[UnittestRoot optionalPublicImportMessageExtension]],
      optionalPublicImportMessage);
  XCTAssertNotEqual(
      [message2 getExtension:[UnittestRoot optionalLazyMessageExtension]],
      optionalLazyMessage);

  // Clear values, and on next access you get back new submessages.

  [message setExtension:[UnittestRoot optionalGroupExtension] value:nil];
  [message setExtension:[UnittestRoot optionalGroupExtension] value:nil];
  [message setExtension:[UnittestRoot optionalNestedMessageExtension]
                  value:nil];
  [message setExtension:[UnittestRoot optionalForeignMessageExtension]
                  value:nil];
  [message setExtension:[UnittestRoot optionalImportMessageExtension]
                  value:nil];
  [message setExtension:[UnittestRoot optionalPublicImportMessageExtension]
                  value:nil];
  [message setExtension:[UnittestRoot optionalLazyMessageExtension] value:nil];

  XCTAssertFalse([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertFalse([message hasExtension:[UnittestRoot optionalGroupExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalNestedMessageExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalForeignMessageExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalImportMessageExtension]]);
  XCTAssertFalse([message
      hasExtension:[UnittestRoot optionalPublicImportMessageExtension]]);
  XCTAssertFalse(
      [message hasExtension:[UnittestRoot optionalLazyMessageExtension]]);

  XCTAssertEqual([message getExtension:[UnittestRoot optionalGroupExtension]],
                 optionalGroup);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalNestedMessageExtension]],
      optionalNestedMessage);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalForeignMessageExtension]],
      optionalForeignMessage);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalImportMessageExtension]],
      optionalImportMessage);
  XCTAssertEqual(
      [message
          getExtension:[UnittestRoot optionalPublicImportMessageExtension]],
      optionalPublicImportMessage);
  XCTAssertEqual(
      [message getExtension:[UnittestRoot optionalLazyMessageExtension]],
      optionalLazyMessage);
}

- (void)testMultiplePointersToAutocreatedExtension {
  // 2 objects point to the same auto-created extension. One should "has" it.
  // The other should not.
  TestAllExtensions *message = [TestAllExtensions message];
  TestAllExtensions *message2 = [TestAllExtensions message];
  GPBExtensionDescriptor *extension = [UnittestRoot optionalGroupExtension];
  [message setExtension:extension value:[message2 getExtension:extension]];
  XCTAssertEqual([message getExtension:extension],
                 [message2 getExtension:extension]);
  XCTAssertFalse([message2 hasExtension:extension]);
  XCTAssertTrue([message hasExtension:extension]);

  TestAllTypes_OptionalGroup *extensionValue =
      [message2 getExtension:extension];
  extensionValue.a = 1;
  XCTAssertTrue([message2 hasExtension:extension]);
  XCTAssertTrue([message hasExtension:extension]);
}

- (void)testCopyWithAutocreatedExtension {
  // Mutable copy shouldn't copy autocreated extensions.
  TestAllExtensions *message = [TestAllExtensions message];
  GPBExtensionDescriptor *optionalGroupExtension =
      [UnittestRoot optionalGroupExtension];
  GPBExtensionDescriptor *optionalNestedMessageExtesion =
      [UnittestRoot optionalNestedMessageExtension];
  TestAllTypes_OptionalGroup *optionalGroup =
      [message getExtension:optionalGroupExtension];
  optionalGroup.a = 42;
  XCTAssertNotNil(optionalGroup);
  XCTAssertNotNil([message getExtension:optionalNestedMessageExtesion]);
  XCTAssertTrue([message hasExtension:optionalGroupExtension]);
  XCTAssertFalse([message hasExtension:optionalNestedMessageExtesion]);

  TestAllExtensions *message2 = [[message copy] autorelease];

  // message2 should end up with its own copy of the optional group.
  XCTAssertTrue([message2 hasExtension:optionalGroupExtension]);
  XCTAssertEqualObjects([message getExtension:optionalGroupExtension],
                        [message2 getExtension:optionalGroupExtension]);
  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual([message getExtension:optionalGroupExtension],
                    [message2 getExtension:optionalGroupExtension]);

  XCTAssertFalse([message2 hasExtension:optionalNestedMessageExtesion]);
  // Intentionally doing a pointer comparison (auto creation should be
  // different)
  XCTAssertNotEqual([message getExtension:optionalNestedMessageExtesion],
                    [message2 getExtension:optionalNestedMessageExtesion]);
}

- (void)testClearMessageAutocreatedExtension {
  // Call clear should cause it to recreate its autocreated extensions.
  TestAllExtensions *message = [TestAllExtensions message];
  GPBExtensionDescriptor *optionalGroupExtension =
      [UnittestRoot optionalGroupExtension];
  TestAllTypes_OptionalGroup *optionalGroup =
      [[message getExtension:optionalGroupExtension] retain];
  [message clear];
  TestAllTypes_OptionalGroup *optionalGroupNew =
      [message getExtension:optionalGroupExtension];

  // Intentionally doing a pointer comparison.
  XCTAssertNotEqual(optionalGroup, optionalGroupNew);
  [optionalGroup release];
}

- (void)testRetainAutocreatedExtension {
  // Should be able to retain autocreated extension while the creator is
  // dealloced.
  TestAllExtensions *message = [TestAllExtensions message];
  GPBExtensionDescriptor *optionalGroupExtension =
      [UnittestRoot optionalGroupExtension];

  @autoreleasepool {
    TestAllExtensions *message2 = [TestAllExtensions message];
    [message setExtension:optionalGroupExtension
                    value:[message2 getExtension:optionalGroupExtension]];
    XCTAssertTrue(GPBWasMessageAutocreatedBy(
        [message getExtension:optionalGroupExtension], message2));
  }

  XCTAssertFalse(GPBWasMessageAutocreatedBy(
      [message getExtension:optionalGroupExtension], message));
}

- (void)testClearAutocreatedExtension {
  // Clearing autocreated extension should NOT cause it to lose its creator.
  TestAllExtensions *message = [TestAllExtensions message];
  GPBExtensionDescriptor *optionalGroupExtension =
      [UnittestRoot optionalGroupExtension];
  TestAllTypes_OptionalGroup *optionalGroup =
      [[message getExtension:optionalGroupExtension] retain];
  [message clearExtension:optionalGroupExtension];
  TestAllTypes_OptionalGroup *optionalGroupNew =
      [message getExtension:optionalGroupExtension];
  XCTAssertEqual(optionalGroup, optionalGroupNew);
  XCTAssertFalse([message hasExtension:optionalGroupExtension]);
  [optionalGroup release];

  // Clearing autocreated extension should not cause its creator to become
  // visible
  GPBExtensionDescriptor *recursiveExtension =
      [UnittestObjcRoot recursiveExtension];
  TestAllExtensions *message_lvl2 = [message getExtension:recursiveExtension];
  TestAllExtensions *message_lvl3 =
      [message_lvl2 getExtension:recursiveExtension];
  [message_lvl3 clearExtension:recursiveExtension];
  XCTAssertFalse([message hasExtension:recursiveExtension]);
}

- (void)testSetAutocreatedExtensionBecomesVisible {
  // Setting an extension should cause the extension to appear to its creator.
  // Test this several levels deep.
  TestAllExtensions *message = [TestAllExtensions message];
  GPBExtensionDescriptor *recursiveExtension =
      [UnittestObjcRoot recursiveExtension];
  TestAllExtensions *message_lvl2 = [message getExtension:recursiveExtension];
  TestAllExtensions *message_lvl3 =
      [message_lvl2 getExtension:recursiveExtension];
  TestAllExtensions *message_lvl4 =
      [message_lvl3 getExtension:recursiveExtension];
  XCTAssertFalse([message hasExtension:recursiveExtension]);
  XCTAssertFalse([message_lvl2 hasExtension:recursiveExtension]);
  XCTAssertFalse([message_lvl3 hasExtension:recursiveExtension]);
  XCTAssertFalse([message_lvl4 hasExtension:recursiveExtension]);
  [message_lvl4 setExtension:[UnittestRoot optionalInt32Extension] value:@(1)];
  XCTAssertTrue([message hasExtension:recursiveExtension]);
  XCTAssertTrue([message_lvl2 hasExtension:recursiveExtension]);
  XCTAssertTrue([message_lvl3 hasExtension:recursiveExtension]);
  XCTAssertFalse([message_lvl4 hasExtension:recursiveExtension]);
  XCTAssertFalse(GPBWasMessageAutocreatedBy(message_lvl4, message_lvl3));
  XCTAssertFalse(GPBWasMessageAutocreatedBy(message_lvl3, message_lvl2));
  XCTAssertFalse(GPBWasMessageAutocreatedBy(message_lvl2, message));
}

- (void)testSetAutocreatedExtensionToSelf {
  // Setting extension to itself should cause it to become visible.
  TestAllExtensions *message = [TestAllExtensions message];
  GPBExtensionDescriptor *optionalGroupExtension =
      [UnittestRoot optionalGroupExtension];
  XCTAssertNotNil([message getExtension:optionalGroupExtension]);
  XCTAssertFalse([message hasExtension:optionalGroupExtension]);
  [message setExtension:optionalGroupExtension
                  value:[message getExtension:optionalGroupExtension]];
  XCTAssertTrue([message hasExtension:optionalGroupExtension]);
}

- (void)testAutocreatedExtensionMemoryLeaks {
  GPBExtensionDescriptor *recursiveExtension =
      [UnittestObjcRoot recursiveExtension];

  // Test for memory leaks with autocreated extensions.
  TestAllExtensions *message;
  TestAllExtensions *message_lvl2;
  TestAllExtensions *message_lvl3;
  TestAllExtensions *message_lvl4;
  @autoreleasepool {
    message = [[TestAllExtensions alloc] init];
    message_lvl2 = [[message getExtension:recursiveExtension] retain];
    message_lvl3 = [[message_lvl2 getExtension:recursiveExtension] retain];
    message_lvl4 = [[message_lvl3 getExtension:recursiveExtension] retain];
    [message_lvl2 setExtension:[UnittestRoot optionalInt32Extension]
                         value:@(1)];
  }

  XCTAssertEqual(message.retainCount, (NSUInteger)1);
  @autoreleasepool {
    [message release];
  }
  XCTAssertEqual(message_lvl2.retainCount, (NSUInteger)1);
  @autoreleasepool {
    [message_lvl2 release];
  }
  XCTAssertEqual(message_lvl3.retainCount, (NSUInteger)1);
  @autoreleasepool {
    [message_lvl3 release];
  }
  XCTAssertEqual(message_lvl4.retainCount, (NSUInteger)1);
  [message_lvl4 release];
}

- (void)testSetExtensionWithAutocreatedValue {
  GPBExtensionDescriptor *recursiveExtension =
      [UnittestObjcRoot recursiveExtension];

  TestAllExtensions *message;
  @autoreleasepool {
    message = [[TestAllExtensions alloc] init];
    [message getExtension:recursiveExtension];
  }

  // This statements checks that the extension value isn't accidentally
  // dealloced when removing it from the autocreated map.
  [message setExtension:recursiveExtension
                  value:[message getExtension:recursiveExtension]];
  XCTAssertTrue([message hasExtension:recursiveExtension]);
  [message release];
}

- (void)testRecursion {
  TestRecursiveMessage *message = [TestRecursiveMessage message];
  XCTAssertNotNil(message.a);
  XCTAssertNotNil(message.a.a);
  XCTAssertEqual(message.a.a.i, 0);
}

- (void)testGenerateAndParseUnknownMessage {
  GPBUnknownFieldSet *unknowns =
      [[[GPBUnknownFieldSet alloc] init] autorelease];
  [unknowns mergeVarintField:123 value:456];
  GPBMessage *message = [GPBMessage message];
  [message setUnknownFields:unknowns];
  NSData *data = [message data];
  GPBMessage *message2 =
      [GPBMessage parseFromData:data extensionRegistry:nil error:NULL];
  XCTAssertEqualObjects(message, message2);
}

- (void)testDelimitedWriteAndParseMultipleMessages {
  GPBUnknownFieldSet *unknowns1 =
      [[[GPBUnknownFieldSet alloc] init] autorelease];
  [unknowns1 mergeVarintField:123 value:456];
  GPBMessage *message1 = [GPBMessage message];
  [message1 setUnknownFields:unknowns1];

  GPBUnknownFieldSet *unknowns2 =
      [[[GPBUnknownFieldSet alloc] init] autorelease];
  [unknowns2 mergeVarintField:789 value:987];
  [unknowns2 mergeVarintField:654 value:321];
  GPBMessage *message2 = [GPBMessage message];
  [message2 setUnknownFields:unknowns2];

  NSMutableData *delimitedData = [NSMutableData data];
  [delimitedData appendData:[message1 delimitedData]];
  [delimitedData appendData:[message2 delimitedData]];
  GPBCodedInputStream *input =
      [GPBCodedInputStream streamWithData:delimitedData];
  GPBMessage *message3 = [GPBMessage parseDelimitedFromCodedInputStream:input
                                                      extensionRegistry:nil
                                                                  error:NULL];
  GPBMessage *message4 = [GPBMessage parseDelimitedFromCodedInputStream:input
                                                      extensionRegistry:nil
                                                                  error:NULL];
  XCTAssertEqualObjects(message1, message3);
  XCTAssertEqualObjects(message2, message4);
}

- (void)testDuplicateEnums {
  XCTAssertEqual(TestEnumWithDupValue_Foo1, TestEnumWithDupValue_Foo2);
}

- (void)testWeirdDefaults {
  ObjcWeirdDefaults *message = [ObjcWeirdDefaults message];
  GPBDescriptor *descriptor = [[message class] descriptor];
  GPBFieldDescriptor *fieldDesc = [descriptor fieldWithName:@"foo"];
  XCTAssertNotNil(fieldDesc);
  XCTAssertTrue(fieldDesc.hasDefaultValue);
  XCTAssertFalse(message.hasFoo);
  XCTAssertEqualObjects(message.foo, @"");

  fieldDesc = [descriptor fieldWithName:@"bar"];
  XCTAssertNotNil(fieldDesc);
  XCTAssertTrue(fieldDesc.hasDefaultValue);
  XCTAssertFalse(message.hasBar);
  XCTAssertEqualObjects(message.bar, GPBEmptyNSData());
}

- (void)testEnumDescriptorFromExtensionDescriptor {
  GPBExtensionDescriptor *extDescriptor =
      [UnittestRoot optionalForeignEnumExtension];
  XCTAssertEqual(extDescriptor.dataType, GPBDataTypeEnum);
  GPBEnumDescriptor *enumDescriptor = extDescriptor.enumDescriptor;
  GPBEnumDescriptor *expectedDescriptor = ForeignEnum_EnumDescriptor();
  XCTAssertEqualObjects(enumDescriptor, expectedDescriptor);
}

- (void)testPropertyNaming {
  // objectivec_helpers.cc has some special handing to get proper all caps
  // for a few cases to meet objc developer expectations.
  //
  // This "test" confirms that the expected names are generated, otherwise the
  // test itself will fail to compile.
  ObjCPropertyNaming *msg = [ObjCPropertyNaming message];
  // On their own, at the end, in the middle.
  msg.URL = @"good";
  msg.thumbnailURL = @"good";
  msg.URLFoo = @"good";
  msg.someURLBlah = @"good";
  msg.HTTP = @"good";
  msg.HTTPS = @"good";
  // No caps since it was "urls".
  [msg.urlsArray addObject:@"good"];
}

- (void)testEnumNaming {
  // objectivec_helpers.cc has some interesting cases to deal with in
  // EnumValueName/EnumValueShortName.  Confirm that things generated as
  // expected.

  // This block just has to compile to confirm we got the expected types/names.
  // The *_IsValidValue() calls are just there to keep the projects warnings
  // flags happy by providing use of the variables/values.

  Foo aFoo = Foo_SerializedSize;
  Foo_IsValidValue(aFoo);
  aFoo = Foo_Size;
  Foo_IsValidValue(aFoo);

  Category_Enum aCat = Category_Enum_Red;
  Category_Enum_IsValidValue(aCat);

  Time aTime = Time_Base;
  Time_IsValidValue(aTime);
  aTime = Time_SomethingElse;
  Time_IsValidValue(aTime);

  // This block confirms the names in the decriptors is what we wanted.

  GPBEnumDescriptor *descriptor;
  NSString *valueName;

  descriptor = Foo_EnumDescriptor();
  XCTAssertNotNil(descriptor);
  XCTAssertEqualObjects(@"Foo", descriptor.name);
  valueName = [descriptor enumNameForValue:Foo_SerializedSize];
  XCTAssertEqualObjects(@"Foo_SerializedSize", valueName);
  valueName = [descriptor enumNameForValue:Foo_Size];
  XCTAssertEqualObjects(@"Foo_Size", valueName);

  descriptor = Category_Enum_EnumDescriptor();
  XCTAssertNotNil(descriptor);
  XCTAssertEqualObjects(@"Category_Enum", descriptor.name);
  valueName = [descriptor enumNameForValue:Category_Enum_Red];
  XCTAssertEqualObjects(@"Category_Enum_Red", valueName);

  descriptor = Time_EnumDescriptor();
  XCTAssertNotNil(descriptor);
  XCTAssertEqualObjects(@"Time", descriptor.name);
  valueName = [descriptor enumNameForValue:Time_Base];
  XCTAssertEqualObjects(@"Time_Base", valueName);
  valueName = [descriptor enumNameForValue:Time_SomethingElse];
  XCTAssertEqualObjects(@"Time_SomethingElse", valueName);
}

- (void)testNegativeEnums {
  EnumTestMsg *msg = [EnumTestMsg message];

  // Defaults
  XCTAssertEqual(msg.foo, EnumTestMsg_MyEnum_Zero);
  XCTAssertEqual(msg.bar, EnumTestMsg_MyEnum_One);
  XCTAssertEqual(msg.baz, EnumTestMsg_MyEnum_NegOne);
  // Bounce to wire and back.
  NSData *data = [msg data];
  XCTAssertNotNil(data);
  EnumTestMsg *msgPrime = [EnumTestMsg parseFromData:data error:NULL];
  XCTAssertEqualObjects(msgPrime, msg);
  XCTAssertEqual(msgPrime.foo, EnumTestMsg_MyEnum_Zero);
  XCTAssertEqual(msgPrime.bar, EnumTestMsg_MyEnum_One);
  XCTAssertEqual(msgPrime.baz, EnumTestMsg_MyEnum_NegOne);

  // Other values
  msg.bar = EnumTestMsg_MyEnum_Two;
  msg.baz = EnumTestMsg_MyEnum_NegTwo;
  XCTAssertEqual(msg.bar, EnumTestMsg_MyEnum_Two);
  XCTAssertEqual(msg.baz, EnumTestMsg_MyEnum_NegTwo);
  // Bounce to wire and back.
  data = [msg data];
  XCTAssertNotNil(data);
  msgPrime = [EnumTestMsg parseFromData:data error:NULL];
  XCTAssertEqualObjects(msgPrime, msg);
  XCTAssertEqual(msgPrime.foo, EnumTestMsg_MyEnum_Zero);
  XCTAssertEqual(msgPrime.bar, EnumTestMsg_MyEnum_Two);
  XCTAssertEqual(msgPrime.baz, EnumTestMsg_MyEnum_NegTwo);

  // Repeated field (shouldn't ever be an issue since developer has to use the
  // right GPBArray methods themselves).
  msg.mumbleArray = [GPBEnumArray
      arrayWithValidationFunction:EnumTestMsg_MyEnum_IsValidValue];
  [msg.mumbleArray addValue:EnumTestMsg_MyEnum_Zero];
  [msg.mumbleArray addValue:EnumTestMsg_MyEnum_One];
  [msg.mumbleArray addValue:EnumTestMsg_MyEnum_Two];
  [msg.mumbleArray addValue:EnumTestMsg_MyEnum_NegOne];
  [msg.mumbleArray addValue:EnumTestMsg_MyEnum_NegTwo];
  XCTAssertEqual([msg.mumbleArray valueAtIndex:0], EnumTestMsg_MyEnum_Zero);
  XCTAssertEqual([msg.mumbleArray valueAtIndex:1], EnumTestMsg_MyEnum_One);
  XCTAssertEqual([msg.mumbleArray valueAtIndex:2], EnumTestMsg_MyEnum_Two);
  XCTAssertEqual([msg.mumbleArray valueAtIndex:3], EnumTestMsg_MyEnum_NegOne);
  XCTAssertEqual([msg.mumbleArray valueAtIndex:4], EnumTestMsg_MyEnum_NegTwo);
  // Bounce to wire and back.
  data = [msg data];
  XCTAssertNotNil(data);
  msgPrime = [EnumTestMsg parseFromData:data error:NULL];
  XCTAssertEqualObjects(msgPrime, msg);
  XCTAssertEqual([msgPrime.mumbleArray valueAtIndex:0],
                 EnumTestMsg_MyEnum_Zero);
  XCTAssertEqual([msgPrime.mumbleArray valueAtIndex:1], EnumTestMsg_MyEnum_One);
  XCTAssertEqual([msgPrime.mumbleArray valueAtIndex:2], EnumTestMsg_MyEnum_Two);
  XCTAssertEqual([msgPrime.mumbleArray valueAtIndex:3],
                 EnumTestMsg_MyEnum_NegOne);
  XCTAssertEqual([msgPrime.mumbleArray valueAtIndex:4],
                 EnumTestMsg_MyEnum_NegTwo);
}

- (void)testReservedWordNaming {
  // objectivec_helpers.cc has some special handing to make sure that
  // some "reserved" objc names get renamed in a way so they
  // don't conflict.
  //
  // This "test" confirms that the expected names are generated,
  // otherwise the test itself will fail to compile.
  self_Class *msg = [self_Class message];

  // Some ObjC/C/C++ keywords.
  msg.className_p = msg.hasClassName_p;
  msg.cmd = msg.hasCmd;
  msg.nullable_p = msg.hasNullable_p;
  msg.typeof_p = msg.hasTypeof_p;
  msg.instancetype_p = msg.hasInstancetype_p;
  msg.nil_p = msg.hasNil_p;
  msg.instancetype_p = msg.hasInstancetype_p;
  msg.public_p = msg.hasPublic_p;

  // Some that would override NSObject methods
  msg.camltype = msg.hasCamltype;
  msg.isNsdictionary = msg.hasIsNsdictionary;
  msg.dealloc_p = msg.hasDealloc_p;
  msg.zone_p = msg.hasZone_p;
  msg.accessibilityLabel_p = msg.hasAccessibilityLabel_p;

  // Some that we shouldn't need to handle.
  msg.atomic = msg.hasAtomic;
  msg.nonatomic = msg.hasNonatomic;
  msg.strong = msg.hasStrong;
  msg.nullResettable = msg.hasNullResettable;

  // Some that would override GPBMessage methods
  msg.clear_p = msg.hasClear_p;
  msg.data_p = msg.hasData_p;

  // Some MacTypes
  msg.fixed = msg.hasFixed;
  msg.style = msg.hasStyle;

  // Some C Identifiers
  msg.generic = msg.hasGeneric;
  msg.block = msg.hasBlock;
}

- (void)testOneBasedEnumHolder {
  // Test case for https://github.com/protocolbuffers/protobuf/issues/1453
  // Message with no explicit defaults, but a non zero default for an enum.
  MessageWithOneBasedEnum *enumMsg = [MessageWithOneBasedEnum message];
  XCTAssertEqual(enumMsg.enumField, MessageWithOneBasedEnum_OneBasedEnum_One);
}

- (void)testBoolOffsetUsage {
  // Bools use storage within has_bits; this test ensures that this is honored
  // in all places where things should crash or fail based on reading out of
  // field storage instead.
  BoolOnlyMessage *msg1 = [BoolOnlyMessage message];
  BoolOnlyMessage *msg2 = [BoolOnlyMessage message];

  msg1.boolField1 = YES;
  msg2.boolField1 = YES;
  msg1.boolField3 = YES;
  msg2.boolField3 = YES;
  msg1.boolField5 = YES;
  msg2.boolField5 = YES;
  msg1.boolField7 = YES;
  msg2.boolField7 = YES;
  msg1.boolField9 = YES;
  msg2.boolField9 = YES;
  msg1.boolField11 = YES;
  msg2.boolField11 = YES;
  msg1.boolField13 = YES;
  msg2.boolField13 = YES;
  msg1.boolField15 = YES;
  msg2.boolField15 = YES;
  msg1.boolField17 = YES;
  msg2.boolField17 = YES;
  msg1.boolField19 = YES;
  msg2.boolField19 = YES;
  msg1.boolField21 = YES;
  msg2.boolField21 = YES;
  msg1.boolField23 = YES;
  msg2.boolField23 = YES;
  msg1.boolField25 = YES;
  msg2.boolField25 = YES;
  msg1.boolField27 = YES;
  msg2.boolField27 = YES;
  msg1.boolField29 = YES;
  msg2.boolField29 = YES;
  msg1.boolField31 = YES;
  msg2.boolField31 = YES;

  msg1.boolField32 = YES;
  msg2.boolField32 = YES;

  XCTAssertTrue(msg1 != msg2); // Different pointers.
  XCTAssertEqual([msg1 hash], [msg2 hash]);
  XCTAssertEqualObjects(msg1, msg2);

  BoolOnlyMessage *msg1Prime = [[msg1 copy] autorelease];
  XCTAssertTrue(msg1Prime != msg1); // Different pointers.
  XCTAssertEqual([msg1 hash], [msg1Prime hash]);
  XCTAssertEqualObjects(msg1, msg1Prime);

  // Field set in one, but not the other means they don't match (even if
  // set to default value).
  msg1Prime.boolField2 = NO;
  XCTAssertNotEqualObjects(msg1Prime, msg1);
  // And when set to different values.
  msg1.boolField2 = YES;
  XCTAssertNotEqualObjects(msg1Prime, msg1);
  // And then they match again.
  msg1.boolField2 = NO;
  XCTAssertEqualObjects(msg1Prime, msg1);
  XCTAssertEqual([msg1 hash], [msg1Prime hash]);
}

- (void)testCopyingMapFields {
  TestMessageOfMaps *msg = [TestMessageOfMaps message];

  msg.strToStr[@"foo"] = @"bar";

  [msg.strToInt setInt32:1 forKey:@"mumble"];
  [msg.intToStr setObject:@"wee" forKey:42];
  [msg.intToInt setInt32:123 forKey:321];

  [msg.strToBool setBool:YES forKey:@"one"];
  [msg.boolToStr setObject:@"something" forKey:YES];
  [msg.boolToBool setBool:YES forKey:NO];

  [msg.intToBool setBool:YES forKey:13];
  [msg.boolToInt setInt32:111 forKey:NO];

  TestAllTypes *subMsg1 = [TestAllTypes message];
  subMsg1.optionalInt32 = 1;
  TestAllTypes *subMsg2 = [TestAllTypes message];
  subMsg1.optionalInt32 = 2;
  TestAllTypes *subMsg3 = [TestAllTypes message];
  subMsg1.optionalInt32 = 3;

  msg.strToMsg[@"baz"] = subMsg1;
  [msg.intToMsg setObject:subMsg2 forKey:222];
  [msg.boolToMsg setObject:subMsg3 forKey:YES];

  TestMessageOfMaps *msg2 = [[msg copy] autorelease];
  XCTAssertNotNil(msg2);
  XCTAssertEqualObjects(msg2, msg);
  XCTAssertTrue(msg2 != msg);  // ptr compare
  XCTAssertTrue(msg.strToStr != msg2.strToStr);  // ptr compare
  XCTAssertTrue(msg.intToStr != msg2.intToStr);  // ptr compare
  XCTAssertTrue(msg.intToInt != msg2.intToInt);  // ptr compare
  XCTAssertTrue(msg.strToBool != msg2.strToBool);  // ptr compare
  XCTAssertTrue(msg.boolToStr != msg2.boolToStr);  // ptr compare
  XCTAssertTrue(msg.boolToBool != msg2.boolToBool);  // ptr compare
  XCTAssertTrue(msg.intToBool != msg2.intToBool);  // ptr compare
  XCTAssertTrue(msg.boolToInt != msg2.boolToInt);  // ptr compare
  XCTAssertTrue(msg.strToMsg != msg2.strToMsg);  // ptr compare
  XCTAssertTrue(msg.intToMsg != msg2.intToMsg);  // ptr compare
  XCTAssertTrue(msg.boolToMsg != msg2.boolToMsg);  // ptr compare

  XCTAssertTrue(msg.strToMsg[@"baz"] != msg2.strToMsg[@"baz"]);  // ptr compare
  XCTAssertEqualObjects(msg.strToMsg[@"baz"], msg2.strToMsg[@"baz"]);
  XCTAssertTrue([msg.intToMsg objectForKey:222] != [msg2.intToMsg objectForKey:222]);  // ptr compare
  XCTAssertEqualObjects([msg.intToMsg objectForKey:222], [msg2.intToMsg objectForKey:222]);
  XCTAssertTrue([msg.boolToMsg objectForKey:YES] != [msg2.boolToMsg objectForKey:YES]);  // ptr compare
  XCTAssertEqualObjects([msg.boolToMsg objectForKey:YES], [msg2.boolToMsg objectForKey:YES]);
}

- (void)testPrefixedNames {
  // The fact that this compiles is sufficient as a test.
  // The assertions are just there to avoid "not-used" warnings.

  // Verify that enum types and values get the prefix.
  GPBTESTTestObjcProtoPrefixEnum value = GPBTESTTestObjcProtoPrefixEnum_Value;
  XCTAssertNotEqual(value, 0);

  // Verify that roots get the prefix.
  GPBTESTUnittestObjcOptionsRoot *root = nil;
  XCTAssertNil(root);

  // Verify that messages that don't already have the prefix get a prefix.
  GPBTESTTestObjcProtoPrefixMessage *prefixedMessage = nil;
  XCTAssertNil(prefixedMessage);

  // Verify that messages that already have a prefix aren't prefixed twice.
  GPBTESTTestHasAPrefixMessage *alreadyPrefixedMessage = nil;
  XCTAssertNil(alreadyPrefixedMessage);

  // Verify that enums that already have a prefix aren't prefixed twice.
  GPBTESTTestHasAPrefixEnum prefixedValue = GPBTESTTestHasAPrefixEnum_ValueB;
  XCTAssertNotEqual(prefixedValue, 0);

  // Verify that classes named the same as prefixes are prefixed.
  GPBTESTGPBTEST *prefixMessage = nil;
  XCTAssertNil(prefixMessage);

  // Verify that classes that have the prefix followed by a lowercase
  // letter DO get the prefix.
  GPBTESTGPBTESTshouldGetAPrefixMessage *shouldGetAPrefixMessage = nil;
  XCTAssertNil(shouldGetAPrefixMessage);
}

@end
