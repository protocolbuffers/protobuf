// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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
#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestImport.pbobjc.h"
#import "objectivec/Tests/UnittestObjc.pbobjc.h"

//
// This file really just uses the unittests framework as a testbed to
// run some simple performance tests. The data can then be used to help
// evaluate changes to the runtime.
//

static const uint32_t kRepeatedCount = 100;

@interface PerfTests : GPBTestCase
@end

@implementation PerfTests

- (void)setUp {
  // A convenient place to put a break point if you want to connect instruments.
  [super setUp];
}

- (void)testMessagePerformance {
  [self measureBlock:^{
    for (int i = 0; i < 200; ++i) {
      TestAllTypes* message = [[TestAllTypes alloc] init];
      [self setAllFields:message repeatedCount:kRepeatedCount];
      NSData* rawBytes = [message data];
      [message release];
      message = [[TestAllTypes alloc] initWithData:rawBytes error:NULL];
      [message release];
    }
  }];
}

- (void)testMessageSerialParsingPerformance {
  // This and the next test are meant to monitor that the parsing functionality of protos does not
  // lock across threads when parsing different instances. The Serial version of the test should run
  // around ~2 times slower than the Parallel version since it's parsing the protos in the same
  // thread.
  TestAllTypes* allTypesMessage = [TestAllTypes message];
  [self setAllFields:allTypesMessage repeatedCount:2];
  NSData* allTypesData = allTypesMessage.data;

  [self measureBlock:^{
    for (int i = 0; i < 500; ++i) {
      [TestAllTypes parseFromData:allTypesData error:NULL];
      [TestAllTypes parseFromData:allTypesData error:NULL];
    }
  }];
}

- (void)testMessageParallelParsingPerformance {
  // This and the previous test are meant to monitor that the parsing functionality of protos does
  // not lock across threads when parsing different instances. The Serial version of the test should
  // run around ~2 times slower than the Parallel version since it's parsing the protos in the same
  // thread.
  TestAllTypes* allTypesMessage = [TestAllTypes message];
  [self setAllFields:allTypesMessage repeatedCount:2];
  NSData* allTypesData = allTypesMessage.data;

  dispatch_queue_t concurrentQueue = dispatch_queue_create("perfQueue", DISPATCH_QUEUE_CONCURRENT);

  [self measureBlock:^{
    for (int i = 0; i < 500; ++i) {
      dispatch_group_t group = dispatch_group_create();

      dispatch_group_async(group, concurrentQueue, ^{
        [TestAllTypes parseFromData:allTypesData error:NULL];
      });

      dispatch_group_async(group, concurrentQueue, ^{
        [TestAllTypes parseFromData:allTypesData error:NULL];
      });

      dispatch_group_notify(group, concurrentQueue,
                            ^{
                            });

      dispatch_release(group);
    }
  }];

  dispatch_release(concurrentQueue);
}

- (void)testMessageSerialExtensionsParsingPerformance {
  // This and the next test are meant to monitor that the parsing functionality of protos does not
  // lock across threads when parsing different instances when using extensions. The Serial version
  // of the test should run around ~2 times slower than the Parallel version since it's parsing the
  // protos in the same thread.
  TestAllExtensions* allExtensionsMessage = [TestAllExtensions message];
  [self setAllExtensions:allExtensionsMessage repeatedCount:2];
  NSData* allExtensionsData = allExtensionsMessage.data;

  [self measureBlock:^{
    for (int i = 0; i < 500; ++i) {
      [TestAllExtensions parseFromData:allExtensionsData
                     extensionRegistry:[self extensionRegistry]
                                 error:NULL];
      [TestAllExtensions parseFromData:allExtensionsData
                     extensionRegistry:[self extensionRegistry]
                                 error:NULL];
    }
  }];
}

