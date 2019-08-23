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
 * @fileoverview This file contains utilities for decoding primitive values
 * (signed and unsigned integers, varints, booleans, enums, hashes, strings,
 * and raw bytes) embedded in Uint8Arrays into their corresponding Javascript
 * types.
 *
 * Major caveat - Javascript is unable to accurately represent integers larger
 * than 2^53 due to its use of a double-precision floating point format or all
 * numbers. If you need to guarantee that 64-bit values survive with all bits
 * intact, you _must_ read them using one of the Hash64 methods, which return
 * an 8-character string.
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.provide('jspb.BinaryDecoder');

goog.require('goog.asserts');
goog.require('goog.crypt');
goog.require('jspb.utils');



/**
 * BinaryDecoder implements the decoders for all the wire types specified in
 * https://developers.google.com/protocol-buffers/docs/encoding.
 *
 * @param {jspb.ByteSource=} opt_bytes The bytes we're reading from.
 * @param {number=} opt_start The optional offset to start reading at.
 * @param {number=} opt_length The optional length of the block to read -
 *     we'll throw an assertion if we go off the end of the block.
 * @constructor
 * @struct
 */
jspb.BinaryDecoder = function(opt_bytes, opt_start, opt_length) {
  /**
   * Typed byte-wise view of the source buffer.
   * @private {?Uint8Array}
   */
  this.bytes_ = null;

  /**
   * Start point of the block to read.
   * @private {number}
   */
  this.start_ = 0;

  /**
   * End point of the block to read.
   * @private {number}
   */
  this.end_ = 0;

  /**
   * Current read location in bytes_.
   * @private {number}
   */
  this.cursor_ = 0;

  /**
   * Set to true if this decoder encountered an error due to corrupt data.
   * @private {boolean}
   */
  this.error_ = false;

  if (opt_bytes) {
    this.setBlock(opt_bytes, opt_start, opt_length);
  }
};


/**
 * Global pool of BinaryDecoder instances.
 * @private {!Array<!jspb.BinaryDecoder>}
 */
jspb.BinaryDecoder.instanceCache_ = [];


/**
 * Pops an instance off the instance cache, or creates one if the cache is
 * empty.
 * @param {jspb.ByteSource=} opt_bytes The bytes we're reading from.
 * @param {number=} opt_start The optional offset to start reading at.
 * @param {number=} opt_length The optional length of the block to read -
 *     we'll throw an assertion if we go off the end of the block.
 * @return {!jspb.BinaryDecoder}
 */
jspb.BinaryDecoder.alloc = function(opt_bytes, opt_start, opt_length) {
  if (jspb.BinaryDecoder.instanceCache_.length) {
    var newDecoder = jspb.BinaryDecoder.instanceCache_.pop();
    if (opt_bytes) {
      newDecoder.setBlock(opt_bytes, opt_start, opt_length);
    }
    return newDecoder;
  } else {
    return new jspb.BinaryDecoder(opt_bytes, opt_start, opt_length);
  }
};


/**
 * Puts this instance back in the instance cache.
 */
jspb.BinaryDecoder.prototype.free = function() {
  this.clear();
  if (jspb.BinaryDecoder.instanceCache_.length < 100) {
    jspb.BinaryDecoder.instanceCache_.push(this);
  }
};


/**
 * Makes a copy of this decoder.
 * @return {!jspb.BinaryDecoder}
 */
jspb.BinaryDecoder.prototype.clone = function() {
  return jspb.BinaryDecoder.alloc(this.bytes_,
      this.start_, this.end_ - this.start_);
};


/**
 * Clears the decoder.
 */
jspb.BinaryDecoder.prototype.clear = function() {
  this.bytes_ = null;
  this.start_ = 0;
  this.end_ = 0;
  this.cursor_ = 0;
  this.error_ = false;
};


/**
 * Returns the raw buffer.
 * @return {?Uint8Array} The raw buffer.
 */
jspb.BinaryDecoder.prototype.getBuffer = function() {
  return this.bytes_;
};


