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

goog.require('goog.testing.asserts');
goog.require('proto.jspb.test.ExtendsWithMessage');
goog.require('proto.jspb.test.ForeignEnum');
goog.require('proto.jspb.test.ForeignMessage');
goog.require('proto.jspb.test.TestAllTypes');
goog.require('proto.jspb.test.TestExtendable');

var suite = {};

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
  msg.setOptionalBytes('bytes');
  msg.setOptionalGroup(new proto.jspb.test.TestAllTypes.OptionalGroup());
  msg.getOptionalGroup().setA(100);
  var submsg = new proto.jspb.test.ForeignMessage();
  submsg.setC(16);
  msg.setOptionalForeignMessage(submsg);
  msg.setOptionalForeignEnum(proto.jspb.test.ForeignEnum.FOREIGN_FOO);
  msg.setOptionalInt32String('-12345');
  msg.setOptionalUint32String('12345');
  msg.setOptionalInt64String('-123456789012345');
  msg.setOptionalUint64String('987654321098765');
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
  msg.setRepeatedBytesList(['bytes']);
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

  msg.setRepeatedInt32StringList(['-12345']);
  msg.setRepeatedUint32StringList(['12345']);
  msg.setRepeatedInt64StringList(['-123456789012345']);
  msg.setRepeatedUint64StringList(['987654321098765']);
  msg.setPackedRepeatedInt32StringList(['-12345']);
  msg.setPackedRepeatedUint32StringList(['12345']);
  msg.setPackedRepeatedInt64StringList(['-123456789012345']);
  msg.setPackedRepeatedUint64StringList(['987654321098765']);
}


/**
 * Helper: compare a bytes field to a string with codepoints 0--255.
 * @param {Uint8Array|string} arr
 * @param {string} str
 * @return {boolean}
 */
function bytesCompare(arr, str) {
  if (arr.length != str.length) {
    return false;
  }
  if (typeof arr == 'string') {
    for (var i = 0; i < arr.length; i++) {
      if (arr.charCodeAt(i) != str.charCodeAt(i)) {
        return false;
      }
    }
    return true;
  } else {
    for (var i = 0; i < arr.length; i++) {
      if (arr[i] != str.charCodeAt(i)) {
        return false;
      }
    }
    return true;
  }
}


/**
 * Helper: verify contents of given TestAllTypes message as set by
 * fillAllFields().
 * @param {proto.jspb.test.TestAllTypes} msg
 */
