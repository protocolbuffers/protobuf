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

/**
 * @fileoverview Test cases for jspb's binary protocol buffer decoder.
 *
 * There are two particular magic numbers that need to be pointed out -
 * 2^64-1025 is the largest number representable as both a double and an
 * unsigned 64-bit integer, and 2^63-513 is the largest number representable as
 * both a double and a signed 64-bit integer.
 *
 * Test suite is written using Jasmine -- see http://jasmine.github.io/
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.require('goog.testing.asserts');
goog.require('jspb.BinaryConstants');
goog.require('jspb.BinaryDecoder');
goog.require('jspb.BinaryEncoder');
goog.require('jspb.utils');


/**
 * Tests encoding and decoding of unsigned types.
 * @param {Function} readValue
 * @param {Function} writeValue
 * @param {number} epsilon
 * @param {number} upperLimit
 * @param {Function} filter
 * @suppress {missingProperties|visibility}
 */
function doTestUnsignedValue(readValue,
    writeValue, epsilon, upperLimit, filter) {
  var encoder = new jspb.BinaryEncoder();

  // Encode zero and limits.
  writeValue.call(encoder, filter(0));
  writeValue.call(encoder, filter(epsilon));
  writeValue.call(encoder, filter(upperLimit));

  // Encode positive values.
  for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
    writeValue.call(encoder, filter(cursor));
  }

  var decoder = jspb.BinaryDecoder.alloc(encoder.end());

  // Check zero and limits.
  assertEquals(filter(0), readValue.call(decoder));
  assertEquals(filter(epsilon), readValue.call(decoder));
  assertEquals(filter(upperLimit), readValue.call(decoder));

  // Check positive values.
  for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
    if (filter(cursor) != readValue.call(decoder)) throw 'fail!';
  }

  // Encoding values outside the valid range should assert.
  assertThrows(function() {writeValue.call(encoder, -1);});
  assertThrows(function() {writeValue.call(encoder, upperLimit * 1.1);});
}


/**
 * Tests encoding and decoding of signed types.
 * @param {Function} readValue
 * @param {Function} writeValue
 * @param {number} epsilon
 * @param {number} lowerLimit
 * @param {number} upperLimit
 * @param {Function} filter
 * @suppress {missingProperties}
 */
function doTestSignedValue(readValue,
    writeValue, epsilon, lowerLimit, upperLimit, filter) {
  var encoder = new jspb.BinaryEncoder();

  // Encode zero and limits.
  writeValue.call(encoder, filter(lowerLimit));
  writeValue.call(encoder, filter(-epsilon));
  writeValue.call(encoder, filter(0));
  writeValue.call(encoder, filter(epsilon));
  writeValue.call(encoder, filter(upperLimit));

  var inputValues = [];

  // Encode negative values.
  for (var cursor = lowerLimit; cursor < -epsilon; cursor /= 1.1) {
    var val = filter(cursor);
    writeValue.call(encoder, val);
    inputValues.push(val);
  }

  // Encode positive values.
  for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
    var val = filter(cursor);
    writeValue.call(encoder, val);
    inputValues.push(val);
  }

  var decoder = jspb.BinaryDecoder.alloc(encoder.end());

  // Check zero and limits.
  assertEquals(filter(lowerLimit), readValue.call(decoder));
  assertEquals(filter(-epsilon), readValue.call(decoder));
  assertEquals(filter(0), readValue.call(decoder));
  assertEquals(filter(epsilon), readValue.call(decoder));
  assertEquals(filter(upperLimit), readValue.call(decoder));

  // Verify decoded values.
  for (var i = 0; i < inputValues.length; i++) {
    assertEquals(inputValues[i], readValue.call(decoder));
  }

  // Encoding values outside the valid range should assert.
  var pastLowerLimit = lowerLimit * 1.1;
  var pastUpperLimit = upperLimit * 1.1;
  if (pastLowerLimit !== -Infinity) {
    expect(() => void writeValue.call(encoder, pastLowerLimit)).toThrow();
  }
  if (pastUpperLimit !== Infinity) {
    expect(() => void writeValue.call(encoder, pastUpperLimit)).toThrow();
  }
}