/**
 * Changes the block of bytes we're decoding.
 * @param {!jspb.ByteSource} data The bytes we're reading from.
 * @param {number=} opt_start The optional offset to start reading at.
 * @param {number=} opt_length The optional length of the block to read -
 *     we'll throw an assertion if we go off the end of the block.
 */
jspb.BinaryDecoder.prototype.setBlock =
    function(data, opt_start, opt_length) {
  this.bytes_ = jspb.utils.byteSourceToUint8Array(data);
  this.start_ = goog.isDef(opt_start) ? opt_start : 0;
  this.end_ =
      goog.isDef(opt_length) ? this.start_ + opt_length : this.bytes_.length;
  this.cursor_ = this.start_;
};


/**
 * @return {number}
 */
jspb.BinaryDecoder.prototype.getEnd = function() {
  return this.end_;
};


/**
 * @param {number} end
 */
jspb.BinaryDecoder.prototype.setEnd = function(end) {
  this.end_ = end;
};


/**
 * Moves the read cursor back to the start of the block.
 */
jspb.BinaryDecoder.prototype.reset = function() {
  this.cursor_ = this.start_;
};


/**
 * Returns the internal read cursor.
 * @return {number} The internal read cursor.
 */
jspb.BinaryDecoder.prototype.getCursor = function() {
  return this.cursor_;
};


/**
 * Returns the internal read cursor.
 * @param {number} cursor The new cursor.
 */
jspb.BinaryDecoder.prototype.setCursor = function(cursor) {
  this.cursor_ = cursor;
};


/**
 * Advances the stream cursor by the given number of bytes.
 * @param {number} count The number of bytes to advance by.
 */
jspb.BinaryDecoder.prototype.advance = function(count) {
  this.cursor_ += count;
  goog.asserts.assert(this.cursor_ <= this.end_);
};


/**
 * Returns true if this decoder is at the end of the block.
 * @return {boolean}
 */
jspb.BinaryDecoder.prototype.atEnd = function() {
  return this.cursor_ == this.end_;
};


/**
 * Returns true if this decoder is at the end of the block.
 * @return {boolean}
 */
jspb.BinaryDecoder.prototype.pastEnd = function() {
  return this.cursor_ > this.end_;
};


/**
 * Returns true if this decoder encountered an error due to corrupt data.
 * @return {boolean}
 */
jspb.BinaryDecoder.prototype.getError = function() {
  return this.error_ ||
         (this.cursor_ < 0) ||
         (this.cursor_ > this.end_);
};


/**
 * Reads an unsigned varint from the binary stream and invokes the conversion
 * function with the value in two signed 32 bit integers to produce the result.
 * Since this does not convert the value to a number, no precision is lost.
 *
 * It's possible for an unsigned varint to be incorrectly encoded - more than
 * 64 bits' worth of data could be present. If this happens, this method will
 * throw an error.
 *
 * Decoding varints requires doing some funny base-128 math - for more
 * details on the format, see
 * https://developers.google.com/protocol-buffers/docs/encoding
 *
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.BinaryDecoder.prototype.readSplitVarint64 = function(convert) {
  var temp = 128;
  var lowBits = 0;
  var highBits = 0;

  // Read the first four bytes of the varint, stopping at the terminator if we
  // see it.
  for (var i = 0; i < 4 && temp >= 128; i++) {
    temp = this.bytes_[this.cursor_++];
    lowBits |= (temp & 0x7F) << (i * 7);
  }

  if (temp >= 128) {
    // Read the fifth byte, which straddles the low and high dwords.
    temp = this.bytes_[this.cursor_++];
    lowBits |= (temp & 0x7F) << 28;
    highBits |= (temp & 0x7F) >> 4;
  }

  if (temp >= 128) {
    // Read the sixth through tenth byte.
    for (var i = 0; i < 5 && temp >= 128; i++) {
      temp = this.bytes_[this.cursor_++];
      highBits |= (temp & 0x7F) << (i * 7 + 3);
    }
  }

  if (temp < 128) {
    return convert(lowBits >>> 0, highBits >>> 0);
  }

  // If we did not see the terminator, the encoding was invalid.
  goog.asserts.fail('Failed to read varint, encoding is invalid.');
  this.error_ = true;
};


/**
 * Reads a signed zigzag encoded varint from the binary stream and invokes
 * the conversion function with the value in two signed 32 bit integers to
 * produce the result. Since this does not convert the value to a number, no
 * precision is lost.
 *
 * It's possible for an unsigned varint to be incorrectly encoded - more than
 * 64 bits' worth of data could be present. If this happens, this method will
 * throw an error.
 *
 * Zigzag encoding is a modification of varint encoding that reduces the
 * storage overhead for small negative integers - for more details on the
 * format, see https://developers.google.com/protocol-buffers/docs/encoding
 *
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.BinaryDecoder.prototype.readSplitZigzagVarint64 = function(convert) {
  return this.readSplitVarint64(function(low, high) {
    return jspb.utils.fromZigzag64(low, high, convert);
  });
};


/**
 * Reads a 64-bit fixed-width value from the stream and invokes the conversion
 * function with the value in two signed 32 bit integers to produce the result.
 * Since this does not convert the value to a number, no precision is lost.
 *
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.BinaryDecoder.prototype.readSplitFixed64 = function(convert) {
  var bytes = this.bytes_;
  var cursor = this.cursor_;
  this.cursor_ += 8;
  var lowBits = 0;
  var highBits = 0;
  for (var i = cursor + 7; i >= cursor; i--) {
    lowBits = (lowBits << 8) | bytes[i];
    highBits = (highBits << 8) | bytes[i + 4];
  }
  return convert(lowBits, highBits);
};


/**
 * Skips over a varint in the block without decoding it.
 */
