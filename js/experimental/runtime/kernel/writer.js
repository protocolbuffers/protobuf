/**
 * @fileoverview Implements Writer for writing data as the binary wire format
 * bytes array.
 */
goog.module('protobuf.binary.Writer');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');
const WireType = goog.require('protobuf.binary.WireType');
const {POLYFILL_TEXT_ENCODING, checkFieldNumber, checkTypeUnsignedInt32, checkWireType} = goog.require('protobuf.internal.checks');
const {concatenateByteArrays} = goog.require('protobuf.binary.uint8arrays');
const {encode} = goog.require('protobuf.binary.textencoding');

/**
 * Returns a valid utf-8 encoder function based on TextEncoder if available or
 * a polyfill.
 * Some of the environments we run in do not have TextEncoder defined.
 * TextEncoder is faster than our polyfill so we prefer it over the polyfill.
 * @return {function(string):!Uint8Array}
 */
function getEncoderFunction() {
  if (goog.global['TextEncoder']) {
    const textEncoder = new goog.global['TextEncoder']('utf-8');
    return s => s.length === 0 ? new Uint8Array(0) : textEncoder.encode(s);
  }
  if (POLYFILL_TEXT_ENCODING) {
    return encode;
  } else {
    throw new Error(
        'TextEncoder is missing. ' +
        'Enable protobuf.defines.POLYFILL_TEXT_ENCODING');
  }
}

/** @const {function(string): !Uint8Array} */
const encoderFunction = getEncoderFunction();

/**
 * Writer provides methods for encoding all protobuf supported type into a
 * binary format bytes array.
 * Check https://developers.google.com/protocol-buffers/docs/encoding for binary
 * format definition.
 * @final
 * @package
 */
class Writer {
  constructor() {
    /**
     * Blocks of data that needs to be serialized. After writing all the data,
     * the blocks are concatenated into a single Uint8Array.
     * @private {!Array<!Uint8Array>}
     */
    this.blocks_ = [];

    /**
     * A buffer for writing varint data (tag number + field number for each
     * field, int32, uint32 etc.). Before writing a non-varint data block
     * (string, fixed32 etc.), the buffer is appended to the block array as a
     * new block, and a new buffer is started.
     *
     * We could've written each varint as a new block instead of writing
     * multiple varints in this buffer. But this will increase the number of
     * blocks, and concatenating many small blocks is slower than concatenating
     * few large blocks.
     *
     * TODO: Experiment with writing data in a fixed-length
     * Uint8Array instead of using a growing buffer.
     *
     * @private {!Array<number>}
     */
    this.currentBuffer_ = [];
  }

  /**
   * Converts the encoded data into a Uint8Array.
   * The writer is also reset.
   * @return {!ArrayBuffer}
   */
  getAndResetResultBuffer() {
    this.closeAndStartNewBuffer_();
    const result = concatenateByteArrays(this.blocks_);
    this.blocks_ = [];
    return result.buffer;
  }

  /**
   * Encodes a (field number, wire type) tuple into a wire-format field header.
   * @param {number} fieldNumber
   * @param {!WireType} wireType
   */
  writeTag(fieldNumber, wireType) {
    checkFieldNumber(fieldNumber);
    checkWireType(wireType);
    const tag = fieldNumber << 3 | wireType;
    this.writeUnsignedVarint32_(tag);
  }

  /**
   * Appends the current buffer into the blocks array and starts a new buffer.
   * @private
   */
  closeAndStartNewBuffer_() {
    this.blocks_.push(new Uint8Array(this.currentBuffer_));
    this.currentBuffer_ = [];
  }

  /**
   * Encodes a 32-bit integer into its wire-format varint representation and
   * stores it in the buffer.
   * @param {number} value
   * @private
   */
  writeUnsignedVarint32_(value) {
    checkTypeUnsignedInt32(value);
    while (value > 0x7f) {
      this.currentBuffer_.push((value & 0x7f) | 0x80);
      value = value >>> 7;
    }
    this.currentBuffer_.push(value);
  }

  /****************************************************************************
   *                        OPTIONAL METHODS
   ****************************************************************************/

  /**
   * Writes a boolean value field to the buffer as a varint.
   * @param {boolean} value
   * @private
   */
  writeBoolValue_(value) {
    this.currentBuffer_.push(value ? 1 : 0);
  }

