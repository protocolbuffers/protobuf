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

goog.require('goog.crypt.base64');
goog.require('goog.testing.asserts');
// CommonJS-LoadFromFile: testbinary_pb proto.jspb.test
goog.require('proto.jspb.test.ForeignMessage');
// CommonJS-LoadFromFile: proto3_test_pb proto.jspb.test
goog.require('proto.jspb.test.Proto3Enum');
goog.require('proto.jspb.test.TestProto3');
// CommonJS-LoadFromFile: google/protobuf/any_pb proto.google.protobuf
goog.require('proto.google.protobuf.Any');
// CommonJS-LoadFromFile: google/protobuf/timestamp_pb proto.google.protobuf
goog.require('proto.google.protobuf.Timestamp');
// CommonJS-LoadFromFile: google/protobuf/struct_pb proto.google.protobuf
goog.require('proto.google.protobuf.Struct');


var BYTES = new Uint8Array([1, 2, 8, 9]);
var BYTES_B64 = goog.crypt.base64.encodeByteArray(BYTES);


/**
 * Helper: compare a bytes field to an expected value
 * @param {Uint8Array|string} arr
 * @param {Uint8Array} expected
 * @return {boolean}
 */
function bytesCompare(arr, expected) {
  if (goog.isString(arr)) {
    arr = goog.crypt.base64.decodeStringToUint8Array(arr);
  }
  if (arr.length != expected.length) {
    return false;
  }
  for (var i = 0; i < arr.length; i++) {
    if (arr[i] != expected[i]) {
      return false;
    }
  }
  return true;
}