jspb.BinaryDecoder.prototype.skipVarint = function() {
  while (this.bytes_[this.cursor_] & 0x80) {
    this.cursor_++;
  }
  this.cursor_++;
};


/**
 * Skips backwards over a varint in the block - to do this correctly, we have
 * to know the value we're skipping backwards over or things are ambiguous.
 * @param {number} value The varint value to unskip.
 */
jspb.BinaryDecoder.prototype.unskipVarint = function(value) {
  while (value > 128) {
    this.cursor_--;
    value = value >>> 7;
  }
  this.cursor_--;
};


/**
 * Reads a 32-bit varint from the binary stream. Due to a quirk of the encoding
 * format and Javascript's handling of bitwise math, this actually works
 * correctly for both signed and unsigned 32-bit varints.
 *
 * This function is called vastly more frequently than any other in
 * BinaryDecoder, so it has been unrolled and tweaked for performance.
 *
 * If there are more than 32 bits of data in the varint, it _must_ be due to
 * sign-extension. If we're in debug mode and the high 32 bits don't match the
 * expected sign extension, this method will throw an error.
 *
 * Decoding varints requires doing some funny base-128 math - for more
 * details on the format, see
 * https://developers.google.com/protocol-buffers/docs/encoding
 *
 * @return {number} The decoded unsigned 32-bit varint.
 */
jspb.BinaryDecoder.prototype.readUnsignedVarint32 = function() {
  var temp;
  var bytes = this.bytes_;

  temp = bytes[this.cursor_ + 0];
  var x = (temp & 0x7F);
  if (temp < 128) {
    this.cursor_ += 1;
    goog.asserts.assert(this.cursor_ <= this.end_);
    return x;
  }

  temp = bytes[this.cursor_ + 1];
  x |= (temp & 0x7F) << 7;
  if (temp < 128) {
    this.cursor_ += 2;
    goog.asserts.assert(this.cursor_ <= this.end_);
    return x;
  }

  temp = bytes[this.cursor_ + 2];
  x |= (temp & 0x7F) << 14;
  if (temp < 128) {
    this.cursor_ += 3;
    goog.asserts.assert(this.cursor_ <= this.end_);
    return x;
  }

  temp = bytes[this.cursor_ + 3];
  x |= (temp & 0x7F) << 21;
  if (temp < 128) {
    this.cursor_ += 4;
    goog.asserts.assert(this.cursor_ <= this.end_);
    return x;
  }

  temp = bytes[this.cursor_ + 4];
  x |= (temp & 0x0F) << 28;
  if (temp < 128) {
    // We're reading the high bits of an unsigned varint. The byte we just read
    // also contains bits 33 through 35, which we're going to discard.
    this.cursor_ += 5;
    goog.asserts.assert(this.cursor_ <= this.end_);
    return x >>> 0;
  }

  // If we get here, we need to truncate coming bytes. However we need to make
  // sure cursor place is correct.
  this.cursor_ += 5;
  if (bytes[this.cursor_++] >= 128 &&
      bytes[this.cursor_++] >= 128 &&
      bytes[this.cursor_++] >= 128 &&
      bytes[this.cursor_++] >= 128 &&
      bytes[this.cursor_++] >= 128) {
    // If we get here, the varint is too long.
    goog.asserts.assert(false);
  }

  goog.asserts.assert(this.cursor_ <= this.end_);
  return x;
};


