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
 * @fileoverview This file contains utilities for converting binary,
 * wire-format protocol buffers into Javascript data structures.
 *
 * jspb's BinaryReader class wraps the BinaryDecoder class to add methods
 * that understand the protocol buffer syntax and can do the type checking and
 * bookkeeping necessary to parse trees of nested messages.
 *
 * Major caveat - Users of this library _must_ keep their Javascript proto
 * parsing code in sync with the original .proto file - presumably you'll be
 * using the typed jspb code generator, but if you bypass that you'll need
 * to keep things in sync by hand.
 *
 * @suppress {missingRequire} TODO(b/152540451): this shouldn't be needed
 * @author aappleby@google.com (Austin Appleby)
 */

goog.provide('jspb.BinaryReader');

goog.require('goog.asserts');
goog.require('jspb.BinaryConstants');
goog.require('jspb.BinaryDecoder');
goog.require('jspb.utils');



/**
 * BinaryReader implements the decoders for all the wire types specified in
 * https://developers.google.com/protocol-buffers/docs/encoding.
 *
 * @param {jspb.ByteSource=} opt_bytes The bytes we're reading from.
 * @param {number=} opt_start The optional offset to start reading at.
 * @param {number=} opt_length The optional length of the block to read -
 *     we'll throw an assertion if we go off the end of the block.
 * @constructor
 * @struct
 */
jspb.BinaryReader = function(opt_bytes, opt_start, opt_length) {
  /**
   * Wire-format decoder.
   * @private {!jspb.BinaryDecoder}
   */
  this.decoder_ = jspb.BinaryDecoder.alloc(opt_bytes, opt_start, opt_length);

  /**
   * Cursor immediately before the field tag.
   * @private {number}
   */
  this.fieldCursor_ = this.decoder_.getCursor();

  /**
   * Field number of the next field in the buffer, filled in by nextField().
   * @private {number}
   */
  this.nextField_ = jspb.BinaryConstants.INVALID_FIELD_NUMBER;

  /**
   * Wire type of the next proto field in the buffer, filled in by
   * nextField().
   * @private {jspb.BinaryConstants.WireType}
   */
  this.nextWireType_ = jspb.BinaryConstants.WireType.INVALID;

  /**
   * Set to true if this reader encountered an error due to corrupt data.
   * @private {boolean}
   */
  this.error_ = false;

  /**
   * User-defined reader callbacks.
   * @private {?Object<string, function(!jspb.BinaryReader):*>}
   */
  this.readCallbacks_ = null;
};


/**
 * Global pool of BinaryReader instances.
 * @private {!Array<!jspb.BinaryReader>}
 */
jspb.BinaryReader.instanceCache_ = [];


/**
 * Pops an instance off the instance cache, or creates one if the cache is
 * empty.
 * @param {jspb.ByteSource=} opt_bytes The bytes we're reading from.
 * @param {number=} opt_start The optional offset to start reading at.
 * @param {number=} opt_length The optional length of the block to read -
 *     we'll throw an assertion if we go off the end of the block.
 * @return {!jspb.BinaryReader}
 */
jspb.BinaryReader.alloc =
    function(opt_bytes, opt_start, opt_length) {
  if (jspb.BinaryReader.instanceCache_.length) {
    var newReader = jspb.BinaryReader.instanceCache_.pop();
    if (opt_bytes) {
      newReader.decoder_.setBlock(opt_bytes, opt_start, opt_length);
    }
    return newReader;
  } else {
    return new jspb.BinaryReader(opt_bytes, opt_start, opt_length);
  }
};


/**
 * Alias for the above method.
 * @param {jspb.ByteSource=} opt_bytes The bytes we're reading from.
 * @param {number=} opt_start The optional offset to start reading at.
 * @param {number=} opt_length The optional length of the block to read -
 *     we'll throw an assertion if we go off the end of the block.
 * @return {!jspb.BinaryReader}
 */
jspb.BinaryReader.prototype.alloc = jspb.BinaryReader.alloc;


/**
 * Puts this instance back in the instance cache.
 */
