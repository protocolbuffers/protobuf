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
 * @fileoverview This file contains utilities for encoding Javascript objects
 * into binary, wire-format protocol buffers (in the form of Uint8Arrays) that
 * a server can consume directly.
 *
 * jspb's BinaryWriter class defines methods for efficiently encoding
 * Javascript objects into binary, wire-format protocol buffers and supports
 * all the fundamental field types used in protocol buffers.
 *
 * Major caveat 1 - Users of this library _must_ keep their Javascript proto
 * parsing code in sync with the original .proto file - presumably you'll be
 * using the typed jspb code generator, but if you bypass that you'll need
 * to keep things in sync by hand.
 *
 * Major caveat 2 - Javascript is unable to accurately represent integers
 * larger than 2^53 due to its use of a double-precision floating point format
 * for all numbers. BinaryWriter does not make any special effort to preserve
 * precision for values above this limit - if you need to pass 64-bit integers
 * (hash codes, for example) between the client and server without precision
 * loss, do _not_ use this library.
 *
 * Major caveat 3 - This class uses typed arrays and must not be used on older
 * browsers that do not support them.
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.provide('jspb.BinaryWriter');

goog.require('goog.asserts');
goog.require('goog.crypt.base64');
goog.require('jspb.BinaryConstants');
goog.require('jspb.arith.Int64');
goog.require('jspb.arith.UInt64');
goog.require('jspb.utils');

goog.forwardDeclare('jspb.Message');



/**
 * BinaryWriter implements encoders for all the wire types specified in
 * https://developers.google.com/protocol-buffers/docs/encoding.
 *
 * @constructor
 * @struct
 */
jspb.BinaryWriter = function() {
  /**
   * Blocks of serialized data that will be concatenated once all messages have
   * been written.
   * @private {!Array<!Uint8Array|!Array<number>>}
   */
  this.blocks_ = [];

  /**
   * Total number of bytes in the blocks_ array. Does _not_ include the temp
   * buffer.
   * @private {number}
   */
  this.totalLength_ = 0;

  /**
   * Temporary buffer holding a message that we're still serializing. When we
   * get to a stopping point (either the start of a new submessage, or when we
   * need to append a raw Uint8Array), the temp buffer will be added to the
   * block array above and a new temp buffer will be created.
   * @private {!Array.<number>}
   */
  this.temp_ = [];

  /**
   * A stack of bookmarks containing the parent blocks for each message started
   * via beginSubMessage(), needed as bookkeeping for endSubMessage().
   * TODO(aappleby): Deprecated, users should be calling writeMessage().
   * @private {!Array.<!jspb.BinaryWriter.Bookmark_>}
   */
  this.bookmarks_ = [];
};


/**
 * @typedef {{block: !Array.<number>, length: number}}
 * @private
 */
jspb.BinaryWriter.Bookmark_;


/**
 * Saves the current temp buffer in the blocks_ array and starts a new one.
 * @return {!Array.<number>} Returns a reference to the old temp buffer.
 * @private
 */
jspb.BinaryWriter.prototype.saveTempBuffer_ = function() {
  var oldTemp = this.temp_;
  this.blocks_.push(this.temp_);
  this.totalLength_ += this.temp_.length;
  this.temp_ = [];
  return oldTemp;
};


/**
 * Append a typed array of bytes onto the buffer.
 *
 * @param {!Uint8Array} arr The byte array to append.
 * @private
 */
jspb.BinaryWriter.prototype.appendUint8Array_ = function(arr) {
  if (this.temp_.length) {
    this.saveTempBuffer_();
  }
  this.blocks_.push(arr);
  this.totalLength_ += arr.length;
};


/**
 * Append an untyped array of bytes onto the buffer.
 *
 * @param {!Array.<number>} arr The byte array to append.
 * @private
 */
jspb.BinaryWriter.prototype.appendArray_ = function(arr) {
  if (this.temp_.length) {
    this.saveTempBuffer_();
  }
  this.temp_ = arr;
};


/**
 * Begins a length-delimited section by writing the field header to the current
 * temp buffer and then saving it in the block array. Returns the saved block,
 * which we will append the length to in endDelimited_ below.
 * @param {number} field
 * @return {!jspb.BinaryWriter.Bookmark_}
 * @private
 */
jspb.BinaryWriter.prototype.beginDelimited_ = function(field) {
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  return {block: this.saveTempBuffer_(), length: this.totalLength_};
};


/**
 * Ends a length-delimited block by encoding the _change_ in length of the
 * buffer to the parent block and adds the number of bytes needed to encode
 * that length to the total byte length. Note that 'parentLength' _must_ be the
 * total length _after_ the field header was written in beginDelimited_ above.
 * @param {!jspb.BinaryWriter.Bookmark_} bookmark
 * @private
 */
