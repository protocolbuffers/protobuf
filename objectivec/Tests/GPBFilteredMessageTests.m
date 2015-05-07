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

// Tests our filter system for ObjC.
// The proto being filtered is unittest_filter.proto.
// The filter file is Filter.txt.

#import "GPBTestUtilities.h"

#import "google/protobuf/UnittestFilter.pbobjc.h"

// If we get an error about this already being defined, it is most likely
// because of an error in protoc which is supposed to be filtering
// the Remove message.
enum { Other_FieldNumber_B = 0 };

@interface FilteredMessageTests : GPBTestCase
@end

@implementation FilteredMessageTests

- (void)testEnumFiltering {
  // If compile fails here it is because protoc did not generate KeepEnum.
  XCTAssertTrue(KeepEnum_IsValidValue(KeepEnum_KeepValue));
  XCTAssertNotNil(KeepEnum_EnumDescriptor());

  // If compile fails here it is because protoc did not generate
  // KeepEnumInsideEnum and is probably due to nested enum handling being
  // broken.
  XCTAssertTrue(RemoveEnumMessage_KeepEnumInside_IsValidValue(
      RemoveEnumMessage_KeepEnumInside_KeepValue));
  XCTAssertNotNil(RemoveEnumMessage_KeepEnumInside_EnumDescriptor());
}

- (void)testMessageFiltering {
  // Messages that should be generated.
  XCTAssertNil([UnittestFilterRoot extensionRegistry]);
  XCTAssertNotNil([[[Keep alloc] init] autorelease]);
  XCTAssertNotNil([[[Other alloc] init] autorelease]);
  XCTAssertNotNil([[[RemoveJustKidding alloc] init] autorelease]);
  XCTAssertNotNil(
      [[[RemoveEnumMessage_KeepNestedInside alloc] init] autorelease]);

  // Messages that should not be generated
  XCTAssertNil(NSClassFromString(@"Remove"));
  XCTAssertNil(NSClassFromString(@"RemoveEnumMessage"));
  XCTAssertNil(NSClassFromString(@"RemoveEnumMessage_RemoveNestedInside"));

  // These should all fail compile if protoc is bad.
  XCTAssertTrue([Other instancesRespondToSelector:@selector(hasA)]);
  XCTAssertTrue([Other instancesRespondToSelector:@selector(setHasA:)]);
  XCTAssertTrue([Other instancesRespondToSelector:@selector(a)]);
  XCTAssertTrue([Other instancesRespondToSelector:@selector(setA:)]);

  // These the compiler should not generate.
  XCTAssertFalse(
      [Other instancesRespondToSelector:NSSelectorFromString(@"hasB")]);
  XCTAssertFalse(
      [Other instancesRespondToSelector:NSSelectorFromString(@"setHasB:")]);
  XCTAssertFalse([Other instancesRespondToSelector:NSSelectorFromString(@"b")]);
  XCTAssertFalse(
      [Other instancesRespondToSelector:NSSelectorFromString(@"setB:")]);

  // This should fail if protoc filters it.
  XCTAssertEqual(Other_FieldNumber_A, 1);

  // Make sure the definition at the top of the file is providing the value.
  XCTAssertEqual(Other_FieldNumber_B, 0);
}

@end
