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
goog.require('jspb.BinaryEncoder');
goog.require('jspb.arith.Int64');
goog.require('jspb.arith.UInt64');
goog.require('jspb.utils');



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
   * Total number of bytes in the blocks_ array. Does _not_ include bytes in
   * the encoder below.
   * @private {number}
   */
  this.totalLength_ = 0;

  /**
   * Binary encoder holding pieces of a message that we're still serializing.
   * When we get to a stopping point (either the start of a new submessage, or
   * when we need to append a raw Uint8Array), the encoder's buffer will be
   * added to the block array above and the encoder will be reset.
   * @private {!jspb.BinaryEncoder}
   */
  this.encoder_ = new jspb.BinaryEncoder();

  /**
   * A stack of bookmarks containing the parent blocks for each message started
   * via beginSubMessage(), needed as bookkeeping for endSubMessage().
   * TODO(aappleby): Deprecated, users should be calling writeMessage().
   * @private {!Array<!Array<number>>}
   */
  this.bookmarks_ = [];
};


/**
 * Append a typed array of bytes onto the buffer.
 *
 * @param {!Uint8Array} arr The byte array to append.
 * @private
 */
jspb.BinaryWriter.prototype.appendUint8Array_ = function(arr) {
  var temp = this.encoder_.end();
  this.blocks_.push(temp);
  this.blocks_.push(arr);
  this.totalLength_ += temp.length + arr.length;
};


/**
 * Begins a new message by writing the field header and returning a bookmark
 * which we will use to patch in the message length to in endDelimited_ below.
 * @param {number} field
 * @return {!Array<number>}
 * @private
 */
jspb.BinaryWriter.prototype.beginDelimited_ = function(field) {
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  var bookmark = this.encoder_.end();
  this.blocks_.push(bookmark);
  this.totalLength_ += bookmark.length;
  bookmark.push(this.totalLength_);
  return bookmark;
};


/**
 * Ends a message by encoding the _change_ in length of the buffer to the
 * parent block and adds the number of bytes needed to encode that length to
 * the total byte length.
 * @param {!Array<number>} bookmark
 * @private
 */
jspb.BinaryWriter.prototype.endDelimited_ = function(bookmark) {
  var oldLength = bookmark.pop();
  var messageLength = this.totalLength_ + this.encoder_.length() - oldLength;
  goog.asserts.assert(messageLength >= 0);

  while (messageLength > 127) {
    bookmark.push((messageLength & 0x7f) | 0x80);
    messageLength = messageLength >>> 7;
    this.totalLength_++;
  }

  bookmark.push(messageLength);
  this.totalLength_++;
};


/**
 * Writes a pre-serialized message to the buffer.
 * @param {!Uint8Array} bytes The array of bytes to write.
 * @param {number} start The start of the range to write.
 * @param {number} end The end of the range to write.
 */
jspb.BinaryWriter.prototype.writeSerializedMessage = function(
    bytes, start, end) {
  this.appendUint8Array_(bytes.subarray(start, end));
};


/**
 * Writes a pre-serialized message to the buffer if the message and endpoints
 * are non-null.
 * @param {?Uint8Array} bytes The array of bytes to write.
 * @param {?number} start The start of the range to write.
 * @param {?number} end The end of the range to write.
 */
jspb.BinaryWriter.prototype.maybeWriteSerializedMessage = function(
    bytes, start, end) {
  if (bytes != null && start != null && end != null) {
    this.writeSerializedMessage(bytes, start, end);
  }
};


/**
 * Resets the writer, throwing away any accumulated buffers.
 */
jspb.BinaryWriter.prototype.reset = function() {
  this.blocks_ = [];
  this.encoder_.end();
  this.totalLength_ = 0;
  this.bookmarks_ = [];
};


/**
 * Converts the encoded data into a Uint8Array.
 * @return {!Uint8Array}
 */