/**
 * The readUnsignedVarint32 above deals with signed 32-bit varints correctly,
 * so this is just an alias.
 *
 * @return {number} The decoded signed 32-bit varint.
 */
jspb.BinaryDecoder.prototype.readSignedVarint32 =
    jspb.BinaryDecoder.prototype.readUnsignedVarint32;


/**
 * Reads a 32-bit unsigned variant and returns its value as a string.
 *
 * @return {string} The decoded unsigned 32-bit varint as a string.
 */
jspb.BinaryDecoder.prototype.readUnsignedVarint32String = function() {
  // 32-bit integers fit in JavaScript numbers without loss of precision, so
  // string variants of 32-bit varint readers can simply delegate then convert
  // to string.
  var value = this.readUnsignedVarint32();
  return value.toString();
};


/**
 * Reads a 32-bit signed variant and returns its value as a string.
 *
 * @return {string} The decoded signed 32-bit varint as a string.
 */
jspb.BinaryDecoder.prototype.readSignedVarint32String = function() {
  // 32-bit integers fit in JavaScript numbers without loss of precision, so
  // string variants of 32-bit varint readers can simply delegate then convert
  // to string.
  var value = this.readSignedVarint32();
  return value.toString();
};


/**
 * Reads a signed, zigzag-encoded 32-bit varint from the binary stream.
 *
 * Zigzag encoding is a modification of varint encoding that reduces the
 * storage overhead for small negative integers - for more details on the
 * format, see https://developers.google.com/protocol-buffers/docs/encoding
 *
 * @return {number} The decoded signed, zigzag-encoded 32-bit varint.
 */
jspb.BinaryDecoder.prototype.readZigzagVarint32 = function() {
  var result = this.readUnsignedVarint32();
  return (result >>> 1) ^ - (result & 1);
};


/**
 * Reads an unsigned 64-bit varint from the binary stream. Note that since
 * Javascript represents all numbers as double-precision floats, there will be
 * precision lost if the absolute value of the varint is larger than 2^53.
 *
 * @return {number} The decoded unsigned varint. Precision will be lost if the
 *     integer exceeds 2^53.
 */
jspb.BinaryDecoder.prototype.readUnsignedVarint64 = function() {
  return this.readSplitVarint64(jspb.utils.joinUint64);
};


/**
 * Reads an unsigned 64-bit varint from the binary stream and returns the value
 * as a decimal string.
 *
 * @return {string} The decoded unsigned varint as a decimal string.
 */
jspb.BinaryDecoder.prototype.readUnsignedVarint64String = function() {
  return this.readSplitVarint64(jspb.utils.joinUnsignedDecimalString);
};


/**
 * Reads a signed 64-bit varint from the binary stream. Note that since
 * Javascript represents all numbers as double-precision floats, there will be
 * precision lost if the absolute value of the varint is larger than 2^53.
 *
 * @return {number} The decoded signed varint. Precision will be lost if the
 *     integer exceeds 2^53.
 */
jspb.BinaryDecoder.prototype.readSignedVarint64 = function() {
  return this.readSplitVarint64(jspb.utils.joinInt64);
};


/**
 * Reads an signed 64-bit varint from the binary stream and returns the value
 * as a decimal string.
 *
 * @return {string} The decoded signed varint as a decimal string.
 */
jspb.BinaryDecoder.prototype.readSignedVarint64String = function() {
  return this.readSplitVarint64(jspb.utils.joinSignedDecimalString);
};


