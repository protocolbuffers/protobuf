/**
 * @fileoverview Test data for float encoding and decoding.
 */
goog.module('protobuf.binary.floatTestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of float values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, floatValue:number, bufferDecoder:
 *     !BufferDecoder}>}
 */
function getFloatPairs() {
  const floatPairs = [
    {
      name: 'zero',
      floatValue: 0,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x00, 0x00),
    },
    {
      name: 'minus zero',
      floatValue: -0,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x00, 0x80)
    },
    {
      name: 'one ',
      floatValue: 1,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x80, 0x3F)
    },
    {
      name: 'minus one',
      floatValue: -1,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x80, 0xBF)
    },
    {
      name: 'two',
      floatValue: 2,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x00, 0x40)
    },
    {
      name: 'max float32',
      floatValue: Math.pow(2, 127) * (2 - 1 / Math.pow(2, 23)),
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0x7F, 0x7F)
    },

    {
      name: 'min float32',
      floatValue: 1 / Math.pow(2, 127 - 1),
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x80, 0x00)
    },

    {
      name: 'Infinity',
      floatValue: Infinity,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x80, 0x7F)
    },
    {
      name: 'minus Infinity',
      floatValue: -Infinity,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0x80, 0xFF)
    },
    {
      name: '1.5',
      floatValue: 1.5,
      bufferDecoder: createBufferDecoder(0x00, 0x00, 0xC0, 0x3F)
    },
    {
      name: '1.6',
      floatValue: 1.6,
      bufferDecoder: createBufferDecoder(0xCD, 0xCC, 0xCC, 0x3F)
    },
  ];
  return [...floatPairs];
}

exports = {getFloatPairs};