jspb.BinaryWriter.prototype.getResultBuffer = function() {
  goog.asserts.assert(this.bookmarks_.length == 0);

  var flat = new Uint8Array(this.totalLength_ + this.encoder_.length());

  var blocks = this.blocks_;
  var blockCount = blocks.length;
  var offset = 0;

  for (var i = 0; i < blockCount; i++) {
    var block = blocks[i];
    flat.set(block, offset);
    offset += block.length;
  }

  var tail = this.encoder_.end();
  flat.set(tail, offset);
  offset += tail.length;

  // Post condition: `flattened` must have had every byte written.
  goog.asserts.assert(offset == flat.length);

  // Replace our block list with the flattened block, which lets GC reclaim
  // the temp blocks sooner.
  this.blocks_ = [flat];

  return flat;
};


/**
 * Converts the encoded data into a base64-encoded string.
 * @param {boolean=} opt_webSafe True indicates we should use a websafe
 *     alphabet, which does not require escaping for use in URLs.
 * @return {string}
 */
jspb.BinaryWriter.prototype.getResultBase64String = function(opt_webSafe) {
  return goog.crypt.base64.encodeByteArray(this.getResultBuffer(), opt_webSafe);
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
 * Encodes a (field number, wire type) tuple into a wire-format field header
 * and stores it in the buffer as a varint.
 * @param {number} field The field number.
 * @param {number} wireType The wire-type of the field, as specified in the
 *     protocol buffer documentation.
 * @private
 */
jspb.BinaryWriter.prototype.writeFieldHeader_ =
    function(field, wireType) {
  goog.asserts.assert(field >= 1 && field == Math.floor(field));
  var x = field * 8 + wireType;
  this.encoder_.writeUnsignedVarint32(x);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeUnsignedVarint32(value);
};


/**
 * Writes a varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeSignedVarint32_ = function(field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeSignedVarint32(value);
};


/**
 * Writes a varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeUnsignedVarint64_ = function(field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeUnsignedVarint64(value);
};


/**
 * Writes a varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeSignedVarint64_ = function(field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeSignedVarint64(value);
};


/**
 * Writes a zigzag varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeZigzagVarint32_ = function(field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeZigzagVarint32(value);
};


/**
 * Writes a zigzag varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeZigzagVarint64_ = function(field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeZigzagVarint64(value);
};


/**
 * Writes a zigzag varint field to the buffer without range checking.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 * @private
 */
jspb.BinaryWriter.prototype.writeZigzagVarint64String_ = function(
    field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeZigzagVarint64String(value);
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
  this.writeSignedVarint64_(field, value);
};


/**
 * Writes a int64 field (with value as a string) to the buffer.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeInt64String = function(field, value) {
  if (value == null) return;
  var num = jspb.arith.Int64.fromString(value);
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeSplitVarint64(num.lo, num.hi);
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
  this.writeUnsignedVarint64_(field, value);
};


/**
 * Writes a uint64 field (with value as a string) to the buffer.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeUint64String = function(field, value) {
  if (value == null) return;
  var num = jspb.arith.UInt64.fromString(value);
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeSplitVarint64(num.lo, num.hi);
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
  this.writeZigzagVarint64_(field, value);
};


/**
 * Writes a sint64 field to the buffer. Numbers outside the range [-2^63,2^63)
 * will be truncated.
 * @param {number} field The field number.
 * @param {string?} value The decimal string to write.
 */
jspb.BinaryWriter.prototype.writeSint64String = function(field, value) {
  if (value == null) return;
  goog.asserts.assert((+value >= -jspb.BinaryConstants.TWO_TO_63) &&
                      (+value < jspb.BinaryConstants.TWO_TO_63));
  this.writeZigzagVarint64String_(field, value);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED32);
  this.encoder_.writeUint32(value);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.encoder_.writeUint64(value);
};


