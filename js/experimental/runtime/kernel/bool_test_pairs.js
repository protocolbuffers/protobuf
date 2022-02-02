/**
 * @fileoverview Test data for bool encoding and decoding.
 */
goog.module('protobuf.binary.boolTestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of boolean values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, boolValue: boolean, bufferDecoder:
 *     !BufferDecoder, error: ?boolean, skip_writer: ?boolean}>}
 */
function getBoolPairs() {
  const boolPairs = [
    {
      name: 'true',
      boolValue: true,
      bufferDecoder: createBufferDecoder(0x01),
    },
    {
      name: 'false',
      boolValue: false,
      bufferDecoder: createBufferDecoder(0x00),
    },
    {
      name: 'two-byte true',
      boolValue: true,
      bufferDecoder: createBufferDecoder(0x80, 0x01),
      skip_writer: true,
    },
    {
      name: 'two-byte false',
      boolValue: false,
      bufferDecoder: createBufferDecoder(0x80, 0x00),
      skip_writer: true,
    },
    {
      name: 'minus one',
      boolValue: true,
      bufferDecoder: createBufferDecoder(
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01),
      skip_writer: true,
    },
    {
      name: 'max signed int 2^63 - 1',
      boolValue: true,
      bufferDecoder: createBufferDecoder(
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F),
      skip_writer: true,
    },
    {
      name: 'min signed int -2^63',
      boolValue: true,
      bufferDecoder: createBufferDecoder(
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01),
      skip_writer: true,
    },
    {
      name: 'overflowed but valid varint',
      boolValue: false,
      bufferDecoder: createBufferDecoder(
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02),
      skip_writer: true,
    },
    {
      name: 'errors out for 11 bytes',
      boolValue: true,
      bufferDecoder: createBufferDecoder(
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF),
      error: true,
      skip_writer: true,
    },
  ];
  return [...boolPairs];
}

exports = {getBoolPairs};