describe('binaryDecoderTest', function() {
  /**
   * Tests the decoder instance cache.
   */
  it('testInstanceCache', /** @suppress {visibility} */ function() {
    // Empty the instance caches.
    jspb.BinaryDecoder.instanceCache_ = [];

    // Allocating and then freeing a decoder should put it in the instance
    // cache.
    jspb.BinaryDecoder.alloc().free();

    assertEquals(1, jspb.BinaryDecoder.instanceCache_.length);

    // Allocating and then freeing three decoders should leave us with three in
    // the cache.

    var decoder1 = jspb.BinaryDecoder.alloc();
    var decoder2 = jspb.BinaryDecoder.alloc();
    var decoder3 = jspb.BinaryDecoder.alloc();
    decoder1.free();
    decoder2.free();
    decoder3.free();

    assertEquals(3, jspb.BinaryDecoder.instanceCache_.length);
  });


  describe('varint64', function() {
    var /** !jspb.BinaryEncoder */ encoder;
    var /** !jspb.BinaryDecoder */ decoder;

    var hashA = String.fromCharCode(0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00);
    var hashB = String.fromCharCode(0x12, 0x34, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00);
    var hashC = String.fromCharCode(0x12, 0x34, 0x56, 0x78,
                                    0x87, 0x65, 0x43, 0x21);
    var hashD = String.fromCharCode(0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF);
    beforeEach(function() {
      encoder = new jspb.BinaryEncoder();

      encoder.writeVarintHash64(hashA);
      encoder.writeVarintHash64(hashB);
      encoder.writeVarintHash64(hashC);
      encoder.writeVarintHash64(hashD);

      encoder.writeFixedHash64(hashA);
      encoder.writeFixedHash64(hashB);
      encoder.writeFixedHash64(hashC);
      encoder.writeFixedHash64(hashD);

      decoder = jspb.BinaryDecoder.alloc(encoder.end());
    });

    it('reads 64-bit integers as hash strings', function() {
      assertEquals(hashA, decoder.readVarintHash64());
      assertEquals(hashB, decoder.readVarintHash64());
      assertEquals(hashC, decoder.readVarintHash64());
      assertEquals(hashD, decoder.readVarintHash64());

      assertEquals(hashA, decoder.readFixedHash64());
      assertEquals(hashB, decoder.readFixedHash64());
      assertEquals(hashC, decoder.readFixedHash64());
      assertEquals(hashD, decoder.readFixedHash64());
    });

    it('reads split 64 bit integers', function() {
      function hexJoin(bitsLow, bitsHigh) {
        return `0x${(bitsHigh >>> 0).toString(16)}:0x${
            (bitsLow >>> 0).toString(16)}`;
      }
      function hexJoinHash(hash64) {
        jspb.utils.splitHash64(hash64);
        return hexJoin(jspb.utils.split64Low, jspb.utils.split64High);
      }

      expect(decoder.readSplitVarint64(hexJoin)).toEqual(hexJoinHash(hashA));
      expect(decoder.readSplitVarint64(hexJoin)).toEqual(hexJoinHash(hashB));
      expect(decoder.readSplitVarint64(hexJoin)).toEqual(hexJoinHash(hashC));
      expect(decoder.readSplitVarint64(hexJoin)).toEqual(hexJoinHash(hashD));

      expect(decoder.readSplitFixed64(hexJoin)).toEqual(hexJoinHash(hashA));
      expect(decoder.readSplitFixed64(hexJoin)).toEqual(hexJoinHash(hashB));
      expect(decoder.readSplitFixed64(hexJoin)).toEqual(hexJoinHash(hashC));
      expect(decoder.readSplitFixed64(hexJoin)).toEqual(hexJoinHash(hashD));
    });
  });

  describe('sint64', function() {
    var /** !jspb.BinaryDecoder */ decoder;

    var hashA =
        String.fromCharCode(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    var hashB =
        String.fromCharCode(0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    var hashC =
        String.fromCharCode(0x12, 0x34, 0x56, 0x78, 0x87, 0x65, 0x43, 0x21);
    var hashD =
        String.fromCharCode(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    beforeEach(function() {
      var encoder = new jspb.BinaryEncoder();

      encoder.writeZigzagVarintHash64(hashA);
      encoder.writeZigzagVarintHash64(hashB);
      encoder.writeZigzagVarintHash64(hashC);
      encoder.writeZigzagVarintHash64(hashD);

      decoder = jspb.BinaryDecoder.alloc(encoder.end());
    });

    it('reads 64-bit integers as decimal strings', function() {
      const signed = true;
      expect(decoder.readZigzagVarint64String())
          .toEqual(jspb.utils.hash64ToDecimalString(hashA, signed));
      expect(decoder.readZigzagVarint64String())
          .toEqual(jspb.utils.hash64ToDecimalString(hashB, signed));
      expect(decoder.readZigzagVarint64String())
          .toEqual(jspb.utils.hash64ToDecimalString(hashC, signed));
      expect(decoder.readZigzagVarint64String())
          .toEqual(jspb.utils.hash64ToDecimalString(hashD, signed));
    });

    it('reads 64-bit integers as hash strings', function() {
      expect(decoder.readZigzagVarintHash64()).toEqual(hashA);
      expect(decoder.readZigzagVarintHash64()).toEqual(hashB);
      expect(decoder.readZigzagVarintHash64()).toEqual(hashC);
      expect(decoder.readZigzagVarintHash64()).toEqual(hashD);
    });

    it('reads split 64 bit zigzag integers', function() {
      function hexJoin(bitsLow, bitsHigh) {
        return `0x${(bitsHigh >>> 0).toString(16)}:0x${
            (bitsLow >>> 0).toString(16)}`;
      }
      function hexJoinHash(hash64) {
        jspb.utils.splitHash64(hash64);
        return hexJoin(jspb.utils.split64Low, jspb.utils.split64High);
      }

      expect(decoder.readSplitZigzagVarint64(hexJoin))
          .toEqual(hexJoinHash(hashA));
      expect(decoder.readSplitZigzagVarint64(hexJoin))
          .toEqual(hexJoinHash(hashB));
      expect(decoder.readSplitZigzagVarint64(hexJoin))
          .toEqual(hexJoinHash(hashC));
      expect(decoder.readSplitZigzagVarint64(hexJoin))
          .toEqual(hexJoinHash(hashD));
    });

    it('does zigzag encoding properly', function() {
      // Test cases directly from the protobuf dev guide.
      // https://engdoc.corp.google.com/eng/howto/protocolbuffers/developerguide/encoding.shtml?cl=head#types
      var testCases = [
        {original: '0', zigzag: '0'},
        {original: '-1', zigzag: '1'},
        {original: '1', zigzag: '2'},
        {original: '-2', zigzag: '3'},
        {original: '2147483647', zigzag: '4294967294'},
        {original: '-2147483648', zigzag: '4294967295'},
        // 64-bit extremes, not in dev guide.
        {original: '9223372036854775807', zigzag: '18446744073709551614'},
        {original: '-9223372036854775808', zigzag: '18446744073709551615'},
      ];
      var encoder = new jspb.BinaryEncoder();
      testCases.forEach(function(c) {
        encoder.writeZigzagVarint64String(c.original);
      });
      var buffer = encoder.end();
      var zigzagDecoder = jspb.BinaryDecoder.alloc(buffer);
      var varintDecoder = jspb.BinaryDecoder.alloc(buffer);
      testCases.forEach(function(c) {
        expect(zigzagDecoder.readZigzagVarint64String()).toEqual(c.original);
        expect(varintDecoder.readUnsignedVarint64String()).toEqual(c.zigzag);
      });
    });
  });

  /**
   * Tests reading and writing large strings
   */
  it('testLargeStrings', function() {
    var encoder = new jspb.BinaryEncoder();

    var len = 150000;
    var long_string = '';
    for (var i = 0; i < len; i++) {
      long_string += 'a';
    }

    encoder.writeString(long_string);

    var decoder = jspb.BinaryDecoder.alloc(encoder.end());

    assertEquals(long_string, decoder.readString(len));
  });

  /**
   * Test encoding and decoding utf-8.
   */
   it('testUtf8', function() {
    var encoder = new jspb.BinaryEncoder();

    var ascii = "ASCII should work in 3, 2, 1...";
    var utf8_two_bytes = "©";
    var utf8_three_bytes = "❄";
    var utf8_four_bytes = "😁";

    encoder.writeString(ascii);
    encoder.writeString(utf8_two_bytes);
    encoder.writeString(utf8_three_bytes);
    encoder.writeString(utf8_four_bytes);

    var decoder = jspb.BinaryDecoder.alloc(encoder.end());

    assertEquals(ascii, decoder.readString(ascii.length));
    assertEquals(utf8_two_bytes, decoder.readString(utf8_two_bytes.length));
    assertEquals(utf8_three_bytes, decoder.readString(utf8_three_bytes.length));
    assertEquals(utf8_four_bytes, decoder.readString(utf8_four_bytes.length));
   });

  /**
   * Verifies that misuse of the decoder class triggers assertions.
   */
  it('testDecodeErrors', function() {
    // Reading a value past the end of the stream should trigger an assertion.
    var decoder = jspb.BinaryDecoder.alloc([0, 1, 2]);
    assertThrows(function() {decoder.readUint64()});

    // Overlong varints should trigger assertions.
    decoder.setBlock([255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 0]);
    assertThrows(function() {decoder.readUnsignedVarint64()});
    decoder.reset();
    assertThrows(function() {decoder.readSignedVarint64()});
    decoder.reset();
    assertThrows(function() {decoder.readZigzagVarint64()});
    decoder.reset();
    assertThrows(function() {decoder.readUnsignedVarint32()});
  });


  /**
   * Tests encoding and decoding of unsigned integers.
   */
  it('testUnsignedIntegers', function() {
    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint8,
        jspb.BinaryEncoder.prototype.writeUint8,
        1, 0xFF, Math.round);

    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint16,
        jspb.BinaryEncoder.prototype.writeUint16,
        1, 0xFFFF, Math.round);

    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint32,
        jspb.BinaryEncoder.prototype.writeUint32,
        1, 0xFFFFFFFF, Math.round);

    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint64,
        jspb.BinaryEncoder.prototype.writeUint64,
        1, Math.pow(2, 64) - 1025, Math.round);
  });


  /**
   * Tests encoding and decoding of signed integers.
   */
  it('testSignedIntegers', function() {
    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt8,
        jspb.BinaryEncoder.prototype.writeInt8,
        1, -0x80, 0x7F, Math.round);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt16,
        jspb.BinaryEncoder.prototype.writeInt16,
        1, -0x8000, 0x7FFF, Math.round);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt32,
        jspb.BinaryEncoder.prototype.writeInt32,
        1, -0x80000000, 0x7FFFFFFF, Math.round);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt64,
        jspb.BinaryEncoder.prototype.writeInt64,
        1, -Math.pow(2, 63), Math.pow(2, 63) - 513, Math.round);
  });


  /**
   * Tests encoding and decoding of floats.
   */
  it('testFloats', function() {
    /**
     * @param {number} x
     * @return {number}
     */
    function truncate(x) {
      var temp = new Float32Array(1);
      temp[0] = x;
      return temp[0];
    }
    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readFloat,
        jspb.BinaryEncoder.prototype.writeFloat,
        jspb.BinaryConstants.FLOAT32_EPS,
        -jspb.BinaryConstants.FLOAT32_MAX,
        jspb.BinaryConstants.FLOAT32_MAX,
        truncate);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readDouble,
        jspb.BinaryEncoder.prototype.writeDouble,
        jspb.BinaryConstants.FLOAT64_EPS * 10,
        -jspb.BinaryConstants.FLOAT64_MAX,
        jspb.BinaryConstants.FLOAT64_MAX,
        function(x) { return x; });
  });
});
