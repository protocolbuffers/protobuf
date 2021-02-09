/**
 * @fileoverview Test data for int32 encoding and decoding.
 */
goog.module('protobuf.binary.int32TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of float values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, intValue:number, bufferDecoder:
 *     !BufferDecoder, error: ?boolean, skip_writer: ?boolean}>}
 */
function getInt32Pairs() {
  const int32Pairs = [
    {
      name: 'zero',
      intValue: 0,
      bufferDecoder: createBufferDecoder(0x00),
    },
    {
      name: 'one ',
      intValue: 1,
      bufferDecoder: createBufferDecoder(0x01),
    },
    {
      name: 'minus one',
      intValue: -1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0x0F),
      // The writer will encode this with 64 bits, see below
      skip_writer: true,
    },
    {
      name: 'minus one (64bits)',
      intValue: -1,
      bufferDecoder: createBufferDecoder(
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01),
    },
    {
      name: 'max signed int 2^31 - 1',
      intValue: Math.pow(2, 31) - 1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0x07),

    },
    {
      name: 'min signed int -2^31',
      intValue: -Math.pow(2, 31),
      bufferDecoder: createBufferDecoder(0x80, 0x80, 0x80, 0x80, 0x08),
      // The writer will encode this with 64 bits, see below
      skip_writer: true,
    },
    {
      name: 'value min signed int -2^31 (64 bit)',
      intValue: -Math.pow(2, 31),
      bufferDecoder: createBufferDecoder(
          0x80, 0x80, 0x80, 0x80, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0x01),
    },
    {
      name: 'errors out for 11 bytes',
      intValue: -1,
      bufferDecoder: createBufferDecoder(
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF),
      error: true,
      skip_writer: true,
    },
  ];
  return [...int32Pairs];
}

exports = {getInt32Pairs};