jspb.BinaryReader.prototype.free = function() {
  this.decoder_.clear();
  this.nextField_ = jspb.BinaryConstants.INVALID_FIELD_NUMBER;
  this.nextWireType_ = jspb.BinaryConstants.WireType.INVALID;
  this.error_ = false;
  this.readCallbacks_ = null;

  if (jspb.BinaryReader.instanceCache_.length < 100) {
    jspb.BinaryReader.instanceCache_.push(this);
  }
};


/**
 * Returns the cursor immediately before the current field's tag.
 * @return {number} The internal read cursor.
 */
jspb.BinaryReader.prototype.getFieldCursor = function() {
  return this.fieldCursor_;
};


/**
 * Returns the internal read cursor.
 * @return {number} The internal read cursor.
 */
jspb.BinaryReader.prototype.getCursor = function() {
  return this.decoder_.getCursor();
};


/**
 * Returns the raw buffer.
 * @return {?Uint8Array} The raw buffer.
 */
jspb.BinaryReader.prototype.getBuffer = function() {
  return this.decoder_.getBuffer();
};


/**
 * @return {number} The field number of the next field in the buffer, or
 *     INVALID_FIELD_NUMBER if there is no next field.
 */
jspb.BinaryReader.prototype.getFieldNumber = function() {
  return this.nextField_;
};


/**
 * @return {jspb.BinaryConstants.WireType} The wire type of the next field
 *     in the stream, or WireType.INVALID if there is no next field.
 */
jspb.BinaryReader.prototype.getWireType = function() {
  return this.nextWireType_;
};


/**
 * @return {boolean} Whether the current wire type is a delimited field. Used to
 * conditionally parse packed repeated fields.
 */
jspb.BinaryReader.prototype.isDelimited = function() {
  return this.nextWireType_ == jspb.BinaryConstants.WireType.DELIMITED;
};


/**
 * @return {boolean} Whether the current wire type is an end-group tag. Used as
 * an exit condition in decoder loops in generated code.
 */
jspb.BinaryReader.prototype.isEndGroup = function() {
  return this.nextWireType_ == jspb.BinaryConstants.WireType.END_GROUP;
};


/**
 * Returns true if this reader hit an error due to corrupt data.
 * @return {boolean}
 */
jspb.BinaryReader.prototype.getError = function() {
  return this.error_ || this.decoder_.getError();
};


/**
 * Points this reader at a new block of bytes.
 * @param {!Uint8Array} bytes The block of bytes we're reading from.
 * @param {number} start The offset to start reading at.
 * @param {number} length The length of the block to read.
 */
jspb.BinaryReader.prototype.setBlock = function(bytes, start, length) {
  this.decoder_.setBlock(bytes, start, length);
  this.nextField_ = jspb.BinaryConstants.INVALID_FIELD_NUMBER;
  this.nextWireType_ = jspb.BinaryConstants.WireType.INVALID;
};


/**
 * Rewinds the stream cursor to the beginning of the buffer and resets all
 * internal state.
 */
jspb.BinaryReader.prototype.reset = function() {
  this.decoder_.reset();
  this.nextField_ = jspb.BinaryConstants.INVALID_FIELD_NUMBER;
  this.nextWireType_ = jspb.BinaryConstants.WireType.INVALID;
};


/**
 * Advances the stream cursor by the given number of bytes.
 * @param {number} count The number of bytes to advance by.
 */
jspb.BinaryReader.prototype.advance = function(count) {
  this.decoder_.advance(count);
};


/**
 * Reads the next field header in the stream if there is one, returns true if
 * we saw a valid field header or false if we've read the whole stream.
 * Throws an error if we encountered a deprecated START_GROUP/END_GROUP field.
 * @return {boolean} True if the stream contains more fields.
 */