describe('proto3Test', function() {

  /**
   * Test default values don't affect equality test.
   */
  it('testEqualsProto3', function() {
    var msg1 = new proto.jspb.test.TestProto3();
    var msg2 = new proto.jspb.test.TestProto3();
    msg2.setOptionalString('');

    assertTrue(jspb.Message.equals(msg1, msg2));
  });


  /**
   * Test setting when a field has default semantics.
   */
  it('testSetProto3ToValueAndBackToDefault', function() {
    var msg = new proto.jspb.test.TestProto3();

    // Setting should work normally.
    msg.setOptionalString('optionalString');
    assertEquals(msg.getOptionalString(), 'optionalString');

    // Clearing should work too ...
    msg.setOptionalString('');
    assertEquals(msg.getOptionalString(), '');

    // ... and shouldn't affect the equality with a brand new message.
    assertTrue(jspb.Message.equals(msg, new proto.jspb.test.TestProto3()));
  });

  /**
   * Test defaults for proto3 message fields.
   */
  it('testProto3FieldDefaults', function() {
    var msg = new proto.jspb.test.TestProto3();

    assertEquals(msg.getOptionalInt32(), 0);
    assertEquals(msg.getOptionalInt64(), 0);
    assertEquals(msg.getOptionalUint32(), 0);
    assertEquals(msg.getOptionalUint64(), 0);
    assertEquals(msg.getOptionalSint32(), 0);
    assertEquals(msg.getOptionalSint64(), 0);
    assertEquals(msg.getOptionalFixed32(), 0);
    assertEquals(msg.getOptionalFixed64(), 0);
    assertEquals(msg.getOptionalSfixed32(), 0);
    assertEquals(msg.getOptionalSfixed64(), 0);
    assertEquals(msg.getOptionalFloat(), 0);
    assertEquals(msg.getOptionalDouble(), 0);
    assertEquals(msg.getOptionalString(), '');

    // TODO(b/26173701): when we change bytes fields default getter to return
    // Uint8Array, we'll want to switch this assertion to match the u8 case.
    assertEquals(typeof msg.getOptionalBytes(), 'string');
    assertEquals(msg.getOptionalBytes_asU8() instanceof Uint8Array, true);
    assertEquals(typeof msg.getOptionalBytes_asB64(), 'string');
    assertEquals(msg.getOptionalBytes().length, 0);
    assertEquals(msg.getOptionalBytes_asU8().length, 0);
    assertEquals(msg.getOptionalBytes_asB64(), '');

    assertEquals(msg.getOptionalForeignEnum(),
                 proto.jspb.test.Proto3Enum.PROTO3_FOO);
    assertEquals(msg.getOptionalForeignMessage(), undefined);
    assertEquals(msg.getOptionalForeignMessage(), undefined);

    assertEquals(msg.getRepeatedInt32List().length, 0);
    assertEquals(msg.getRepeatedInt64List().length, 0);
    assertEquals(msg.getRepeatedUint32List().length, 0);
    assertEquals(msg.getRepeatedUint64List().length, 0);
    assertEquals(msg.getRepeatedSint32List().length, 0);
    assertEquals(msg.getRepeatedSint64List().length, 0);
    assertEquals(msg.getRepeatedFixed32List().length, 0);
    assertEquals(msg.getRepeatedFixed64List().length, 0);
    assertEquals(msg.getRepeatedSfixed32List().length, 0);
    assertEquals(msg.getRepeatedSfixed64List().length, 0);
    assertEquals(msg.getRepeatedFloatList().length, 0);
    assertEquals(msg.getRepeatedDoubleList().length, 0);
    assertEquals(msg.getRepeatedStringList().length, 0);
    assertEquals(msg.getRepeatedBytesList().length, 0);
    assertEquals(msg.getRepeatedForeignEnumList().length, 0);
    assertEquals(msg.getRepeatedForeignMessageList().length, 0);

  });


  /**
   * Test that all fields can be set and read via a serialization roundtrip.
   */
  it('testProto3FieldSetGet', function() {
    var msg = new proto.jspb.test.TestProto3();

    msg.setOptionalInt32(-42);
    msg.setOptionalInt64(-0x7fffffff00000000);
    msg.setOptionalUint32(0x80000000);
    msg.setOptionalUint64(0xf000000000000000);
    msg.setOptionalSint32(-100);
    msg.setOptionalSint64(-0x8000000000000000);
    msg.setOptionalFixed32(1234);
    msg.setOptionalFixed64(0x1234567800000000);
    msg.setOptionalSfixed32(-1234);
    msg.setOptionalSfixed64(-0x1234567800000000);
    msg.setOptionalFloat(1.5);
    msg.setOptionalDouble(-1.5);
    msg.setOptionalBool(true);
    msg.setOptionalString('hello world');
    msg.setOptionalBytes(BYTES);
    var submsg = new proto.jspb.test.ForeignMessage();
    submsg.setC(16);
    msg.setOptionalForeignMessage(submsg);
    msg.setOptionalForeignEnum(proto.jspb.test.Proto3Enum.PROTO3_BAR);

    msg.setRepeatedInt32List([-42]);
    msg.setRepeatedInt64List([-0x7fffffff00000000]);
    msg.setRepeatedUint32List([0x80000000]);
    msg.setRepeatedUint64List([0xf000000000000000]);
    msg.setRepeatedSint32List([-100]);
    msg.setRepeatedSint64List([-0x8000000000000000]);
    msg.setRepeatedFixed32List([1234]);
    msg.setRepeatedFixed64List([0x1234567800000000]);
    msg.setRepeatedSfixed32List([-1234]);
    msg.setRepeatedSfixed64List([-0x1234567800000000]);
    msg.setRepeatedFloatList([1.5]);
    msg.setRepeatedDoubleList([-1.5]);
    msg.setRepeatedBoolList([true]);
    msg.setRepeatedStringList(['hello world']);
    msg.setRepeatedBytesList([BYTES]);
    submsg = new proto.jspb.test.ForeignMessage();
    submsg.setC(1000);
    msg.setRepeatedForeignMessageList([submsg]);
    msg.setRepeatedForeignEnumList([proto.jspb.test.Proto3Enum.PROTO3_BAR]);

    msg.setOneofString('asdf');

    var serialized = msg.serializeBinary();
    msg = proto.jspb.test.TestProto3.deserializeBinary(serialized);

    assertEquals(msg.getOptionalInt32(), -42);
    assertEquals(msg.getOptionalInt64(), -0x7fffffff00000000);
    assertEquals(msg.getOptionalUint32(), 0x80000000);
    assertEquals(msg.getOptionalUint64(), 0xf000000000000000);
    assertEquals(msg.getOptionalSint32(), -100);
    assertEquals(msg.getOptionalSint64(), -0x8000000000000000);
    assertEquals(msg.getOptionalFixed32(), 1234);
    assertEquals(msg.getOptionalFixed64(), 0x1234567800000000);
    assertEquals(msg.getOptionalSfixed32(), -1234);
    assertEquals(msg.getOptionalSfixed64(), -0x1234567800000000);
    assertEquals(msg.getOptionalFloat(), 1.5);
    assertEquals(msg.getOptionalDouble(), -1.5);
    assertEquals(msg.getOptionalBool(), true);
    assertEquals(msg.getOptionalString(), 'hello world');
    assertEquals(true, bytesCompare(msg.getOptionalBytes(), BYTES));
    assertEquals(msg.getOptionalForeignMessage().getC(), 16);
    assertEquals(msg.getOptionalForeignEnum(),
        proto.jspb.test.Proto3Enum.PROTO3_BAR);

    assertElementsEquals(msg.getRepeatedInt32List(), [-42]);
    assertElementsEquals(msg.getRepeatedInt64List(), [-0x7fffffff00000000]);
    assertElementsEquals(msg.getRepeatedUint32List(), [0x80000000]);
    assertElementsEquals(msg.getRepeatedUint64List(), [0xf000000000000000]);
    assertElementsEquals(msg.getRepeatedSint32List(), [-100]);
    assertElementsEquals(msg.getRepeatedSint64List(), [-0x8000000000000000]);
    assertElementsEquals(msg.getRepeatedFixed32List(), [1234]);
    assertElementsEquals(msg.getRepeatedFixed64List(), [0x1234567800000000]);
    assertElementsEquals(msg.getRepeatedSfixed32List(), [-1234]);
    assertElementsEquals(msg.getRepeatedSfixed64List(), [-0x1234567800000000]);
    assertElementsEquals(msg.getRepeatedFloatList(), [1.5]);
    assertElementsEquals(msg.getRepeatedDoubleList(), [-1.5]);
    assertElementsEquals(msg.getRepeatedBoolList(), [true]);
    assertElementsEquals(msg.getRepeatedStringList(), ['hello world']);
    assertEquals(msg.getRepeatedBytesList().length, 1);
    assertEquals(true, bytesCompare(msg.getRepeatedBytesList()[0], BYTES));
    assertEquals(msg.getRepeatedForeignMessageList().length, 1);
    assertEquals(msg.getRepeatedForeignMessageList()[0].getC(), 1000);
    assertElementsEquals(msg.getRepeatedForeignEnumList(),
        [proto.jspb.test.Proto3Enum.PROTO3_BAR]);

    assertEquals(msg.getOneofString(), 'asdf');
  });


  /**
   * Test that oneofs continue to have a notion of field presence.
   */
  it('testOneofs', function() {
    // Default instance.
    var msg = new proto.jspb.test.TestProto3();
    assertEquals(msg.getOneofUint32(), 0);
    assertEquals(msg.getOneofForeignMessage(), undefined);
    assertEquals(msg.getOneofString(), '');
    assertEquals(msg.getOneofBytes(), '');

    assertFalse(msg.hasOneofUint32());
    assertFalse(msg.hasOneofForeignMessage());
    assertFalse(msg.hasOneofString());
    assertFalse(msg.hasOneofBytes());

    // Integer field.
    msg.setOneofUint32(42);
    assertEquals(msg.getOneofUint32(), 42);
    assertEquals(msg.getOneofForeignMessage(), undefined);
    assertEquals(msg.getOneofString(), '');
    assertEquals(msg.getOneofBytes(), '');

    assertTrue(msg.hasOneofUint32());
    assertFalse(msg.hasOneofForeignMessage());
    assertFalse(msg.hasOneofString());
    assertFalse(msg.hasOneofBytes());

    // Sub-message field.
    var submsg = new proto.jspb.test.ForeignMessage();
    msg.setOneofForeignMessage(submsg);
    assertEquals(msg.getOneofUint32(), 0);
    assertEquals(msg.getOneofForeignMessage(), submsg);
    assertEquals(msg.getOneofString(), '');
    assertEquals(msg.getOneofBytes(), '');

    assertFalse(msg.hasOneofUint32());
    assertTrue(msg.hasOneofForeignMessage());
    assertFalse(msg.hasOneofString());
    assertFalse(msg.hasOneofBytes());

    // String field.
    msg.setOneofString('hello');
    assertEquals(msg.getOneofUint32(), 0);
    assertEquals(msg.getOneofForeignMessage(), undefined);
    assertEquals(msg.getOneofString(), 'hello');
    assertEquals(msg.getOneofBytes(), '');

    assertFalse(msg.hasOneofUint32());
    assertFalse(msg.hasOneofForeignMessage());
    assertTrue(msg.hasOneofString());
    assertFalse(msg.hasOneofBytes());

    // Bytes field.
    msg.setOneofBytes(goog.crypt.base64.encodeString('\u00FF\u00FF'));
    assertEquals(msg.getOneofUint32(), 0);
    assertEquals(msg.getOneofForeignMessage(), undefined);
    assertEquals(msg.getOneofString(), '');
    assertEquals(msg.getOneofBytes_asB64(),
        goog.crypt.base64.encodeString('\u00FF\u00FF'));

    assertFalse(msg.hasOneofUint32());
    assertFalse(msg.hasOneofForeignMessage());
    assertFalse(msg.hasOneofString());
    assertTrue(msg.hasOneofBytes());
  });


  /**
   * Test that "default"-valued primitive fields are not emitted on the wire.
   */
  it('testNoSerializeDefaults', function() {
    var msg = new proto.jspb.test.TestProto3();

    // Set each primitive to a non-default value, then back to its default, to
    // ensure that the serialization is actually checking the value and not just
    // whether it has ever been set.
    msg.setOptionalInt32(42);
    msg.setOptionalInt32(0);
    msg.setOptionalDouble(3.14);
    msg.setOptionalDouble(0.0);
    msg.setOptionalBool(true);
    msg.setOptionalBool(false);
    msg.setOptionalString('hello world');
    msg.setOptionalString('');
    msg.setOptionalBytes(goog.crypt.base64.encodeString('\u00FF\u00FF'));
    msg.setOptionalBytes('');
    msg.setOptionalForeignMessage(new proto.jspb.test.ForeignMessage());
    msg.setOptionalForeignMessage(null);
    msg.setOptionalForeignEnum(proto.jspb.test.Proto3Enum.PROTO3_BAR);
    msg.setOptionalForeignEnum(proto.jspb.test.Proto3Enum.PROTO3_FOO);
    msg.setOneofUint32(32);
    msg.clearOneofUint32();


    var serialized = msg.serializeBinary();
    assertEquals(0, serialized.length);
  });

  /**
   * Test that base64 string and Uint8Array are interchangeable in bytes fields.
   */
  it('testBytesFieldsInterop', function() {
    var msg = new proto.jspb.test.TestProto3();
    // Set as a base64 string and check all the getters work.
    msg.setOptionalBytes(BYTES_B64);
    assertTrue(bytesCompare(msg.getOptionalBytes_asU8(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes_asB64(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));

    // Test binary serialize round trip doesn't break it.
    msg = proto.jspb.test.TestProto3.deserializeBinary(msg.serializeBinary());
    assertTrue(bytesCompare(msg.getOptionalBytes_asU8(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes_asB64(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));

    msg = new proto.jspb.test.TestProto3();
    // Set as a Uint8Array and check all the getters work.
    msg.setOptionalBytes(BYTES);
    assertTrue(bytesCompare(msg.getOptionalBytes_asU8(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes_asB64(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));

  });


  it('testTimestampWellKnownType', function() {
    var msg = new proto.google.protobuf.Timestamp();
    msg.fromDate(new Date(123456789));
    assertEquals(123456, msg.getSeconds());
    assertEquals(789000000, msg.getNanos());
    var date = msg.toDate();
    assertEquals(123456789, date.getTime());
    var anotherMsg = proto.google.protobuf.Timestamp.fromDate(date);
    assertEquals(msg.getSeconds(), anotherMsg.getSeconds());
    assertEquals(msg.getNanos(), anotherMsg.getNanos());
  });

  it('testStructWellKnownType', function() {
    var jsObj = {
      abc: "def",
      number: 12345.678,
      nullKey: null,
      boolKey: true,
      listKey: [1, null, true, false, "abc"],
      structKey: {foo: "bar", somenum: 123},
      complicatedKey: [{xyz: {abc: [3, 4, null, false]}}, "zzz"]
    };

    var struct = proto.google.protobuf.Struct.fromJavaScript(jsObj);
    var jsObj2 = struct.toJavaScript();

    assertEquals("def", jsObj2.abc);
    assertEquals(12345.678, jsObj2.number);
    assertEquals(null, jsObj2.nullKey);
    assertEquals(true, jsObj2.boolKey);
    assertEquals("abc", jsObj2.listKey[4]);
    assertEquals("bar", jsObj2.structKey.foo);
    assertEquals(4, jsObj2.complicatedKey[0].xyz.abc[1]);
  });
});