/**
 * Reads a signed, zigzag-encoded 64-bit varint from the binary stream. Note
 * that since Javascript represents all numbers as double-precision floats,
 * there will be precision lost if the absolute value of the varint is larger
 * than 2^53.
 *
 * Zigzag encoding is a modification of varint encoding that reduces the
 * storage overhead for small negative integers - for more details on the
 * format, see https://developers.google.com/protocol-buffers/docs/encoding
 *
 * @return {number} The decoded zigzag varint. Precision will be lost if the
 *     integer exceeds 2^53.
 */
jspb.BinaryDecoder.prototype.readZigzagVarint64 = function() {
  return this.readSplitVarint64(jspb.utils.joinZigzag64);
};


/**
 * Reads a signed, zigzag-encoded 64-bit varint from the binary stream
 * losslessly and returns it as an 8-character Unicode string for use as a hash
 * table key.
 *
 * Zigzag encoding is a modification of varint encoding that reduces the
 * storage overhead for small negative integers - for more details on the
 * format, see https://developers.google.com/protocol-buffers/docs/encoding
 *
 * @return {string} The decoded zigzag varint in hash64 format.
 */
jspb.BinaryDecoder.prototype.readZigzagVarintHash64 = function() {
  return this.readSplitZigzagVarint64(jspb.utils.joinHash64);
};


/**
 * Reads a signed, zigzag-encoded 64-bit varint from the binary stream and
 * returns its value as a string.
 *
 * Zigzag encoding is a modification of varint encoding that reduces the
 * storage overhead for small negative integers - for more details on the
 * format, see https://developers.google.com/protocol-buffers/docs/encoding
 *
 * @return {string} The decoded signed, zigzag-encoded 64-bit varint as a
 * string.
 */
jspb.BinaryDecoder.prototype.readZigzagVarint64String = function() {
  return this.readSplitZigzagVarint64(jspb.utils.joinSignedDecimalString);
};


/**
 * Reads a raw unsigned 8-bit integer from the binary stream.
 *
 * @return {number} The unsigned 8-bit integer read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readUint8 = function() {
  var a = this.bytes_[this.cursor_ + 0];
  this.cursor_ += 1;
  goog.asserts.assert(this.cursor_ <= this.end_);
  return a;
};


/**
 * Reads a raw unsigned 16-bit integer from the binary stream.
 *
 * @return {number} The unsigned 16-bit integer read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readUint16 = function() {
  var a = this.bytes_[this.cursor_ + 0];
  var b = this.bytes_[this.cursor_ + 1];
  this.cursor_ += 2;
  goog.asserts.assert(this.cursor_ <= this.end_);
  return (a << 0) | (b << 8);
};


/**
 * Reads a raw unsigned 32-bit integer from the binary stream.
 *
 * @return {number} The unsigned 32-bit integer read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readUint32 = function() {
  var a = this.bytes_[this.cursor_ + 0];
  var b = this.bytes_[this.cursor_ + 1];
  var c = this.bytes_[this.cursor_ + 2];
  var d = this.bytes_[this.cursor_ + 3];
  this.cursor_ += 4;
  goog.asserts.assert(this.cursor_ <= this.end_);
  return ((a << 0) | (b << 8) | (c << 16) | (d << 24)) >>> 0;
};


/**
 * Reads a raw unsigned 64-bit integer from the binary stream. Note that since
 * Javascript represents all numbers as double-precision floats, there will be
 * precision lost if the absolute value of the integer is larger than 2^53.
 *
 * @return {number} The unsigned 64-bit integer read from the binary stream.
 *     Precision will be lost if the integer exceeds 2^53.
 */
jspb.BinaryDecoder.prototype.readUint64 = function() {
  var bitsLow = this.readUint32();
  var bitsHigh = this.readUint32();
  return jspb.utils.joinUint64(bitsLow, bitsHigh);
};


