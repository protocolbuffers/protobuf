// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

#import "GPBExtensionRegistry.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

@interface GPBExtensionRegistryTest : GPBTestCase
@end

@implementation GPBExtensionRegistryTest

- (void)testBasics {
  GPBExtensionRegistry *reg = [[[GPBExtensionRegistry alloc] init] autorelease];
  XCTAssertNotNil(reg);

  XCTAssertNil([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1]);
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);

  [reg addExtension:[UnittestRoot optionalInt32Extension]];
  [reg addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);
  XCTAssertTrue([reg extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
}

- (void)testCopy {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];

  GPBExtensionRegistry *reg2 = [[reg1 copy] autorelease];
  XCTAssertNotNil(reg2);

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality

  // Message class that had registered extension(s) at the -copy time.

  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg2 addExtension:[UnittestRoot optionalStringExtension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]);  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]);  // ptr equality

  // Message class that did not have any registered extensions at the -copy time.

  [reg1 addExtension:[UnittestRoot packedInt64Extension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]);  // ptr equality
}

- (void)testAddExtensions {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];

  GPBExtensionRegistry *reg2 = [[[GPBExtensionRegistry alloc] init] autorelease];

  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1]);

  [reg2 addExtensions:reg1];

  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality

  // Confirm adding to the first doesn't add to the second.

  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg1 addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]);  // ptr equality
  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);

  // Confirm adding to the second doesn't add to the first.

  [reg2 addExtension:[UnittestRoot optionalStringExtension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];

  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]);  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]);  // ptr equality
}

@end