jspb.BinaryWriter.prototype.endDelimited_ = function(bookmark) {
  var messageLength = this.totalLength_ + this.temp_.length - bookmark.length;
  goog.asserts.assert(messageLength >= 0);

  var bytes = 1;
  while (messageLength > 127) {
    bookmark.block.push((messageLength & 0x7f) | 0x80);
    messageLength = messageLength >>> 7;
    bytes++;
  }

  bookmark.block.push(messageLength);
  this.totalLength_ += bytes;
};


/**
 * Resets the writer, throwing away any accumulated buffers.
 */
jspb.BinaryWriter.prototype.reset = function() {
  this.blocks_ = [];
  this.temp_ = [];
  this.totalLength_ = 0;
  this.bookmarks_ = [];
};


/**
 * Converts the encoded data into a Uint8Array.
 * @return {!Uint8Array}
 */
jspb.BinaryWriter.prototype.getResultBuffer = function() {
  goog.asserts.assert(this.bookmarks_.length == 0);

  var flat = new Uint8Array(this.totalLength_ + this.temp_.length);

  var blocks = this.blocks_;
  var blockCount = blocks.length;
  var offset = 0;

  for (var i = 0; i < blockCount; i++) {
    var block = blocks[i];
    flat.set(block, offset);
    offset += block.length;
  }

  flat.set(this.temp_, offset);
  offset += this.temp_.length;

  // Post condition: `flattened` must have had every byte written.
  goog.asserts.assert(offset == flat.length);

  // Replace our block list with the flattened block, which lets GC reclaim
  // the temp blocks sooner.
  this.blocks_ = [flat];
  this.temp_ = [];

  return flat;
};


/**
 * Converts the encoded data into a bas64-encoded string.
 * @return {string}
 */
jspb.BinaryWriter.prototype.getResultBase64String = function() {
  return goog.crypt.base64.encodeByteArray(this.getResultBuffer());
};


/**
 * Begins a new sub-message. The client must call endSubMessage() when they're
 * done.
 * TODO(aappleby): Deprecated. Move callers to writeMessage().
 * @param {number} field The field number of the sub-message.
 */
jspb.BinaryWriter.prototype.beginSubMessage = function(field) {
  this.bookmarks_.push(this.beginDelimited_(field));
};


/**
 * Finishes a sub-message and packs it into the parent messages' buffer.
 * TODO(aappleby): Deprecated. Move callers to writeMessage().
 */
jspb.BinaryWriter.prototype.endSubMessage = function() {
  goog.asserts.assert(this.bookmarks_.length >= 0);
  this.endDelimited_(this.bookmarks_.pop());
};


/**
 * Encodes a 32-bit unsigned integer into its wire-format varint representation
 * and stores it in the buffer.
 * @param {number} value The integer to convert.
 */
jspb.BinaryWriter.prototype.rawWriteUnsignedVarint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));

  while (value > 127) {
    this.temp_.push((value & 0x7f) | 0x80);
    value = value >>> 7;
  }

  this.temp_.push(value);
};


/**
 * Encodes a 32-bit signed integer into its wire-format varint representation
 * and stores it in the buffer.
 * @param {number} value The integer to convert.
 */
jspb.BinaryWriter.prototype.rawWriteSignedVarint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  if (value >= 0) {
    this.rawWriteUnsignedVarint32(value);
    return;
  }

  // Write nine bytes with a _signed_ right shift so we preserve the sign bit.
  for (var i = 0; i < 9; i++) {
    this.temp_.push((value & 0x7f) | 0x80);
    value = value >> 7;
  }

  // The above loop writes out 63 bits, so the last byte is always the sign bit
  // which is always set for negative numbers.
  this.temp_.push(1);
};


/**
 * Encodes an unsigned 64-bit integer in 32:32 split representation into its
 * wire-format varint representation and stores it in the buffer.
 * @param {number} lowBits The low 32 bits of the int.
 * @param {number} highBits The high 32 bits of the int.
 */
jspb.BinaryWriter.prototype.rawWriteSplitVarint =
    function(lowBits, highBits) {
  // Break the binary representation into chunks of 7 bits, set the 8th bit
  // in each chunk if it's not the final chunk, and append to the result.
  while (highBits > 0 || lowBits > 127) {
    this.temp_.push((lowBits & 0x7f) | 0x80);
    lowBits = ((lowBits >>> 7) | (highBits << 25)) >>> 0;
    highBits = highBits >>> 7;
  }
  this.temp_.push(lowBits);
};


/**
 * Encodes a JavaScript integer into its wire-format varint representation and
 * stores it in the buffer. Due to the way the varint encoding works this
 * behaves correctly for both signed and unsigned integers, though integers
 * that are not representable in 64 bits will still be truncated.
 * @param {number} value The integer to convert.
 */
