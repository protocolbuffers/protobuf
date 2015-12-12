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
goog.require('jspb.BinaryWriter');


/**
 * Tests raw encoding and decoding of unsigned types.
 * @param {Function} readValue
 * @param {Function} writeValue
 * @param {number} epsilon
 * @param {number} upperLimit
 * @param {Function} filter
 * @suppress {missingProperties|visibility}
 */
function doTestUnsignedValue(readValue,
    writeValue, epsilon, upperLimit, filter) {
  var writer = new jspb.BinaryWriter();

  // Encode zero and limits.
  writeValue.call(writer, filter(0));
  writeValue.call(writer, filter(epsilon));
  writeValue.call(writer, filter(upperLimit));

  // Encode positive values.
  for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
    writeValue.call(writer, filter(cursor));
  }

  var reader = jspb.BinaryDecoder.alloc(writer.getResultBuffer());

  // Check zero and limits.
  assertEquals(filter(0), readValue.call(reader));
  assertEquals(filter(epsilon), readValue.call(reader));
  assertEquals(filter(upperLimit), readValue.call(reader));

  // Check positive values.
  for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
    if (filter(cursor) != readValue.call(reader)) throw 'fail!';
  }
}


/**
 * Tests raw encoding and decoding of signed types.
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
  var writer = new jspb.BinaryWriter();

  // Encode zero and limits.
  writeValue.call(writer, filter(lowerLimit));
  writeValue.call(writer, filter(-epsilon));
  writeValue.call(writer, filter(0));
  writeValue.call(writer, filter(epsilon));
  writeValue.call(writer, filter(upperLimit));

  var inputValues = [];

  // Encode negative values.
  for (var cursor = lowerLimit; cursor < -epsilon; cursor /= 1.1) {
    var val = filter(cursor);
    writeValue.call(writer, val);
    inputValues.push(val);
  }

  // Encode positive values.
  for (var cursor = epsilon; cursor < upperLimit; cursor *= 1.1) {
    var val = filter(cursor);
    writeValue.call(writer, val);
    inputValues.push(val);
  }

  var reader = jspb.BinaryDecoder.alloc(writer.getResultBuffer());

  // Check zero and limits.
  assertEquals(filter(lowerLimit), readValue.call(reader));
  assertEquals(filter(-epsilon), readValue.call(reader));
  assertEquals(filter(0), readValue.call(reader));
  assertEquals(filter(epsilon), readValue.call(reader));
  assertEquals(filter(upperLimit), readValue.call(reader));

  // Verify decoded values.
  for (var i = 0; i < inputValues.length; i++) {
    assertEquals(inputValues[i], readValue.call(reader));
  }
}

describe('binaryDecoderTest', function() {
  /**
   * Tests the decoder instance cache.
   * @suppress {visibility}
   */
  it('testInstanceCache', function() {
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
    var writer = new jspb.BinaryWriter();

    var hashA = String.fromCharCode(0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00);
    var hashB = String.fromCharCode(0x12, 0x34, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00);
    var hashC = String.fromCharCode(0x12, 0x34, 0x56, 0x78,
                                    0x87, 0x65, 0x43, 0x21);
    var hashD = String.fromCharCode(0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF);

    writer.rawWriteVarintHash64(hashA);
    writer.rawWriteVarintHash64(hashB);
    writer.rawWriteVarintHash64(hashC);
    writer.rawWriteVarintHash64(hashD);

    writer.rawWriteFixedHash64(hashA);
    writer.rawWriteFixedHash64(hashB);
    writer.rawWriteFixedHash64(hashC);
    writer.rawWriteFixedHash64(hashD);

    var decoder = jspb.BinaryDecoder.alloc(writer.getResultBuffer());

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
    decoder.setBlock(
        [255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0]);
    assertThrows(function() {decoder.readUnsignedVarint64()});
    decoder.reset();
    assertThrows(function() {decoder.readSignedVarint64()});
    decoder.reset();
    assertThrows(function() {decoder.readZigzagVarint64()});

    // Positive 32-bit varints encoded with 1 bits in positions 33 through 35
    // should trigger assertions.
    decoder.setBlock([255, 255, 255, 255, 0x1F]);
    assertThrows(function() {decoder.readUnsignedVarint32()});

    decoder.setBlock([255, 255, 255, 255, 0x2F]);
    assertThrows(function() {decoder.readUnsignedVarint32()});

    decoder.setBlock([255, 255, 255, 255, 0x4F]);
    assertThrows(function() {decoder.readUnsignedVarint32()});

    // Negative 32-bit varints encoded with non-1 bits in the high dword should
    // trigger assertions.
    decoder.setBlock([255, 255, 255, 255, 255, 255, 0, 255, 255, 1]);
    assertThrows(function() {decoder.readUnsignedVarint32()});

    decoder.setBlock([255, 255, 255, 255, 255, 255, 255, 255, 255, 0]);
    assertThrows(function() {decoder.readUnsignedVarint32()});
  });


  /**
   * Tests raw encoding and decoding of unsigned integers.
   */
  it('testRawUnsigned', function() {
    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint8,
        jspb.BinaryWriter.prototype.rawWriteUint8,
        1, 0xFF, Math.round);

    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint16,
        jspb.BinaryWriter.prototype.rawWriteUint16,
        1, 0xFFFF, Math.round);

    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint32,
        jspb.BinaryWriter.prototype.rawWriteUint32,
        1, 0xFFFFFFFF, Math.round);

    doTestUnsignedValue(
        jspb.BinaryDecoder.prototype.readUint64,
        jspb.BinaryWriter.prototype.rawWriteUint64,
        1, Math.pow(2, 64) - 1025, Math.round);
  });


  /**
   * Tests raw encoding and decoding of signed integers.
   */
  it('testRawSigned', function() {
    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt8,
        jspb.BinaryWriter.prototype.rawWriteInt8,
        1, -0x80, 0x7F, Math.round);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt16,
        jspb.BinaryWriter.prototype.rawWriteInt16,
        1, -0x8000, 0x7FFF, Math.round);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt32,
        jspb.BinaryWriter.prototype.rawWriteInt32,
        1, -0x80000000, 0x7FFFFFFF, Math.round);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readInt64,
        jspb.BinaryWriter.prototype.rawWriteInt64,
        1, -Math.pow(2, 63), Math.pow(2, 63) - 513, Math.round);
  });


  /**
   * Tests raw encoding and decoding of floats.
   */
  it('testRawFloats', function() {
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
        jspb.BinaryWriter.prototype.rawWriteFloat,
        jspb.BinaryConstants.FLOAT32_EPS,
        -jspb.BinaryConstants.FLOAT32_MAX,
        jspb.BinaryConstants.FLOAT32_MAX,
        truncate);

    doTestSignedValue(
        jspb.BinaryDecoder.prototype.readDouble,
        jspb.BinaryWriter.prototype.rawWriteDouble,
        jspb.BinaryConstants.FLOAT64_EPS * 10,
        -jspb.BinaryConstants.FLOAT64_MAX,
        jspb.BinaryConstants.FLOAT64_MAX,
        function(x) { return x; });
  });
});
