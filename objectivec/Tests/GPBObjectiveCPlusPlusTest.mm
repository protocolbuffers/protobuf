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
#import "google/protobuf/Unittest.pbobjc.h"
#import "google/protobuf/UnittestImport.pbobjc.h"

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
