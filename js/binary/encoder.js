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
 * @fileoverview BinaryEncode defines methods for encoding Javascript values
 * into arrays of bytes compatible with the Protocol Buffer wire format.
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.provide('jspb.BinaryEncoder');

goog.require('goog.asserts');
goog.require('jspb.BinaryConstants');
goog.require('jspb.utils');



/**
 * BinaryEncoder implements encoders for all the wire types specified in
 * https://developers.google.com/protocol-buffers/docs/encoding.
 *
 * @constructor
 * @struct
 */
jspb.BinaryEncoder = function() {
  /** @private {!Array.<number>} */
  this.buffer_ = [];
};


/**
 * @return {number}
 */
jspb.BinaryEncoder.prototype.length = function() {
  return this.buffer_.length;
};


/**
 * @return {!Array.<number>}
 */
jspb.BinaryEncoder.prototype.end = function() {
  var buffer = this.buffer_;
  this.buffer_ = [];
  return buffer;
};


/**
 * Encodes a 64-bit integer in 32:32 split representation into its wire-format
 * varint representation and stores it in the buffer.
 * @param {number} lowBits The low 32 bits of the int.
 * @param {number} highBits The high 32 bits of the int.
 */
jspb.BinaryEncoder.prototype.writeSplitVarint64 = function(lowBits, highBits) {
  goog.asserts.assert(lowBits == Math.floor(lowBits));
  goog.asserts.assert(highBits == Math.floor(highBits));
  goog.asserts.assert((lowBits >= 0) &&
                      (lowBits < jspb.BinaryConstants.TWO_TO_32));
  goog.asserts.assert((highBits >= 0) &&
                      (highBits < jspb.BinaryConstants.TWO_TO_32));

  // Break the binary representation into chunks of 7 bits, set the 8th bit
  // in each chunk if it's not the final chunk, and append to the result.
  while (highBits > 0 || lowBits > 127) {
    this.buffer_.push((lowBits & 0x7f) | 0x80);
    lowBits = ((lowBits >>> 7) | (highBits << 25)) >>> 0;
    highBits = highBits >>> 7;
  }
  this.buffer_.push(lowBits);
};


/**
 * Encodes a 32-bit unsigned integer into its wire-format varint representation
 * and stores it in the buffer.
 * @param {number} value The integer to convert.
 */
jspb.BinaryEncoder.prototype.writeUnsignedVarint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_32));

  while (value > 127) {
    this.buffer_.push((value & 0x7f) | 0x80);
    value = value >>> 7;
  }

  this.buffer_.push(value);
};


/**
 * Encodes a 32-bit signed integer into its wire-format varint representation
 * and stores it in the buffer.
 * @param {number} value The integer to convert.
 */
jspb.BinaryEncoder.prototype.writeSignedVarint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));

  // Use the unsigned version if the value is not negative.
  if (value >= 0) {
    this.writeUnsignedVarint32(value);
    return;
  }

  // Write nine bytes with a _signed_ right shift so we preserve the sign bit.
  for (var i = 0; i < 9; i++) {
    this.buffer_.push((value & 0x7f) | 0x80);
    value = value >> 7;
  }

  // The above loop writes out 63 bits, so the last byte is always the sign bit
  // which is always set for negative numbers.
  this.buffer_.push(1);
};


/**
 * Encodes a 64-bit unsigned integer into its wire-format varint representation
 * and stores it in the buffer. Integers that are not representable in 64 bits
 * will be truncated.
 * @param {number} value The integer to convert.
 */
jspb.BinaryEncoder.prototype.writeUnsignedVarint64 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_64));
  jspb.utils.splitInt64(value);
  this.writeSplitVarint64(jspb.utils.split64Low,
                          jspb.utils.split64High);
};


/**
 * Encodes a 64-bit signed integer into its wire-format varint representation
 * and stores it in the buffer. Integers that are not representable in 64 bits
 * will be truncated.
 * @param {number} value The integer to convert.
 */
jspb.BinaryEncoder.prototype.writeSignedVarint64 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (value < jspb.BinaryConstants.TWO_TO_63));
  jspb.utils.splitInt64(value);
  this.writeSplitVarint64(jspb.utils.split64Low,
                          jspb.utils.split64High);
};


/**
 * Encodes a JavaScript integer into its wire-format, zigzag-encoded varint
 * representation and stores it in the buffer.
 * @param {number} value The integer to convert.
 */
jspb.BinaryEncoder.prototype.writeZigzagVarint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.writeUnsignedVarint32(((value << 1) ^ (value >> 31)) >>> 0);
};


/**
 * Encodes a JavaScript integer into its wire-format, zigzag-encoded varint
 * representation and stores it in the buffer. Integers not representable in 64
 * bits will be truncated.
 * @param {number} value The integer to convert.
 */
jspb.BinaryEncoder.prototype.writeZigzagVarint64 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (value < jspb.BinaryConstants.TWO_TO_63));
  jspb.utils.splitZigzag64(value);
  this.writeSplitVarint64(jspb.utils.split64Low,
                          jspb.utils.split64High);
};


