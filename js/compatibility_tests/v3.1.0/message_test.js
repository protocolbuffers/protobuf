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

goog.setTestOnly();

goog.require('goog.json');
goog.require('goog.testing.asserts');
goog.require('goog.userAgent');

// CommonJS-LoadFromFile: google-protobuf jspb
goog.require('jspb.Message');

// CommonJS-LoadFromFile: test5_pb proto.jspb.exttest.beta
goog.require('proto.jspb.exttest.beta.floatingStrField');

// CommonJS-LoadFromFile: test3_pb proto.jspb.exttest
goog.require('proto.jspb.exttest.floatingMsgField');

// CommonJS-LoadFromFile: test4_pb proto.jspb.exttest
goog.require('proto.jspb.exttest.floatingMsgFieldTwo');

// CommonJS-LoadFromFile: test_pb proto.jspb.test
goog.require('proto.jspb.test.CloneExtension');
goog.require('proto.jspb.test.Complex');
goog.require('proto.jspb.test.DefaultValues');
goog.require('proto.jspb.test.Empty');
goog.require('proto.jspb.test.EnumContainer');
goog.require('proto.jspb.test.floatingMsgField');
goog.require('proto.jspb.test.FloatingPointFields');
goog.require('proto.jspb.test.floatingStrField');
goog.require('proto.jspb.test.HasExtensions');
goog.require('proto.jspb.test.IndirectExtension');
goog.require('proto.jspb.test.IsExtension');
goog.require('proto.jspb.test.OptionalFields');
goog.require('proto.jspb.test.OuterEnum');
goog.require('proto.jspb.test.OuterMessage.Complex');
goog.require('proto.jspb.test.Simple1');
goog.require('proto.jspb.test.Simple2');
goog.require('proto.jspb.test.SpecialCases');
goog.require('proto.jspb.test.TestClone');
goog.require('proto.jspb.test.TestEndsWithBytes');
goog.require('proto.jspb.test.TestGroup');
goog.require('proto.jspb.test.TestGroup1');
goog.require('proto.jspb.test.TestMessageWithOneof');
goog.require('proto.jspb.test.TestReservedNames');
goog.require('proto.jspb.test.TestReservedNamesExtension');

// CommonJS-LoadFromFile: test2_pb proto.jspb.test
goog.require('proto.jspb.test.ExtensionMessage');
goog.require('proto.jspb.test.TestExtensionsMessage');