jspb.BinaryWriter.prototype.rawWriteVarint = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  jspb.utils.splitInt64(value);
  this.rawWriteSplitVarint(jspb.utils.split64Low,
                           jspb.utils.split64High);
};


/**
 * Encodes a jspb.arith.{Int64,UInt64} instance into its wire-format
 * varint representation and stores it in the buffer. Due to the way the varint
 * encoding works this behaves correctly for both signed and unsigned integers,
 * though integers that are not representable in 64 bits will still be
 * truncated.
 * @param {jspb.arith.Int64|jspb.arith.UInt64} value
 */
jspb.BinaryWriter.prototype.rawWriteVarintFromNum = function(value) {
  this.rawWriteSplitVarint(value.lo, value.hi);
};


/**
 * Encodes a JavaScript integer into its wire-format, zigzag-encoded varint
 * representation and stores it in the buffer.
 * @param {number} value The integer to convert.
 */
jspb.BinaryWriter.prototype.rawWriteZigzagVarint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  this.rawWriteUnsignedVarint32(((value << 1) ^ (value >> 31)) >>> 0);
};


/**
 * Encodes a JavaScript integer into its wire-format, zigzag-encoded varint
 * representation and stores it in the buffer. Integers not representable in 64
 * bits will be truncated.
 * @param {number} value The integer to convert.
 */
jspb.BinaryWriter.prototype.rawWriteZigzagVarint = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  jspb.utils.splitZigzag64(value);
  this.rawWriteSplitVarint(jspb.utils.split64Low,
                           jspb.utils.split64High);
};


/**
 * Writes a raw 8-bit unsigned integer to the buffer. Numbers outside the range
 * [0,2^8) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteUint8 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) && (value < 256));
  this.temp_.push((value >>> 0) & 0xFF);
};


/**
 * Writes a raw 16-bit unsigned integer to the buffer. Numbers outside the
 * range [0,2^16) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteUint16 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) && (value < 65536));
  this.temp_.push((value >>> 0) & 0xFF);
  this.temp_.push((value >>> 8) & 0xFF);
};


/**
 * Writes a raw 32-bit unsigned integer to the buffer. Numbers outside the
 * range [0,2^32) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteUint32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_32));
  this.temp_.push((value >>> 0) & 0xFF);
  this.temp_.push((value >>> 8) & 0xFF);
  this.temp_.push((value >>> 16) & 0xFF);
  this.temp_.push((value >>> 24) & 0xFF);
};


/**
 * Writes a raw 64-bit unsigned integer to the buffer. Numbers outside the
 * range [0,2^64) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteUint64 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_64));
  jspb.utils.splitUint64(value);
  this.rawWriteUint32(jspb.utils.split64Low);
  this.rawWriteUint32(jspb.utils.split64High);
};


/**
 * Writes a raw 8-bit integer to the buffer. Numbers outside the range
 * [-2^7,2^7) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteInt8 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -128) && (value < 128));
  this.temp_.push((value >>> 0) & 0xFF);
};


/**
 * Writes a raw 16-bit integer to the buffer. Numbers outside the range
 * [-2^15,2^15) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteInt16 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -32768) && (value < 32768));
  this.temp_.push((value >>> 0) & 0xFF);
  this.temp_.push((value >>> 8) & 0xFF);
};


/**
 * Writes a raw 32-bit integer to the buffer. Numbers outside the range
 * [-2^31,2^31) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteInt32 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.temp_.push((value >>> 0) & 0xFF);
  this.temp_.push((value >>> 8) & 0xFF);
  this.temp_.push((value >>> 16) & 0xFF);
  this.temp_.push((value >>> 24) & 0xFF);
};


/**
 * Writes a raw 64-bit integer to the buffer. Numbers outside the range
 * [-2^63,2^63) will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteInt64 = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (value < jspb.BinaryConstants.TWO_TO_63));
  jspb.utils.splitInt64(value);
  this.rawWriteUint32(jspb.utils.split64Low);
  this.rawWriteUint32(jspb.utils.split64High);
};


/**
 * Writes a raw single-precision floating point value to the buffer. Numbers
 * requiring more than 32 bits of precision will be truncated.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteFloat = function(value) {
  jspb.utils.splitFloat32(value);
  this.rawWriteUint32(jspb.utils.split64Low);
};


/**
 * Writes a raw double-precision floating point value to the buffer. As this is
 * the native format used by JavaScript, no precision will be lost.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteDouble = function(value) {
  jspb.utils.splitFloat64(value);
  this.rawWriteUint32(jspb.utils.split64Low);
  this.rawWriteUint32(jspb.utils.split64High);
};


/**
 * Writes a raw boolean value to the buffer as a varint.
 * @param {boolean} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteBool = function(value) {
  goog.asserts.assert(goog.isBoolean(value));
  this.temp_.push(~~value);
};


/**
 * Writes an raw enum value to the buffer as a varint.
 * @param {number} value The value to write.
 */
