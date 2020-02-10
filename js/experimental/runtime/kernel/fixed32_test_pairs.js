/**
 * @fileoverview Test data for float encoding and decoding.
 */
goog.module('protobuf.binary.fixed32TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of float values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, intValue: number, bufferDecoder:
 *     !BufferDecoder}>}
 */
function getFixed32Pairs() {
  const fixed32Pairs = [
    {
      name: 'zero',
      intValue: 0,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x00, 0x00),
    },
    {
      name: 'one ',
      intValue: 1,
      bufferDecoder: createBufferDecoder(0x01, 0x00, 0x00, 0x00)
    },
    {
      name: 'max int 2^32 -1',
      intValue: Math.pow(2, 32) - 1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF)
    },
  ];
  return [...fixed32Pairs];
}

exports = {getFixed32Pairs};