jspb.BinaryReader.prototype.nextField = function() {
  // If we're at the end of the block, there are no more fields.
  if (this.decoder_.atEnd()) {
    return false;
  }

  // If we hit an error decoding the previous field, stop now before we
  // try to decode anything else
  if (this.getError()) {
    goog.asserts.fail('Decoder hit an error');
    return false;
  }

  // Otherwise just read the header of the next field.
  this.fieldCursor_ = this.decoder_.getCursor();
  var header = this.decoder_.readUnsignedVarint32();

  var nextField = header >>> 3;
  var nextWireType = /** @type {jspb.BinaryConstants.WireType} */
      (header & 0x7);

  // If the wire type isn't one of the valid ones, something's broken.
  if (nextWireType != jspb.BinaryConstants.WireType.VARINT &&
      nextWireType != jspb.BinaryConstants.WireType.FIXED32 &&
      nextWireType != jspb.BinaryConstants.WireType.FIXED64 &&
      nextWireType != jspb.BinaryConstants.WireType.DELIMITED &&
      nextWireType != jspb.BinaryConstants.WireType.START_GROUP &&
      nextWireType != jspb.BinaryConstants.WireType.END_GROUP) {
    goog.asserts.fail(
        'Invalid wire type: %s (at position %s)', nextWireType,
        this.fieldCursor_);
    this.error_ = true;
    return false;
  }

  this.nextField_ = nextField;
  this.nextWireType_ = nextWireType;

  return true;
};


/**
 * Winds the reader back to just before this field's header.
 */
jspb.BinaryReader.prototype.unskipHeader = function() {
  this.decoder_.unskipVarint((this.nextField_ << 3) | this.nextWireType_);
};


/**
 * Skips all contiguous fields whose header matches the one we just read.
 */
jspb.BinaryReader.prototype.skipMatchingFields = function() {
  var field = this.nextField_;
  this.unskipHeader();

  while (this.nextField() && (this.getFieldNumber() == field)) {
    this.skipField();
  }

  if (!this.decoder_.atEnd()) {
    this.unskipHeader();
  }
};


/**
 * Skips over the next varint field in the binary stream.
 */
jspb.BinaryReader.prototype.skipVarintField = function() {
  if (this.nextWireType_ != jspb.BinaryConstants.WireType.VARINT) {
    goog.asserts.fail('Invalid wire type for skipVarintField');
    this.skipField();
    return;
  }

  this.decoder_.skipVarint();
};


/**
 * Skips over the next delimited field in the binary stream.
 */
jspb.BinaryReader.prototype.skipDelimitedField = function() {
  if (this.nextWireType_ != jspb.BinaryConstants.WireType.DELIMITED) {
    goog.asserts.fail('Invalid wire type for skipDelimitedField');
    this.skipField();
    return;
  }

  var length = this.decoder_.readUnsignedVarint32();
  this.decoder_.advance(length);
};


/**
 * Skips over the next fixed32 field in the binary stream.
 */
jspb.BinaryReader.prototype.skipFixed32Field = function() {
  if (this.nextWireType_ != jspb.BinaryConstants.WireType.FIXED32) {
    goog.asserts.fail('Invalid wire type for skipFixed32Field');
    this.skipField();
    return;
  }

  this.decoder_.advance(4);
};


/**
 * Skips over the next fixed64 field in the binary stream.
 */
jspb.BinaryReader.prototype.skipFixed64Field = function() {
  if (this.nextWireType_ != jspb.BinaryConstants.WireType.FIXED64) {
    goog.asserts.fail('Invalid wire type for skipFixed64Field');
    this.skipField();
    return;
  }

  this.decoder_.advance(8);
};


/**
 * Skips over the next group field in the binary stream.
 */
jspb.BinaryReader.prototype.skipGroup = function() {
  var previousField = this.nextField_;
  do {
    if (!this.nextField()) {
      goog.asserts.fail('Unmatched start-group tag: stream EOF');
      this.error_ = true;
      return;
    }
    if (this.nextWireType_ ==
               jspb.BinaryConstants.WireType.END_GROUP) {
      // Group end: check that it matches top-of-stack.
      if (this.nextField_ != previousField) {
        goog.asserts.fail('Unmatched end-group tag');
        this.error_ = true;
        return;
      }
      return;
    }
    this.skipField();
  } while (true);
};


/**
 * Skips over the next field in the binary stream - this is useful if we're
 * decoding a message that contain unknown fields.
 */