  /**
   * Writes a boolean value field to the buffer as a varint.
   * @param {number} fieldNumber
   * @param {boolean} value
   */
  writeBool(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.VARINT);
    this.writeBoolValue_(value);
  }

  /**
   * Writes a bytes value field to the buffer as a length delimited field.
   * @param {number} fieldNumber
   * @param {!ByteString} value
   */
  writeBytes(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.DELIMITED);
    const buffer = value.toArrayBuffer();
    this.writeUnsignedVarint32_(buffer.byteLength);
    this.writeRaw_(buffer);
  }

  /**
   * Writes a double value field to the buffer without tag.
   * @param {number} value
   * @private
   */
  writeDoubleValue_(value) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    view.setFloat64(0, value, true);
    this.writeRaw_(buffer);
  }

  /**
   * Writes a double value field to the buffer.
   * @param {number} fieldNumber
   * @param {number} value
   */
  writeDouble(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.FIXED64);
    this.writeDoubleValue_(value);
  }

  /**
   * Writes a fixed32 value field to the buffer without tag.
   * @param {number} value
   * @private
   */
  writeFixed32Value_(value) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    view.setUint32(0, value, true);
    this.writeRaw_(buffer);
  }

  /**
   * Writes a fixed32 value field to the buffer.
   * @param {number} fieldNumber
   * @param {number} value
   */
  writeFixed32(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.FIXED32);
    this.writeFixed32Value_(value);
  }

  /**
   * Writes a float value field to the buffer without tag.
   * @param {number} value
   * @private
   */
  writeFloatValue_(value) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    view.setFloat32(0, value, true);
    this.writeRaw_(buffer);
  }

  /**
   * Writes a float value field to the buffer.
   * @param {number} fieldNumber
   * @param {number} value
   */
  writeFloat(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.FIXED32);
    this.writeFloatValue_(value);
  }

  /**
   * Writes a int32 value field to the buffer as a varint without tag.
   * @param {number} value
   * @private
   */
  writeInt32Value_(value) {
    if (value >= 0) {
      this.writeVarint64_(0, value);
    } else {
      this.writeVarint64_(0xFFFFFFFF, value);
    }
  }

  /**
   * Writes a int32 value field to the buffer as a varint.
   * @param {number} fieldNumber
   * @param {number} value
   */
  writeInt32(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.VARINT);
    this.writeInt32Value_(value);
  }

  /**
   * Writes a int64 value field to the buffer as a varint.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  writeInt64(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.VARINT);
    this.writeVarint64_(value.getHighBits(), value.getLowBits());
  }

  /**
   * Writes a sfixed32 value field to the buffer.
   * @param {number} value
   * @private
   */
  writeSfixed32Value_(value) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    view.setInt32(0, value, true);
    this.writeRaw_(buffer);
  }

  /**
   * Writes a sfixed32 value field to the buffer.
   * @param {number} fieldNumber
   * @param {number} value
   */
  writeSfixed32(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.FIXED32);
    this.writeSfixed32Value_(value);
  }

  /**
   * Writes a sfixed64 value field to the buffer without tag.
   * @param {!Int64} value
   * @private
   */
  writeSfixed64Value_(value) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    view.setInt32(0, value.getLowBits(), true);
    view.setInt32(4, value.getHighBits(), true);
    this.writeRaw_(buffer);
  }

  /**
   * Writes a sfixed64 value field to the buffer.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  writeSfixed64(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.FIXED64);
    this.writeSfixed64Value_(value);
  }

  /**
   * Writes a uint32 value field to the buffer as a varint without tag.
   * @param {number} value
   * @private
   */
  writeUint32Value_(value) {
    this.writeVarint64_(0, value);
  }

  /**
   * Writes a uint32 value field to the buffer as a varint.
   * @param {number} fieldNumber
   * @param {number} value
   */
  writeUint32(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.VARINT);
    this.writeUint32Value_(value);
  }

  /**
   * Writes the bits of a 64 bit number to the buffer as a varint.
   * @param {number} highBits
   * @param {number} lowBits
   * @private
   */
  writeVarint64_(highBits, lowBits) {
    for (let i = 0; i < 28; i = i + 7) {
      const shift = lowBits >>> i;
      const hasNext = !((shift >>> 7) === 0 && highBits === 0);
      const byte = (hasNext ? shift | 0x80 : shift) & 0xFF;
      this.currentBuffer_.push(byte);
      if (!hasNext) {
        return;
      }
    }

    const splitBits = ((lowBits >>> 28) & 0x0F) | ((highBits & 0x07) << 4);
    const hasMoreBits = !((highBits >> 3) === 0);
    this.currentBuffer_.push(
        (hasMoreBits ? splitBits | 0x80 : splitBits) & 0xFF);

    if (!hasMoreBits) {
      return;
    }

    for (let i = 3; i < 31; i = i + 7) {
      const shift = highBits >>> i;
      const hasNext = !((shift >>> 7) === 0);
      const byte = (hasNext ? shift | 0x80 : shift) & 0xFF;
      this.currentBuffer_.push(byte);
      if (!hasNext) {
        return;
      }
    }

    this.currentBuffer_.push((highBits >>> 31) & 0x01);
  }

  /**
   * Writes a sint32 value field to the buffer as a varint without tag.
   * @param {number} value
   * @private
   */
  writeSint32Value_(value) {
    value = (value << 1) ^ (value >> 31);
    this.writeVarint64_(0, value);
  }

  /**
   * Writes a sint32 value field to the buffer as a varint.
   * @param {number} fieldNumber
   * @param {number} value
   */
  writeSint32(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.VARINT);
    this.writeSint32Value_(value);
  }

  /**
   * Writes a sint64 value field to the buffer as a varint without tag.
   * @param {!Int64} value
   * @private
   */
  writeSint64Value_(value) {
    const highBits = value.getHighBits();
    const lowBits = value.getLowBits();

    const sign = highBits >> 31;
    const encodedLowBits = (lowBits << 1) ^ sign;
    const encodedHighBits = ((highBits << 1) | (lowBits >>> 31)) ^ sign;
    this.writeVarint64_(encodedHighBits, encodedLowBits);
  }

  /**
   * Writes a sint64 value field to the buffer as a varint.
   * @param {number} fieldNumber
   * @param {!Int64} value
   */
  writeSint64(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.VARINT);
    this.writeSint64Value_(value);
  }

  /**
   * Writes a string value field to the buffer as a varint.
   * @param {number} fieldNumber
   * @param {string} value
   */
  writeString(fieldNumber, value) {
    this.writeTag(fieldNumber, WireType.DELIMITED);
    const array = encoderFunction(value);
    this.writeUnsignedVarint32_(array.length);
    this.writeRaw_(array.buffer);
  }

  /**
   * Writes raw bytes to the buffer.
   * @param {!ArrayBuffer} arrayBuffer
   * @private
   */
  writeRaw_(arrayBuffer) {
    this.closeAndStartNewBuffer_();
    this.blocks_.push(new Uint8Array(arrayBuffer));
  }

  /**
   * Writes raw bytes to the buffer.
   * @param {!BufferDecoder} bufferDecoder
   * @param {number} start
   * @param {!WireType} wireType
   * @package
   */
  writeBufferDecoder(bufferDecoder, start, wireType) {
    this.closeAndStartNewBuffer_();
    const dataLength = this.getLength_(bufferDecoder, start, wireType);
    this.blocks_.push(
        bufferDecoder.subBufferDecoder(start, dataLength).asUint8Array());
  }

  /**
   * Returns the length of the data to serialize. Returns -1 when a STOP GROUP
   * is found.
   * @param {!BufferDecoder} bufferDecoder
   * @param {number} start
   * @param {!WireType} wireType
   * @return {number}
   * @private
   */
  getLength_(bufferDecoder, start, wireType) {
    switch (wireType) {
      case WireType.VARINT:
        return bufferDecoder.skipVarint(start) - start;
      case WireType.FIXED64:
        return 8;
      case WireType.DELIMITED:
        const {lowBits: dataLength, dataStart} = bufferDecoder.getVarint(start);
        return dataLength + dataStart - start;
      case WireType.START_GROUP:
        return this.getGroupLength_(bufferDecoder, start);
      case WireType.FIXED32:
        return 4;
      default:
        throw new Error(`Invalid wire type: ${wireType}`);
    }
  }

  /**
   * Skips over fields until it finds the end of a given group.
   * @param {!BufferDecoder} bufferDecoder
   * @param {number} start
   * @return {number}
   * @private
   */
  getGroupLength_(bufferDecoder, start) {
    // On a start group we need to keep skipping fields until we find a
    // corresponding stop group
    let cursor = start;
    while (cursor < bufferDecoder.endIndex()) {
      const {lowBits: tag, dataStart} = bufferDecoder.getVarint(cursor);
      const wireType = /** @type {!WireType} */ (tag & 0x07);
      if (wireType === WireType.END_GROUP) {
        return dataStart - start;
      }
      cursor = dataStart + this.getLength_(bufferDecoder, dataStart, wireType);
    }
    throw new Error('No end group found');
  }

  /**
   * Write the whole bytes as a length delimited field.
   * @param {number} fieldNumber
   * @param {!ArrayBuffer} arrayBuffer
   */
  writeDelimited(fieldNumber, arrayBuffer) {
    this.writeTag(fieldNumber, WireType.DELIMITED);
    this.writeUnsignedVarint32_(arrayBuffer.byteLength);
    this.writeRaw_(arrayBuffer);
  }

  /****************************************************************************
   *                        REPEATED METHODS
   ****************************************************************************/

  /**
   * Writes repeated boolean values to the buffer as unpacked varints.
   * @param {number} fieldNumber
   * @param {!Array<boolean>} values
   */
  writeRepeatedBool(fieldNumber, values) {
    values.forEach(val => this.writeBool(fieldNumber, val));
  }

  /**
   * Writes repeated boolean values to the buffer as packed varints.
   * @param {number} fieldNumber
   * @param {!Array<boolean>} values
   */
  writePackedBool(fieldNumber, values) {
    this.writeFixedPacked_(
        fieldNumber, values, val => this.writeBoolValue_(val), 1);
  }

  /**
   * Writes repeated double values to the buffer as unpacked fixed64.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writeRepeatedDouble(fieldNumber, values) {
    values.forEach(val => this.writeDouble(fieldNumber, val));
  }

  /**
   * Writes repeated double values to the buffer as packed fixed64.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writePackedDouble(fieldNumber, values) {
    this.writeFixedPacked_(
        fieldNumber, values, val => this.writeDoubleValue_(val), 8);
  }

  /**
   * Writes repeated fixed32 values to the buffer as unpacked fixed32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writeRepeatedFixed32(fieldNumber, values) {
    values.forEach(val => this.writeFixed32(fieldNumber, val));
  }

  /**
   * Writes repeated fixed32 values to the buffer as packed fixed32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writePackedFixed32(fieldNumber, values) {
    this.writeFixedPacked_(
        fieldNumber, values, val => this.writeFixed32Value_(val), 4);
  }

  /**
   * Writes repeated float values to the buffer as unpacked fixed64.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writeRepeatedFloat(fieldNumber, values) {
    values.forEach(val => this.writeFloat(fieldNumber, val));
  }

  /**
   * Writes repeated float values to the buffer as packed fixed64.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writePackedFloat(fieldNumber, values) {
    this.writeFixedPacked_(
        fieldNumber, values, val => this.writeFloatValue_(val), 4);
  }

  /**
   * Writes repeated int32 values to the buffer as unpacked int32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writeRepeatedInt32(fieldNumber, values) {
    values.forEach(val => this.writeInt32(fieldNumber, val));
  }

  /**
   * Writes repeated int32 values to the buffer as packed int32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writePackedInt32(fieldNumber, values) {
    this.writeVariablePacked_(
        fieldNumber, values, (writer, val) => writer.writeInt32Value_(val));
  }

  /**
   * Writes repeated int64 values to the buffer as unpacked varint.
   * @param {number} fieldNumber
   * @param {!Array<!Int64>} values
   */
  writeRepeatedInt64(fieldNumber, values) {
    values.forEach(val => this.writeInt64(fieldNumber, val));
  }

  /**
   * Writes repeated int64 values to the buffer as packed varint.
   * @param {number} fieldNumber
   * @param {!Array<!Int64>} values
   */
  writePackedInt64(fieldNumber, values) {
    this.writeVariablePacked_(
        fieldNumber, values,
        (writer, val) =>
            writer.writeVarint64_(val.getHighBits(), val.getLowBits()));
  }

  /**
   * Writes repeated sfixed32 values to the buffer as unpacked fixed32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writeRepeatedSfixed32(fieldNumber, values) {
    values.forEach(val => this.writeSfixed32(fieldNumber, val));
  }

  /**
   * Writes repeated sfixed32 values to the buffer as packed fixed32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writePackedSfixed32(fieldNumber, values) {
    this.writeFixedPacked_(
        fieldNumber, values, val => this.writeSfixed32Value_(val), 4);
  }

  /**
   * Writes repeated sfixed64 values to the buffer as unpacked fixed64.
   * @param {number} fieldNumber
   * @param {!Array<!Int64>} values
   */
  writeRepeatedSfixed64(fieldNumber, values) {
    values.forEach(val => this.writeSfixed64(fieldNumber, val));
  }

  /**
   * Writes repeated sfixed64 values to the buffer as packed fixed64.
   * @param {number} fieldNumber
   * @param {!Array<!Int64>} values
   */
  writePackedSfixed64(fieldNumber, values) {
    this.writeFixedPacked_(
        fieldNumber, values, val => this.writeSfixed64Value_(val), 8);
  }

  /**
   * Writes repeated sint32 values to the buffer as unpacked sint32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writeRepeatedSint32(fieldNumber, values) {
    values.forEach(val => this.writeSint32(fieldNumber, val));
  }

  /**
   * Writes repeated sint32 values to the buffer as packed sint32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writePackedSint32(fieldNumber, values) {
    this.writeVariablePacked_(
        fieldNumber, values, (writer, val) => writer.writeSint32Value_(val));
  }

  /**
   * Writes repeated sint64 values to the buffer as unpacked varint.
   * @param {number} fieldNumber
   * @param {!Array<!Int64>} values
   */
  writeRepeatedSint64(fieldNumber, values) {
    values.forEach(val => this.writeSint64(fieldNumber, val));
  }

  /**
   * Writes repeated sint64 values to the buffer as packed varint.
   * @param {number} fieldNumber
   * @param {!Array<!Int64>} values
   */
  writePackedSint64(fieldNumber, values) {
    this.writeVariablePacked_(
        fieldNumber, values, (writer, val) => writer.writeSint64Value_(val));
  }

  /**
   * Writes repeated uint32 values to the buffer as unpacked uint32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writeRepeatedUint32(fieldNumber, values) {
    values.forEach(val => this.writeUint32(fieldNumber, val));
  }

  /**
   * Writes repeated uint32 values to the buffer as packed uint32.
   * @param {number} fieldNumber
   * @param {!Array<number>} values
   */
  writePackedUint32(fieldNumber, values) {
    this.writeVariablePacked_(
        fieldNumber, values, (writer, val) => writer.writeUint32Value_(val));
  }

  /**
   * Writes repeated bytes values to the buffer.
   * @param {number} fieldNumber
   * @param {!Array<!ByteString>} values
   */
  writeRepeatedBytes(fieldNumber, values) {
    values.forEach(val => this.writeBytes(fieldNumber, val));
  }

  /**
   * Writes packed fields with fixed length.
   * @param {number} fieldNumber
   * @param {!Array<T>} values
   * @param {function(T)} valueWriter
   * @param {number} entitySize
   * @template T
   * @private
   */
  writeFixedPacked_(fieldNumber, values, valueWriter, entitySize) {
    if (values.length === 0) {
      return;
    }
    this.writeTag(fieldNumber, WireType.DELIMITED);
    this.writeUnsignedVarint32_(values.length * entitySize);
    this.closeAndStartNewBuffer_();
    values.forEach(value => valueWriter(value));
  }

  /**
   * Writes packed fields with variable length.
   * @param {number} fieldNumber
   * @param {!Array<T>} values
   * @param {function(!Writer, T)} valueWriter
   * @template T
   * @private
   */
  writeVariablePacked_(fieldNumber, values, valueWriter) {
    if (values.length === 0) {
      return;
    }
    const writer = new Writer();
    values.forEach(val => valueWriter(writer, val));
    const bytes = writer.getAndResetResultBuffer();
    this.writeDelimited(fieldNumber, bytes);
  }

  /**
   * Writes repeated string values to the buffer.
   * @param {number} fieldNumber
   * @param {!Array<string>} values
   */
  writeRepeatedString(fieldNumber, values) {
    values.forEach(val => this.writeString(fieldNumber, val));
  }
}

exports = Writer;
