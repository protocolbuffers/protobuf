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
 * @fileoverview Test cases for jspb's helper functions.
 *
 * Test suite is written using Jasmine -- see http://jasmine.github.io/
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.require('goog.crypt');
goog.require('goog.crypt.base64');
goog.require('goog.testing.asserts');
goog.require('jspb.BinaryConstants');
goog.require('jspb.BinaryWriter');
goog.require('jspb.utils');


/**
 * @param {number} x
 * @return {number}
 */
function truncate(x) {
  var temp = new Float32Array(1);
  temp[0] = x;
  return temp[0];
}


/**
 * Converts an 64-bit integer in split representation to a 64-bit hash string
 * (8 bits encoded per character).
 * @param {number} bitsLow The low 32 bits of the split 64-bit integer.
 * @param {number} bitsHigh The high 32 bits of the split 64-bit integer.
 * @return {string} The encoded hash string, 8 bits per character.
 */
function toHashString(bitsLow, bitsHigh) {
  return String.fromCharCode((bitsLow >>> 0) & 0xFF,
                             (bitsLow >>> 8) & 0xFF,
                             (bitsLow >>> 16) & 0xFF,
                             (bitsLow >>> 24) & 0xFF,
                             (bitsHigh >>> 0) & 0xFF,
                             (bitsHigh >>> 8) & 0xFF,
                             (bitsHigh >>> 16) & 0xFF,
                             (bitsHigh >>> 24) & 0xFF);
}