/**
 * Writes a fixed64 field (with value as a string) to the buffer.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeFixed64String = function(field, value) {
  if (value == null) return;
  var num = jspb.arith.UInt64.fromString(value);
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.encoder_.writeSplitFixed64(num.lo, num.hi);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED32);
  this.encoder_.writeInt32(value);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.encoder_.writeInt64(value);
};


/**
 * Writes a sfixed64 string field to the buffer. Numbers outside the range
 * [-2^63,2^63) will be truncated.
 * @param {number} field The field number.
 * @param {string?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeSfixed64String = function(field, value) {
  if (value == null) return;
  var num = jspb.arith.Int64.fromString(value);
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.encoder_.writeSplitFixed64(num.lo, num.hi);
};


/**
 * Writes a single-precision floating point field to the buffer. Numbers
 * requiring more than 32 bits of precision will be truncated.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeFloat = function(field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED32);
  this.encoder_.writeFloat(value);
};


/**
 * Writes a double-precision floating point field to the buffer. As this is the
 * native format used by JavaScript, no precision will be lost.
 * @param {number} field The field number.
 * @param {number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeDouble = function(field, value) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.encoder_.writeDouble(value);
};


/**
 * Writes a boolean field to the buffer. We allow numbers as input
 * because the JSPB code generator uses 0/1 instead of true/false to save space
 * in the string representation of the proto.
 * @param {number} field The field number.
 * @param {boolean?|number?} value The value to write.
 */
jspb.BinaryWriter.prototype.writeBool = function(field, value) {
  if (value == null) return;
  goog.asserts.assert(goog.isBoolean(value) || goog.isNumber(value));
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeBool(value);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeSignedVarint32(value);
};


/**
 * Writes a string field to the buffer.
 * @param {number} field The field number.
 * @param {string?} value The string to write.
 */
jspb.BinaryWriter.prototype.writeString = function(field, value) {
  if (value == null) return;
  var bookmark = this.beginDelimited_(field);
  this.encoder_.writeString(value);
  this.endDelimited_(bookmark);
};


/**
 * Writes an arbitrary byte field to the buffer. Note - to match the behavior
 * of the C++ implementation, empty byte arrays _are_ serialized.
 * @param {number} field The field number.
 * @param {?jspb.ByteSource} value The array of bytes to write.
 */
jspb.BinaryWriter.prototype.writeBytes = function(field, value) {
  if (value == null) return;
  var bytes = jspb.utils.byteSourceToUint8Array(value);
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(bytes.length);
  this.appendUint8Array_(bytes);
};


/**
 * Writes a message to the buffer.
 * @param {number} field The field number.
 * @param {?MessageType} value The message to write.
 * @param {function(MessageTypeNonNull, !jspb.BinaryWriter)} writerCallback
 *     Will be invoked with the value to write and the writer to write it with.
 * @template MessageType
 * Use go/closure-ttl to declare a non-nullable version of MessageType.  Replace
 * the null in blah|null with none.  This is necessary because the compiler will
 * infer MessageType to be nullable if the value parameter is nullable.
 * @template MessageTypeNonNull :=
 *     cond(isUnknown(MessageType), unknown(),
 *       mapunion(MessageType, (X) =>
 *         cond(eq(X, 'null'), none(), X)))
 * =:
 */
jspb.BinaryWriter.prototype.writeMessage = function(
    field, value, writerCallback) {
  if (value == null) return;
  var bookmark = this.beginDelimited_(field);
  writerCallback(value, this);
  this.endDelimited_(bookmark);
};


/**
 * Writes a group message to the buffer.
 *
 * @param {number} field The field number.
 * @param {?MessageType} value The message to write, wrapped with START_GROUP /
 *     END_GROUP tags. Will be a no-op if 'value' is null.
 * @param {function(MessageTypeNonNull, !jspb.BinaryWriter)} writerCallback
 *     Will be invoked with the value to write and the writer to write it with.
 * @template MessageType
 * Use go/closure-ttl to declare a non-nullable version of MessageType.  Replace
 * the null in blah|null with none.  This is necessary because the compiler will
 * infer MessageType to be nullable if the value parameter is nullable.
 * @template MessageTypeNonNull :=
 *     cond(isUnknown(MessageType), unknown(),
 *       mapunion(MessageType, (X) =>
 *         cond(eq(X, 'null'), none(), X)))
 * =:
 */
jspb.BinaryWriter.prototype.writeGroup = function(
    field, value, writerCallback) {
  if (value == null) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.START_GROUP);
  writerCallback(value, this);
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.END_GROUP);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.FIXED64);
  this.encoder_.writeFixedHash64(value);
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
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.VARINT);
  this.encoder_.writeVarintHash64(value);
};