jspb.BinaryWriter.prototype.rawWriteEnum = function(value) {
  goog.asserts.assert(value == Math.floor(value));
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.rawWriteSignedVarint32(value);
};


/**
 * Writes a raw string value to the buffer.
 * @param {string} string The string to write.
 */
jspb.BinaryWriter.prototype.rawWriteUtf8String = function(string) {
  for (var i = 0; i < string.length; i++) {
    this.temp_.push(string.charCodeAt(i));
  }
};


/**
 * Writes an arbitrary raw byte array to the buffer.
 * @param {!Uint8Array} bytes The array of bytes to write.
 */
jspb.BinaryWriter.prototype.rawWriteBytes = function(bytes) {
  this.appendUint8Array_(bytes);
};


/**
 * Writes an arbitrary raw byte array to the buffer.
 * @param {!Uint8Array} bytes The array of bytes to write.
 * @param {number} start The start of the range to write.
 * @param {number} end The end of the range to write.
 */
jspb.BinaryWriter.prototype.rawWriteByteRange = function(bytes, start, end) {
  this.appendUint8Array_(bytes.subarray(start, end));
};


/**
 * Writes a 64-bit hash string (8 characters @ 8 bits of data each) to the
 * buffer as a varint.
 * @param {string} hash The hash to write.
 */
jspb.BinaryWriter.prototype.rawWriteVarintHash64 = function(hash) {
  jspb.utils.splitHash64(hash);
  this.rawWriteSplitVarint(jspb.utils.split64Low,
                           jspb.utils.split64High);
};


/**
 * Writes a 64-bit hash string (8 characters @ 8 bits of data each) to the
 * buffer as a fixed64.
 * @param {string} hash The hash to write.
 */
jspb.BinaryWriter.prototype.rawWriteFixedHash64 = function(hash) {
  jspb.utils.splitHash64(hash);
  this.rawWriteUint32(jspb.utils.split64Low);
  this.rawWriteUint32(jspb.utils.split64High);
};


/**
 * Encodes a (field number, wire type) tuple into a wire-format field header
 * and stores it in the buffer as a varint.
 * @param {number} field The field number.
 * @param {number} wireType The wire-type of the field, as specified in the
 *     protocol buffer documentation.
 * @private
 */
jspb.BinaryWriter.prototype.rawWriteFieldHeader_ =
    function(field, wireType) {
  goog.asserts.assert(field >= 1 && field == Math.floor(field));
  var x = field * 8 + wireType;
  this.rawWriteUnsignedVarint32(x);
};


/**
 * Writes a field of any valid scalar type to the binary stream.
 * @param {jspb.BinaryConstants.FieldType} fieldType
 * @param {number} field
 * @param {jspb.AnyFieldType} value
 */
jspb.BinaryWriter.prototype.writeAny = function(fieldType, field, value) {
  var fieldTypes = jspb.BinaryConstants.FieldType;
  switch (fieldType) {
    case fieldTypes.DOUBLE:
      this.writeDouble(field, /** @type {number} */(value));
      return;
    case fieldTypes.FLOAT:
      this.writeFloat(field, /** @type {number} */(value));
      return;
    case fieldTypes.INT64:
      this.writeInt64(field, /** @type {number} */(value));
      return;
    case fieldTypes.UINT64:
      this.writeUint64(field, /** @type {number} */(value));
      return;
    case fieldTypes.INT32:
      this.writeInt32(field, /** @type {number} */(value));
      return;
    case fieldTypes.FIXED64:
      this.writeFixed64(field, /** @type {number} */(value));
      return;
    case fieldTypes.FIXED32:
      this.writeFixed32(field, /** @type {number} */(value));
      return;
    case fieldTypes.BOOL:
      this.writeBool(field, /** @type {boolean} */(value));
      return;
    case fieldTypes.STRING:
      this.writeString(field, /** @type {string} */(value));
      return;
    case fieldTypes.GROUP:
      goog.asserts.fail('Group field type not supported in writeAny()');
      return;
    case fieldTypes.MESSAGE:
      goog.asserts.fail('Message field type not supported in writeAny()');
      return;
    case fieldTypes.BYTES:
      this.writeBytes(field, /** @type {?Uint8Array} */(value));
      return;
    case fieldTypes.UINT32:
      this.writeUint32(field, /** @type {number} */(value));
      return;
    case fieldTypes.ENUM:
      this.writeEnum(field, /** @type {number} */(value));
      return;
    case fieldTypes.SFIXED32:
      this.writeSfixed32(field, /** @type {number} */(value));
      return;
    case fieldTypes.SFIXED64:
      this.writeSfixed64(field, /** @type {number} */(value));
      return;
    case fieldTypes.SINT32:
      this.writeSint32(field, /** @type {number} */(value));
      return;
    case fieldTypes.SINT64:
      this.writeSint64(field, /** @type {number} */(value));
      return;
    case fieldTypes.FHASH64:
      this.writeFixedHash64(field, /** @type {string} */(value));
      return;
    case fieldTypes.VHASH64:
      this.writeVarintHash64(field, /** @type {string} */(value));
      return;
    default:
      goog.asserts.fail('Invalid field type in writeAny()');
      return;
  }
};