/**
 * Reads a raw unsigned 64-bit integer from the binary stream. Note that since
 * Javascript represents all numbers as double-precision floats, there will be
 * precision lost if the absolute value of the integer is larger than 2^53.
 *
 * @return {string} The unsigned 64-bit integer read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readUint64String = function() {
  var bitsLow = this.readUint32();
  var bitsHigh = this.readUint32();
  return jspb.utils.joinUnsignedDecimalString(bitsLow, bitsHigh);
};


/**
 * Reads a raw signed 8-bit integer from the binary stream.
 *
 * @return {number} The signed 8-bit integer read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readInt8 = function() {
  var a = this.bytes_[this.cursor_ + 0];
  this.cursor_ += 1;
  goog.asserts.assert(this.cursor_ <= this.end_);
  return (a << 24) >> 24;
};


/**
 * Reads a raw signed 16-bit integer from the binary stream.
 *
 * @return {number} The signed 16-bit integer read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readInt16 = function() {
  var a = this.bytes_[this.cursor_ + 0];
  var b = this.bytes_[this.cursor_ + 1];
  this.cursor_ += 2;
  goog.asserts.assert(this.cursor_ <= this.end_);
  return (((a << 0) | (b << 8)) << 16) >> 16;
};


/**
 * Reads a raw signed 32-bit integer from the binary stream.
 *
 * @return {number} The signed 32-bit integer read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readInt32 = function() {
  var a = this.bytes_[this.cursor_ + 0];
  var b = this.bytes_[this.cursor_ + 1];
  var c = this.bytes_[this.cursor_ + 2];
  var d = this.bytes_[this.cursor_ + 3];
  this.cursor_ += 4;
  goog.asserts.assert(this.cursor_ <= this.end_);
  return (a << 0) | (b << 8) | (c << 16) | (d << 24);
};


/**
 * Reads a raw signed 64-bit integer from the binary stream. Note that since
 * Javascript represents all numbers as double-precision floats, there will be
 * precision lost if the absolute vlaue of the integer is larger than 2^53.
 *
 * @return {number} The signed 64-bit integer read from the binary stream.
 *     Precision will be lost if the integer exceeds 2^53.
 */
jspb.BinaryDecoder.prototype.readInt64 = function() {
  var bitsLow = this.readUint32();
  var bitsHigh = this.readUint32();
  return jspb.utils.joinInt64(bitsLow, bitsHigh);
};


/**
 * Reads a raw signed 64-bit integer from the binary stream and returns it as a
 * string.
 *
 * @return {string} The signed 64-bit integer read from the binary stream.
 *     Precision will be lost if the integer exceeds 2^53.
 */
jspb.BinaryDecoder.prototype.readInt64String = function() {
  var bitsLow = this.readUint32();
  var bitsHigh = this.readUint32();
  return jspb.utils.joinSignedDecimalString(bitsLow, bitsHigh);
};


/**
 * Reads a 32-bit floating-point number from the binary stream, using the
 * temporary buffer to realign the data.
 *
 * @return {number} The float read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readFloat = function() {
  var bitsLow = this.readUint32();
  var bitsHigh = 0;
  return jspb.utils.joinFloat32(bitsLow, bitsHigh);
};


/**
 * Reads a 64-bit floating-point number from the binary stream, using the
 * temporary buffer to realign the data.
 *
 * @return {number} The double read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readDouble = function() {
  var bitsLow = this.readUint32();
  var bitsHigh = this.readUint32();
  return jspb.utils.joinFloat64(bitsLow, bitsHigh);
};


/**
 * Reads a boolean value from the binary stream.
 * @return {boolean} The boolean read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readBool = function() {
  return !!this.bytes_[this.cursor_++];
};


/**
 * Reads an enum value from the binary stream, which are always encoded as
 * signed varints.
 * @return {number} The enum value read from the binary stream.
 */
jspb.BinaryDecoder.prototype.readEnum = function() {
  return this.readSignedVarint32();
};


/**
 * Reads and parses a UTF-8 encoded unicode string from the stream.
 * The code is inspired by maps.vectortown.parse.StreamedDataViewReader.
 * Supports codepoints from U+0000 up to U+10FFFF.
 * (http://en.wikipedia.org/wiki/UTF-8).
 * @param {number} length The length of the string to read.
 * @return {string} The decoded string.
 */
