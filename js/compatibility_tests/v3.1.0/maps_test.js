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

goog.require('goog.testing.asserts');
goog.require('goog.userAgent');

// CommonJS-LoadFromFile: testbinary_pb proto.jspb.test
goog.require('proto.jspb.test.MapValueEnum');
goog.require('proto.jspb.test.MapValueMessage');
goog.require('proto.jspb.test.TestMapFields');

// CommonJS-LoadFromFile: test_pb proto.jspb.test
goog.require('proto.jspb.test.MapValueMessageNoBinary');
goog.require('proto.jspb.test.TestMapFieldsNoBinary');

goog.requireType('jspb.Map');

/**
 * Helper: check that the given map has exactly this set of (sorted) entries.
 * @param {!jspb.Map} map
 * @param {!Array<!Array<?>>} entries
 */
function checkMapEquals(map, entries) {
  var arr = map.toArray();
  assertEquals(arr.length, entries.length);
  for (var i = 0; i < arr.length; i++) {
    assertElementsEquals(arr[i], entries[i]);
  }
}

/**
 * Converts an ES6 iterator to an array.
 * @template T
 * @param {!Iterator<T>} iter an iterator
 * @return {!Array<T>}
 */
function toArray(iter) {
  var arr = [];
  while (true) {
    var val = iter.next();
    if (val.done) {
      break;
    }
    arr.push(val.value);
  }
  return arr;
}


/**
 * Helper: generate test methods for this TestMapFields class.
 * @param {?} msgInfo
 * @param {?} submessageCtor
 * @param {!string} suffix
 */