- (void)testMessageParallelExtensionsParsingPerformance {
  // This and the previous test are meant to monitor that the parsing functionality of protos does
  // not lock across threads when parsing different instances when using extensions. The Serial
  // version of the test should run around ~2 times slower than the Parallel version since it's
  // parsing the protos in the same thread.
  TestAllExtensions* allExtensionsMessage = [TestAllExtensions message];
  [self setAllExtensions:allExtensionsMessage repeatedCount:2];
  NSData* allExtensionsData = allExtensionsMessage.data;

  dispatch_queue_t concurrentQueue = dispatch_queue_create("perfQueue", DISPATCH_QUEUE_CONCURRENT);

  [self measureBlock:^{
    for (int i = 0; i < 500; ++i) {
      dispatch_group_t group = dispatch_group_create();

      dispatch_group_async(group, concurrentQueue, ^{
        [TestAllExtensions parseFromData:allExtensionsData
                       extensionRegistry:[UnittestRoot extensionRegistry]
                                   error:NULL];
      });

      dispatch_group_async(group, concurrentQueue, ^{
        [TestAllExtensions parseFromData:allExtensionsData
                       extensionRegistry:[UnittestRoot extensionRegistry]
                                   error:NULL];
      });

      dispatch_group_notify(group, concurrentQueue,
                            ^{
                            });

      dispatch_release(group);
    }
  }];

  dispatch_release(concurrentQueue);
}

- (void)testExtensionsPerformance {
  [self measureBlock:^{
    for (int i = 0; i < 200; ++i) {
      TestAllExtensions* message = [[TestAllExtensions alloc] init];
      [self setAllExtensions:message repeatedCount:kRepeatedCount];
      NSData* rawBytes = [message data];
      [message release];
      TestAllExtensions* message2 = [[TestAllExtensions alloc] initWithData:rawBytes error:NULL];
      [message2 release];
    }
  }];
}

- (void)testPackedTypesPerformance {
  [self measureBlock:^{
    for (int i = 0; i < 1000; ++i) {
      TestPackedTypes* message = [[TestPackedTypes alloc] init];
      [self setPackedFields:message repeatedCount:kRepeatedCount];
      NSData* rawBytes = [message data];
      [message release];
      message = [[TestPackedTypes alloc] initWithData:rawBytes error:NULL];
      [message release];
    }
  }];
}

- (void)testPackedExtensionsPerformance {
  [self measureBlock:^{
    for (int i = 0; i < 1000; ++i) {
      TestPackedExtensions* message = [[TestPackedExtensions alloc] init];
      [self setPackedExtensions:message repeatedCount:kRepeatedCount];
      NSData* rawBytes = [message data];
      [message release];
      TestPackedExtensions* message2 = [[TestPackedExtensions alloc] initWithData:rawBytes
                                                                            error:NULL];
      [message2 release];
    }
  }];
}