jspb.BinaryReader.prototype.skipField = function() {
  switch (this.nextWireType_) {
    case jspb.BinaryConstants.WireType.VARINT:
      this.skipVarintField();
      break;
    case jspb.BinaryConstants.WireType.FIXED64:
      this.skipFixed64Field();
      break;
    case jspb.BinaryConstants.WireType.DELIMITED:
      this.skipDelimitedField();
      break;
    case jspb.BinaryConstants.WireType.FIXED32:
      this.skipFixed32Field();
      break;
    case jspb.BinaryConstants.WireType.START_GROUP:
      this.skipGroup();
      break;
    default:
      goog.asserts.fail('Invalid wire encoding for field.');
  }
};


/**
 * Registers a user-defined read callback.
 * @param {string} callbackName
 * @param {function(!jspb.BinaryReader):*} callback
 */
jspb.BinaryReader.prototype.registerReadCallback = function(
    callbackName, callback) {
  if (this.readCallbacks_ === null) {
    this.readCallbacks_ = {};
  }
  goog.asserts.assert(!this.readCallbacks_[callbackName]);
  this.readCallbacks_[callbackName] = callback;
};


/**
 * Runs a registered read callback.
 * @param {string} callbackName The name the callback is registered under.
 * @return {*} The value returned by the callback.
 */
jspb.BinaryReader.prototype.runReadCallback = function(callbackName) {
  goog.asserts.assert(this.readCallbacks_ !== null);
  var callback = this.readCallbacks_[callbackName];
  goog.asserts.assert(callback);
  return callback(this);
};


/**
 * Reads a field of any valid non-message type from the binary stream.
 * @param {jspb.BinaryConstants.FieldType} fieldType
 * @return {jspb.AnyFieldType}
 */
jspb.BinaryReader.prototype.readAny = function(fieldType) {
  this.nextWireType_ = jspb.BinaryConstants.FieldTypeToWireType(fieldType);
  var fieldTypes = jspb.BinaryConstants.FieldType;
  switch (fieldType) {
    case fieldTypes.DOUBLE:
      return this.readDouble();
    case fieldTypes.FLOAT:
      return this.readFloat();
    case fieldTypes.INT64:
      return this.readInt64();
    case fieldTypes.UINT64:
      return this.readUint64();
    case fieldTypes.INT32:
      return this.readInt32();
    case fieldTypes.FIXED64:
      return this.readFixed64();
    case fieldTypes.FIXED32:
      return this.readFixed32();
    case fieldTypes.BOOL:
      return this.readBool();
    case fieldTypes.STRING:
      return this.readString();
    case fieldTypes.GROUP:
      goog.asserts.fail('Group field type not supported in readAny()');
    case fieldTypes.MESSAGE:
      goog.asserts.fail('Message field type not supported in readAny()');
    case fieldTypes.BYTES:
      return this.readBytes();
    case fieldTypes.UINT32:
      return this.readUint32();
    case fieldTypes.ENUM:
      return this.readEnum();
    case fieldTypes.SFIXED32:
      return this.readSfixed32();
    case fieldTypes.SFIXED64:
      return this.readSfixed64();
    case fieldTypes.SINT32:
      return this.readSint32();
    case fieldTypes.SINT64:
      return this.readSint64();
    case fieldTypes.FHASH64:
      return this.readFixedHash64();
    case fieldTypes.VHASH64:
      return this.readVarintHash64();
    default:
      goog.asserts.fail('Invalid field type in readAny()');
  }
  return 0;
};


/**
 * Deserialize a proto into the provided message object using the provided
 * reader function. This function is templated as we currently have one client
 * who is using manual deserialization instead of the code-generated versions.
 * @template T
 * @param {T} message
 * @param {function(T, !jspb.BinaryReader)} reader
 */
jspb.BinaryReader.prototype.readMessage = function(message, reader) {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.DELIMITED);

  // Save the current endpoint of the decoder and move it to the end of the
  // embedded message.
  var oldEnd = this.decoder_.getEnd();
  var length = this.decoder_.readUnsignedVarint32();
  var newEnd = this.decoder_.getCursor() + length;
  this.decoder_.setEnd(newEnd);

  // Deserialize the embedded message.
  reader(message, this);

  // Advance the decoder past the embedded message and restore the endpoint.
  this.decoder_.setCursor(newEnd);
  this.decoder_.setEnd(oldEnd);
};


