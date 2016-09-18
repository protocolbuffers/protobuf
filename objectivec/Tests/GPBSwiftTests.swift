// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

import Foundation
import XCTest

// Test some usage of the ObjC library from Swift.

class GPBBridgeTests: XCTestCase {

  func testProto2Basics() {
    let msg = Message2()
    let msg2 = Message2()
    let msg3 = Message2_OptionalGroup()

    msg.optionalInt32 = 100
    msg.optionalString = "abc"
    msg.optionalEnum = .Bar
    msg2.optionalString = "other"
    msg.optionalMessage = msg2
    msg3.a = 200
    msg.optionalGroup = msg3
    msg.repeatedInt32Array.addValue(300)
    msg.repeatedInt32Array.addValue(301)
    msg.repeatedStringArray.addObject("mno")
    msg.repeatedStringArray.addObject("pqr")
    msg.repeatedEnumArray.addValue(Message2_Enum.Bar.rawValue)
    msg.repeatedEnumArray.addValue(Message2_Enum.Baz.rawValue)
    msg.mapInt32Int32.setInt32(400, forKey:500)
    msg.mapInt32Int32.setInt32(401, forKey:501)
    msg.mapStringString.setObject("foo", forKey:"bar")
    msg.mapStringString.setObject("abc", forKey:"xyz")
    msg.mapInt32Enum.setEnum(Message2_Enum.Bar.rawValue, forKey:600)
    msg.mapInt32Enum.setEnum(Message2_Enum.Baz.rawValue, forKey:601)

    // Check has*.
    XCTAssertTrue(msg.hasOptionalInt32)
    XCTAssertTrue(msg.hasOptionalString)
    XCTAssertTrue(msg.hasOptionalEnum)
    XCTAssertTrue(msg2.hasOptionalString)
    XCTAssertTrue(msg.hasOptionalMessage)
    XCTAssertTrue(msg3.hasA)
    XCTAssertTrue(msg.hasOptionalGroup)
    XCTAssertFalse(msg.hasOptionalInt64)
    XCTAssertFalse(msg.hasOptionalFloat)

    // Check values.
    XCTAssertEqual(msg.optionalInt32, Int32(100))
    XCTAssertEqual(msg.optionalString, "abc")
    XCTAssertEqual(msg2.optionalString, "other")
    XCTAssertTrue(msg.optionalMessage === msg2)
    XCTAssertEqual(msg.optionalEnum, Message2_Enum.Bar)
    XCTAssertEqual(msg3.a, Int32(200))
    XCTAssertTrue(msg.optionalGroup === msg3)
    XCTAssertEqual(msg.repeatedInt32Array.count, UInt(2))
    XCTAssertEqual(msg.repeatedInt32Array.valueAtIndex(0), Int32(300))
    XCTAssertEqual(msg.repeatedInt32Array.valueAtIndex(1), Int32(301))
    XCTAssertEqual(msg.repeatedStringArray.count, Int(2))
    XCTAssertEqual(msg.repeatedStringArray.objectAtIndex(0) as? String, "mno")
    XCTAssertEqual(msg.repeatedStringArray.objectAtIndex(1) as? String, "pqr")
    XCTAssertEqual(msg.repeatedEnumArray.count, UInt(2))
    XCTAssertEqual(msg.repeatedEnumArray.valueAtIndex(0), Message2_Enum.Bar.rawValue)
    XCTAssertEqual(msg.repeatedEnumArray.valueAtIndex(1), Message2_Enum.Baz.rawValue)
    XCTAssertEqual(msg.repeatedInt64Array.count, UInt(0))
    XCTAssertEqual(msg.mapInt32Int32.count, UInt(2))
    var intValue: Int32 = 0
    XCTAssertTrue(msg.mapInt32Int32.getInt32(&intValue, forKey: 500))
    XCTAssertEqual(intValue, Int32(400))
    XCTAssertTrue(msg.mapInt32Int32.getInt32(&intValue, forKey: 501))
    XCTAssertEqual(intValue, Int32(401))
    XCTAssertEqual(msg.mapStringString.count, Int(2))
    XCTAssertEqual(msg.mapStringString.objectForKey("bar") as? String, "foo")
    XCTAssertEqual(msg.mapStringString.objectForKey("xyz") as? String, "abc")
    XCTAssertEqual(msg.mapInt32Enum.count, UInt(2))
    XCTAssertTrue(msg.mapInt32Enum.getEnum(&intValue, forKey:600))
    XCTAssertEqual(intValue, Message2_Enum.Bar.rawValue)
    XCTAssertTrue(msg.mapInt32Enum.getEnum(&intValue, forKey:601))
    XCTAssertEqual(intValue, Message2_Enum.Baz.rawValue)

    // Clearing a string with nil.
    msg2.optionalString = nil
    XCTAssertFalse(msg2.hasOptionalString)
    XCTAssertEqual(msg2.optionalString, "")

    // Clearing a message with nil.
    msg.optionalGroup = nil
    XCTAssertFalse(msg.hasOptionalGroup)
    XCTAssertTrue(msg.optionalGroup !== msg3)  // New instance

    // Clear.
    msg.clear()
    XCTAssertFalse(msg.hasOptionalInt32)
    XCTAssertFalse(msg.hasOptionalString)
    XCTAssertFalse(msg.hasOptionalEnum)
    XCTAssertFalse(msg.hasOptionalMessage)
    XCTAssertFalse(msg.hasOptionalInt64)
    XCTAssertFalse(msg.hasOptionalFloat)
    XCTAssertEqual(msg.optionalInt32, Int32(0))
    XCTAssertEqual(msg.optionalString, "")
    XCTAssertTrue(msg.optionalMessage !== msg2)  // New instance
    XCTAssertEqual(msg.optionalEnum, Message2_Enum.Foo)  // Default
    XCTAssertEqual(msg.repeatedInt32Array.count, UInt(0))
    XCTAssertEqual(msg.repeatedStringArray.count, Int(0))
    XCTAssertEqual(msg.repeatedEnumArray.count, UInt(0))
    XCTAssertEqual(msg.mapInt32Int32.count, UInt(0))
    XCTAssertEqual(msg.mapStringString.count, Int(0))
    XCTAssertEqual(msg.mapInt32Enum.count, UInt(0))
  }

