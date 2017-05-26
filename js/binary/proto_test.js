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

// Test suite is written using Jasmine -- see http://jasmine.github.io/

goog.require('goog.crypt.base64');
goog.require('goog.testing.asserts');
goog.require('jspb.BinaryWriter');
goog.require('jspb.Message');

// CommonJS-LoadFromFile: ../testbinary_pb proto.jspb.test
goog.require('proto.jspb.test.ExtendsWithMessage');
goog.require('proto.jspb.test.ForeignEnum');
goog.require('proto.jspb.test.ForeignMessage');
goog.require('proto.jspb.test.TestAllTypes');
goog.require('proto.jspb.test.TestExtendable');
goog.require('proto.jspb.test.extendOptionalBool');
goog.require('proto.jspb.test.extendOptionalBytes');
goog.require('proto.jspb.test.extendOptionalDouble');
goog.require('proto.jspb.test.extendOptionalFixed32');
goog.require('proto.jspb.test.extendOptionalFixed64');
goog.require('proto.jspb.test.extendOptionalFloat');
goog.require('proto.jspb.test.extendOptionalForeignEnum');
goog.require('proto.jspb.test.extendOptionalInt32');
goog.require('proto.jspb.test.extendOptionalInt64');
goog.require('proto.jspb.test.extendOptionalSfixed32');
goog.require('proto.jspb.test.extendOptionalSfixed64');
goog.require('proto.jspb.test.extendOptionalSint32');
goog.require('proto.jspb.test.extendOptionalSint64');
goog.require('proto.jspb.test.extendOptionalString');
goog.require('proto.jspb.test.extendOptionalUint32');
goog.require('proto.jspb.test.extendOptionalUint64');
goog.require('proto.jspb.test.extendPackedRepeatedBoolList');
goog.require('proto.jspb.test.extendPackedRepeatedDoubleList');
goog.require('proto.jspb.test.extendPackedRepeatedFixed32List');
goog.require('proto.jspb.test.extendPackedRepeatedFixed64List');
goog.require('proto.jspb.test.extendPackedRepeatedFloatList');
goog.require('proto.jspb.test.extendPackedRepeatedForeignEnumList');
goog.require('proto.jspb.test.extendPackedRepeatedInt32List');
goog.require('proto.jspb.test.extendPackedRepeatedInt64List');
goog.require('proto.jspb.test.extendPackedRepeatedSfixed32List');
goog.require('proto.jspb.test.extendPackedRepeatedSfixed64List');
goog.require('proto.jspb.test.extendPackedRepeatedSint32List');
goog.require('proto.jspb.test.extendPackedRepeatedSint64List');
goog.require('proto.jspb.test.extendPackedRepeatedUint32List');
goog.require('proto.jspb.test.extendPackedRepeatedUint64List');
goog.require('proto.jspb.test.extendRepeatedBoolList');
goog.require('proto.jspb.test.extendRepeatedBytesList');
goog.require('proto.jspb.test.extendRepeatedDoubleList');
goog.require('proto.jspb.test.extendRepeatedFixed32List');
goog.require('proto.jspb.test.extendRepeatedFixed64List');
goog.require('proto.jspb.test.extendRepeatedFloatList');
goog.require('proto.jspb.test.extendRepeatedForeignEnumList');
goog.require('proto.jspb.test.extendRepeatedInt32List');
goog.require('proto.jspb.test.extendRepeatedInt64List');
goog.require('proto.jspb.test.extendRepeatedSfixed32List');
goog.require('proto.jspb.test.extendRepeatedSfixed64List');
goog.require('proto.jspb.test.extendRepeatedSint32List');
goog.require('proto.jspb.test.extendRepeatedSint64List');
goog.require('proto.jspb.test.extendRepeatedStringList');
goog.require('proto.jspb.test.extendRepeatedUint32List');
goog.require('proto.jspb.test.extendRepeatedUint64List');

// CommonJS-LoadFromFile: ../node_modules/google-protobuf/google/protobuf/any_pb proto.google.protobuf
goog.require('proto.google.protobuf.Any');