/**
 * Writes a varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeUnsignedVarint32_ = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteSignedVarint32(value);
};


/**
 * Writes a varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeSignedVarint32_ = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteSignedVarint32(value);
};


/**
 * Writes a varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeVarint_ = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteVarint(value);
};


/**
 * Writes a zigzag varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeZigzagVarint32_ = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteZigzagVarint32(value);
};


/**
 * Writes a zigzag varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeZigzagVarint_ = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteZigzagVarint(value);
};


/**
 * Writes an int32 field to the buffer. Numbers outside the range [-2^31,2^31)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeInt32 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.writeSignedVarint32_(field, value);
};


/**
 * Writes an int32 field represented as a string to the buffer. Numbers outside
 * the range [-2^31,2^31) will be truncated.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeInt32String = function(field, value) {
  if (value == null) return;
  var intValue = /** {number} */ parseInt(value, 10);
  goog.asserts.assert((intValue >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (intValue < jspb.BinaryConstants.TWO_TO_31));
  this.writeSignedVarint32_(field, intValue);
};


/**
 * Writes an int64 field to the buffer. Numbers outside the range [-2^63,2^63)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeInt64 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (value < jspb.BinaryConstants.TWO_TO_63));
  this.writeVarint_(field, value);
};


/**
 * Writes a int64 field (with value as a string) to the buffer.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeInt64String = function(field, value) {
  if (value == null) return;
  var num = jspb.arith.Int64.fromString(value);
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteVarintFromNum(num);
};


/**
 * Writes a uint32 field to the buffer. Numbers outside the range [0,2^32)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeUint32 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_32));
  this.writeUnsignedVarint32_(field, value);
};


/**
 * Writes a uint32 field represented as a string to the buffer. Numbers outside
 * the range [0,2^32) will be truncated.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeUint32String = function(field, value) {
  if (value == null) return;
  var intValue = /** {number} */ parseInt(value, 10);
  goog.asserts.assert((intValue >= 0) &&
                      (intValue < jspb.BinaryConstants.TWO_TO_32));
  this.writeUnsignedVarint32_(field, intValue);
};


/**
 * Writes a uint64 field to the buffer. Numbers outside the range [0,2^64)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeUint64 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_64));
  this.writeVarint_(field, value);
};


/**
 * Writes a uint64 field (with value as a string) to the buffer.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeUint64String = function(field, value) {
  if (value == null) return;
  var num = jspb.arith.UInt64.fromString(value);
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteVarintFromNum(num);
};


/**
 * Writes a sint32 field to the buffer. Numbers outside the range [-2^31,2^31)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeSint32 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.writeZigzagVarint32_(field, value);
};


/**
 * Writes a sint64 field to the buffer. Numbers outside the range [-2^63,2^63)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeSint64 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (value < jspb.BinaryConstants.TWO_TO_63));
  this.writeZigzagVarint_(field, value);
};


/**
 * Writes a fixed32 field to the buffer. Numbers outside the range [0,2^32)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeFixed32 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_32));
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED32);
  this.rawWriteUint32(value);
};


/**
 * Writes a fixed64 field to the buffer. Numbers outside the range [0,2^64)
 * will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeFixed64 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= 0) &&
                      (value < jspb.BinaryConstants.TWO_TO_64));
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.rawWriteUint64(value);
};


/**
 * Writes a sfixed32 field to the buffer. Numbers outside the range
 * [-2^31,2^31) will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeSfixed32 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED32);
  this.rawWriteInt32(value);
};


/**
 * Writes a sfixed64 field to the buffer. Numbers outside the range
 * [-2^63,2^63) will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeSfixed64 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (value < jspb.BinaryConstants.TWO_TO_63));
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.rawWriteInt64(value);
};


/**
 * Writes a single-precision floating point field to the buffer. Numbers
 * requiring more than 32 bits of precision will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeFloat = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED32);
  this.rawWriteFloat(value);
};


/**
 * Writes a double-precision floating point field to the buffer. As this is the
 * native format used by JavaScript, no precision will be lost.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeDouble = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.rawWriteDouble(value);
};


/**
 * Writes a boolean field to the buffer.
 * @param {number} field The field number.
 * @param {boolean?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeBool = function(field, value) {
  if (value == null) return;
  goog.asserts.assert(goog.isBoolean(value));
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.temp_.push(~~value);
};


/**
 * Writes an enum field to the buffer.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeEnum = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((value >= -jspb.BinaryConstants.TWO_TO_31) &&
                      (value < jspb.BinaryConstants.TWO_TO_31));
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteSignedVarint32(value);
};


/**
 * Writes a string field to the buffer.
 * @param {number} field The field number.
 * @param {string?} value The string to write.
 */