  func testProto3Basics() {
    let msg = Message3()
    let msg2 = Message3()

    msg.optionalInt32 = 100
    msg.optionalString = "abc"
    msg.optionalEnum = .Bar
    msg2.optionalString = "other"
    msg.optionalMessage = msg2
    msg.repeatedInt32Array.addValue(300)
    msg.repeatedInt32Array.addValue(301)
    msg.repeatedStringArray.addObject("mno")
    msg.repeatedStringArray.addObject("pqr")
    // "proto3" syntax lets enum get unknown values.
    msg.repeatedEnumArray.addValue(Message3_Enum.Bar.rawValue)
    msg.repeatedEnumArray.addRawValue(666)
    SetMessage3_OptionalEnum_RawValue(msg2, 666)
    msg.mapInt32Int32.setInt32(400, forKey:500)
    msg.mapInt32Int32.setInt32(401, forKey:501)
    msg.mapStringString.setObject("foo", forKey:"bar")
    msg.mapStringString.setObject("abc", forKey:"xyz")
    msg.mapInt32Enum.setEnum(Message2_Enum.Bar.rawValue, forKey:600)
    // "proto3" syntax lets enum get unknown values.
    msg.mapInt32Enum.setRawValue(666, forKey:601)

    // Has only exists on for message fields.
    XCTAssertTrue(msg.hasOptionalMessage)
    XCTAssertFalse(msg2.hasOptionalMessage)

    // Check values.
    XCTAssertEqual(msg.optionalInt32, Int32(100))
    XCTAssertEqual(msg.optionalString, "abc")
    XCTAssertEqual(msg2.optionalString, "other")
    XCTAssertTrue(msg.optionalMessage === msg2)
    XCTAssertEqual(msg.optionalEnum, Message3_Enum.Bar)
    XCTAssertEqual(msg.repeatedInt32Array.count, UInt(2))
    XCTAssertEqual(msg.repeatedInt32Array.valueAtIndex(0), Int32(300))
    XCTAssertEqual(msg.repeatedInt32Array.valueAtIndex(1), Int32(301))
    XCTAssertEqual(msg.repeatedStringArray.count, Int(2))
    XCTAssertEqual(msg.repeatedStringArray.objectAtIndex(0) as? String, "mno")
    XCTAssertEqual(msg.repeatedStringArray.objectAtIndex(1) as? String, "pqr")
    XCTAssertEqual(msg.repeatedInt64Array.count, UInt(0))
    XCTAssertEqual(msg.repeatedEnumArray.count, UInt(2))
    XCTAssertEqual(msg.repeatedEnumArray.valueAtIndex(0), Message3_Enum.Bar.rawValue)
    XCTAssertEqual(msg.repeatedEnumArray.valueAtIndex(1), Message3_Enum.GPBUnrecognizedEnumeratorValue.rawValue)
    XCTAssertEqual(msg.repeatedEnumArray.rawValueAtIndex(1), 666)
    XCTAssertEqual(msg2.optionalEnum, Message3_Enum.GPBUnrecognizedEnumeratorValue)
    XCTAssertEqual(Message3_OptionalEnum_RawValue(msg2), Int32(666))
    XCTAssertEqual(msg.mapInt32Int32.count, UInt(2))
    var intValue: Int32 = 0
    XCTAssertTrue(msg.mapInt32Int32.getInt32(&intValue, forKey:500))
    XCTAssertEqual(intValue, Int32(400))
    XCTAssertTrue(msg.mapInt32Int32.getInt32(&intValue, forKey:501))
    XCTAssertEqual(intValue, Int32(401))
    XCTAssertEqual(msg.mapStringString.count, Int(2))
    XCTAssertEqual(msg.mapStringString.objectForKey("bar") as? String, "foo")
    XCTAssertEqual(msg.mapStringString.objectForKey("xyz") as? String, "abc")
    XCTAssertEqual(msg.mapInt32Enum.count, UInt(2))
    XCTAssertTrue(msg.mapInt32Enum.getEnum(&intValue, forKey:600))
    XCTAssertEqual(intValue, Message2_Enum.Bar.rawValue)
    XCTAssertTrue(msg.mapInt32Enum.getEnum(&intValue, forKey:601))
    XCTAssertEqual(intValue, Message3_Enum.GPBUnrecognizedEnumeratorValue.rawValue)
    XCTAssertTrue(msg.mapInt32Enum.getRawValue(&intValue, forKey:601))
    XCTAssertEqual(intValue, 666)

    // Clearing a string with nil.
    msg2.optionalString = nil
    XCTAssertEqual(msg2.optionalString, "")

    // Clearing a message with nil.
    msg.optionalMessage = nil
    XCTAssertFalse(msg.hasOptionalMessage)
    XCTAssertTrue(msg.optionalMessage !== msg2)  // New instance

    // Clear.
    msg.clear()
    XCTAssertFalse(msg.hasOptionalMessage)
    XCTAssertEqual(msg.optionalInt32, Int32(0))
    XCTAssertEqual(msg.optionalString, "")
    XCTAssertTrue(msg.optionalMessage !== msg2)  // New instance
    XCTAssertEqual(msg.optionalEnum, Message3_Enum.Foo)  // Default
    XCTAssertEqual(msg.repeatedInt32Array.count, UInt(0))
    XCTAssertEqual(msg.repeatedStringArray.count, Int(0))
    XCTAssertEqual(msg.repeatedEnumArray.count, UInt(0))
    msg2.clear()
    XCTAssertEqual(msg2.optionalEnum, Message3_Enum.Foo)  // Default
    XCTAssertEqual(Message3_OptionalEnum_RawValue(msg2), Message3_Enum.Foo.rawValue)
    XCTAssertEqual(msg.mapInt32Int32.count, UInt(0))
    XCTAssertEqual(msg.mapStringString.count, Int(0))
    XCTAssertEqual(msg.mapInt32Enum.count, UInt(0))
  }