var suite = {};

var BYTES = new Uint8Array([1, 2, 8, 9]);

var BYTES_B64 = goog.crypt.base64.encodeByteArray(BYTES);


/**
 * Helper: fill all fields on a TestAllTypes message.
 * @param {proto.jspb.test.TestAllTypes} msg
 */
function fillAllFields(msg) {
  msg.setOptionalInt32(-42);
  // can be exactly represented by JS number (64-bit double, i.e., 52-bit
  // mantissa).
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
  msg.setOptionalGroup(new proto.jspb.test.TestAllTypes.OptionalGroup());
  msg.getOptionalGroup().setA(100);
  var submsg = new proto.jspb.test.ForeignMessage();
  submsg.setC(16);
  msg.setOptionalForeignMessage(submsg);
  msg.setOptionalForeignEnum(proto.jspb.test.ForeignEnum.FOREIGN_FOO);
  msg.setOneofString('oneof');


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
  msg.setRepeatedBytesList([BYTES, BYTES]);
  msg.setRepeatedGroupList([new proto.jspb.test.TestAllTypes.RepeatedGroup()]);
  msg.getRepeatedGroupList()[0].setA(100);
  submsg = new proto.jspb.test.ForeignMessage();
  submsg.setC(1000);
  msg.setRepeatedForeignMessageList([submsg]);
  msg.setRepeatedForeignEnumList([proto.jspb.test.ForeignEnum.FOREIGN_FOO]);

  msg.setPackedRepeatedInt32List([-42]);
  msg.setPackedRepeatedInt64List([-0x7fffffff00000000]);
  msg.setPackedRepeatedUint32List([0x80000000]);
  msg.setPackedRepeatedUint64List([0xf000000000000000]);
  msg.setPackedRepeatedSint32List([-100]);
  msg.setPackedRepeatedSint64List([-0x8000000000000000]);
  msg.setPackedRepeatedFixed32List([1234]);
  msg.setPackedRepeatedFixed64List([0x1234567800000000]);
  msg.setPackedRepeatedSfixed32List([-1234]);
  msg.setPackedRepeatedSfixed64List([-0x1234567800000000]);
  msg.setPackedRepeatedFloatList([1.5]);
  msg.setPackedRepeatedDoubleList([-1.5]);
  msg.setPackedRepeatedBoolList([true]);

}


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


/**
 * Helper: verify contents of given TestAllTypes message as set by
 * fillAllFields().
 * @param {proto.jspb.test.TestAllTypes} original
 * @param {proto.jspb.test.TestAllTypes} copy
 */