describe('Message test suite', function() {
  it('testEmptyProto', function() {
    var empty1 = new proto.jspb.test.Empty([]);
    var empty2 = new proto.jspb.test.Empty([]);
    assertObjectEquals({}, empty1.toObject());
    assertObjectEquals('Message should not be corrupted:', empty2, empty1);
  });

  it('testTopLevelEnum', function() {
    var response = new proto.jspb.test.EnumContainer([]);
    response.setOuterEnum(proto.jspb.test.OuterEnum.FOO);
    assertEquals(proto.jspb.test.OuterEnum.FOO, response.getOuterEnum());
  });

  it('testByteStrings', function() {
    var data = new proto.jspb.test.DefaultValues([]);
    data.setBytesField('some_bytes');
    assertEquals('some_bytes', data.getBytesField());
  });

  it('testComplexConversion', function() {
    var data1 = ['a',,, [, 11], [[, 22], [, 33]],, ['s1', 's2'],, 1];
    var data2 = ['a',,, [, 11], [[, 22], [, 33]],, ['s1', 's2'],, 1];
    var foo = new proto.jspb.test.Complex(data1);
    var bar = new proto.jspb.test.Complex(data2);
    var result = foo.toObject();
    assertObjectEquals({
      aString: 'a',
      anOutOfOrderBool: 1,
      aNestedMessage: {
        anInt: 11
      },
      aRepeatedMessageList: [{anInt: 22}, {anInt: 33}],
      aRepeatedStringList: ['s1', 's2']
    }, result);

    // Now test with the jspb instances included.
    result = foo.toObject(true /* opt_includeInstance */);
    assertObjectEquals({
      aString: 'a',
      anOutOfOrderBool: 1,
      aNestedMessage: {
        anInt: 11,
        $jspbMessageInstance: foo.getANestedMessage()
      },
      aRepeatedMessageList: [
        {anInt: 22, $jspbMessageInstance: foo.getARepeatedMessageList()[0]},
        {anInt: 33, $jspbMessageInstance: foo.getARepeatedMessageList()[1]}
      ],
      aRepeatedStringList: ['s1', 's2'],
      $jspbMessageInstance: foo
    }, result);

  });

  it('testMissingFields', function() {
    var foo = new proto.jspb.test.Complex([
        undefined, undefined, undefined, [],
        undefined, undefined, undefined, undefined]);
    var bar = new proto.jspb.test.Complex([
        undefined, undefined, undefined, [],
        undefined, undefined, undefined, undefined]);
    var result = foo.toObject();
    assertObjectEquals({
      aString: undefined,
      anOutOfOrderBool: undefined,
      aNestedMessage: {
        anInt: undefined
      },
      // Note: JsPb converts undefined repeated fields to empty arrays.
      aRepeatedMessageList: [],
      aRepeatedStringList: []
    }, result);

  });

  it('testNestedComplexMessage', function() {
    // Instantiate the message and set a unique field, just to ensure that we
    // are not getting jspb.test.Complex instead.
    var msg = new proto.jspb.test.OuterMessage.Complex();
    msg.setInnerComplexField(5);
  });

  it('testSpecialCases', function() {
    // Note: Some property names are reserved in JavaScript.
    // These names are converted to the Js property named pb_<reserved_name>.
    var special =
        new proto.jspb.test.SpecialCases(['normal', 'default', 'function',
        'var']);
    var result = special.toObject();
    assertObjectEquals({
      normal: 'normal',
      pb_default: 'default',
      pb_function: 'function',
      pb_var: 'var'
    }, result);
  });

  it('testDefaultValues', function() {
    var defaultString = "default<>\'\"abc";
    var response = new proto.jspb.test.DefaultValues();

    // Test toObject
    var expectedObject = {
      stringField: defaultString,
      boolField: true,
      intField: 11,
      enumField: 13,
      emptyField: '',
      bytesField: 'bW9v'
    };
    assertObjectEquals(expectedObject, response.toObject());


    // Test getters
    response = new proto.jspb.test.DefaultValues();
    assertEquals(defaultString, response.getStringField());
    assertEquals(true, response.getBoolField());
    assertEquals(11, response.getIntField());
    assertEquals(13, response.getEnumField());
    assertEquals('', response.getEmptyField());
    assertEquals('bW9v', response.getBytesField());

    function makeDefault(values) {
      return new proto.jspb.test.DefaultValues(values);
    }

    // Test with undefined values,
    // Use push to workaround IE treating undefined array elements as holes.
    response = makeDefault([undefined, undefined, undefined, undefined]);
    assertEquals(defaultString, response.getStringField());
    assertEquals(true, response.getBoolField());
    assertEquals(11, response.getIntField());
    assertEquals(13, response.getEnumField());
    assertFalse(response.hasStringField());
    assertFalse(response.hasBoolField());
    assertFalse(response.hasIntField());
    assertFalse(response.hasEnumField());

    // Test with null values, as would be returned by a JSON serializer.
    response = makeDefault([null, null, null, null]);
    assertEquals(defaultString, response.getStringField());
    assertEquals(true, response.getBoolField());
    assertEquals(11, response.getIntField());
    assertEquals(13, response.getEnumField());
    assertFalse(response.hasStringField());
    assertFalse(response.hasBoolField());
    assertFalse(response.hasIntField());
    assertFalse(response.hasEnumField());

    // Test with false-like values.
    response = makeDefault(['', false, 0, 0]);
    assertEquals('', response.getStringField());
    assertEquals(false, response.getBoolField());
    assertEquals(true, response.getIntField() == 0);
    assertEquals(true, response.getEnumField() == 0);
    assertTrue(response.hasStringField());
    assertTrue(response.hasBoolField());
    assertTrue(response.hasIntField());
    assertTrue(response.hasEnumField());

    // Test that clearing the values reverts them to the default state.
    response = makeDefault(['blah', false, 111, 77]);
    response.clearStringField(); response.clearBoolField();
    response.clearIntField(); response.clearEnumField();
    assertEquals(defaultString, response.getStringField());
    assertEquals(true, response.getBoolField());
    assertEquals(11, response.getIntField());
    assertEquals(13, response.getEnumField());
    assertFalse(response.hasStringField());
    assertFalse(response.hasBoolField());
    assertFalse(response.hasIntField());
    assertFalse(response.hasEnumField());

    // Test that setFoo(null) clears the values.
    response = makeDefault(['blah', false, 111, 77]);
    response.setStringField(null); response.setBoolField(null);
    response.setIntField(undefined); response.setEnumField(undefined);
    assertEquals(defaultString, response.getStringField());
    assertEquals(true, response.getBoolField());
    assertEquals(11, response.getIntField());
    assertEquals(13, response.getEnumField());
    assertFalse(response.hasStringField());
    assertFalse(response.hasBoolField());
    assertFalse(response.hasIntField());
    assertFalse(response.hasEnumField());
  });

  it('testClearFields', function() {
    var data = ['str', true, [11], [[22], [33]], ['s1', 's2']];
    var foo = new proto.jspb.test.OptionalFields(data);
    foo.clearAString();
    foo.clearABool();
    foo.clearANestedMessage();
    foo.clearARepeatedMessageList();
    foo.clearARepeatedStringList();
    assertEquals('', foo.getAString());
    assertEquals(false, foo.getABool());
    assertUndefined(foo.getANestedMessage());
    assertFalse(foo.hasAString());
    assertFalse(foo.hasABool());
    assertObjectEquals([], foo.getARepeatedMessageList());
    assertObjectEquals([], foo.getARepeatedStringList());
    // NOTE: We want the missing fields in 'expected' to be undefined,
    // but we actually get a sparse array instead. We could use something
    // like [1,undefined,2] to avoid this, except that this is still
    // sparse on IE. No comment...
    var expected = [,,, [], []];
    expected[0] = expected[1] = expected[2] = undefined;
    assertObjectEquals(expected, foo.toArray());
  });

  it('testDifferenceRawObject', /** @suppress {visibility} */ function() {
    var p1 = new proto.jspb.test.HasExtensions(['hi', 'diff', {}]);
    var p2 = new proto.jspb.test.HasExtensions(['hi', 'what',
                                               {1000: 'unique'}]);
    var diff = /** @type {proto.jspb.test.HasExtensions} */
        (jspb.Message.difference(p1, p2));
    assertEquals('', diff.getStr1());
    assertEquals('what', diff.getStr2());
    assertEquals('', diff.getStr3());
    assertEquals('unique', diff.extensionObject_[1000]);
  });

  it('testEqualsSimple', function() {
    var s1 = new proto.jspb.test.Simple1(['hi']);
    assertTrue(jspb.Message.equals(s1, new proto.jspb.test.Simple1(['hi'])));
    assertFalse(jspb.Message.equals(s1, new proto.jspb.test.Simple1(['bye'])));
    var s1b = new proto.jspb.test.Simple1(['hi', ['hello']]);
    assertTrue(jspb.Message.equals(s1b,
        new proto.jspb.test.Simple1(['hi', ['hello']])));
    assertTrue(jspb.Message.equals(s1b,
        new proto.jspb.test.Simple1(['hi', ['hello', undefined,
                                            undefined, undefined]])));
    assertFalse(jspb.Message.equals(s1b,
        new proto.jspb.test.Simple1(['no', ['hello']])));
    // Test with messages of different types
    var s2 = new proto.jspb.test.Simple2(['hi']);
    assertFalse(jspb.Message.equals(s1, s2));
  });

  it('testEquals_softComparison', function() {
    var s1 = new proto.jspb.test.Simple1(['hi', [], null]);
    assertTrue(jspb.Message.equals(s1,
        new proto.jspb.test.Simple1(['hi', []])));

    var s1b = new proto.jspb.test.Simple1(['hi', [], true]);
    assertTrue(jspb.Message.equals(s1b,
        new proto.jspb.test.Simple1(['hi', [], 1])));
  });

  it('testEqualsComplex', function() {
    var data1 = ['a',,, [, 11], [[, 22], [, 33]],, ['s1', 's2'],, 1];
    var data2 = ['a',,, [, 11], [[, 22], [, 34]],, ['s1', 's2'],, 1];
    var data3 = ['a',,, [, 11], [[, 22]],, ['s1', 's2'],, 1];
    var data4 = ['hi'];
    var c1a = new proto.jspb.test.Complex(data1);
    var c1b = new proto.jspb.test.Complex(data1);
    var c2 = new proto.jspb.test.Complex(data2);
    var c3 = new proto.jspb.test.Complex(data3);
    var s1 = new proto.jspb.test.Simple1(data4);

    assertTrue(jspb.Message.equals(c1a, c1b));
    assertFalse(jspb.Message.equals(c1a, c2));
    assertFalse(jspb.Message.equals(c2, c3));
    assertFalse(jspb.Message.equals(c1a, s1));
  });

  it('testEqualsExtensionsConstructed', function() {
    assertTrue(jspb.Message.equals(
        new proto.jspb.test.HasExtensions([]),
        new proto.jspb.test.HasExtensions([{}])
    ));
    assertTrue(jspb.Message.equals(
        new proto.jspb.test.HasExtensions(['hi', {100: [{200: 'a'}]}]),
        new proto.jspb.test.HasExtensions(['hi', {100: [{200: 'a'}]}])
    ));
    assertFalse(jspb.Message.equals(
        new proto.jspb.test.HasExtensions(['hi', {100: [{200: 'a'}]}]),
        new proto.jspb.test.HasExtensions(['hi', {100: [{200: 'b'}]}])
    ));
    assertTrue(jspb.Message.equals(
        new proto.jspb.test.HasExtensions([{100: [{200: 'a'}]}]),
        new proto.jspb.test.HasExtensions([{100: [{200: 'a'}]}])
    ));
    assertTrue(jspb.Message.equals(
        new proto.jspb.test.HasExtensions([{100: [{200: 'a'}]}]),
        new proto.jspb.test.HasExtensions([,,, {100: [{200: 'a'}]}])
    ));
    assertTrue(jspb.Message.equals(
        new proto.jspb.test.HasExtensions([,,, {100: [{200: 'a'}]}]),
        new proto.jspb.test.HasExtensions([{100: [{200: 'a'}]}])
    ));
    assertTrue(jspb.Message.equals(
        new proto.jspb.test.HasExtensions(['hi', {100: [{200: 'a'}]}]),
        new proto.jspb.test.HasExtensions(['hi',,, {100: [{200: 'a'}]}])
    ));
    assertTrue(jspb.Message.equals(
        new proto.jspb.test.HasExtensions(['hi',,, {100: [{200: 'a'}]}]),
        new proto.jspb.test.HasExtensions(['hi', {100: [{200: 'a'}]}])
    ));
  });

  it('testEqualsExtensionsUnconstructed', function() {
    assertTrue(jspb.Message.compareFields([], [{}]));
    assertTrue(jspb.Message.compareFields([,,, {}], []));
    assertTrue(jspb.Message.compareFields([,,, {}], [,, {}]));
    assertTrue(jspb.Message.compareFields(
        ['hi', {100: [{200: 'a'}]}], ['hi', {100: [{200: 'a'}]}]));
    assertFalse(jspb.Message.compareFields(
        ['hi', {100: [{200: 'a'}]}], ['hi', {100: [{200: 'b'}]}]));
    assertTrue(jspb.Message.compareFields(
        [{100: [{200: 'a'}]}], [{100: [{200: 'a'}]}]));
    assertTrue(jspb.Message.compareFields(
        [{100: [{200: 'a'}]}], [,,, {100: [{200: 'a'}]}]));
    assertTrue(jspb.Message.compareFields(
        [,,, {100: [{200: 'a'}]}], [{100: [{200: 'a'}]}]));
    assertTrue(jspb.Message.compareFields(
        ['hi', {100: [{200: 'a'}]}], ['hi',,, {100: [{200: 'a'}]}]));
    assertTrue(jspb.Message.compareFields(
        ['hi',,, {100: [{200: 'a'}]}], ['hi', {100: [{200: 'a'}]}]));
  });

  it('testToMap', function() {
    var p1 = new proto.jspb.test.Simple1(['k', ['v']]);
    var p2 = new proto.jspb.test.Simple1(['k1', ['v1', 'v2']]);
    var soymap = jspb.Message.toMap([p1, p2],
        proto.jspb.test.Simple1.prototype.getAString,
        proto.jspb.test.Simple1.prototype.toObject);
    assertEquals('k', soymap['k'].aString);
    assertArrayEquals(['v'], soymap['k'].aRepeatedStringList);
    var protomap = jspb.Message.toMap([p1, p2],
        proto.jspb.test.Simple1.prototype.getAString);
    assertEquals('k', protomap['k'].getAString());
    assertArrayEquals(['v'], protomap['k'].getARepeatedStringList());
  });

  it('testClone', function() {
    var supportsUint8Array =
        !goog.userAgent.IE || goog.userAgent.isVersionOrHigher('10');
    var original = new proto.jspb.test.TestClone();
    original.setStr('v1');
    var simple1 = new proto.jspb.test.Simple1(['x1', ['y1', 'z1']]);
    var simple2 = new proto.jspb.test.Simple1(['x2', ['y2', 'z2']]);
    var simple3 = new proto.jspb.test.Simple1(['x3', ['y3', 'z3']]);
    original.setSimple1(simple1);
    original.setSimple2List([simple2, simple3]);
    var bytes1 = supportsUint8Array ? new Uint8Array([1, 2, 3]) : '123';
    original.setBytesField(bytes1);
    var extension = new proto.jspb.test.CloneExtension();
    extension.setExt('e1');
    original.setExtension(proto.jspb.test.IsExtension.extField, extension);
    var clone = original.clone();
    assertArrayEquals(['v1',, ['x1', ['y1', 'z1']],,
      [['x2', ['y2', 'z2']], ['x3', ['y3', 'z3']]], bytes1,, { 100: [, 'e1'] }],
        clone.toArray());
    clone.setStr('v2');
    var simple4 = new proto.jspb.test.Simple1(['a1', ['b1', 'c1']]);
    var simple5 = new proto.jspb.test.Simple1(['a2', ['b2', 'c2']]);
    var simple6 = new proto.jspb.test.Simple1(['a3', ['b3', 'c3']]);
    clone.setSimple1(simple4);
    clone.setSimple2List([simple5, simple6]);
    if (supportsUint8Array) {
      clone.getBytesField()[0] = 4;
      assertObjectEquals(bytes1, original.getBytesField());
    }
    var bytes2 = supportsUint8Array ? new Uint8Array([4, 5, 6]) : '456';
    clone.setBytesField(bytes2);
    var newExtension = new proto.jspb.test.CloneExtension();
    newExtension.setExt('e2');
    clone.setExtension(proto.jspb.test.CloneExtension.extField, newExtension);
    assertArrayEquals(['v2',, ['a1', ['b1', 'c1']],,
      [['a2', ['b2', 'c2']], ['a3', ['b3', 'c3']]], bytes2,, { 100: [, 'e2'] }],
        clone.toArray());
    assertArrayEquals(['v1',, ['x1', ['y1', 'z1']],,
      [['x2', ['y2', 'z2']], ['x3', ['y3', 'z3']]], bytes1,, { 100: [, 'e1'] }],
        original.toArray());
  });

  it('testCopyInto', function() {
    var supportsUint8Array =
        !goog.userAgent.IE || goog.userAgent.isVersionOrHigher('10');
    var original = new proto.jspb.test.TestClone();
    original.setStr('v1');
    var dest = new proto.jspb.test.TestClone();
    dest.setStr('override');
    var simple1 = new proto.jspb.test.Simple1(['x1', ['y1', 'z1']]);
    var simple2 = new proto.jspb.test.Simple1(['x2', ['y2', 'z2']]);
    var simple3 = new proto.jspb.test.Simple1(['x3', ['y3', 'z3']]);
    var destSimple1 = new proto.jspb.test.Simple1(['ox1', ['oy1', 'oz1']]);
    var destSimple2 = new proto.jspb.test.Simple1(['ox2', ['oy2', 'oz2']]);
    var destSimple3 = new proto.jspb.test.Simple1(['ox3', ['oy3', 'oz3']]);
    original.setSimple1(simple1);
    original.setSimple2List([simple2, simple3]);
    dest.setSimple1(destSimple1);
    dest.setSimple2List([destSimple2, destSimple3]);
    var bytes1 = supportsUint8Array ? new Uint8Array([1, 2, 3]) : '123';
    var bytes2 = supportsUint8Array ? new Uint8Array([4, 5, 6]) : '456';
    original.setBytesField(bytes1);
    dest.setBytesField(bytes2);
    var extension = new proto.jspb.test.CloneExtension();
    extension.setExt('e1');
    original.setExtension(proto.jspb.test.CloneExtension.extField, extension);

    jspb.Message.copyInto(original, dest);
    assertArrayEquals(original.toArray(), dest.toArray());
    assertEquals('x1', dest.getSimple1().getAString());
    assertEquals('e1',
        dest.getExtension(proto.jspb.test.CloneExtension.extField).getExt());
    dest.getSimple1().setAString('new value');
    assertNotEquals(dest.getSimple1().getAString(),
        original.getSimple1().getAString());
    if (supportsUint8Array) {
      dest.getBytesField()[0] = 7;
      assertObjectEquals(bytes1, original.getBytesField());
      assertObjectEquals(new Uint8Array([7, 2, 3]), dest.getBytesField());
    } else {
      dest.setBytesField('789');
      assertObjectEquals(bytes1, original.getBytesField());
      assertObjectEquals('789', dest.getBytesField());
    }
    dest.getExtension(proto.jspb.test.CloneExtension.extField).
        setExt('new value');
    assertNotEquals(
        dest.getExtension(proto.jspb.test.CloneExtension.extField).getExt(),
        original.getExtension(
            proto.jspb.test.CloneExtension.extField).getExt());
  });

  it('testCopyInto_notSameType', function() {
    var a = new proto.jspb.test.TestClone();
    var b = new proto.jspb.test.Simple1(['str', ['s1', 's2']]);

    var e = assertThrows(function() {
      jspb.Message.copyInto(a, b);
    });
    assertContains('should have the same type', e.message);
  });

  it('testExtensions', function() {
    var extension1 = new proto.jspb.test.IsExtension(['ext1field']);
    var extension2 = new proto.jspb.test.Simple1(['str', ['s1', 's2']]);
    var extendable = new proto.jspb.test.HasExtensions(['v1', 'v2', 'v3']);
    extendable.setExtension(proto.jspb.test.IsExtension.extField, extension1);
    extendable.setExtension(proto.jspb.test.IndirectExtension.simple,
                            extension2);
    extendable.setExtension(proto.jspb.test.IndirectExtension.str, 'xyzzy');
    extendable.setExtension(proto.jspb.test.IndirectExtension.repeatedStrList,
        ['a', 'b']);
    var s1 = new proto.jspb.test.Simple1(['foo', ['s1', 's2']]);
    var s2 = new proto.jspb.test.Simple1(['bar', ['t1', 't2']]);
    extendable.setExtension(
        proto.jspb.test.IndirectExtension.repeatedSimpleList,
        [s1, s2]);
    assertObjectEquals(extension1,
        extendable.getExtension(proto.jspb.test.IsExtension.extField));
    assertObjectEquals(extension2,
        extendable.getExtension(proto.jspb.test.IndirectExtension.simple));
    assertObjectEquals('xyzzy',
        extendable.getExtension(proto.jspb.test.IndirectExtension.str));
    assertObjectEquals(['a', 'b'], extendable.getExtension(
        proto.jspb.test.IndirectExtension.repeatedStrList));
    assertObjectEquals([s1, s2], extendable.getExtension(
        proto.jspb.test.IndirectExtension.repeatedSimpleList));
    // Not supported yet, but it should work...
    extendable.setExtension(proto.jspb.test.IndirectExtension.simple, null);
    assertNull(
        extendable.getExtension(proto.jspb.test.IndirectExtension.simple));
    extendable.setExtension(proto.jspb.test.IndirectExtension.str, null);
    assertNull(extendable.getExtension(proto.jspb.test.IndirectExtension.str));


    // Extension fields with jspb.ignore = true are ignored.
    assertUndefined(proto.jspb.test.IndirectExtension['ignored']);
    assertUndefined(proto.jspb.test.HasExtensions['ignoredFloating']);
  });

  it('testFloatingExtensions', function() {
    // From an autogenerated container.
    var extendable = new proto.jspb.test.HasExtensions(['v1', 'v2', 'v3']);
    var extension = new proto.jspb.test.Simple1(['foo', ['s1', 's2']]);
    extendable.setExtension(proto.jspb.test.simple1, extension);
    assertObjectEquals(extension,
        extendable.getExtension(proto.jspb.test.simple1));

    // From _lib mode.
    extension = new proto.jspb.test.ExtensionMessage(['s1']);
    extendable = new proto.jspb.test.TestExtensionsMessage([16]);
    extendable.setExtension(proto.jspb.test.floatingMsgField, extension);
    extendable.setExtension(proto.jspb.test.floatingStrField, 's2');
    assertObjectEquals(extension,
        extendable.getExtension(proto.jspb.test.floatingMsgField));
    assertObjectEquals('s2',
        extendable.getExtension(proto.jspb.test.floatingStrField));
    assertNotUndefined(proto.jspb.exttest.floatingMsgField);
    assertNotUndefined(proto.jspb.exttest.floatingMsgFieldTwo);
    assertNotUndefined(proto.jspb.exttest.beta.floatingStrField);
  });

  it('testToObject_extendedObject', function() {
    var extension1 = new proto.jspb.test.IsExtension(['ext1field']);
    var extension2 = new proto.jspb.test.Simple1(['str', ['s1', 's2'], true]);
    var extendable = new proto.jspb.test.HasExtensions(['v1', 'v2', 'v3']);
    extendable.setExtension(proto.jspb.test.IsExtension.extField, extension1);
    extendable.setExtension(proto.jspb.test.IndirectExtension.simple,
                            extension2);
    extendable.setExtension(proto.jspb.test.IndirectExtension.str, 'xyzzy');
    extendable.setExtension(proto.jspb.test.IndirectExtension.repeatedStrList,
        ['a', 'b']);
    var s1 = new proto.jspb.test.Simple1(['foo', ['s1', 's2'], true]);
    var s2 = new proto.jspb.test.Simple1(['bar', ['t1', 't2'], false]);
    extendable.setExtension(
        proto.jspb.test.IndirectExtension.repeatedSimpleList,
        [s1, s2]);
    assertObjectEquals({
      str1: 'v1', str2: 'v2', str3: 'v3',
      extField: { ext1: 'ext1field' },
      simple: {
        aString: 'str', aRepeatedStringList: ['s1', 's2'], aBoolean: true
      },
      str: 'xyzzy',
      repeatedStrList: ['a', 'b'],
      repeatedSimpleList: [
        { aString: 'foo', aRepeatedStringList: ['s1', 's2'], aBoolean: true},
        { aString: 'bar', aRepeatedStringList: ['t1', 't2'], aBoolean: false}
      ]
    }, extendable.toObject());

    // Now, with instances included.
    assertObjectEquals({
      str1: 'v1', str2: 'v2', str3: 'v3',
      extField: {
        ext1: 'ext1field',
        $jspbMessageInstance:
            extendable.getExtension(proto.jspb.test.IsExtension.extField)
      },
      simple: {
        aString: 'str',
        aRepeatedStringList: ['s1', 's2'],
        aBoolean: true,
        $jspbMessageInstance:
            extendable.getExtension(proto.jspb.test.IndirectExtension.simple)
      },
      str: 'xyzzy',
      repeatedStrList: ['a', 'b'],
      repeatedSimpleList: [{
        aString: 'foo',
        aRepeatedStringList: ['s1', 's2'],
        aBoolean: true,
        $jspbMessageInstance: s1
      }, {
        aString: 'bar',
        aRepeatedStringList: ['t1', 't2'],
        aBoolean: false,
        $jspbMessageInstance: s2
      }],
      $jspbMessageInstance: extendable
    }, extendable.toObject(true /* opt_includeInstance */));
  });

  it('testInitialization_emptyArray', function() {
    var msg = new proto.jspb.test.HasExtensions([]);
    assertArrayEquals([], msg.toArray());
  });

  it('testInitialization_justExtensionObject', function() {
    var msg = new proto.jspb.test.Empty([{1: 'hi'}]);
    // The extensionObject is not moved from its original location.
    assertArrayEquals([{1: 'hi'}], msg.toArray());
  });

  it('testInitialization_incompleteList', function() {
    var msg = new proto.jspb.test.Empty([1, {4: 'hi'}]);
    // The extensionObject is not moved from its original location.
    assertArrayEquals([1, {4: 'hi'}], msg.toArray());
  });

  it('testInitialization_forwardCompatible', function() {
    var msg = new proto.jspb.test.Empty([1, 2, 3, {1: 'hi'}]);
    assertArrayEquals([1, 2, 3, {1: 'hi'}], msg.toArray());
  });

  it('testExtendedMessageEnsureObject',
     /** @suppress {visibility} */ function() {
       var data =
           new proto.jspb.test.HasExtensions(['str1', {'a_key': 'an_object'}]);
       assertEquals('an_object', data.extensionObject_['a_key']);
     });

  it('testToObject_hasExtensionField', function() {
    var data = new proto.jspb.test.HasExtensions(['str1', {100: ['ext1']}]);
    var obj = data.toObject();
    assertEquals('str1', obj.str1);
    assertEquals('ext1', obj.extField.ext1);
  });

  it('testGetExtension', function() {
    var data = new proto.jspb.test.HasExtensions(['str1', {100: ['ext1']}]);
    assertEquals('str1', data.getStr1());
    var extension = data.getExtension(proto.jspb.test.IsExtension.extField);
    assertNotNull(extension);
    assertEquals('ext1', extension.getExt1());
  });

  it('testSetExtension', function() {
    var data = new proto.jspb.test.HasExtensions();
    var extensionMessage = new proto.jspb.test.IsExtension(['is_extension']);
    data.setExtension(proto.jspb.test.IsExtension.extField, extensionMessage);
    var obj = data.toObject();
    assertNotNull(
        data.getExtension(proto.jspb.test.IsExtension.extField));
    assertEquals('is_extension', obj.extField.ext1);
  });

  /**
   * Note that group is long deprecated, we only support it because JsPb has
   * a goal of being able to generate JS classes for all proto descriptors.
   */
  it('testGroups', function() {
    var group = new proto.jspb.test.TestGroup();
    var someGroup = new proto.jspb.test.TestGroup.RepeatedGroup();
    someGroup.setId('g1');
    someGroup.setSomeBoolList([true, false]);
    group.setRepeatedGroupList([someGroup]);
    var groups = group.getRepeatedGroupList();
    assertEquals('g1', groups[0].getId());
    assertObjectEquals([true, false], groups[0].getSomeBoolList());
    assertObjectEquals({id: 'g1', someBoolList: [true, false]},
        groups[0].toObject());
    assertObjectEquals({
      repeatedGroupList: [{id: 'g1', someBoolList: [true, false]}],
      requiredGroup: {id: undefined},
      optionalGroup: undefined,
      requiredSimple: {aRepeatedStringList: [], aString: undefined},
      optionalSimple: undefined,
      id: undefined
    }, group.toObject());
    var group1 = new proto.jspb.test.TestGroup1();
    group1.setGroup(someGroup);
    assertEquals(someGroup, group1.getGroup());
  });

  it('testNonExtensionFieldsAfterExtensionRange', function() {
    var data = [{'1': 'a_string'}];
    var message = new proto.jspb.test.Complex(data);
    assertArrayEquals([], message.getARepeatedStringList());
  });

  it('testReservedGetterNames', function() {
    var message = new proto.jspb.test.TestReservedNames();
    message.setExtension$(11);
    message.setExtension(proto.jspb.test.TestReservedNamesExtension.foo, 12);
    assertEquals(11, message.getExtension$());
    assertEquals(12, message.getExtension(
        proto.jspb.test.TestReservedNamesExtension.foo));
    assertObjectEquals({extension: 11, foo: 12}, message.toObject());
  });

  it('testInitializeMessageWithUnsetOneof', function() {
    var message = new proto.jspb.test.TestMessageWithOneof([]);
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.
            PARTIAL_ONEOF_NOT_SET,
        message.getPartialOneofCase());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.RecursiveOneofCase.
            RECURSIVE_ONEOF_NOT_SET,
        message.getRecursiveOneofCase());
  });

  it('testInitializeMessageWithSingleValueSetInOneof', function() {
    var message = new proto.jspb.test.TestMessageWithOneof([,, 'x']);

    assertEquals('x', message.getPone());
    assertEquals('', message.getPthree());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.PONE,
        message.getPartialOneofCase());
  });

  it('testKeepsLastWireValueSetInUnion_multipleValues', function() {
    var message = new proto.jspb.test.TestMessageWithOneof([,, 'x',, 'y']);

    assertEquals('', message.getPone());
    assertEquals('y', message.getPthree());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.PTHREE,
        message.getPartialOneofCase());
  });

  it('testSettingOneofFieldClearsOthers', function() {
    var message = new proto.jspb.test.TestMessageWithOneof;
    assertEquals('', message.getPone());
    assertEquals('', message.getPthree());
    assertFalse(message.hasPone());
    assertFalse(message.hasPthree());

    message.setPone('hi');
    assertEquals('hi', message.getPone());
    assertEquals('', message.getPthree());
    assertTrue(message.hasPone());
    assertFalse(message.hasPthree());

    message.setPthree('bye');
    assertEquals('', message.getPone());
    assertEquals('bye', message.getPthree());
    assertFalse(message.hasPone());
    assertTrue(message.hasPthree());
  });

  it('testSettingOneofFieldDoesNotClearFieldsFromOtherUnions', function() {
    var other = new proto.jspb.test.TestMessageWithOneof;
    var message = new proto.jspb.test.TestMessageWithOneof;
    assertEquals('', message.getPone());
    assertEquals('', message.getPthree());
    assertUndefined(message.getRone());
    assertFalse(message.hasPone());
    assertFalse(message.hasPthree());

    message.setPone('hi');
    message.setRone(other);
    assertEquals('hi', message.getPone());
    assertEquals('', message.getPthree());
    assertEquals(other, message.getRone());
    assertTrue(message.hasPone());
    assertFalse(message.hasPthree());

    message.setPthree('bye');
    assertEquals('', message.getPone());
    assertEquals('bye', message.getPthree());
    assertEquals(other, message.getRone());
    assertFalse(message.hasPone());
    assertTrue(message.hasPthree());
  });

  it('testUnsetsOneofCaseWhenFieldIsCleared', function() {
    var message = new proto.jspb.test.TestMessageWithOneof;
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.
            PARTIAL_ONEOF_NOT_SET,
        message.getPartialOneofCase());

    message.setPone('hi');
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.PONE,
        message.getPartialOneofCase());

    message.clearPone();
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.
            PARTIAL_ONEOF_NOT_SET,
        message.getPartialOneofCase());
  });

  it('testMessageWithDefaultOneofValues', function() {
    var message = new proto.jspb.test.TestMessageWithOneof;
    assertEquals(1234, message.getAone());
    assertEquals(0, message.getAtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofACase
            .DEFAULT_ONEOF_A_NOT_SET,
        message.getDefaultOneofACase());

    message.setAone(567);
    assertEquals(567, message.getAone());
    assertEquals(0, message.getAtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofACase.AONE,
        message.getDefaultOneofACase());

    message.setAtwo(890);
    assertEquals(1234, message.getAone());
    assertEquals(890, message.getAtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofACase.ATWO,
        message.getDefaultOneofACase());

    message.clearAtwo();
    assertEquals(1234, message.getAone());
    assertEquals(0, message.getAtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofACase
            .DEFAULT_ONEOF_A_NOT_SET,
        message.getDefaultOneofACase());
  });

  it('testMessageWithDefaultOneofValues_defaultNotOnFirstField', function() {
    var message = new proto.jspb.test.TestMessageWithOneof;
    assertEquals(0, message.getBone());
    assertEquals(1234, message.getBtwo());
    assertFalse(message.hasBone());
    assertFalse(message.hasBtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofBCase
            .DEFAULT_ONEOF_B_NOT_SET,
        message.getDefaultOneofBCase());

    message.setBone(2);
    assertEquals(2, message.getBone());
    assertEquals(1234, message.getBtwo());
    assertTrue(message.hasBone());
    assertFalse(message.hasBtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofBCase.BONE,
        message.getDefaultOneofBCase());

    message.setBtwo(3);
    assertEquals(0, message.getBone());
    assertFalse(message.hasBone());
    assertTrue(message.hasBtwo());
    assertEquals(3, message.getBtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofBCase.BTWO,
        message.getDefaultOneofBCase());

    message.clearBtwo();
    assertEquals(0, message.getBone());
    assertFalse(message.hasBone());
    assertFalse(message.hasBtwo());
    assertEquals(1234, message.getBtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofBCase
            .DEFAULT_ONEOF_B_NOT_SET,
        message.getDefaultOneofBCase());
  });

  it('testInitializeMessageWithOneofDefaults', function() {
    var message =
        new proto.jspb.test.TestMessageWithOneof(new Array(9).concat(567));
    assertEquals(567, message.getAone());
    assertEquals(0, message.getAtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofACase.AONE,
        message.getDefaultOneofACase());

    message =
        new proto.jspb.test.TestMessageWithOneof(new Array(10).concat(890));
    assertEquals(1234, message.getAone());
    assertEquals(890, message.getAtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofACase.ATWO,
        message.getDefaultOneofACase());

    message =
        new proto.jspb.test.TestMessageWithOneof(new Array(9).concat(567, 890));
    assertEquals(1234, message.getAone());
    assertEquals(890, message.getAtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.DefaultOneofACase.ATWO,
        message.getDefaultOneofACase());
  });

  it('testInitializeMessageWithOneofDefaults_defaultNotSetOnFirstField',
      function() {
        var message;

        message =
            new proto.jspb.test.TestMessageWithOneof(new Array(11).concat(567));
        assertEquals(567, message.getBone());
        assertEquals(1234, message.getBtwo());
        assertEquals(
            proto.jspb.test.TestMessageWithOneof.DefaultOneofBCase.BONE,
            message.getDefaultOneofBCase());

        message =
            new proto.jspb.test.TestMessageWithOneof(new Array(12).concat(890));
        assertEquals(0, message.getBone());
        assertEquals(890, message.getBtwo());
        assertEquals(
            proto.jspb.test.TestMessageWithOneof.DefaultOneofBCase.BTWO,
            message.getDefaultOneofBCase());

        message = new proto.jspb.test.TestMessageWithOneof(
            new Array(11).concat(567, 890));
        assertEquals(0, message.getBone());
        assertEquals(890, message.getBtwo());
        assertEquals(
            proto.jspb.test.TestMessageWithOneof.DefaultOneofBCase.BTWO,
            message.getDefaultOneofBCase());
      });

  it('testOneofContainingAnotherMessage', function() {
    var message = new proto.jspb.test.TestMessageWithOneof;
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.RecursiveOneofCase.
            RECURSIVE_ONEOF_NOT_SET,
        message.getRecursiveOneofCase());

    var other = new proto.jspb.test.TestMessageWithOneof;
    message.setRone(other);
    assertEquals(other, message.getRone());
    assertEquals('', message.getRtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.RecursiveOneofCase.RONE,
        message.getRecursiveOneofCase());

    message.setRtwo('hi');
    assertUndefined(message.getRone());
    assertEquals('hi', message.getRtwo());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.RecursiveOneofCase.RTWO,
        message.getRecursiveOneofCase());
  });

  it('testQueryingOneofCaseEnsuresOnlyOneFieldIsSetInUnderlyingArray',
     function() {
    var message = new proto.jspb.test.TestMessageWithOneof;
    message.setPone('x');
    assertEquals('x', message.getPone());
    assertEquals('', message.getPthree());
    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.PONE,
        message.getPartialOneofCase());

    var array = message.toArray();
    assertEquals('x', array[2]);
    assertUndefined(array[4]);
    array[4] = 'y';

    assertEquals(
        proto.jspb.test.TestMessageWithOneof.PartialOneofCase.PTHREE,
        message.getPartialOneofCase());
    assertUndefined(array[2]);
    assertEquals('y', array[4]);
  });

  it('testFloatingPointFieldsSupportNan', function() {
    var assertNan = function(x) {
      assertTrue('Expected ' + x + ' (' + goog.typeOf(x) + ') to be NaN.',
          goog.isNumber(x) && isNaN(x));
    };

    var message = new proto.jspb.test.FloatingPointFields([
      'NaN', 'NaN', ['NaN', 'NaN'], 'NaN',
      'NaN', 'NaN', ['NaN', 'NaN'], 'NaN'
    ]);
    assertNan(message.getOptionalFloatField());
    assertNan(message.getRequiredFloatField());
    assertNan(message.getRepeatedFloatFieldList()[0]);
    assertNan(message.getRepeatedFloatFieldList()[1]);
    assertNan(message.getDefaultFloatField());
    assertNan(message.getOptionalDoubleField());
    assertNan(message.getRequiredDoubleField());
    assertNan(message.getRepeatedDoubleFieldList()[0]);
    assertNan(message.getRepeatedDoubleFieldList()[1]);
    assertNan(message.getDefaultDoubleField());
  });

});