function checkAllFields(msg) {
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
  assertEquals(true, bytesCompare(msg.getOptionalBytes(), 'bytes'));
  assertEquals(msg.getOptionalGroup().getA(), 100);
  assertEquals(msg.getOptionalForeignMessage().getC(), 16);
  assertEquals(msg.getOptionalForeignEnum(),
      proto.jspb.test.ForeignEnum.FOREIGN_FOO);
  assertEquals(msg.getOptionalInt32String(), '-12345');
  assertEquals(msg.getOptionalUint32String(), '12345');
  assertEquals(msg.getOptionalInt64String(), '-123456789012345');
  assertEquals(msg.getOptionalUint64String(), '987654321098765');
  assertEquals(msg.getOneofString(), 'oneof');
  assertEquals(msg.getOneofFieldCase(),
      proto.jspb.test.TestAllTypes.OneofFieldCase.ONEOF_STRING);

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
  assertEquals(true, bytesCompare(msg.getRepeatedBytesList()[0], 'bytes'));
  assertEquals(msg.getRepeatedGroupList().length, 1);
  assertEquals(msg.getRepeatedGroupList()[0].getA(), 100);
  assertEquals(msg.getRepeatedForeignMessageList().length, 1);
  assertEquals(msg.getRepeatedForeignMessageList()[0].getC(), 1000);
  assertElementsEquals(msg.getRepeatedForeignEnumList(),
      [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);

  assertElementsEquals(msg.getPackedRepeatedInt32List(), [-42]);
  assertElementsEquals(msg.getPackedRepeatedInt64List(),
      [-0x7fffffff00000000]);
  assertElementsEquals(msg.getPackedRepeatedUint32List(), [0x80000000]);
  assertElementsEquals(msg.getPackedRepeatedUint64List(), [0xf000000000000000]);
  assertElementsEquals(msg.getPackedRepeatedSint32List(), [-100]);
  assertElementsEquals(msg.getPackedRepeatedSint64List(),
      [-0x8000000000000000]);
  assertElementsEquals(msg.getPackedRepeatedFixed32List(), [1234]);
  assertElementsEquals(msg.getPackedRepeatedFixed64List(),
      [0x1234567800000000]);
  assertElementsEquals(msg.getPackedRepeatedSfixed32List(), [-1234]);
  assertElementsEquals(msg.getPackedRepeatedSfixed64List(),
      [-0x1234567800000000]);
  assertElementsEquals(msg.getPackedRepeatedFloatList(), [1.5]);
  assertElementsEquals(msg.getPackedRepeatedDoubleList(), [-1.5]);
  assertElementsEquals(msg.getPackedRepeatedBoolList(), [true]);

  assertEquals(msg.getRepeatedInt32StringList().length, 1);
  assertElementsEquals(msg.getRepeatedInt32StringList(), ['-12345']);
  assertEquals(msg.getRepeatedUint32StringList().length, 1);
  assertElementsEquals(msg.getRepeatedUint32StringList(), ['12345']);
  assertEquals(msg.getRepeatedInt64StringList().length, 1);
  assertElementsEquals(msg.getRepeatedInt64StringList(), ['-123456789012345']);
  assertEquals(msg.getRepeatedUint64StringList().length, 1);
  assertElementsEquals(msg.getRepeatedUint64StringList(), ['987654321098765']);

  assertEquals(msg.getPackedRepeatedInt32StringList().length, 1);
  assertElementsEquals(msg.getPackedRepeatedInt32StringList(), ['-12345']);
  assertEquals(msg.getPackedRepeatedUint32StringList().length, 1);
  assertElementsEquals(msg.getPackedRepeatedUint32StringList(), ['12345']);
  assertEquals(msg.getPackedRepeatedInt64StringList().length, 1);
  assertElementsEquals(msg.getPackedRepeatedInt64StringList(),
      ['-123456789012345']);
  assertEquals(msg.getPackedRepeatedUint64StringList().length, 1);
  assertElementsEquals(msg.getPackedRepeatedUint64StringList(),
      ['987654321098765']);
}


/**
 * Helper: verify that all expected extensions are present.
 * @param {!proto.jspb.test.TestExtendable} msg
 */
function checkExtensions(msg) {
  assertEquals(-42,
      msg.getExtension(proto.jspb.test.extendOptionalInt32));
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
  assertEquals(true,
      bytesCompare(msg.getExtension(proto.jspb.test.extendOptionalBytes),
        'bytes'));
  assertEquals(16,
      msg.getExtension(
          proto.jspb.test.ExtendsWithMessage.optionalExtension).getFoo());
  assertEquals(proto.jspb.test.ForeignEnum.FOREIGN_FOO,
      msg.getExtension(proto.jspb.test.extendOptionalForeignEnum));
  assertEquals('-12345',
      msg.getExtension(proto.jspb.test.extendOptionalInt32String));
  assertEquals('12345',
      msg.getExtension(proto.jspb.test.extendOptionalUint32String));
  assertEquals('-123456789012345',
      msg.getExtension(proto.jspb.test.extendOptionalInt64String));
  assertEquals('987654321098765',
      msg.getExtension(proto.jspb.test.extendOptionalUint64String));

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
  assertEquals(true,
      bytesCompare(
          msg.getExtension(proto.jspb.test.extendRepeatedBytesList)[0],
          'bytes'));
  assertEquals(1000,
      msg.getExtension(
          proto.jspb.test.ExtendsWithMessage.repeatedExtensionList)[0]
      .getFoo());
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedForeignEnumList),
      [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);

  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedInt32StringList),
      ['-12345']);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedUint32StringList),
      ['12345']);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedInt64StringList),
      ['-123456789012345']);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendRepeatedUint64StringList),
      ['987654321098765']);

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

  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedInt32StringList),
      ['-12345']);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedUint32StringList),
      ['12345']);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedInt64StringList),
      ['-123456789012345']);
  assertElementsEquals(
      msg.getExtension(proto.jspb.test.extendPackedRepeatedUint64StringList),
      ['987654321098765']);
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
    checkAllFields(decoded);
  });


  /**
   * Helper: fill all extension values.
   * @param {proto.jspb.test.TestExtendable} msg
   */
  function fillExtensions(msg) {
    msg.setExtension(
        proto.jspb.test.extendOptionalInt32, -42);
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
    msg.setExtension(
        proto.jspb.test.extendOptionalBytes, 'bytes');
    var submsg = new proto.jspb.test.ExtendsWithMessage();
    submsg.setFoo(16);
    msg.setExtension(
        proto.jspb.test.ExtendsWithMessage.optionalExtension, submsg);
    msg.setExtension(
        proto.jspb.test.extendOptionalForeignEnum,
        proto.jspb.test.ForeignEnum.FOREIGN_FOO);
    msg.setExtension(
        proto.jspb.test.extendOptionalInt32String, '-12345');
    msg.setExtension(
        proto.jspb.test.extendOptionalUint32String, '12345');
    msg.setExtension(
        proto.jspb.test.extendOptionalInt64String, '-123456789012345');
    msg.setExtension(
        proto.jspb.test.extendOptionalUint64String, '987654321098765');

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
    msg.setExtension(
        proto.jspb.test.extendRepeatedBytesList, ['bytes']);
    submsg = new proto.jspb.test.ExtendsWithMessage();
    submsg.setFoo(1000);
    msg.setExtension(
        proto.jspb.test.ExtendsWithMessage.repeatedExtensionList, [submsg]);
    msg.setExtension(proto.jspb.test.extendRepeatedForeignEnumList,
        [proto.jspb.test.ForeignEnum.FOREIGN_FOO]);

    msg.setExtension(
        proto.jspb.test.extendRepeatedInt32StringList, ['-12345']);
    msg.setExtension(
        proto.jspb.test.extendRepeatedUint32StringList, ['12345']);
    msg.setExtension(
        proto.jspb.test.extendRepeatedInt64StringList, ['-123456789012345']);
    msg.setExtension(
        proto.jspb.test.extendRepeatedUint64StringList, ['987654321098765']);

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

    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedInt32StringList,
        ['-12345']);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedUint32StringList,
        ['12345']);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedInt64StringList,
        ['-123456789012345']);
    msg.setExtension(
        proto.jspb.test.extendPackedRepeatedUint64StringList,
        ['987654321098765']);
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
});
