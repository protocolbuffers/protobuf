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
  assertThrows(function() {writeValue.call(encoder, lowerLimit * 1.1);});
  assertThrows(function() {writeValue.call(encoder, upperLimit * 1.1);});
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


  /**
   * Tests reading 64-bit integers as hash strings.
   */
  it('testHashStrings', function() {
    var encoder = new jspb.BinaryEncoder();

    var hashA = String.fromCharCode(0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00);
    var hashB = String.fromCharCode(0x12, 0x34, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00);
    var hashC = String.fromCharCode(0x12, 0x34, 0x56, 0x78,
                                    0x87, 0x65, 0x43, 0x21);
    var hashD = String.fromCharCode(0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF);

    encoder.writeVarintHash64(hashA);
    encoder.writeVarintHash64(hashB);
    encoder.writeVarintHash64(hashC);
    encoder.writeVarintHash64(hashD);

    encoder.writeFixedHash64(hashA);
    encoder.writeFixedHash64(hashB);
    encoder.writeFixedHash64(hashC);
    encoder.writeFixedHash64(hashD);

    var decoder = jspb.BinaryDecoder.alloc(encoder.end());

    assertEquals(hashA, decoder.readVarintHash64());
    assertEquals(hashB, decoder.readVarintHash64());
    assertEquals(hashC, decoder.readVarintHash64());
    assertEquals(hashD, decoder.readVarintHash64());

    assertEquals(hashA, decoder.readFixedHash64());
    assertEquals(hashB, decoder.readFixedHash64());
    assertEquals(hashC, decoder.readFixedHash64());
    assertEquals(hashD, decoder.readFixedHash64());
  });


  /**
   * Verifies that misuse of the decoder class triggers assertions.
   * @suppress {checkTypes|visibility}
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
