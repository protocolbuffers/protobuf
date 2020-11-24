/**
 * @fileoverview Helper methods for reading data from the binary wire format.
 */
goog.module('protobuf.binary.reader');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');
const {checkState} = goog.require('protobuf.internal.checks');


/******************************************************************************
 *                        OPTIONAL FUNCTIONS
 ******************************************************************************/

/**
 * Reads a boolean value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {boolean}
 * @package
 */
function readBool(bufferDecoder, start) {
  const {lowBits, highBits} = bufferDecoder.getVarint(start);
  return lowBits !== 0 || highBits !== 0;
}

/**
 * Reads a ByteString value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!ByteString}
 * @package
 */
function readBytes(bufferDecoder, start) {
  return readDelimited(bufferDecoder, start).asByteString();
}

/**
 * Reads a int32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readInt32(bufferDecoder, start) {
  // Negative 32 bit integers are encoded with 64 bit values.
  // Clients are expected to truncate back to 32 bits.
  // This is why we are dropping the upper bytes here.
  return bufferDecoder.getUnsignedVarint32At(start) | 0;
}

/**
 * Reads a int32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Int64}
 * @package
 */
function readInt64(bufferDecoder, start) {
  const {lowBits, highBits} = bufferDecoder.getVarint(start);
  return Int64.fromBits(lowBits, highBits);
}

/**
 * Reads a fixed int32 value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readFixed32(bufferDecoder, start) {
  return bufferDecoder.getUint32(start);
}

/**
 * Reads a float value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readFloat(bufferDecoder, start) {
  return bufferDecoder.getFloat32(start);
}

/**
 * Reads a fixed int64 value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Int64}
 * @package
 */
function readSfixed64(bufferDecoder, start) {
  const lowBits = bufferDecoder.getInt32(start);
  const highBits = bufferDecoder.getInt32(start + 4);
  return Int64.fromBits(lowBits, highBits);
}

/**
 * Reads a sint32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readSint32(bufferDecoder, start) {
  const bits = bufferDecoder.getUnsignedVarint32At(start);
  // Truncate upper bits and convert from zig zag to signd int
  return (bits >>> 1) ^ -(bits & 0x01);
}

/**
 * Reads a sint64 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Int64}
 * @package
 */
function readSint64(bufferDecoder, start) {
  const {lowBits, highBits} = bufferDecoder.getVarint(start);
  const sign = -(lowBits & 0x01);
  const decodedLowerBits = ((lowBits >>> 1) | (highBits & 0x01) << 31) ^ sign;
  const decodedUpperBits = (highBits >>> 1) ^ sign;
  return Int64.fromBits(decodedLowerBits, decodedUpperBits);
}

/**
 * Read a subarray of bytes representing a length delimited field.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!BufferDecoder}
 * @package
 */
function readDelimited(bufferDecoder, start) {
  const unsignedLength = bufferDecoder.getUnsignedVarint32At(start);
  return bufferDecoder.subBufferDecoder(bufferDecoder.cursor(), unsignedLength);
}

/**
 * Reads a string value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {string}
 * @package
 */
function readString(bufferDecoder, start) {
  return readDelimited(bufferDecoder, start).asString();
}

/**
 * Reads a uint32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readUint32(bufferDecoder, start) {
  return bufferDecoder.getUnsignedVarint32At(start);
}

/**
 * Reads a double value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readDouble(bufferDecoder, start) {
  return bufferDecoder.getFloat64(start);
}

/**
 * Reads a fixed int32 value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readSfixed32(bufferDecoder, start) {
  return bufferDecoder.getInt32(start);
}

/******************************************************************************
 *                        REPEATED FUNCTIONS
 ******************************************************************************/

/**
 * Reads a packed bool field, which consists of a length header and a list of
 * unsigned varints.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<boolean>}
 * @package
 */
function readPackedBool(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readBool);
}

/**
 * Reads a packed double field, which consists of a length header and a list of
 * fixed64.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<number>}
 * @package
 */
function readPackedDouble(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readDouble);
}

/**
 * Reads a packed fixed32 field, which consists of a length header and a list of
 * fixed32.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<number>}
 * @package
 */
function readPackedFixed32(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readFixed32);
}

/**
 * Reads a packed float field, which consists of a length header and a list of
 * fixed64.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<number>}
 * @package
 */
function readPackedFloat(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readFloat);
}

/**
 * Reads a packed int32 field, which consists of a length header and a list of
 * varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<number>}
 * @package
 */
function readPackedInt32(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readInt32);
}

/**
 * Reads a packed int64 field, which consists of a length header and a list
 * of int64.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<!Int64>}
 * @package
 */
function readPackedInt64(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readInt64);
}

/**
 * Reads a packed sfixed32 field, which consists of a length header and a list
 * of sfixed32.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<number>}
 * @package
 */
function readPackedSfixed32(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readSfixed32);
}

/**
 * Reads a packed sfixed64 field, which consists of a length header and a list
 * of sfixed64.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<!Int64>}
 * @package
 */
function readPackedSfixed64(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readSfixed64);
}

/**
 * Reads a packed sint32 field, which consists of a length header and a list of
 * varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<number>}
 * @package
 */
function readPackedSint32(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readSint32);
}

/**
 * Reads a packed sint64 field, which consists of a length header and a list
 * of sint64.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<!Int64>}
 * @package
 */
function readPackedSint64(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readSint64);
}

/**
 * Reads a packed uint32 field, which consists of a length header and a list of
 * varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Array<number>}
 * @package
 */
function readPackedUint32(bufferDecoder, start) {
  return readPacked(bufferDecoder, start, readUint32);
}

/**
 * Read packed values.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @param {function(!BufferDecoder, number):T} valueFunction
 * @return {!Array<T>}
 * @package
 * @template T
 */
function readPacked(bufferDecoder, start, valueFunction) {
  const /** !Array<T> */ result = [];
  const unsignedLength = bufferDecoder.getUnsignedVarint32At(start);
  const dataStart = bufferDecoder.cursor();
  while (bufferDecoder.cursor() < dataStart + unsignedLength) {
    checkState(bufferDecoder.cursor() > 0);
    result.push(valueFunction(bufferDecoder, bufferDecoder.cursor()));
  }
  return result;
}

exports = {
  readBool,
  readBytes,
  readDelimited,
  readDouble,
  readFixed32,
  readFloat,
  readInt32,
  readInt64,
  readSint32,
  readSint64,
  readSfixed32,
  readSfixed64,
  readString,
  readUint32,
  readPackedBool,
  readPackedDouble,
  readPackedFixed32,
  readPackedFloat,
  readPackedInt32,
  readPackedInt64,
  readPackedSfixed32,
  readPackedSfixed64,
  readPackedSint32,
  readPackedSint64,
  readPackedUint32,
};