/**
 * Deserialize a proto into the provided message object using the provided
 * reader function, assuming that the message is serialized as a group
 * with the given tag.
 * @template T
 * @param {number} field
 * @param {T} message
 * @param {function(T, !jspb.BinaryReader)} reader
 */
jspb.BinaryReader.prototype.readGroup =
    function(field, message, reader) {
  // Ensure that the wire type is correct.
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.START_GROUP);
  // Ensure that the field number is correct.
  goog.asserts.assert(this.nextField_ == field);

  // Deserialize the message. The deserialization will stop at an END_GROUP tag.
  reader(message, this);

  if (!this.error_ &&
      this.nextWireType_ != jspb.BinaryConstants.WireType.END_GROUP) {
    goog.asserts.fail('Group submessage did not end with an END_GROUP tag');
    this.error_ = true;
  }
};


/**
 * Return a decoder that wraps the current delimited field.
 * @return {!jspb.BinaryDecoder}
 */
jspb.BinaryReader.prototype.getFieldDecoder = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.DELIMITED);

  var length = this.decoder_.readUnsignedVarint32();
  var start = this.decoder_.getCursor();
  var end = start + length;

  var innerDecoder =
      jspb.BinaryDecoder.alloc(this.decoder_.getBuffer(), start, length);
  this.decoder_.setCursor(end);
  return innerDecoder;
};


/**
 * Reads a signed 32-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * @return {number} The value of the signed 32-bit integer field.
 */
jspb.BinaryReader.prototype.readInt32 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readSignedVarint32();
};


/**
 * Reads a signed 32-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * Returns the value as a string.
 *
 * @return {string} The value of the signed 32-bit integer field as a decimal
 * string.
 */
jspb.BinaryReader.prototype.readInt32String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readSignedVarint32String();
};


/**
 * Reads a signed 64-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * @return {number} The value of the signed 64-bit integer field.
 */
jspb.BinaryReader.prototype.readInt64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readSignedVarint64();
};


/**
 * Reads a signed 64-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * Returns the value as a string.
 *
 * @return {string} The value of the signed 64-bit integer field as a decimal
 * string.
 */
jspb.BinaryReader.prototype.readInt64String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readSignedVarint64String();
};


/**
 * Reads an unsigned 32-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * @return {number} The value of the unsigned 32-bit integer field.
 */
jspb.BinaryReader.prototype.readUint32 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readUnsignedVarint32();
};


/**
 * Reads an unsigned 32-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * Returns the value as a string.
 *
 * @return {string} The value of the unsigned 32-bit integer field as a decimal
 * string.
 */
jspb.BinaryReader.prototype.readUint32String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readUnsignedVarint32String();
};


/**
 * Reads an unsigned 64-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * @return {number} The value of the unsigned 64-bit integer field.
 */
jspb.BinaryReader.prototype.readUint64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readUnsignedVarint64();
};


/**
 * Reads an unsigned 64-bit integer field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * Returns the value as a string.
 *
 * @return {string} The value of the unsigned 64-bit integer field as a decimal
 * string.
 */
jspb.BinaryReader.prototype.readUint64String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readUnsignedVarint64String();
};


/**
 * Reads a signed zigzag-encoded 32-bit integer field from the binary stream,
 * or throws an error if the next field in the stream is not of the correct
 * wire type.
 *
 * @return {number} The value of the signed 32-bit integer field.
 */
jspb.BinaryReader.prototype.readSint32 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readZigzagVarint32();
};


/**
 * Reads a signed zigzag-encoded 64-bit integer field from the binary stream,
 * or throws an error if the next field in the stream is not of the correct
 * wire type.
 *
 * @return {number} The value of the signed 64-bit integer field.
 */
jspb.BinaryReader.prototype.readSint64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readZigzagVarint64();
};