/**
 * Writes a 8-bit unsigned integer to the buffer. Numbers outside the range
 * [0,2^8) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeUint8 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) && (value < 256));
  this.buffer_.push((value >>> 0) & 0xFF);
};


/**
 * Writes a 16-bit unsigned integer to the buffer. Numbers outside the
 * range [0,2^16) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeUint16 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) && (value < 65536));
  this.buffer_.push((value >>> 0) & 0xFF);
  this.buffer_.push((value >>> 8) & 0xFF);
};


/**
 * Writes a 32-bit unsigned integer to the buffer. Numbers outside the
 * range [0,2^32) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeUint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_32));
  this.buffer_.push((value >>> 0) & 0xFF);
  this.buffer_.push((value >>> 8) & 0xFF);
  this.buffer_.push((value >>> 16) & 0xFF);
  this.buffer_.push((value >>> 24) & 0xFF);
};


/**
 * Writes a 64-bit unsigned integer to the buffer. Numbers outside the
 * range [0,2^64) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeUint64 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_64));
  jspb.utils.splitUint64(value);
  this.writeUint32(jspb.utils.split64Low);
  this.writeUint32(jspb.utils.split64High);
};


/**
 * Writes a 8-bit integer to the buffer. Numbers outside the range
 * [-2^7,2^7) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeInt8 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -128) && (value < 128));
  this.buffer_.push((value >>> 0) & 0xFF);
};


/**
 * Writes a 16-bit integer to the buffer. Numbers outside the range
 * [-2^15,2^15) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeInt16 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -32768) && (value < 32768));
  this.buffer_.push((value >>> 0) & 0xFF);
  this.buffer_.push((value >>> 8) & 0xFF);
};


/**
 * Writes a 32-bit integer to the buffer. Numbers outside the range
 * [-2^31,2^31) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeInt32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.buffer_.push((value >>> 0) & 0xFF);
  this.buffer_.push((value >>> 8) & 0xFF);
  this.buffer_.push((value >>> 16) & 0xFF);
  this.buffer_.push((value >>> 24) & 0xFF);
};


/**
 * Writes a 64-bit integer to the buffer. Numbers outside the range
 * [-2^63,2^63) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeInt64 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (value < jspb.BinaryConstants.TWO_TO_63));
  jspb.utils.splitInt64(value);
  this.writeUint32(jspb.utils.split64Low);
  this.writeUint32(jspb.utils.split64High);
};


/**
 * Writes a single-precision floating point value to the buffer. Numbers
 * requiring more than 32 bits of precision will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeFloat = function(value) {
  goog.asserts.assert((value >= -jspb.BinaryConstants.FLOAT32_MAX) &&
                      (value <= jspb.BinaryConstants.FLOAT32_MAX));
  jspb.utils.splitFloat32(value);
  this.writeUint32(jspb.utils.split64Low);
};


/**
 * Writes a double-precision floating point value to the buffer. As this is
 * the native format used by JavaScript, no precision will be lost.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeDouble = function(value) {
  goog.asserts.assert((value >= -jspb.BinaryConstants.FLOAT64_MAX) &&
                      (value <= jspb.BinaryConstants.FLOAT64_MAX));
  jspb.utils.splitFloat64(value);
  this.writeUint32(jspb.utils.split64Low);
  this.writeUint32(jspb.utils.split64High);
};


/**
 * Writes a boolean value to the buffer as a varint.
 * @param {boolean} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeBool = function(value) {
  goog.asserts.assert(goog.isBoolean(value));
  this.buffer_.push(value ? 1 : 0);
};


/**
 * Writes an enum value to the buffer as a varint.
 * @param {number} value The value to write.
 */
jspb.BinaryEncoder.prototype.writeEnum = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.writeSignedVarint32(value);
};


/**
 * Writes an arbitrary byte array to the buffer.
 * @param {!Uint8Array} bytes The array of bytes to write.
 */
jspb.BinaryEncoder.prototype.writeBytes = function(bytes) {
  this.buffer_.push.apply(this.buffer_, bytes);
};


/**
 * Writes a 64-bit hash string (8 characters @ 8 bits of data each) to the
 * buffer as a varint.
 * @param {string} hash The hash to write.
 */
jspb.BinaryEncoder.prototype.writeVarintHash64 = function(hash) {
  jspb.utils.splitHash64(hash);
  this.writeSplitVarint64(jspb.utils.split64Low,
                          jspb.utils.split64High);
};


/**
 * Writes a 64-bit hash string (8 characters @ 8 bits of data each) to the
 * buffer as a fixed64.
 * @param {string} hash The hash to write.
 */
jspb.BinaryEncoder.prototype.writeFixedHash64 = function(hash) {
  jspb.utils.splitHash64(hash);
  this.writeUint32(jspb.utils.split64Low);
  this.writeUint32(jspb.utils.split64High);
};


/**
 * Writes a UTF16 Javascript string to the buffer encoded as UTF8.
 * TODO(aappleby): Add support for surrogate pairs, reject unpaired surrogates.
 * @param {string} value The string to write.
 * @return {number} The number of bytes used to encode the string.
 */
jspb.BinaryEncoder.prototype.writeString = function(value) {
  var oldLength = this.buffer_.length;

  // UTF16 to UTF8 conversion loop swiped from goog.crypt.stringToUtf8ByteArray.
  for (var i = 0; i < value.length; i++) {
    var c = value.charCodeAt(i);
    if (c < 128) {
      this.buffer_.push(c);
    } else if (c < 2048) {
      this.buffer_.push((c >> 6) | 192);
      this.buffer_.push((c & 63) | 128);
    } else {
      this.buffer_.push((c >> 12) | 224);
      this.buffer_.push(((c >> 6) & 63) | 128);
      this.buffer_.push((c & 63) | 128);
    }
  }

  var length = this.buffer_.length - oldLength;
  return length;
};
