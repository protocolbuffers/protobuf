// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBExtensionRegistry.h"
#import "GPBTestUtilities.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

@interface GPBExtensionRegistryTest : GPBTestCase
@end

@implementation GPBExtensionRegistryTest

- (void)testBasics {
  GPBExtensionRegistry *reg = [[[GPBExtensionRegistry alloc] init] autorelease];
  XCTAssertNotNil(reg);

  XCTAssertNil([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1]);
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg addExtension:Objc_Protobuf_Tests_extension_OptionalInt32Extension()];
  [reg addExtension:Objc_Protobuf_Tests_extension_PackedInt64Extension()];

  XCTAssertTrue([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);
  XCTAssertTrue([reg extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                Objc_Protobuf_Tests_extension_PackedInt64Extension());  // ptr equality
#else
  [reg addExtension:[UnittestRoot optionalInt32Extension]];
  [reg addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);
  XCTAssertTrue([reg extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
#endif
}

- (void)testCopy {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalInt32Extension()];
#else
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];
#endif

  GPBExtensionRegistry *reg2 = [[reg1 copy] autorelease];
  XCTAssertNotNil(reg2);

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
#else
  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
#endif

  // Message class that had registered extension(s) at the -copy time.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalBoolExtension()];
  [reg2 addExtension:Objc_Protobuf_Tests_extension_OptionalStringExtension()];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                Objc_Protobuf_Tests_extension_OptionalBoolExtension());  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                Objc_Protobuf_Tests_extension_OptionalStringExtension());  // ptr equality
#else
  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg2 addExtension:[UnittestRoot optionalStringExtension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]);  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]);  // ptr equality
#endif

  // Message class that did not have any registered extensions at the -copy time.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_PackedInt64Extension()];
  [reg2 addExtension:Objc_Protobuf_Tests_extension_PackedSint32Extension()];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                Objc_Protobuf_Tests_extension_PackedInt64Extension());  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                Objc_Protobuf_Tests_extension_PackedSint32Extension());  // ptr equality
#else
  [reg1 addExtension:[UnittestRoot packedInt64Extension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]);  // ptr equality
#endif
}

- (void)testAddExtensions {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalInt32Extension()];
#else
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];
#endif

  GPBExtensionRegistry *reg2 = [[[GPBExtensionRegistry alloc] init] autorelease];

  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1]);

  [reg2 addExtensions:reg1];

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
#else
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
#endif

  // Confirm adding to the first doesn't add to the second.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalBoolExtension()];
  [reg1 addExtension:Objc_Protobuf_Tests_extension_PackedInt64Extension()];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                Objc_Protobuf_Tests_extension_OptionalBoolExtension());  // ptr equality
  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                Objc_Protobuf_Tests_extension_PackedInt64Extension());  // ptr equality
#else
  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg1 addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]);  // ptr equality
  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
#endif
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);

  // Confirm adding to the second doesn't add to the first.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg2 addExtension:Objc_Protobuf_Tests_extension_OptionalStringExtension()];
  [reg2 addExtension:Objc_Protobuf_Tests_extension_PackedSint32Extension()];
#else
  [reg2 addExtension:[UnittestRoot optionalStringExtension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];
#endif

  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                Objc_Protobuf_Tests_extension_OptionalStringExtension());  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                Objc_Protobuf_Tests_extension_PackedSint32Extension());  // ptr equality
#else
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]);  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]);  // ptr equality
#endif
}

@end