jspb.BinaryDecoder.prototype.readString = function(length) {
  var bytes = this.bytes_;
  var cursor = this.cursor_;
  var end = cursor + length;
  var codeUnits = [];

  var result = '';
  while (cursor < end) {
    var c = bytes[cursor++];
    if (c < 128) { // Regular 7-bit ASCII.
      codeUnits.push(c);
    } else if (c < 192) {
      // UTF-8 continuation mark. We are out of sync. This
      // might happen if we attempted to read a character
      // with more than four bytes.
      continue;
    } else if (c < 224) { // UTF-8 with two bytes.
      var c2 = bytes[cursor++];
      codeUnits.push(((c & 31) << 6) | (c2 & 63));
    } else if (c < 240) { // UTF-8 with three bytes.
      var c2 = bytes[cursor++];
      var c3 = bytes[cursor++];
      codeUnits.push(((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
    } else if (c < 248) { // UTF-8 with 4 bytes.
      var c2 = bytes[cursor++];
      var c3 = bytes[cursor++];
      var c4 = bytes[cursor++];
      // Characters written on 4 bytes have 21 bits for a codepoint.
      // We can't fit that on 16bit characters, so we use surrogates.
      var codepoint = ((c & 7) << 18) | ((c2 & 63) << 12) | ((c3 & 63) << 6) | (c4 & 63);
      // Surrogates formula from wikipedia.
      // 1. Subtract 0x10000 from codepoint
      codepoint -= 0x10000;
      // 2. Split this into the high 10-bit value and the low 10-bit value
      // 3. Add 0xD800 to the high value to form the high surrogate
      // 4. Add 0xDC00 to the low value to form the low surrogate:
      var low = (codepoint & 1023) + 0xDC00;
      var high = ((codepoint >> 10) & 1023) + 0xD800;
      codeUnits.push(high, low);
    }

    // Avoid exceeding the maximum stack size when calling `apply`.
    if (codeUnits.length >= 8192) {
      result += String.fromCharCode.apply(null, codeUnits);
      codeUnits.length = 0;
    }
  }
  result += goog.crypt.byteArrayToString(codeUnits);
  this.cursor_ = cursor;
  return result;
};


/**
 * Reads and parses a UTF-8 encoded unicode string (with length prefix) from
 * the stream.
 * @return {string} The decoded string.
 */
jspb.BinaryDecoder.prototype.readStringWithLength = function() {
  var length = this.readUnsignedVarint32();
  return this.readString(length);
};


/**
 * Reads a block of raw bytes from the binary stream.
 *
 * @param {number} length The number of bytes to read.
 * @return {!Uint8Array} The decoded block of bytes, or an empty block if the
 *     length was invalid.
 */
jspb.BinaryDecoder.prototype.readBytes = function(length) {
  if (length < 0 ||
      this.cursor_ + length > this.bytes_.length) {
    this.error_ = true;
    goog.asserts.fail('Invalid byte length!');
    return new Uint8Array(0);
  }

  var result = this.bytes_.subarray(this.cursor_, this.cursor_ + length);

  this.cursor_ += length;
  goog.asserts.assert(this.cursor_ <= this.end_);
  return result;
};


/**
 * Reads a 64-bit varint from the stream and returns it as an 8-character
 * Unicode string for use as a hash table key.
 *
 * @return {string} The hash value.
 */
jspb.BinaryDecoder.prototype.readVarintHash64 = function() {
  return this.readSplitVarint64(jspb.utils.joinHash64);
};


/**
 * Reads a 64-bit fixed-width value from the stream and returns it as an
 * 8-character Unicode string for use as a hash table key.
 *
 * @return {string} The hash value.
 */
jspb.BinaryDecoder.prototype.readFixedHash64 = function() {
  var bytes = this.bytes_;
  var cursor = this.cursor_;

  var a = bytes[cursor + 0];
  var b = bytes[cursor + 1];
  var c = bytes[cursor + 2];
  var d = bytes[cursor + 3];
  var e = bytes[cursor + 4];
  var f = bytes[cursor + 5];
  var g = bytes[cursor + 6];
  var h = bytes[cursor + 7];

  this.cursor_ += 8;

  return String.fromCharCode(a, b, c, d, e, f, g, h);
};