/**
 * Reads a signed zigzag-encoded 64-bit integer field from the binary stream,
 * or throws an error if the next field in the stream is not of the correct
 * wire type.
 *
 * @return {string} The value of the signed 64-bit integer field as a decimal string.
 */
jspb.BinaryReader.prototype.readSint64String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readZigzagVarint64String();
};


/**
 * Reads an unsigned 32-bit fixed-length integer fiield from the binary stream,
 * or throws an error if the next field in the stream is not of the correct
 * wire type.
 *
 * @return {number} The value of the double field.
 */
jspb.BinaryReader.prototype.readFixed32 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED32);
  return this.decoder_.readUint32();
};


/**
 * Reads an unsigned 64-bit fixed-length integer fiield from the binary stream,
 * or throws an error if the next field in the stream is not of the correct
 * wire type.
 *
 * @return {number} The value of the float field.
 */
jspb.BinaryReader.prototype.readFixed64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED64);
  return this.decoder_.readUint64();
};


/**
 * Reads a signed 64-bit integer field from the binary stream as a string, or
 * throws an error if the next field in the stream is not of the correct wire
 * type.
 *
 * Returns the value as a string.
 *
 * @return {string} The value of the unsigned 64-bit integer field as a decimal
 * string.
 */
jspb.BinaryReader.prototype.readFixed64String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED64);
  return this.decoder_.readUint64String();
};


/**
 * Reads a signed 32-bit fixed-length integer fiield from the binary stream, or
 * throws an error if the next field in the stream is not of the correct wire
 * type.
 *
 * @return {number} The value of the signed 32-bit integer field.
 */
jspb.BinaryReader.prototype.readSfixed32 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED32);
  return this.decoder_.readInt32();
};


/**
 * Reads a signed 32-bit fixed-length integer fiield from the binary stream, or
 * throws an error if the next field in the stream is not of the correct wire
 * type.
 *
 * @return {string} The value of the signed 32-bit integer field as a decimal
 * string.
 */
jspb.BinaryReader.prototype.readSfixed32String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED32);
  return this.decoder_.readInt32().toString();
};


/**
 * Reads a signed 64-bit fixed-length integer fiield from the binary stream, or
 * throws an error if the next field in the stream is not of the correct wire
 * type.
 *
 * @return {number} The value of the sfixed64 field.
 */
jspb.BinaryReader.prototype.readSfixed64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED64);
  return this.decoder_.readInt64();
};


/**
 * Reads a signed 64-bit fixed-length integer fiield from the binary stream, or
 * throws an error if the next field in the stream is not of the correct wire
 * type.
 *
 * Returns the value as a string.
 *
 * @return {string} The value of the sfixed64 field as a decimal string.
 */
jspb.BinaryReader.prototype.readSfixed64String = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED64);
  return this.decoder_.readInt64String();
};


/**
 * Reads a 32-bit floating-point field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * @return {number} The value of the float field.
 */
jspb.BinaryReader.prototype.readFloat = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED32);
  return this.decoder_.readFloat();
};


/**
 * Reads a 64-bit floating-point field from the binary stream, or throws an
 * error if the next field in the stream is not of the correct wire type.
 *
 * @return {number} The value of the double field.
 */
jspb.BinaryReader.prototype.readDouble = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED64);
  return this.decoder_.readDouble();
};


/**
 * Reads a boolean field from the binary stream, or throws an error if the next
 * field in the stream is not of the correct wire type.
 *
 * @return {boolean} The value of the boolean field.
 */
jspb.BinaryReader.prototype.readBool = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return !!this.decoder_.readUnsignedVarint32();
};


/**
 * Reads an enum field from the binary stream, or throws an error if the next
 * field in the stream is not of the correct wire type.
 *
 * @return {number} The value of the enum field.
 */
jspb.BinaryReader.prototype.readEnum = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readSignedVarint64();
};


/**
 * Reads a string field from the binary stream, or throws an error if the next
 * field in the stream is not of the correct wire type.
 *
 * @return {string} The value of the string field.
 */
jspb.BinaryReader.prototype.readString = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.DELIMITED);
  var length = this.decoder_.readUnsignedVarint32();
  return this.decoder_.readString(length);
};


