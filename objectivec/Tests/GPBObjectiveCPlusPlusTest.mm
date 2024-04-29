// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

//
// This is just a compile test (here to make sure things never regress).
//
// Objective C++ can run into issues with how the NS_ENUM/CF_ENUM declaration
// works because of the C++ spec being used for that compilation unit. So
// the fact that these imports all work without errors/warning means things
// are still good.
//
// The "well know types" should have cross file enums needing imports.
#import "GPBProtocolBuffers.h"
// Some of the tests explicitly use cross file enums also.
#import "objectivec/Tests/Unittest.pbobjc.h"
#import "objectivec/Tests/UnittestImport.pbobjc.h"

// Sanity check the conditions of the test within the Xcode project.
#if !__cplusplus
#error This isn't compiled as Objective C++?
#elif __cplusplus >= 201103L
// If this trips, it means the Xcode default might have change (or someone
// edited the testing project) and it might be time to revisit the GPB_ENUM
// define in GPBBootstrap.h.
#warning Did the Xcode default for C++ spec change?
#endif

// Dummy XCTest.
@interface GPBObjectiveCPlusPlusTests : GPBTestCase
@end

@implementation GPBObjectiveCPlusPlusTests
- (void)testCPlusPlus {
  // Nothing, This was a compile test.
  XCTAssertTrue(YES);
}
@end