/**
 * Writes an array of numbers to the buffer as a repeated 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt32 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeSignedVarint32_(field, value[i]);
  }
};


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt32String = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeInt32String(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt64 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeSignedVarint64_(field, value[i]);
  }
};


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedInt64String = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeInt64String(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated unsigned 32-bit int
 *     field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint32 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeUnsignedVarint32_(field, value[i]);
  }
};


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * unsigned 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint32String = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeUint32String(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated unsigned 64-bit int
 *     field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint64 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeUnsignedVarint64_(field, value[i]);
  }
};


/**
 * Writes an array of numbers formatted as strings to the buffer as a repeated
 * unsigned 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedUint64String = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeUint64String(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated signed 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSint32 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeZigzagVarint32_(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated signed 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSint64 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeZigzagVarint64_(field, value[i]);
  }
};


/**
 * Writes an array numbers to the buffer as a repeated signed 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSint64String = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeZigzagVarint64String_(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated fixed32 field. This
 * works for both signed and unsigned fixed32s.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
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
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedFixed64 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeFixed64(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated fixed64 field. This
 * works for both signed and unsigned fixed64s.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of decimal strings to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedFixed64String = function(
    field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeFixed64String(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated sfixed32 field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
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
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSfixed64 = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeSfixed64(field, value[i]);
  }
};


/**
 * Writes an array of decimal strings to the buffer as a repeated sfixed64
 * field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of decimal strings to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedSfixed64String = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeSfixed64String(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a repeated float field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
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
 * @param {?Array<number>} value The array of ints to write.
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
 * @param {?Array<boolean>} value The array of ints to write.
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
 * @param {?Array<number>} value The array of ints to write.
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
 * @param {?Array<string>} value The array of strings to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedString = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeString(field, value[i]);
  }
};


/**
 * Writes an array of arbitrary byte fields to the buffer.
 * @param {number} field The field number.
 * @param {?Array<!jspb.ByteSource>} value The arrays of arrays of bytes to
 *     write.
 */
jspb.BinaryWriter.prototype.writeRepeatedBytes = function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeBytes(field, value[i]);
  }
};


/**
 * Writes an array of messages to the buffer.
 * @template MessageType
 * @param {number} field The field number.
 * @param {?Array<MessageType>} value The array of messages to
 *    write.
 * @param {function(MessageType, !jspb.BinaryWriter)} writerCallback
 *     Will be invoked with the value to write and the writer to write it with.
 */
jspb.BinaryWriter.prototype.writeRepeatedMessage = function(
    field, value, writerCallback) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    var bookmark = this.beginDelimited_(field);
    writerCallback(value[i], this);
    this.endDelimited_(bookmark);
  }
};


/**
 * Writes an array of group messages to the buffer.
 * @template MessageType
 * @param {number} field The field number.
 * @param {?Array<MessageType>} value The array of messages to
 *    write.
 * @param {function(MessageType, !jspb.BinaryWriter)} writerCallback
 *     Will be invoked with the value to write and the writer to write it with.
 */
jspb.BinaryWriter.prototype.writeRepeatedGroup = function(
    field, value, writerCallback) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.START_GROUP);
    writerCallback(value[i], this);
    this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.END_GROUP);
  }
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of hashes to write.
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
 * @param {?Array<string>} value The array of hashes to write.
 */