- (void)testHas {
  TestAllTypes* message = [self allSetRepeatedCount:1];
  [self measureBlock:^{
    for (int i = 0; i < 10000; ++i) {
      [message hasOptionalInt32];
      message.hasOptionalInt32 = NO;
      [message hasOptionalInt32];

      [message hasOptionalInt64];
      message.hasOptionalInt64 = NO;
      [message hasOptionalInt64];

      [message hasOptionalUint32];
      message.hasOptionalUint32 = NO;
      [message hasOptionalUint32];

      [message hasOptionalUint64];
      message.hasOptionalUint64 = NO;
      [message hasOptionalUint64];

      [message hasOptionalSint32];
      message.hasOptionalSint32 = NO;
      [message hasOptionalSint32];

      [message hasOptionalSint64];
      message.hasOptionalSint64 = NO;
      [message hasOptionalSint64];

      [message hasOptionalFixed32];
      message.hasOptionalFixed32 = NO;
      [message hasOptionalFixed32];

      [message hasOptionalFixed64];
      message.hasOptionalFixed64 = NO;
      [message hasOptionalFixed64];

      [message hasOptionalSfixed32];
      message.hasOptionalSfixed32 = NO;
      [message hasOptionalSfixed32];

      [message hasOptionalSfixed64];
      message.hasOptionalSfixed64 = NO;
      [message hasOptionalSfixed64];

      [message hasOptionalFloat];
      message.hasOptionalFloat = NO;
      [message hasOptionalFloat];

      [message hasOptionalDouble];
      message.hasOptionalDouble = NO;
      [message hasOptionalDouble];

      [message hasOptionalBool];
      message.hasOptionalBool = NO;
      [message hasOptionalBool];

      [message hasOptionalString];
      message.hasOptionalString = NO;
      [message hasOptionalString];

      [message hasOptionalBytes];
      message.hasOptionalBytes = NO;
      [message hasOptionalBytes];

      [message hasOptionalGroup];
      message.hasOptionalGroup = NO;
      [message hasOptionalGroup];

      [message hasOptionalNestedMessage];
      message.hasOptionalNestedMessage = NO;
      [message hasOptionalNestedMessage];

      [message hasOptionalForeignMessage];
      message.hasOptionalForeignMessage = NO;
      [message hasOptionalForeignMessage];

      [message hasOptionalImportMessage];
      message.hasOptionalImportMessage = NO;
      [message hasOptionalImportMessage];

      [message.optionalGroup hasA];
      message.optionalGroup.hasA = NO;
      [message.optionalGroup hasA];

      [message.optionalNestedMessage hasBb];
      message.optionalNestedMessage.hasBb = NO;
      [message.optionalNestedMessage hasBb];

      [message.optionalForeignMessage hasC];
      message.optionalForeignMessage.hasC = NO;
      [message.optionalForeignMessage hasC];

      [message.optionalImportMessage hasD];
      message.optionalImportMessage.hasD = NO;
      [message.optionalImportMessage hasD];

      [message hasOptionalNestedEnum];
      message.hasOptionalNestedEnum = NO;
      [message hasOptionalNestedEnum];

      [message hasOptionalForeignEnum];
      message.hasOptionalForeignEnum = NO;
      [message hasOptionalForeignEnum];

      [message hasOptionalImportEnum];
      message.hasOptionalImportEnum = NO;
      [message hasOptionalImportEnum];

      [message hasOptionalStringPiece];
      message.hasOptionalStringPiece = NO;
      [message hasOptionalStringPiece];

      [message hasOptionalCord];
      message.hasOptionalCord = NO;
      [message hasOptionalCord];

      [message hasDefaultInt32];
      message.hasDefaultInt32 = NO;
      [message hasDefaultInt32];

      [message hasDefaultInt64];
      message.hasDefaultInt64 = NO;
      [message hasDefaultInt64];

      [message hasDefaultUint32];
      message.hasDefaultUint32 = NO;
      [message hasDefaultUint32];

      [message hasDefaultUint64];
      message.hasDefaultUint64 = NO;
      [message hasDefaultUint64];

      [message hasDefaultSint32];
      message.hasDefaultSint32 = NO;
      [message hasDefaultSint32];

      [message hasDefaultSint64];
      message.hasDefaultSint64 = NO;
      [message hasDefaultSint64];

      [message hasDefaultFixed32];
      message.hasDefaultFixed32 = NO;
      [message hasDefaultFixed32];

      [message hasDefaultFixed64];
      message.hasDefaultFixed64 = NO;
      [message hasDefaultFixed64];

      [message hasDefaultSfixed32];
      message.hasDefaultSfixed32 = NO;
      [message hasDefaultSfixed32];

      [message hasDefaultSfixed64];
      message.hasDefaultSfixed64 = NO;
      [message hasDefaultSfixed64];

      [message hasDefaultFloat];
      message.hasDefaultFloat = NO;
      [message hasDefaultFloat];

      [message hasDefaultDouble];
      message.hasDefaultDouble = NO;
      [message hasDefaultDouble];

      [message hasDefaultBool];
      message.hasDefaultBool = NO;
      [message hasDefaultBool];

      [message hasDefaultString];
      message.hasDefaultString = NO;
      [message hasDefaultString];

      [message hasDefaultBytes];
      message.hasDefaultBytes = NO;
      [message hasDefaultBytes];

      [message hasDefaultNestedEnum];
      message.hasDefaultNestedEnum = NO;
      [message hasDefaultNestedEnum];

      [message hasDefaultForeignEnum];
      message.hasDefaultForeignEnum = NO;
      [message hasDefaultForeignEnum];

      [message hasDefaultImportEnum];
      message.hasDefaultImportEnum = NO;
      [message hasDefaultImportEnum];

      [message hasDefaultStringPiece];
      message.hasDefaultStringPiece = NO;
      [message hasDefaultStringPiece];

      [message hasDefaultCord];
      message.hasDefaultCord = NO;
      [message hasDefaultCord];
    }
  }];
}

@end