function checkAllFields(original, copy) {
  assertEquals(copy.getOptionalInt32(), -42);
  assertEquals(copy.getOptionalInt64(), -0x7fffffff00000000);
  assertEquals(copy.getOptionalUint32(), 0x80000000);
  assertEquals(copy.getOptionalUint64(), 0xf000000000000000);
  assertEquals(copy.getOptionalSint32(), -100);
  assertEquals(copy.getOptionalSint64(), -0x8000000000000000);
  assertEquals(copy.getOptionalFixed32(), 1234);
  assertEquals(copy.getOptionalFixed64(), 0x1234567800000000);
  assertEquals(copy.getOptionalSfixed32(), -1234);
  assertEquals(copy.getOptionalSfixed64(), -0x1234567800000000);
  assertEquals(copy.getOptionalFloat(), 1.5);
  assertEquals(copy.getOptionalDouble(), -1.5);
  assertEquals(copy.getOptionalBool(), true);
  assertEquals(copy.getOptionalString(), 'hello world');
  assertEquals(true, bytesCompare(copy.getOptionalBytes(), BYTES));
  assertEquals(true, bytesCompare(copy.getOptionalBytes_asU8(), BYTES));
  assertEquals(
      copy.getOptionalBytes_asB64(), goog.crypt.base64.encodeByteArray(BYTES));

  assertEquals(copy.getOptionalGroup().getA(), 100);
  assertEquals(copy.getOptionalForeignMessage().getC(), 16);
  assertEquals(copy.getOptionalForeignEnum(),
      proto.jspb.test.ForeignEnum.FOREIGN_FOO);


  assertEquals(copy.getOneofString(), 'oneof');
  assertEquals(copy.getOneofFieldCase(),
      proto.jspb.test.TestAllTypes.OneofFieldCase.ONEOF_STRING);

  assertElementsEquals(copy.getRepeatedInt32List(), [-42]);
  assertElementsEquals(copy.getRepeatedInt64List(), [-0x7fffffff00000000]);
  assertElementsEquals(copy.getRepeatedUint32List(), [0x80000000]);
  assertElementsEquals(copy.getRepeatedUint64List(), [0xf000000000000000]);
  assertElementsEquals(copy.getRepeatedSint32List(), [-100]);
  assertElementsEquals(copy.getRepeatedSint64List(), [-0x8000000000000000]);
  assertElementsEquals(copy.getRepeatedFixed32List(), [1234]);
  assertElementsEquals(copy.getRepeatedFixed64List(), [0x1234567800000000]);
  assertElementsEquals(copy.getRepeatedSfixed32List(), [-1234]);
  assertElementsEquals(copy.getRepeatedSfixed64List(), [-0x1234567800000000]);
  assertElementsEquals(copy.getRepeatedFloatList(), [1.5]);
  assertElementsEquals(copy.getRepeatedDoubleList(), [-1.5]);
  assertElementsEquals(copy.getRepeatedBoolList(), [true]);
  assertElementsEquals(copy.getRepeatedStringList(), ['hello world']);
  assertEquals(copy.getRepeatedBytesList().length, 2);
  assertEquals(true, bytesCompare(copy.getRepeatedBytesList_asU8()[0], BYTES));
  assertEquals(true, bytesCompare(copy.getRepeatedBytesList()[0], BYTES));
  assertEquals(true, bytesCompare(copy.getRepeatedBytesList_asU8()[1], BYTES));
  assertEquals(copy.getRepeatedBytesList_asB64()[0], BYTES_B64);
  assertEquals(copy.getRepeatedBytesList_asB64()[1], BYTES_B64);
  assertEquals(copy.getRepeatedGroupList().length, 1);
  assertEquals(copy.getRepeatedGroupList()[0].getA(), 100);
  assertEquals(copy.getRepeatedForeignMessageList().length, 1);
  assertEquals(copy.getRepeatedForeignMessageList()[0].getC(), 1000);
  assertElementsEquals(copy.getRepeatedForeignEnumList(),
      [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);

  assertElementsEquals(copy.getPackedRepeatedInt32List(), [-42]);
  assertElementsEquals(copy.getPackedRepeatedInt64List(),
      [-0x7fffffff00000000]);
  assertElementsEquals(copy.getPackedRepeatedUint32List(), [0x80000000]);
  assertElementsEquals(copy.getPackedRepeatedUint64List(),
      [0xf000000000000000]);
  assertElementsEquals(copy.getPackedRepeatedSint32List(), [-100]);
  assertElementsEquals(copy.getPackedRepeatedSint64List(),
      [-0x8000000000000000]);
  assertElementsEquals(copy.getPackedRepeatedFixed32List(), [1234]);
  assertElementsEquals(copy.getPackedRepeatedFixed64List(),
      [0x1234567800000000]);
  assertElementsEquals(copy.getPackedRepeatedSfixed32List(), [-1234]);
  assertElementsEquals(copy.getPackedRepeatedSfixed64List(),
      [-0x1234567800000000]);
  assertElementsEquals(copy.getPackedRepeatedFloatList(), [1.5]);
  assertElementsEquals(copy.getPackedRepeatedDoubleList(), [-1.5]);


  // Check last so we get more granular errors first.
  assertTrue(jspb.Message.equals(original, copy));
}


/**
 * Helper: verify that all expected extensions are present.
 * @param {!proto.jspb.test.TestExtendable} msg
 */
function checkExtensions(msg) {
  assertEquals(0, msg.getExtension(proto.jspb.test.extendOptionalInt32));
  assertEquals(-0x7fffffff00000000,
      msg.getExtension(proto.jspb.test.extendOptionalInt64));
  assertEquals(0x80000000,
      msg.getExtension(proto.jspb.test.extendOptionalUint32));
  assertEquals(0xf000000000000000,
      msg.getExtension(proto.jspb.test.extendOptionalUint64));
  assertEquals(-100,
      msg.getExtension(proto.jspb.test.extendOptionalSint32));
  assertEquals(-0x8000000000000000,
      msg.getExtension(proto.jspb.test.extendOptionalSint64));
  assertEquals(1234,
      msg.getExtension(proto.jspb.test.extendOptionalFixed32));
  assertEquals(0x1234567800000000,
      msg.getExtension(proto.jspb.test.extendOptionalFixed64));
  assertEquals(-1234,
      msg.getExtension(proto.jspb.test.extendOptionalSfixed32));
  assertEquals(-0x1234567800000000,
      msg.getExtension(proto.jspb.test.extendOptionalSfixed64));
  assertEquals(1.5,
      msg.getExtension(proto.jspb.test.extendOptionalFloat));
  assertEquals(-1.5,
      msg.getExtension(proto.jspb.test.extendOptionalDouble));
  assertEquals(true,
      msg.getExtension(proto.jspb.test.extendOptionalBool));
  assertEquals('hello world',
      msg.getExtension(proto.jspb.test.extendOptionalString));
  assertEquals(
      true, bytesCompare(
                msg.getExtension(proto.jspb.test.extendOptionalBytes), BYTES));
  assertEquals(16,
      msg.getExtension(
          proto.jspb.test.ExtendsWithMessage.optionalExtension).getFoo());


  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedInt32List),
      [-42]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedInt64List),
      [-0x7fffffff00000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedUint32List),
      [0x80000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedUint64List),
      [0xf000000000000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedSint32List),
      [-100]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedSint64List),
      [-0x8000000000000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedFixed32List),
      [1234]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedFixed64List),
      [0x1234567800000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedSfixed32List),
      [-1234]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedSfixed64List),
      [-0x1234567800000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedFloatList),
      [1.5]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedDoubleList),
      [-1.5]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedBoolList),
      [true]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedStringList),
      ['hello world']);
  assertEquals(
      true,
      bytesCompare(
          msg.getExtension(proto.jspb.test.extendRepeatedBytesList)[0], BYTES));
  assertEquals(1000,
      msg.getExtension(
          proto.jspb.test.ExtendsWithMessage.repeatedExtensionList)[0]
      .getFoo());
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedForeignEnumList),
      [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);


  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedInt32List),
      [-42]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedInt64List),
      [-0x7fffffff00000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedUint32List),
      [0x80000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedUint64List),
      [0xf000000000000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedSint32List),
      [-100]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedSint64List),
      [-0x8000000000000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedFixed32List),
      [1234]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedFixed64List),
      [0x1234567800000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedSfixed32List),
      [-1234]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedSfixed64List),
      [-0x1234567800000000]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedFloatList),
      [1.5]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedDoubleList),
      [-1.5]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedBoolList),
      [true]);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedForeignEnumList),
      [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);

}