/**
 * Reads a length-prefixed block of bytes from the binary stream, or returns
 * null if the next field in the stream has an invalid length value.
 *
 * @return {!Uint8Array} The block of bytes.
 */
jspb.BinaryReader.prototype.readBytes = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.DELIMITED);
  var length = this.decoder_.readUnsignedVarint32();
  return this.decoder_.readBytes(length);
};


/**
 * Reads a 64-bit varint or fixed64 field from the stream and returns it as an
 * 8-character Unicode string for use as a hash table key, or throws an error
 * if the next field in the stream is not of the correct wire type.
 *
 * @return {string} The hash value.
 */
jspb.BinaryReader.prototype.readVarintHash64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readVarintHash64();
};


/**
 * Reads an sint64 field from the stream and returns it as an 8-character
 * Unicode string for use as a hash table key, or throws an error if the next
 * field in the stream is not of the correct wire type.
 *
 * @return {string} The hash value.
 */
jspb.BinaryReader.prototype.readSintHash64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readZigzagVarintHash64();
};


/**
 * Reads a 64-bit varint field from the stream and invokes `convert` to produce
 * the return value, or throws an error if the next field in the stream is not
 * of the correct wire type.
 *
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.BinaryReader.prototype.readSplitVarint64 = function(convert) {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readSplitVarint64(convert);
};


/**
 * Reads a 64-bit zig-zag varint field from the stream and invokes `convert` to
 * produce the return value, or throws an error if the next field in the stream
 * is not of the correct wire type.
 *
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.BinaryReader.prototype.readSplitZigzagVarint64 = function(convert) {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.VARINT);
  return this.decoder_.readSplitVarint64(function(lowBits, highBits) {
    return jspb.utils.fromZigzag64(lowBits, highBits, convert);
  });
};


/**
 * Reads a 64-bit varint or fixed64 field from the stream and returns it as a
 * 8-character Unicode string for use as a hash table key, or throws an error
 * if the next field in the stream is not of the correct wire type.
 *
 * @return {string} The hash value.
 */
jspb.BinaryReader.prototype.readFixedHash64 = function() {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED64);
  return this.decoder_.readFixedHash64();
};


/**
 * Reads a 64-bit fixed64 field from the stream and invokes `convert`
 * to produce the return value, or throws an error if the next field in the
 * stream is not of the correct wire type.
 *
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.BinaryReader.prototype.readSplitFixed64 = function(convert) {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.FIXED64);
  return this.decoder_.readSplitFixed64(convert);
};


/**
 * Reads a packed scalar field using the supplied raw reader function.
 * @param {function(this:jspb.BinaryDecoder)} decodeMethod
 * @return {!Array}
 * @private
 */
jspb.BinaryReader.prototype.readPackedField_ = function(decodeMethod) {
  goog.asserts.assert(
      this.nextWireType_ == jspb.BinaryConstants.WireType.DELIMITED);
  var length = this.decoder_.readUnsignedVarint32();
  var end = this.decoder_.getCursor() + length;
  var result = [];
  while (this.decoder_.getCursor() < end) {
    // TODO(aappleby): .call is slow
    result.push(decodeMethod.call(this.decoder_));
  }
  return result;
};


