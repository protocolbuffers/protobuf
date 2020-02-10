/**
 * @fileoverview Helper methods for reading data from the binary wire format.
 */
goog.module('protobuf.binary.reader');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');


/******************************************************************************
 *                        OPTIONAL FUNCTIONS
 ******************************************************************************/

/**
 * Reads a boolean from the binary bytes.
 * Also returns the first position after the boolean.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} index Start of the data.
 * @return {{value: boolean, nextCursor: number}}
 */
function readBoolValue(bufferDecoder, index) {
  const {lowBits, highBits, dataStart} = bufferDecoder.getVarint(index);
  return {value: lowBits !== 0 || highBits !== 0, nextCursor: dataStart};
}

/**
 * Reads a boolean value from the binary bytes.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {boolean}
 * @package
 */
function readBool(bufferDecoder, start) {
  return readBoolValue(bufferDecoder, start).value;
}

/**
 * Reads a double value from the binary bytes.
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
 * @param {number} index Start of the data.
 * @return {{value: number, nextCursor: number}}
 * @package
 */
function readInt32Value(bufferDecoder, index) {
  const {lowBits, dataStart} = bufferDecoder.getVarint(index);
  // Negative 32 bit integers are encoded with 64 bit values.
  // Clients are expected to truncate back to 32 bits.
  // This is why we are dropping the upper bytes here.
  return {value: lowBits | 0, nextCursor: dataStart};
}

/**
 * Reads a int32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readInt32(bufferDecoder, start) {
  return readInt32Value(bufferDecoder, start).value;
}

/**
 * Reads a int32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} index Start of the data.
 * @return {{ value: !Int64, nextCursor: number}}
 * @package
 */
function readInt64Value(bufferDecoder, index) {
  const {lowBits, highBits, dataStart} = bufferDecoder.getVarint(index);
  return {value: Int64.fromBits(lowBits, highBits), nextCursor: dataStart};
}

/**
 * Reads a int32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Int64}
 * @package
 */
function readInt64(bufferDecoder, start) {
  return readInt64Value(bufferDecoder, start).value;
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
 * Also returns the first position after the boolean.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} index Start of the data.
 * @return {{value: number, nextCursor: number}}
 */
function readSint32Value(bufferDecoder, index) {
  const {lowBits, dataStart} = bufferDecoder.getVarint(index);
  // Truncate upper bits and convert from zig zag to signd int
  return {value: (lowBits >>> 1) ^ -(lowBits & 0x01), nextCursor: dataStart};
}

/**
 * Reads a sint32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readSint32(bufferDecoder, start) {
  return readSint32Value(bufferDecoder, start).value;
}

/**
 * Reads a sint64 value from the binary bytes encoded as varint.
 * Also returns the first position after the value.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} index Start of the data.
 * @return {{value: !Int64, nextCursor: number}}
 * @package
 */
function readSint64Value(bufferDecoder, index) {
  const {lowBits, highBits, dataStart} = bufferDecoder.getVarint(index);
  const sign = -(lowBits & 0x01);
  const decodedLowerBits = ((lowBits >>> 1) | (highBits & 0x01) << 31) ^ sign;
  const decodedUpperBits = (highBits >>> 1) ^ sign;
  return {
    value: Int64.fromBits(decodedLowerBits, decodedUpperBits),
    nextCursor: dataStart
  };
}

/**
 * Reads a sint64 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!Int64}
 * @package
 */
function readSint64(bufferDecoder, start) {
  return readSint64Value(bufferDecoder, start).value;
}

/**
 * Read a subarray of bytes representing a length delimited field.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {!BufferDecoder}
 * @package
 */
function readDelimited(bufferDecoder, start) {
  const {lowBits, dataStart} = bufferDecoder.getVarint(start);
  const unsignedLength = lowBits >>> 0;
  return bufferDecoder.subBufferDecoder(dataStart, unsignedLength);
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
 * Also returns the first position after the value.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} index Start of the data.
 * @return {{value: number, nextCursor: number}}
 */
function readUint32Value(bufferDecoder, index) {
  const {lowBits, dataStart} = bufferDecoder.getVarint(index);
  return {value: lowBits >>> 0, nextCursor: dataStart};
}

/**
 * Reads a uint32 value from the binary bytes encoded as varint.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @return {number}
 * @package
 */
function readUint32(bufferDecoder, start) {
  return readUint32Value(bufferDecoder, start).value;
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
  return readPackedVariableLength(bufferDecoder, start, readBoolValue);
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
  return readPackedFixed(bufferDecoder, start, 8, readDouble);
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
  return readPackedFixed(bufferDecoder, start, 4, readFixed32);
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
  return readPackedFixed(bufferDecoder, start, 4, readFloat);
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
  return readPackedVariableLength(bufferDecoder, start, readInt32Value);
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
  return readPackedVariableLength(bufferDecoder, start, readInt64Value);
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
  return readPackedFixed(bufferDecoder, start, 4, readSfixed32);
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
  return readPackedFixed(bufferDecoder, start, 8, readSfixed64);
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
  return readPackedVariableLength(bufferDecoder, start, readSint32Value);
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
  return readPackedVariableLength(bufferDecoder, start, readSint64Value);
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
  return readPackedVariableLength(bufferDecoder, start, readUint32Value);
}

/**
 * Read packed variable length values.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @param {function(!BufferDecoder, number):{value:T, nextCursor: number}}
 *     valueFunction
 * @return {!Array<T>}
 * @package
 * @template T
 */
function readPackedVariableLength(bufferDecoder, start, valueFunction) {
  const /** !Array<T> */ result = [];
  const {lowBits, dataStart} = bufferDecoder.getVarint(start);
  let cursor = dataStart;
  const unsignedLength = lowBits >>> 0;
  while (cursor < dataStart + unsignedLength) {
    const {value, nextCursor} = valueFunction(bufferDecoder, cursor);
    cursor = nextCursor;
    result.push(value);
  }
  return result;
}

/**
 * Read a packed fixed values.
 * @param {!BufferDecoder} bufferDecoder Binary format encoded bytes.
 * @param {number} start Start of the data.
 * @param {number} size End of the data.
 * @param {function(!BufferDecoder, number):T} valueFunction
 * @return {!Array<T>}
 * @package
 * @template T
 */
function readPackedFixed(bufferDecoder, start, size, valueFunction) {
  const {lowBits, dataStart} = bufferDecoder.getVarint(start);
  const unsignedLength = lowBits >>> 0;
  const noOfEntries = unsignedLength / size;
  const /** !Array<T> */ result = new Array(noOfEntries);
  let cursor = dataStart;
  for (let i = 0; i < noOfEntries; i++) {
    result[i] = valueFunction(bufferDecoder, cursor);
    cursor += size;
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