  func testAutoCreation() {
    let msg = Message2()

    XCTAssertFalse(msg.hasOptionalGroup)
    XCTAssertFalse(msg.hasOptionalMessage)

    // Access shouldn't result in has* but should return objects.
    let msg2 = msg.optionalGroup
    let msg3 = msg.optionalMessage.optionalMessage
    let msg4 = msg.optionalMessage
    XCTAssertNotNil(msg2)
    XCTAssertNotNil(msg3)
    XCTAssertFalse(msg.hasOptionalGroup)
    XCTAssertFalse(msg.optionalMessage.hasOptionalMessage)
    XCTAssertFalse(msg.hasOptionalMessage)

    // Setting things should trigger has* getting set.
    msg.optionalGroup.a = 10
    msg.optionalMessage.optionalMessage.optionalInt32 = 100
    XCTAssertTrue(msg.hasOptionalGroup)
    XCTAssertTrue(msg.optionalMessage.hasOptionalMessage)
    XCTAssertTrue(msg.hasOptionalMessage)

    // And they should be the same pointer as before.
    XCTAssertTrue(msg2 === msg.optionalGroup)
    XCTAssertTrue(msg3 === msg.optionalMessage.optionalMessage)
    XCTAssertTrue(msg4 === msg.optionalMessage)

    // Clear gets us new objects next time around.
    msg.clear()
    XCTAssertFalse(msg.hasOptionalGroup)
    XCTAssertFalse(msg.optionalMessage.hasOptionalMessage)
    XCTAssertFalse(msg.hasOptionalMessage)
    msg.optionalGroup.a = 20
    msg.optionalMessage.optionalMessage.optionalInt32 = 200
    XCTAssertTrue(msg.hasOptionalGroup)
    XCTAssertTrue(msg.optionalMessage.hasOptionalMessage)
    XCTAssertTrue(msg.hasOptionalMessage)
    XCTAssertTrue(msg2 !== msg.optionalGroup)
    XCTAssertTrue(msg3 !== msg.optionalMessage.optionalMessage)
    XCTAssertTrue(msg4 !== msg.optionalMessage)

    // Explicit set of a message, means autocreated object doesn't bind.
    msg.clear()
    let autoCreated = msg.optionalMessage
    XCTAssertFalse(msg.hasOptionalMessage)
    let msg5 = Message2()
    msg5.optionalInt32 = 123
    msg.optionalMessage = msg5
    XCTAssertTrue(msg.hasOptionalMessage)
    // Modifing the autocreated doesn't replaced the explicit set one.
    autoCreated.optionalInt32 = 456
    XCTAssertTrue(msg.hasOptionalMessage)
    XCTAssertTrue(msg.optionalMessage === msg5)
    XCTAssertEqual(msg.optionalMessage.optionalInt32, Int32(123))
  }