/**
 * Reads a packed int32 field, which consists of a length header and a list of
 * signed varints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedInt32 = function() {
  return this.readPackedField_(this.decoder_.readSignedVarint32);
};


/**
 * Reads a packed int32 field, which consists of a length header and a list of
 * signed varints. Returns a list of strings.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedInt32String = function() {
  return this.readPackedField_(this.decoder_.readSignedVarint32String);
};


/**
 * Reads a packed int64 field, which consists of a length header and a list of
 * signed varints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedInt64 = function() {
  return this.readPackedField_(this.decoder_.readSignedVarint64);
};


/**
 * Reads a packed int64 field, which consists of a length header and a list of
 * signed varints. Returns a list of strings.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedInt64String = function() {
  return this.readPackedField_(this.decoder_.readSignedVarint64String);
};


/**
 * Reads a packed uint32 field, which consists of a length header and a list of
 * unsigned varints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedUint32 = function() {
  return this.readPackedField_(this.decoder_.readUnsignedVarint32);
};


/**
 * Reads a packed uint32 field, which consists of a length header and a list of
 * unsigned varints. Returns a list of strings.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedUint32String = function() {
  return this.readPackedField_(this.decoder_.readUnsignedVarint32String);
};


/**
 * Reads a packed uint64 field, which consists of a length header and a list of
 * unsigned varints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedUint64 = function() {
  return this.readPackedField_(this.decoder_.readUnsignedVarint64);
};


/**
 * Reads a packed uint64 field, which consists of a length header and a list of
 * unsigned varints. Returns a list of strings.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedUint64String = function() {
  return this.readPackedField_(this.decoder_.readUnsignedVarint64String);
};


/**
 * Reads a packed sint32 field, which consists of a length header and a list of
 * zigzag varints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedSint32 = function() {
  return this.readPackedField_(this.decoder_.readZigzagVarint32);
};


/**
 * Reads a packed sint64 field, which consists of a length header and a list of
 * zigzag varints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedSint64 = function() {
  return this.readPackedField_(this.decoder_.readZigzagVarint64);
};


/**
 * Reads a packed sint64 field, which consists of a length header and a list of
 * zigzag varints.  Returns a list of strings.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedSint64String = function() {
  return this.readPackedField_(this.decoder_.readZigzagVarint64String);
};


/**
 * Reads a packed fixed32 field, which consists of a length header and a list
 * of unsigned 32-bit ints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedFixed32 = function() {
  return this.readPackedField_(this.decoder_.readUint32);
};


/**
 * Reads a packed fixed64 field, which consists of a length header and a list
 * of unsigned 64-bit ints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedFixed64 = function() {
  return this.readPackedField_(this.decoder_.readUint64);
};


/**
 * Reads a packed fixed64 field, which consists of a length header and a list
 * of unsigned 64-bit ints.  Returns a list of strings.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedFixed64String = function() {
  return this.readPackedField_(this.decoder_.readUint64String);
};


/**
 * Reads a packed sfixed32 field, which consists of a length header and a list
 * of 32-bit ints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedSfixed32 = function() {
  return this.readPackedField_(this.decoder_.readInt32);
};


/**
 * Reads a packed sfixed64 field, which consists of a length header and a list
 * of 64-bit ints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedSfixed64 = function() {
  return this.readPackedField_(this.decoder_.readInt64);
};


/**
 * Reads a packed sfixed64 field, which consists of a length header and a list
 * of 64-bit ints.  Returns a list of strings.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedSfixed64String = function() {
  return this.readPackedField_(this.decoder_.readInt64String);
};


/**
 * Reads a packed float field, which consists of a length header and a list of
 * floats.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedFloat = function() {
  return this.readPackedField_(this.decoder_.readFloat);
};


/**
 * Reads a packed double field, which consists of a length header and a list of
 * doubles.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedDouble = function() {
  return this.readPackedField_(this.decoder_.readDouble);
};


/**
 * Reads a packed bool field, which consists of a length header and a list of
 * unsigned varints.
 * @return {!Array<boolean>}
 */
jspb.BinaryReader.prototype.readPackedBool = function() {
  return this.readPackedField_(this.decoder_.readBool);
};


/**
 * Reads a packed enum field, which consists of a length header and a list of
 * unsigned varints.
 * @return {!Array<number>}
 */
jspb.BinaryReader.prototype.readPackedEnum = function() {
  return this.readPackedField_(this.decoder_.readEnum);
};


/**
 * Reads a packed varint hash64 field, which consists of a length header and a
 * list of varint hash64s.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedVarintHash64 = function() {
  return this.readPackedField_(this.decoder_.readVarintHash64);
};


/**
 * Reads a packed fixed hash64 field, which consists of a length header and a
 * list of fixed hash64s.
 * @return {!Array<string>}
 */
jspb.BinaryReader.prototype.readPackedFixedHash64 = function() {
  return this.readPackedField_(this.decoder_.readFixedHash64);
};