jspb.BinaryWriter.prototype.writeString = function(field, value) {
  if (value == null) return;
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);

  // Conversion loop swiped from goog.crypt.stringToUtf8ByteArray. Note that
  // 'bytes' will be at least as long as 'value', but could be longer if we
  // need to unpack unicode characters.
  var bytes = [];
  for (var i = 0; i < value.length; i++) {
    var c = value.charCodeAt(i);
    if (c < 128) {
      bytes.push(c);
    } else if (c < 2048) {
      bytes.push((c >> 6) | 192);
      bytes.push((c & 63) | 128);
    } else {
      bytes.push((c >> 12) | 224);
      bytes.push(((c >> 6) & 63) | 128);
      bytes.push((c & 63) | 128);
    }
  }

  this.rawWriteUnsignedVarint32(bytes.length);
  this.appendArray_(bytes);
};


/**
 * Writes an arbitrary byte field to the buffer. Note - to match the behavior
 * of the C++ implementation, empty byte arrays _are_ serialized.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {jspb.ByteSource} value The array of bytes to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 * @param {boolean=} opt_stringIsRawBytes If `value` is a string, interpret it
 * as a series of raw bytes (codepoints 0--255 inclusive) rather than base64
 * data.
 */
jspb.BinaryWriter.prototype.writeBytes =
    function(field, value, opt_buffer, opt_start, opt_end,
             opt_stringIsRawBytes) {
  if (value != null) {
    this.rawWriteFieldHeader_(field,
        jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length);
    this.rawWriteBytes(
        jspb.utils.byteSourceToUint8Array(value, opt_stringIsRawBytes));
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an arbitrary byte field to the buffer, with `opt_stringIsRawBytes`
 * flag implicitly true.
 * @param {number} field
 * @param {jspb.ByteSource} value The array of bytes to write.
 */
jspb.BinaryWriter.prototype.writeBytesRawString = function(field, value) {
  this.writeBytes(field, value, null, null, null, true);
};


/**
 * Writes a message to the buffer.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @template MessageType
 * @param {number} field The field number.
 * @param {?MessageType} value The message to write.
 * @param {!jspb.WriterFunction} writerCallback Will be invoked with the value
 *     to write and the writer to write it with.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writeMessage =
    function(field, value, writerCallback, opt_buffer, opt_start, opt_end) {
  if (value !== null) {
    var bookmark = this.beginDelimited_(field);

    writerCallback(value, this);

    this.endDelimited_(bookmark);
  } else if (opt_buffer && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes a group message to the buffer.
 *
 * @template MessageType
 * @param {number} field The field number.
 * @param {?MessageType} value The message to write, wrapped with START_GROUP /
 *     END_GROUP tags. Will be a no-op if 'value' is null.
 * @param {!jspb.WriterFunction} writerCallback Will be invoked with the value
 *     to write and the writer to write it with.
 */
jspb.BinaryWriter.prototype.writeGroup =
    function(field, value, writerCallback) {
  if (value) {
    this.rawWriteFieldHeader_(
        field, jspb.BinaryConstants.WireType.START_GROUP);
    writerCallback(value, this);
    this.rawWriteFieldHeader_(
        field, jspb.BinaryConstants.WireType.END_GROUP);
  }
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 * @param {number} field The field number.
 * @param {string?} value The hash string.
 */
jspb.BinaryWriter.prototype.writeFixedHash64 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert(value.length == 8);
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.rawWriteFixedHash64(value);
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 * @param {number} field The field number.
 * @param {string?} value The hash string.
 */
jspb.BinaryWriter.prototype.writeVarintHash64 = function(field, value) {
  if (value == null) return;
  goog.asserts.assert(value.length == 8);
  this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.rawWriteVarintHash64(value);
};


/**
 * Writes an array of numbers to the buffer as a repeated varint field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeRepeatedUnsignedVarint32_ =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeUnsignedVarint32_(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated varint field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeRepeatedSignedVarint32_ =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeSignedVarint32_(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated varint field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeRepeatedVarint_ = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeVarint_(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated zigzag field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeRepeatedZigzag32_ = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeZigzagVarint32_(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated zigzag field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeRepeatedZigzag_ = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeZigzagVarint_(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt32 =
    jspb.BinaryWriter.prototype.writeRepeatedSignedVarint32_;


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt32String =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeInt32String(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt64 =
    jspb.BinaryWriter.prototype.writeRepeatedVarint_;


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt64String =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeInt64String(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated unsigned 32-bit int
 *     field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint32 =
    jspb.BinaryWriter.prototype.writeRepeatedUnsignedVarint32_;


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * unsigned 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint32String =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeUint32String(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated unsigned 64-bit int
 *     field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint64 =
    jspb.BinaryWriter.prototype.writeRepeatedVarint_;


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * unsigned 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint64String =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeUint64String(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated signed 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSint32 =
    jspb.BinaryWriter.prototype.writeRepeatedZigzag32_;


/**
 * Writes an array numbers to the buffer as a repeated signed 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSint64 =
    jspb.BinaryWriter.prototype.writeRepeatedZigzag_;


/**
 * Writes an array of numbers to the buffer as a repeated fixed32 field. This
 * works for both signed and unsigned fixed32s.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedFixed32 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeFixed32(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated fixed64 field. This
 * works for both signed and unsigned fixed64s.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedFixed64 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeFixed64(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated sfixed32 field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSfixed32 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeSfixed32(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated sfixed64 field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSfixed64 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeSfixed64(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated float field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedFloat = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeFloat(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated double field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedDouble = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeDouble(field, value[i]);
  }
};


/**
 * Writes an array of booleans to the buffer as a repeated bool field.
 * @param {number} field The field number.
 * @param {?Array.<boolean>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedBool = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeBool(field, value[i]);
  }
};


/**
 * Writes an array of enums to the buffer as a repeated enum field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedEnum = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeEnum(field, value[i]);
  }
};


/**
 * Writes an array of strings to the buffer as a repeated string field.
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of strings to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedString = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeString(field, value[i]);
  }
};


/**
 * Writes an array of arbitrary byte fields to the buffer.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<!Uint8Array|string>} value
 *     The arrays of arrays of bytes to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 * @param {boolean=} opt_stringIsRawBytes Any values that are strings are
 * interpreted as raw bytes rather than base64 data.
 */
jspb.BinaryWriter.prototype.writeRepeatedBytes =
    function(field, value, opt_buffer, opt_start, opt_end,
             opt_stringIsRawBytes) {
  if (value != null) {
    for (var i = 0; i < value.length; i++) {
      this.writeBytes(field, value[i], null, null, null, opt_stringIsRawBytes);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of arbitrary byte fields to the buffer, with
 * `opt_stringIsRawBytes` implicitly true.
 * @param {number} field
 * @param {?Array.<string>} value
 */
jspb.BinaryWriter.prototype.writeRepeatedBytesRawString =
    function(field, value) {
  this.writeRepeatedBytes(field, value, null, null, null, true);
};


/**
 * Writes an array of messages to the buffer.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @template MessageType
 * @param {number} field The field number.
 * @param {?Array.<!MessageType>} value The array of messages to
 *    write.
 * @param {!jspb.WriterFunction} writerCallback Will be invoked with the value
 *     to write and the writer to write it with.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writeRepeatedMessage =
    function(field, value, writerCallback, opt_buffer, opt_start, opt_end) {
  if (value) {
    for (var i = 0; i < value.length; i++) {
      var bookmark = this.beginDelimited_(field);

      writerCallback(value[i], this);

      this.endDelimited_(bookmark);
    }
  } else if (opt_buffer && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of group messages to the buffer.
 *
 * @template MessageType
 * @param {number} field The field number.
 * @param {?Array.<!MessageType>} value The array of messages to
 *    write.
 * @param {!jspb.WriterFunction} writerCallback Will be invoked with the value
 *     to write and the writer to write it with.
 */
jspb.BinaryWriter.prototype.writeRepeatedGroup =
    function(field, value, writerCallback) {
  if (value) {
    for (var i = 0; i < value.length; i++) {
      this.rawWriteFieldHeader_(
          field, jspb.BinaryConstants.WireType.START_GROUP);
      writerCallback(value[i], this);
      this.rawWriteFieldHeader_(
          field, jspb.BinaryConstants.WireType.END_GROUP);
    }
  }
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of hashes to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedFixedHash64 =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeFixedHash64(field, value[i]);
  }
};


/**
 * Writes a repeated 64-bit hash string field (8 characters @ 8 bits of data
 * each) to the buffer.
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of hashes to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedVarintHash64 =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeVarintHash64(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed varint field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 * @private
 */
jspb.BinaryWriter.prototype.writePackedUnsignedVarint32_ =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    var bookmark = this.beginDelimited_(field);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteUnsignedVarint32(value[i]);
    }
    this.endDelimited_(bookmark);
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed varint field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 * @private
 */
jspb.BinaryWriter.prototype.writePackedSignedVarint32_ =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    var bookmark = this.beginDelimited_(field);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteSignedVarint32(value[i]);
    }
    this.endDelimited_(bookmark);
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed varint field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 * @private
 */
jspb.BinaryWriter.prototype.writePackedVarint_ =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    var bookmark = this.beginDelimited_(field);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteVarint(value[i]);
    }
    this.endDelimited_(bookmark);
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed zigzag field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 * @private
 */
jspb.BinaryWriter.prototype.writePackedZigzag_ =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    var bookmark = this.beginDelimited_(field);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteZigzagVarint(value[i]);
    }
    this.endDelimited_(bookmark);
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed 32-bit int field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedInt32 =
    jspb.BinaryWriter.prototype.writePackedSignedVarint32_;


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * 32-bit int field.
 * @param {number} field
 * @param {?Array.<string>} value
 */
jspb.BinaryWriter.prototype.writePackedInt32String = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.rawWriteSignedVarint32(parseInt(value[i], 10));
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers to the buffer as a packed 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedInt64 =
    jspb.BinaryWriter.prototype.writePackedVarint_;


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * 64-bit int field.
 * @param {number} field
 * @param {?Array.<string>} value
 */
jspb.BinaryWriter.prototype.writePackedInt64String =
    function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    var num = jspb.arith.Int64.fromString(value[i]);
    this.rawWriteVarintFromNum(num);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array numbers to the buffer as a packed unsigned 32-bit int field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedUint32 =
    jspb.BinaryWriter.prototype.writePackedUnsignedVarint32_;


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * unsigned 32-bit int field.
 * @param {number} field
 * @param {?Array.<string>} value
 */
jspb.BinaryWriter.prototype.writePackedUint32String =
    function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.rawWriteUnsignedVarint32(parseInt(value[i], 10));
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array numbers to the buffer as a packed unsigned 64-bit int field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedUint64 =
    jspb.BinaryWriter.prototype.writePackedVarint_;


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * unsigned 64-bit int field.
 * @param {number} field
 * @param {?Array.<string>} value
 */
jspb.BinaryWriter.prototype.writePackedUint64String =
    function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    var num = jspb.arith.UInt64.fromString(value[i]);
    this.rawWriteVarintFromNum(num);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array numbers to the buffer as a packed signed 32-bit int field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedSint32 =
    jspb.BinaryWriter.prototype.writePackedZigzag_;


/**
 * Writes an array numbers to the buffer as a packed signed 64-bit int field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedSint64 =
    jspb.BinaryWriter.prototype.writePackedZigzag_;


/**
 * Writes an array of numbers to the buffer as a packed fixed32 field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedFixed32 =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length * 4);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteUint32(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed fixed64 field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedFixed64 =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length * 8);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteUint64(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed sfixed32 field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedSfixed32 =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length * 4);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteInt32(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed sfixed64 field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedSfixed64 =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length * 8);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteInt64(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed float field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedFloat =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length * 4);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteFloat(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed double field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedDouble =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length * 8);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteDouble(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of booleans to the buffer as a packed bool field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<boolean>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedBool =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteBool(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes an array of enums to the buffer as a packed enum field.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<number>} value The array of ints to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedEnum =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    var bookmark = this.beginDelimited_(field);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteEnum(value[i]);
    }
    this.endDelimited_(bookmark);
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of hashes to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedFixedHash64 =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    this.rawWriteFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
    this.rawWriteUnsignedVarint32(value.length * 8);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteFixedHash64(value[i]);
    }
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 *
 * If 'value' is null, this method will try and copy the pre-serialized value
 * in 'opt_buffer' if present.
 *
 * @param {number} field The field number.
 * @param {?Array.<string>} value The array of hashes to write.
 * @param {?Uint8Array=} opt_buffer A buffer containing pre-packed values.
 * @param {?number=} opt_start The starting point in the above buffer.
 * @param {?number=} opt_end The ending point in the above buffer.
 */
jspb.BinaryWriter.prototype.writePackedVarintHash64 =
    function(field, value, opt_buffer, opt_start, opt_end) {
  if (value != null && value.length) {
    var bookmark = this.beginDelimited_(field);
    for (var i = 0; i < value.length; i++) {
      this.rawWriteVarintHash64(value[i]);
    }
    this.endDelimited_(bookmark);
  } else if ((opt_buffer != null) && (opt_start != null) && (opt_end != null)) {
    this.rawWriteByteRange(opt_buffer, opt_start, opt_end);
  }
};