  func testProto2OneOfSupport() {
    let msg = Message2()

    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.GPBUnsetOneOfCase)
    XCTAssertEqual(msg.oneofInt32, Int32(100))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(110.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message2_Enum.Baz)  // Default
    let autoCreated = msg.oneofMessage  // Default create one.
    XCTAssertNotNil(autoCreated)
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.GPBUnsetOneOfCase)

    msg.oneofInt32 = 10
    XCTAssertEqual(msg.oneofInt32, Int32(10))
    XCTAssertEqual(msg.oneofFloat, Float(110.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message2_Enum.Baz)  // Default
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.OneofInt32)

    msg.oneofFloat = 20.0
    XCTAssertEqual(msg.oneofInt32, Int32(100))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(20.0))
    XCTAssertEqual(msg.oneofEnum, Message2_Enum.Baz)  // Default
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.OneofFloat)

    msg.oneofEnum = .Bar
    XCTAssertEqual(msg.oneofInt32, Int32(100))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(110.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message2_Enum.Bar)
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.OneofEnum)

    // Sets via the autocreated instance.
    msg.oneofMessage.optionalInt32 = 200
    XCTAssertEqual(msg.oneofInt32, Int32(100))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(110.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message2_Enum.Baz)  // Default
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(200))
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.OneofMessage)

    // Clear the oneof.
    Message2_ClearOOneOfCase(msg)
    XCTAssertEqual(msg.oneofInt32, Int32(100))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(110.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message2_Enum.Baz)  // Default
    let autoCreated2 = msg.oneofMessage  // Default create one
    XCTAssertNotNil(autoCreated2)
    XCTAssertTrue(autoCreated2 !== autoCreated)  // New instance
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.GPBUnsetOneOfCase)

    msg.oneofInt32 = 10
    XCTAssertEqual(msg.oneofInt32, Int32(10))
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.OneofInt32)

    // Confirm Message.clear() handles the oneof correctly.
    msg.clear()
    XCTAssertEqual(msg.oneofInt32, Int32(100))  // Default
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.GPBUnsetOneOfCase)

    // Sets via the autocreated instance.
    msg.oneofMessage.optionalInt32 = 300
    XCTAssertTrue(msg.oneofMessage !== autoCreated)  // New instance
    XCTAssertTrue(msg.oneofMessage !== autoCreated2)  // New instance
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(300))
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.OneofMessage)

    // Set message to nil clears the oneof.
    msg.oneofMessage = nil
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oOneOfCase, Message2_O_OneOfCase.GPBUnsetOneOfCase)
}

  func testProto3OneOfSupport() {
    let msg = Message3()

    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.GPBUnsetOneOfCase)
    XCTAssertEqual(msg.oneofInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(0.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message3_Enum.Foo)  // Default
    let autoCreated = msg.oneofMessage  // Default create one.
    XCTAssertNotNil(autoCreated)
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.GPBUnsetOneOfCase)

    msg.oneofInt32 = 10
    XCTAssertEqual(msg.oneofInt32, Int32(10))
    XCTAssertEqual(msg.oneofFloat, Float(0.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message3_Enum.Foo)  // Default
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.OneofInt32)

    msg.oneofFloat = 20.0
    XCTAssertEqual(msg.oneofInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(20.0))
    XCTAssertEqual(msg.oneofEnum, Message3_Enum.Foo)  // Default
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.OneofFloat)

    msg.oneofEnum = .Bar
    XCTAssertEqual(msg.oneofInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(0.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message3_Enum.Bar)
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.OneofEnum)

    // Sets via the autocreated instance.
    msg.oneofMessage.optionalInt32 = 200
    XCTAssertEqual(msg.oneofInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(0.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message3_Enum.Foo)  // Default
    XCTAssertTrue(msg.oneofMessage === autoCreated)  // Still the same
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(200))
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.OneofMessage)

    // Clear the oneof.
    Message3_ClearOOneOfCase(msg)
    XCTAssertEqual(msg.oneofInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oneofFloat, Float(0.0))  // Default
    XCTAssertEqual(msg.oneofEnum, Message3_Enum.Foo)  // Default
    let autoCreated2 = msg.oneofMessage  // Default create one
    XCTAssertNotNil(autoCreated2)
    XCTAssertTrue(autoCreated2 !== autoCreated)  // New instance
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.GPBUnsetOneOfCase)

    msg.oneofInt32 = 10
    XCTAssertEqual(msg.oneofInt32, Int32(10))
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.OneofInt32)

    // Confirm Message.clear() handles the oneof correctly.
    msg.clear()
    XCTAssertEqual(msg.oneofInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.GPBUnsetOneOfCase)

    // Sets via the autocreated instance.
    msg.oneofMessage.optionalInt32 = 300
    XCTAssertTrue(msg.oneofMessage !== autoCreated)  // New instance
    XCTAssertTrue(msg.oneofMessage !== autoCreated2)  // New instance
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(300))
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.OneofMessage)

    // Set message to nil clears the oneof.
    msg.oneofMessage = nil
    XCTAssertEqual(msg.oneofMessage.optionalInt32, Int32(0))  // Default
    XCTAssertEqual(msg.oOneOfCase, Message3_O_OneOfCase.GPBUnsetOneOfCase)
  }

  func testSerialization() {
    let msg = Message2()

    msg.optionalInt32 = 100
    msg.optionalInt64 = 101
    msg.optionalGroup.a = 102
    msg.repeatedStringArray.addObject("abc")
    msg.repeatedStringArray.addObject("def")
    msg.mapInt32Int32.setInt32(200, forKey:300)
    msg.mapInt32Int32.setInt32(201, forKey:201)
    msg.mapStringString.setObject("foo", forKey:"bar")
    msg.mapStringString.setObject("abc", forKey:"xyz")

    let data = msg.data()

    let msg2 = try! Message2(data: data!)
    XCTAssertTrue(msg2 !== msg)  // New instance
    XCTAssertEqual(msg.optionalInt32, Int32(100))
    XCTAssertEqual(msg.optionalInt64, Int64(101))
    XCTAssertEqual(msg.optionalGroup.a, Int32(102))
    XCTAssertEqual(msg.repeatedStringArray.count, Int(2))
    XCTAssertEqual(msg.mapInt32Int32.count, UInt(2))
    XCTAssertEqual(msg.mapStringString.count, Int(2))
    XCTAssertEqual(msg2, msg)
  }

}