jspb.BinaryWriter.prototype.writeRepeatedVarintHash64 =
    function(field, value) {
  if (value == null) return;
  for (var i = 0; i < value.length; i++) {
    this.writeVarintHash64(field, value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedInt32 = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeSignedVarint32(value[i]);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * 32-bit int field.
 * @param {number} field
 * @param {?Array<string>} value
 */
jspb.BinaryWriter.prototype.writePackedInt32String = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeSignedVarint32(parseInt(value[i], 10));
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers to the buffer as a packed 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedInt64 = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeSignedVarint64(value[i]);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * 64-bit int field.
 * @param {number} field
 * @param {?Array<string>} value
 */
jspb.BinaryWriter.prototype.writePackedInt64String = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    var num = jspb.arith.Int64.fromString(value[i]);
    this.encoder_.writeSplitVarint64(num.lo, num.hi);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array numbers to the buffer as a packed unsigned 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedUint32 = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeUnsignedVarint32(value[i]);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * unsigned 32-bit int field.
 * @param {number} field
 * @param {?Array<string>} value
 */
jspb.BinaryWriter.prototype.writePackedUint32String =
    function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeUnsignedVarint32(parseInt(value[i], 10));
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array numbers to the buffer as a packed unsigned 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedUint64 = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeUnsignedVarint64(value[i]);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * unsigned 64-bit int field.
 * @param {number} field
 * @param {?Array<string>} value
 */
jspb.BinaryWriter.prototype.writePackedUint64String =
    function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    var num = jspb.arith.UInt64.fromString(value[i]);
    this.encoder_.writeSplitVarint64(num.lo, num.hi);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array numbers to the buffer as a packed signed 32-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedSint32 = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeZigzagVarint32(value[i]);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers to the buffer as a packed signed 64-bit int field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedSint64 = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeZigzagVarint64(value[i]);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of decimal strings to the buffer as a packed signed 64-bit
 * int field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of decimal strings to write.
 */
jspb.BinaryWriter.prototype.writePackedSint64String = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    // TODO(haberman): make lossless
    this.encoder_.writeZigzagVarint64(parseInt(value[i], 10));
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes an array of numbers to the buffer as a packed fixed32 field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedFixed32 = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 4);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeUint32(value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed fixed64 field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedFixed64 = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 8);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeUint64(value[i]);
  }
};


/**
 * Writes an array of numbers represented as strings to the buffer as a packed
 * fixed64 field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of strings to write.
 */
jspb.BinaryWriter.prototype.writePackedFixed64String = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 8);
  for (var i = 0; i < value.length; i++) {
    var num = jspb.arith.UInt64.fromString(value[i]);
    this.encoder_.writeSplitFixed64(num.lo, num.hi);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed sfixed32 field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedSfixed32 = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 4);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeInt32(value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed sfixed64 field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedSfixed64 = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 8);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeInt64(value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed sfixed64 field.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of decimal strings to write.
 */
jspb.BinaryWriter.prototype.writePackedSfixed64String = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 8);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeInt64String(value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed float field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedFloat = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 4);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeFloat(value[i]);
  }
};


/**
 * Writes an array of numbers to the buffer as a packed double field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedDouble = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 8);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeDouble(value[i]);
  }
};


/**
 * Writes an array of booleans to the buffer as a packed bool field.
 * @param {number} field The field number.
 * @param {?Array<boolean>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedBool = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeBool(value[i]);
  }
};


/**
 * Writes an array of enums to the buffer as a packed enum field.
 * @param {number} field The field number.
 * @param {?Array<number>} value The array of ints to write.
 */
jspb.BinaryWriter.prototype.writePackedEnum = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeEnum(value[i]);
  }
  this.endDelimited_(bookmark);
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of hashes to write.
 */
jspb.BinaryWriter.prototype.writePackedFixedHash64 = function(field, value) {
  if (value == null || !value.length) return;
  this.writeFieldHeader_(field, jspb.BinaryConstants.WireType.DELIMITED);
  this.encoder_.writeUnsignedVarint32(value.length * 8);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeFixedHash64(value[i]);
  }
};


/**
 * Writes a 64-bit hash string field (8 characters @ 8 bits of data each) to
 * the buffer.
 * @param {number} field The field number.
 * @param {?Array<string>} value The array of hashes to write.
 */
jspb.BinaryWriter.prototype.writePackedVarintHash64 = function(field, value) {
  if (value == null || !value.length) return;
  var bookmark = this.beginDelimited_(field);
  for (var i = 0; i < value.length; i++) {
    this.encoder_.writeVarintHash64(value[i]);
  }
  this.endDelimited_(bookmark);
};
