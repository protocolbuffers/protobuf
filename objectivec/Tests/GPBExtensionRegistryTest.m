// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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

#import "GPBExtensionRegistry.h"
#import "google/protobuf/Unittest.pbobjc.h"

@interface GPBExtensionRegistryTest : GPBTestCase
@end

@implementation GPBExtensionRegistryTest

- (void)testBasics {
  GPBExtensionRegistry *reg = [[[GPBExtensionRegistry alloc] init] autorelease];
  XCTAssertNotNil(reg);

  XCTAssertNil([reg extensionForDescriptor:[TestAllExtensions descriptor]
                               fieldNumber:1]);
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor]
                               fieldNumber:1]);

  [reg addExtension:[UnittestRoot optionalInt32Extension]];
  [reg addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]); // ptr equality
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor]
                               fieldNumber:1]);
  XCTAssertTrue([reg extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]); // ptr equality
}

- (void)testCopy {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];

  GPBExtensionRegistry *reg2 = [[reg1 copy] autorelease];
  XCTAssertNotNil(reg2);

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]); // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]); // ptr equality

  // Message class that had registered extension(s) at the -copy time.

  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg2 addExtension:[UnittestRoot optionalStringExtension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]); // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]); // ptr equality

  // Message class that did not have any registered extensions at the -copy time.

  [reg1 addExtension:[UnittestRoot packedInt64Extension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]); // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]); // ptr equality

}

- (void)testAddExtensions {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];

  GPBExtensionRegistry *reg2 = [[[GPBExtensionRegistry alloc] init] autorelease];

  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor]
                                fieldNumber:1]);

  [reg2 addExtensions:reg1];

  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]); // ptr equality

  // Confirm adding to the first doesn't add to the second.

  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg1 addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]); // ptr equality
  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]); // ptr equality
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);

  // Confirm adding to the second doesn't add to the first.

  [reg2 addExtension:[UnittestRoot optionalStringExtension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];

  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]); // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]); // ptr equality
}

@end