function makeTests(msgInfo, submessageCtor, suffix) {
  /**
   * Helper: fill all maps on a TestMapFields.
   * @param {?} msg
   */
  var fillMapFields = function(msg) {
    msg.getMapStringStringMap().set('asdf', 'jkl;').set('key 2', 'hello world');
    msg.getMapStringInt32Map().set('a', 1).set('b', -2);
    msg.getMapStringInt64Map().set('c', 0x100000000).set('d', 0x200000000);
    msg.getMapStringBoolMap().set('e', true).set('f', false);
    msg.getMapStringDoubleMap().set('g', 3.14159).set('h', 2.71828);
    msg.getMapStringEnumMap()
        .set('i', proto.jspb.test.MapValueEnum.MAP_VALUE_BAR)
        .set('j', proto.jspb.test.MapValueEnum.MAP_VALUE_BAZ);
    msg.getMapStringMsgMap()
        .set('k', new submessageCtor())
        .set('l', new submessageCtor());
    msg.getMapStringMsgMap().get('k').setFoo(42);
    msg.getMapStringMsgMap().get('l').setFoo(84);
    msg.getMapInt32StringMap().set(-1, 'a').set(42, 'b');
    msg.getMapInt64StringMap()
        .set(0x123456789abc, 'c')
        .set(0xcba987654321, 'd');
    msg.getMapBoolStringMap().set(false, 'e').set(true, 'f');
  };

  /**
   * Helper: check all maps on a TestMapFields.
   * @param {?} msg
   */
  var checkMapFields = function(msg) {
    checkMapEquals(
        msg.getMapStringStringMap(),
        [['asdf', 'jkl;'], ['key 2', 'hello world']]);
    checkMapEquals(msg.getMapStringInt32Map(), [['a', 1], ['b', -2]]);
    checkMapEquals(
        msg.getMapStringInt64Map(), [['c', 0x100000000], ['d', 0x200000000]]);
    checkMapEquals(msg.getMapStringBoolMap(), [['e', true], ['f', false]]);
    checkMapEquals(
        msg.getMapStringDoubleMap(), [['g', 3.14159], ['h', 2.71828]]);
    checkMapEquals(msg.getMapStringEnumMap(), [
      ['i', proto.jspb.test.MapValueEnum.MAP_VALUE_BAR],
      ['j', proto.jspb.test.MapValueEnum.MAP_VALUE_BAZ]
    ]);
    checkMapEquals(msg.getMapInt32StringMap(), [[-1, 'a'], [42, 'b']]);
    checkMapEquals(
        msg.getMapInt64StringMap(),
        [[0x123456789abc, 'c'], [0xcba987654321, 'd']]);
    checkMapEquals(msg.getMapBoolStringMap(), [[false, 'e'], [true, 'f']]);

    assertEquals(msg.getMapStringMsgMap().getLength(), 2);
    assertEquals(msg.getMapStringMsgMap().get('k').getFoo(), 42);
    assertEquals(msg.getMapStringMsgMap().get('l').getFoo(), 84);

    var entries = toArray(msg.getMapStringMsgMap().entries());
    assertEquals(entries.length, 2);
    entries.forEach(function(entry) {
      var key = entry[0];
      var val = entry[1];
      assert(val === msg.getMapStringMsgMap().get(key));
    });

    msg.getMapStringMsgMap().forEach(function(val, key) {
      assert(val === msg.getMapStringMsgMap().get(key));
    });
  };

  it('testMapStringStringField' + suffix, function() {
    var msg = new msgInfo.constructor();
    assertEquals(msg.getMapStringStringMap().getLength(), 0);
    assertEquals(msg.getMapStringInt32Map().getLength(), 0);
    assertEquals(msg.getMapStringInt64Map().getLength(), 0);
    assertEquals(msg.getMapStringBoolMap().getLength(), 0);
    assertEquals(msg.getMapStringDoubleMap().getLength(), 0);
    assertEquals(msg.getMapStringEnumMap().getLength(), 0);
    assertEquals(msg.getMapStringMsgMap().getLength(), 0);

    // Re-create to clear out any internally-cached wrappers, etc.
    msg = new msgInfo.constructor();
    var m = msg.getMapStringStringMap();
    assertEquals(m.has('asdf'), false);
    assertEquals(m.get('asdf'), undefined);
    m.set('asdf', 'hello world');
    assertEquals(m.has('asdf'), true);
    assertEquals(m.get('asdf'), 'hello world');
    m.set('jkl;', 'key 2');
    assertEquals(m.has('jkl;'), true);
    assertEquals(m.get('jkl;'), 'key 2');
    assertEquals(m.getLength(), 2);
    var it = m.entries();
    assertElementsEquals(it.next().value, ['asdf', 'hello world']);
    assertElementsEquals(it.next().value, ['jkl;', 'key 2']);
    assertEquals(it.next().done, true);
    checkMapEquals(m, [['asdf', 'hello world'], ['jkl;', 'key 2']]);
    m.del('jkl;');
    assertEquals(m.has('jkl;'), false);
    assertEquals(m.get('jkl;'), undefined);
    assertEquals(m.getLength(), 1);
    it = m.keys();
    assertEquals(it.next().value, 'asdf');
    assertEquals(it.next().done, true);
    it = m.values();
    assertEquals(it.next().value, 'hello world');
    assertEquals(it.next().done, true);

    var count = 0;
    m.forEach(function(value, key, map) {
      assertEquals(map, m);
      assertEquals(key, 'asdf');
      assertEquals(value, 'hello world');
      count++;
    });
    assertEquals(count, 1);

    m.clear();
    assertEquals(m.getLength(), 0);
  });


  /**
   * Tests operations on maps with all key and value types.
   */
  it('testAllMapTypes' + suffix, function() {
    var msg = new msgInfo.constructor();
    fillMapFields(msg);
    checkMapFields(msg);
  });


  if (msgInfo.deserializeBinary) {
    /**
     * Tests serialization and deserialization in binary format.
     */
    it('testBinaryFormat' + suffix, function() {
      if (goog.userAgent.IE && !goog.userAgent.isDocumentModeOrHigher(10)) {
        // IE8/9 currently doesn't support binary format because they lack
        // TypedArray.
        return;
      }

      // Check that the format is correct.
      var msg = new msgInfo.constructor();
      msg.getMapStringStringMap().set('A', 'a');
      var serialized = msg.serializeBinary();
      var expectedSerialized = [
        0x0a, 0x6,  // field 1 (map_string_string), delimited, length 6
        0x0a, 0x1,  // field 1 in submessage (key), delimited, length 1
        0x41,       // ASCII 'A'
        0x12, 0x1,  // field 2 in submessage (value), delimited, length 1
        0x61        // ASCII 'a'
      ];
      assertEquals(serialized.length, expectedSerialized.length);
      for (var i = 0; i < serialized.length; i++) {
        assertEquals(serialized[i], expectedSerialized[i]);
      }

      // Check that all map fields successfully round-trip.
      msg = new msgInfo.constructor();
      fillMapFields(msg);
      serialized = msg.serializeBinary();
      var decoded = msgInfo.deserializeBinary(serialized);
      checkMapFields(decoded);
    });
  }

  /**
   * Exercises the lazy map<->underlying array sync.
   */
  it('testLazyMapSync' + suffix, function() {
    // Start with a JSPB array containing a few map entries.
    var entries = [['a', 'entry 1'], ['c', 'entry 2'], ['b', 'entry 3']];
    var msg = new msgInfo.constructor([entries]);
    assertEquals(entries.length, 3);
    assertEquals(entries[0][0], 'a');
    assertEquals(entries[1][0], 'c');
    assertEquals(entries[2][0], 'b');
    msg.getMapStringStringMap().del('a');
    assertEquals(entries.length, 3);  // not yet sync'd
    msg.toArray();                    // force a sync
    assertEquals(entries.length, 2);
    assertEquals(entries[0][0], 'b');  // now in sorted order
    assertEquals(entries[1][0], 'c');

    var a = msg.toArray();
    assertEquals(a[0], entries);  // retains original reference
  });
}

describe('mapsTest', function() {
  makeTests(
      {
        constructor: proto.jspb.test.TestMapFields,
        deserializeBinary: proto.jspb.test.TestMapFields.deserializeBinary
      },
      proto.jspb.test.MapValueMessage, '_Binary');
  makeTests(
      {
        constructor: proto.jspb.test.TestMapFieldsNoBinary,
        deserializeBinary: null
      },
      proto.jspb.test.MapValueMessageNoBinary, '_NoBinary');
});