describe('protoBinaryTest', function() {
  /**
   * Tests a basic serialization-deserializaton round-trip with all supported
   * field types (on the TestAllTypes message type).
   */
  it('testRoundTrip', function() {
    var msg = new proto.jspb.test.TestAllTypes();
    fillAllFields(msg);
    var encoded = msg.serializeBinary();
    var decoded = proto.jspb.test.TestAllTypes.deserializeBinary(encoded);
    checkAllFields(msg, decoded);
  });

  /**
   * Test that base64 string and Uint8Array are interchangeable in bytes fields.
   */
  it('testBytesFieldsGettersInterop', function() {
    var msg = new proto.jspb.test.TestAllTypes();
    // Set from a base64 string and check all the getters work.
    msg.setOptionalBytes(BYTES_B64);
    assertTrue(bytesCompare(msg.getOptionalBytes_asU8(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes_asB64(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));

    // Test binary serialize round trip doesn't break it.
    msg = proto.jspb.test.TestAllTypes.deserializeBinary(msg.serializeBinary());
    assertTrue(bytesCompare(msg.getOptionalBytes_asU8(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes_asB64(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));

    msg = new proto.jspb.test.TestAllTypes();
    // Set from a Uint8Array and check all the getters work.
    msg.setOptionalBytes(BYTES);
    assertTrue(bytesCompare(msg.getOptionalBytes_asU8(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes_asB64(), BYTES));
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));

  });

  /**
   * Test that bytes setters will receive result of any of the getters.
   */
  it('testBytesFieldsSettersInterop', function() {
    var msg = new proto.jspb.test.TestAllTypes();
    msg.setOptionalBytes(BYTES);
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));

    msg.setOptionalBytes(msg.getOptionalBytes());
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));
    msg.setOptionalBytes(msg.getOptionalBytes_asB64());
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));
    msg.setOptionalBytes(msg.getOptionalBytes_asU8());
    assertTrue(bytesCompare(msg.getOptionalBytes(), BYTES));
  });

  /**
   * Test that bytes setters will receive result of any of the getters.
   */
  it('testRepeatedBytesGetters', function() {
    var msg = new proto.jspb.test.TestAllTypes();

    function assertGetters() {
      assertTrue(goog.isString(msg.getRepeatedBytesList_asB64()[0]));
      assertTrue(goog.isString(msg.getRepeatedBytesList_asB64()[1]));
      assertTrue(msg.getRepeatedBytesList_asU8()[0] instanceof Uint8Array);
      assertTrue(msg.getRepeatedBytesList_asU8()[1] instanceof Uint8Array);

      assertTrue(bytesCompare(msg.getRepeatedBytesList()[0], BYTES));
      assertTrue(bytesCompare(msg.getRepeatedBytesList()[1], BYTES));
      assertTrue(bytesCompare(msg.getRepeatedBytesList_asB64()[0], BYTES));
      assertTrue(bytesCompare(msg.getRepeatedBytesList_asB64()[1], BYTES));
      assertTrue(bytesCompare(msg.getRepeatedBytesList_asU8()[0], BYTES));
      assertTrue(bytesCompare(msg.getRepeatedBytesList_asU8()[1], BYTES));
    }

    msg.setRepeatedBytesList([BYTES, BYTES]);
    assertGetters();

    msg.setRepeatedBytesList([BYTES_B64, BYTES_B64]);
    assertGetters();

    msg.setRepeatedBytesList([]);
    assertEquals(0, msg.getRepeatedBytesList().length);
    assertEquals(0, msg.getRepeatedBytesList_asB64().length);
    assertEquals(0, msg.getRepeatedBytesList_asU8().length);
  });

  /**
   * Helper: fill all extension values.
   * @param {proto.jspb.test.TestExtendable} msg
   */
  function fillExtensions(msg) {
    msg.setExtension(proto.jspb.test.extendOptionalInt32, 0);
    msg.setExtension(
        proto.jspb.test.extendOptionalInt64, -0x7fffffff00000000);
    msg.setExtension(
        proto.jspb.test.extendOptionalUint32, 0x80000000);
    msg.setExtension(
        proto.jspb.test.extendOptionalUint64, 0xf000000000000000);
    msg.setExtension(
        proto.jspb.test.extendOptionalSint32, -100);
    msg.setExtension(
        proto.jspb.test.extendOptionalSint64, -0x8000000000000000);
    msg.setExtension(
        proto.jspb.test.extendOptionalFixed32, 1234);
    msg.setExtension(
        proto.jspb.test.extendOptionalFixed64, 0x1234567800000000);
    msg.setExtension(
        proto.jspb.test.extendOptionalSfixed32, -1234);
    msg.setExtension(
        proto.jspb.test.extendOptionalSfixed64, -0x1234567800000000);
    msg.setExtension(
        proto.jspb.test.extendOptionalFloat, 1.5);
    msg.setExtension(
        proto.jspb.test.extendOptionalDouble, -1.5);
    msg.setExtension(
        proto.jspb.test.extendOptionalBool, true);
    msg.setExtension(
        proto.jspb.test.extendOptionalString, 'hello world');
    msg.setExtension(proto.jspb.test.extendOptionalBytes, BYTES);
    var submsg = new proto.jspb.test.ExtendsWithMessage();
    submsg.setFoo(16);
    msg.setExtension(
        proto.jspb.test.ExtendsWithMessage.optionalExtension, submsg);
    msg.setExtension(
        proto.jspb.test.extendOptionalForeignEnum,
        proto.jspb.test.ForeignEnum.FOREIGN_FOO);


    msg.setExtension(
        proto.jspb.test.extendRepeatedInt32List, [-42]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedInt64List, [-0x7fffffff00000000]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedUint32List, [0x80000000]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedUint64List, [0xf000000000000000]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedSint32List, [-100]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedSint64List, [-0x8000000000000000]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedFixed32List, [1234]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedFixed64List, [0x1234567800000000]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedSfixed32List, [-1234]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedSfixed64List, [-0x1234567800000000]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedFloatList, [1.5]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedDoubleList, [-1.5]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedBoolList, [true]);
    msg.setExtension(
        proto.jspb.test.extendRepeatedStringList, ['hello world']);
    msg.setExtension(proto.jspb.test.extendRepeatedBytesList, [BYTES]);
    submsg = new proto.jspb.test.ExtendsWithMessage();
    submsg.setFoo(1000);
    msg.setExtension(
        proto.jspb.test.ExtendsWithMessage.repeatedExtensionList, [submsg]);
    msg.setExtension(proto.jspb.test.extendRepeatedForeignEnumList,
        [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);


    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedInt32List, [-42]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedInt64List, [-0x7fffffff00000000]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedUint32List, [0x80000000]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedUint64List, [0xf000000000000000]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedSint32List, [-100]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedSint64List, [-0x8000000000000000]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedFixed32List, [1234]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedFixed64List, [0x1234567800000000]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedSfixed32List, [-1234]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedSfixed64List,
        [-0x1234567800000000]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedFloatList, [1.5]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedDoubleList, [-1.5]);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedBoolList, [true]);
    msg.setExtension(proto.jspb.test.extendPackedRepeatedForeignEnumList,
        [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);

  }


  /**
   * Tests extension serialization and deserialization.
   */
  it('testExtensions', function() {
    var msg = new proto.jspb.test.TestExtendable();
    fillExtensions(msg);
    var encoded = msg.serializeBinary();
    var decoded = proto.jspb.test.TestExtendable.deserializeBinary(encoded);
    checkExtensions(decoded);
  });

  /**
   * Tests that unknown extensions don't cause deserialization failure.
   */
  it('testUnknownExtension', function() {
    var msg = new proto.jspb.test.TestExtendable();
    fillExtensions(msg);
    var writer = new jspb.BinaryWriter();
    writer.writeBool((1 << 29) - 1, true);
    proto.jspb.test.TestExtendable.serializeBinaryToWriter(msg, writer);
    var encoded = writer.getResultBuffer();
    var decoded = proto.jspb.test.TestExtendable.deserializeBinary(encoded);
    checkExtensions(decoded);
  });

  it('testAnyWellKnownType', function() {
    var any = new proto.google.protobuf.Any();
    var msg = new proto.jspb.test.TestAllTypes();

    fillAllFields(msg);

    any.pack(msg.serializeBinary(), 'jspb.test.TestAllTypes');

    assertEquals('type.googleapis.com/jspb.test.TestAllTypes',
                 any.getTypeUrl());

    var msg2 = any.unpack(
        proto.jspb.test.TestAllTypes.deserializeBinary,
        'jspb.test.TestAllTypes');

    checkAllFields(msg, msg2);
  });
});