describe('binaryUtilsTest', function() {
  /**
   * Tests lossless binary-to-decimal conversion.
   */
  it('testDecimalConversion', function() {
    // Check some magic numbers.
    var result =
        jspb.utils.joinUnsignedDecimalString(0x89e80001, 0x8ac72304);
    assertEquals('10000000000000000001', result);

    result = jspb.utils.joinUnsignedDecimalString(0xacd05f15, 0x1b69b4b);
    assertEquals('123456789123456789', result);

    result = jspb.utils.joinUnsignedDecimalString(0xeb1f0ad2, 0xab54a98c);
    assertEquals('12345678901234567890', result);

    result = jspb.utils.joinUnsignedDecimalString(0xe3b70cb1, 0x891087b8);
    assertEquals('9876543210987654321', result);

    // Check limits.
    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x00000000);
    assertEquals('0', result);

    result = jspb.utils.joinUnsignedDecimalString(0xFFFFFFFF, 0xFFFFFFFF);
    assertEquals('18446744073709551615', result);

    // Check each bit of the low dword.
    for (var i = 0; i < 32; i++) {
      var low = (1 << i) >>> 0;
      result = jspb.utils.joinUnsignedDecimalString(low, 0);
      assertEquals('' + Math.pow(2, i), result);
    }

    // Check the first 20 bits of the high dword.
    for (var i = 0; i < 20; i++) {
      var high = (1 << i) >>> 0;
      result = jspb.utils.joinUnsignedDecimalString(0, high);
      assertEquals('' + Math.pow(2, 32 + i), result);
    }

    // V8's internal double-to-string conversion is inaccurate for values above
    // 2^52, even if they're representable integers - check the rest of the bits
    // manually against the correct string representations of 2^N.

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x00100000);
    assertEquals('4503599627370496', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x00200000);
    assertEquals('9007199254740992', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x00400000);
    assertEquals('18014398509481984', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x00800000);
    assertEquals('36028797018963968', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x01000000);
    assertEquals('72057594037927936', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x02000000);
    assertEquals('144115188075855872', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x04000000);
    assertEquals('288230376151711744', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x08000000);
    assertEquals('576460752303423488', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x10000000);
    assertEquals('1152921504606846976', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x20000000);
    assertEquals('2305843009213693952', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x40000000);
    assertEquals('4611686018427387904', result);

    result = jspb.utils.joinUnsignedDecimalString(0x00000000, 0x80000000);
    assertEquals('9223372036854775808', result);
  });


  /**
   * Going from hash strings to decimal strings should also be lossless.
   */
  it('testHashToDecimalConversion', function() {
    var result;
    var convert = jspb.utils.hash64ToDecimalString;

    result = convert(toHashString(0x00000000, 0x00000000), false);
    assertEquals('0', result);

    result = convert(toHashString(0x00000000, 0x00000000), true);
    assertEquals('0', result);

    result = convert(toHashString(0xFFFFFFFF, 0xFFFFFFFF), false);
    assertEquals('18446744073709551615', result);

    result = convert(toHashString(0xFFFFFFFF, 0xFFFFFFFF), true);
    assertEquals('-1', result);

    result = convert(toHashString(0x00000000, 0x80000000), false);
    assertEquals('9223372036854775808', result);

    result = convert(toHashString(0x00000000, 0x80000000), true);
    assertEquals('-9223372036854775808', result);

    result = convert(toHashString(0xacd05f15, 0x01b69b4b), false);
    assertEquals('123456789123456789', result);

    result = convert(toHashString(~0xacd05f15 + 1, ~0x01b69b4b), true);
    assertEquals('-123456789123456789', result);

    // And converting arrays of hashes should work the same way.
    result = jspb.utils.hash64ArrayToDecimalStrings([
      toHashString(0xFFFFFFFF, 0xFFFFFFFF),
      toHashString(0x00000000, 0x80000000),
      toHashString(0xacd05f15, 0x01b69b4b)], false);
    assertEquals(3, result.length);
    assertEquals('18446744073709551615', result[0]);
    assertEquals('9223372036854775808', result[1]);
    assertEquals('123456789123456789', result[2]);
  });

  /*
   * Going from decimal strings to hash strings should be lossless.
   */
  it('testDecimalToHashConversion', function() {
    var result;
    var convert = jspb.utils.decimalStringToHash64;

    result = convert('0');
    assertEquals(goog.crypt.byteArrayToString(
      [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), result);

    result = convert('-1');
    assertEquals(goog.crypt.byteArrayToString(
      [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]), result);

    result = convert('18446744073709551615');
    assertEquals(goog.crypt.byteArrayToString(
      [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]), result);

    result = convert('9223372036854775808');
    assertEquals(goog.crypt.byteArrayToString(
      [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80]), result);

    result = convert('-9223372036854775808');
    assertEquals(goog.crypt.byteArrayToString(
      [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80]), result);

    result = convert('123456789123456789');
    assertEquals(goog.crypt.byteArrayToString(
      [0x15, 0x5F, 0xD0, 0xAC, 0x4B, 0x9B, 0xB6, 0x01]), result);

    result = convert('-123456789123456789');
    assertEquals(goog.crypt.byteArrayToString(
      [0xEB, 0xA0, 0x2F, 0x53, 0xB4, 0x64, 0x49, 0xFE]), result);
  });

  /**
   * Going from hash strings to hex strings should be lossless.
   */
  it('testHashToHexConversion', function() {
    var result;
    var convert = jspb.utils.hash64ToHexString;

    result = convert(toHashString(0x00000000, 0x00000000));
    assertEquals('0x0000000000000000', result);

    result = convert(toHashString(0xFFFFFFFF, 0xFFFFFFFF));
    assertEquals('0xffffffffffffffff', result);

    result = convert(toHashString(0x12345678, 0x9ABCDEF0));
    assertEquals('0x9abcdef012345678', result);
  });


  /**
   * Going from hex strings to hash strings should be lossless.
   */
  it('testHexToHashConversion', function() {
    var result;
    var convert = jspb.utils.hexStringToHash64;

    result = convert('0x0000000000000000');
    assertEquals(goog.crypt.byteArrayToString(
        [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]), result);

    result = convert('0xffffffffffffffff');
    assertEquals(goog.crypt.byteArrayToString(
        [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]), result);

    // Hex string is big-endian, hash string is little-endian.
    result = convert('0x123456789ABCDEF0');
    assertEquals(goog.crypt.byteArrayToString(
        [0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12]), result);

    // Capitalization should not matter.
    result = convert('0x0000abcdefABCDEF');
    assertEquals(goog.crypt.byteArrayToString(
        [0xEF, 0xCD, 0xAB, 0xEF, 0xCD, 0xAB, 0x00, 0x00]), result);
  });


  /**
   * Going from numbers to hash strings should be lossless for up to 53 bits of
   * precision.
   */
  it('testNumberToHashConversion', function() {
    var result;
    var convert = jspb.utils.numberToHash64;

    result = convert(0x0000000000000);
    assertEquals('0x0000000000000000', jspb.utils.hash64ToHexString(result));

    result = convert(0xFFFFFFFFFFFFF);
    assertEquals('0x000fffffffffffff', jspb.utils.hash64ToHexString(result));

    result = convert(0x123456789ABCD);
    assertEquals('0x000123456789abcd', jspb.utils.hash64ToHexString(result));

    result = convert(0xDCBA987654321);
    assertEquals('0x000dcba987654321', jspb.utils.hash64ToHexString(result));

    // 53 bits of precision should not be truncated.
    result = convert(0x10000000000001);
    assertEquals('0x0010000000000001', jspb.utils.hash64ToHexString(result));

    // 54 bits of precision should be truncated.
    result = convert(0x20000000000001);
    assertNotEquals(
        '0x0020000000000001', jspb.utils.hash64ToHexString(result));
  });


  /**
   * Sanity check the behavior of Javascript's strings when doing funny things
   * with unicode characters.
   */
  it('sanityCheckUnicodeStrings', function() {
    var strings = new Array(65536);

    // All possible unsigned 16-bit values should be storable in a string, they
    // shouldn't do weird things with the length of the string, and they should
    // come back out of the string unchanged.
    for (var i = 0; i < 65536; i++) {
      strings[i] = 'a' + String.fromCharCode(i) + 'a';
      if (3 != strings[i].length) throw 'fail!';
      if (i != strings[i].charCodeAt(1)) throw 'fail!';
    }

    // Each unicode character should compare equal to itself and not equal to a
    // different unicode character.
    for (var i = 0; i < 65536; i++) {
      if (strings[i] != strings[i]) throw 'fail!';
      if (strings[i] == strings[(i + 1) % 65536]) throw 'fail!';
    }
  });


  /**
   * Tests conversion from 32-bit floating point numbers to split64 numbers.
   */
  it('testFloat32ToSplit64', function() {
    var f32_eps = jspb.BinaryConstants.FLOAT32_EPS;
    var f32_min = jspb.BinaryConstants.FLOAT32_MIN;
    var f32_max = jspb.BinaryConstants.FLOAT32_MAX;

    // NaN.
    jspb.utils.splitFloat32(NaN);
    if (!isNaN(jspb.utils.joinFloat32(jspb.utils.split64Low,
                                      jspb.utils.split64High))) {
      throw 'fail!';
    }

    /**
     * @param {number} x
     * @param {number=} opt_bits
     */
    function test(x, opt_bits) {
      jspb.utils.splitFloat32(x);
      if (goog.isDef(opt_bits)) {
        if (opt_bits != jspb.utils.split64Low) throw 'fail!';
      }
      if (truncate(x) != jspb.utils.joinFloat32(jspb.utils.split64Low,
          jspb.utils.split64High)) {
        throw 'fail!';
      }
    }

    // Positive and negative infinity.
    test(Infinity, 0x7f800000);
    test(-Infinity, 0xff800000);

    // Positive and negative zero.
    test(0, 0x00000000);
    test(-0, 0x80000000);

    // Positive and negative epsilon.
    test(f32_eps, 0x00000001);
    test(-f32_eps, 0x80000001);

    // Positive and negative min.
    test(f32_min, 0x00800000);
    test(-f32_min, 0x80800000);

    // Positive and negative max.
    test(f32_max, 0x7F7FFFFF);
    test(-f32_max, 0xFF7FFFFF);

    // Various positive values.
    var cursor = f32_eps * 10;
    while (cursor != Infinity) {
      test(cursor);
      cursor *= 1.1;
    }

    // Various negative values.
    cursor = -f32_eps * 10;
    while (cursor != -Infinity) {
      test(cursor);
      cursor *= 1.1;
    }
  });


  /**
   * Tests conversion from 64-bit floating point numbers to split64 numbers.
   */
  it('testFloat64ToSplit64', function() {
    var f64_eps = jspb.BinaryConstants.FLOAT64_EPS;
    var f64_min = jspb.BinaryConstants.FLOAT64_MIN;
    var f64_max = jspb.BinaryConstants.FLOAT64_MAX;

    // NaN.
    jspb.utils.splitFloat64(NaN);
    if (!isNaN(jspb.utils.joinFloat64(jspb.utils.split64Low,
        jspb.utils.split64High))) {
      throw 'fail!';
    }

    /**
     * @param {number} x
     * @param {number=} opt_highBits
     * @param {number=} opt_lowBits
     */
    function test(x, opt_highBits, opt_lowBits) {
      jspb.utils.splitFloat64(x);
      if (goog.isDef(opt_highBits)) {
        if (opt_highBits != jspb.utils.split64High) throw 'fail!';
      }
      if (goog.isDef(opt_lowBits)) {
        if (opt_lowBits != jspb.utils.split64Low) throw 'fail!';
      }
      if (x != jspb.utils.joinFloat64(jspb.utils.split64Low,
          jspb.utils.split64High)) {
        throw 'fail!';
      }
    }

    // Positive and negative infinity.
    test(Infinity, 0x7ff00000, 0x00000000);
    test(-Infinity, 0xfff00000, 0x00000000);

    // Positive and negative zero.
    test(0, 0x00000000, 0x00000000);
    test(-0, 0x80000000, 0x00000000);

    // Positive and negative epsilon.
    test(f64_eps, 0x00000000, 0x00000001);
    test(-f64_eps, 0x80000000, 0x00000001);

    // Positive and negative min.
    test(f64_min, 0x00100000, 0x00000000);
    test(-f64_min, 0x80100000, 0x00000000);

    // Positive and negative max.
    test(f64_max, 0x7FEFFFFF, 0xFFFFFFFF);
    test(-f64_max, 0xFFEFFFFF, 0xFFFFFFFF);

    // Various positive values.
    var cursor = f64_eps * 10;
    while (cursor != Infinity) {
      test(cursor);
      cursor *= 1.1;
    }

    // Various negative values.
    cursor = -f64_eps * 10;
    while (cursor != -Infinity) {
      test(cursor);
      cursor *= 1.1;
    }
  });


  /**
   * Tests counting packed varints.
   */
  it('testCountVarints', function() {
    var values = [];
    for (var i = 1; i < 1000000000; i *= 1.1) {
      values.push(Math.floor(i));
    }

    var writer = new jspb.BinaryWriter();
    writer.writePackedUint64(1, values);

    var buffer = new Uint8Array(writer.getResultBuffer());

    // We should have two more varints than we started with - one for the field
    // tag, one for the packed length.
    assertEquals(values.length + 2,
                 jspb.utils.countVarints(buffer, 0, buffer.length));
  });


  /**
   * Tests counting matching varint fields.
   */
  it('testCountVarintFields', function() {
    var writer = new jspb.BinaryWriter();

    var count = 0;
    for (var i = 1; i < 1000000000; i *= 1.1) {
      writer.writeUint64(1, Math.floor(i));
      count++;
    }
    writer.writeString(2, 'terminator');

    var buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countVarintFields(buffer, 0, buffer.length, 1));

    writer = new jspb.BinaryWriter();

    count = 0;
    for (var i = 1; i < 1000000000; i *= 1.1) {
      writer.writeUint64(123456789, Math.floor(i));
      count++;
    }
    writer.writeString(2, 'terminator');

    buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countVarintFields(buffer, 0, buffer.length, 123456789));
  });


  /**
   * Tests counting matching fixed32 fields.
   */
  it('testCountFixed32Fields', function() {
    var writer = new jspb.BinaryWriter();

    var count = 0;
    for (var i = 1; i < 1000000000; i *= 1.1) {
      writer.writeFixed32(1, Math.floor(i));
      count++;
    }
    writer.writeString(2, 'terminator');

    var buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countFixed32Fields(buffer, 0, buffer.length, 1));

    writer = new jspb.BinaryWriter();

    count = 0;
    for (var i = 1; i < 1000000000; i *= 1.1) {
      writer.writeFixed32(123456789, Math.floor(i));
      count++;
    }
    writer.writeString(2, 'terminator');

    buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countFixed32Fields(buffer, 0, buffer.length, 123456789));
  });


  /**
   * Tests counting matching fixed64 fields.
   */
  it('testCountFixed64Fields', function() {
    var writer = new jspb.BinaryWriter();

    var count = 0;
    for (var i = 1; i < 1000000000; i *= 1.1) {
      writer.writeDouble(1, i);
      count++;
    }
    writer.writeString(2, 'terminator');

    var buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countFixed64Fields(buffer, 0, buffer.length, 1));

    writer = new jspb.BinaryWriter();

    count = 0;
    for (var i = 1; i < 1000000000; i *= 1.1) {
      writer.writeDouble(123456789, i);
      count++;
    }
    writer.writeString(2, 'terminator');

    buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countFixed64Fields(buffer, 0, buffer.length, 123456789));
  });


  /**
   * Tests counting matching delimited fields.
   */
  it('testCountDelimitedFields', function() {
    var writer = new jspb.BinaryWriter();

    var count = 0;
    for (var i = 1; i < 1000; i *= 1.1) {
      writer.writeBytes(1, [Math.floor(i)]);
      count++;
    }
    writer.writeString(2, 'terminator');

    var buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countDelimitedFields(buffer, 0, buffer.length, 1));

    writer = new jspb.BinaryWriter();

    count = 0;
    for (var i = 1; i < 1000; i *= 1.1) {
      writer.writeBytes(123456789, [Math.floor(i)]);
      count++;
    }
    writer.writeString(2, 'terminator');

    buffer = new Uint8Array(writer.getResultBuffer());
    assertEquals(count,
        jspb.utils.countDelimitedFields(buffer, 0, buffer.length, 123456789));
  });


  /**
   * Tests byte format for debug strings.
   */
  it('testDebugBytesToTextFormat', function() {
    assertEquals('""', jspb.utils.debugBytesToTextFormat(null));
    assertEquals('"\\x00\\x10\\xff"',
        jspb.utils.debugBytesToTextFormat([0, 16, 255]));
  });


  /**
   * Tests converting byte blob sources into byte blobs.
   */
  it('testByteSourceToUint8Array', function() {
    var convert = jspb.utils.byteSourceToUint8Array;

    var sourceData = [];
    for (var i = 0; i < 256; i++) {
      sourceData.push(i);
    }

    var sourceBytes = new Uint8Array(sourceData);
    var sourceBuffer = sourceBytes.buffer;
    var sourceBase64 = goog.crypt.base64.encodeByteArray(sourceData);
    var sourceString = goog.crypt.byteArrayToString(sourceData);

    function check(result) {
      assertEquals(Uint8Array, result.constructor);
      assertEquals(sourceData.length, result.length);
      for (var i = 0; i < result.length; i++) {
        assertEquals(sourceData[i], result[i]);
      }
    }

    // Converting Uint8Arrays into Uint8Arrays should be a no-op.
    assertEquals(sourceBytes, convert(sourceBytes));

    // Converting Array.<numbers> into Uint8Arrays should work.
    check(convert(sourceData));

    // Converting ArrayBuffers into Uint8Arrays should work.
    check(convert(sourceBuffer));

    // Converting base64-encoded strings into Uint8Arrays should work.
    check(convert(sourceBase64));
  });
});
