/**
 * @fileoverview Test data for double encoding and decoding.
 */
goog.module('protobuf.binary.doubleTestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of double values and their bit representation.
 * This is used to test encoding and decoding from the protobuf wire format.
 * @return {!Array<{name: string, doubleValue:number, bufferDecoder:
 *     !BufferDecoder}>}
 */
function getDoublePairs() {
  const doublePairs = [
    {
      name: 'zero',
      doubleValue: 0,
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
    },
    {
      name: 'minus zero',
      doubleValue: -0,
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80)
    },
    {
      name: 'one',
      doubleValue: 1,
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F)
    },
    {
      name: 'minus one',
      doubleValue: -1,
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF)
    },

    {
      name: 'PI',
      doubleValue: Math.PI,
      bufferDecoder:
          createBufferDecoder(0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0x40)

    },
    {
      name: 'max value',
      doubleValue: Number.MAX_VALUE,
      bufferDecoder:
          createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F)
    },
    {
      name: 'min value',
      doubleValue: Number.MIN_VALUE,
      bufferDecoder:
          createBufferDecoder(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
    },
    {
      name: 'Infinity',
      doubleValue: Infinity,
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x7F)
    },
    {
      name: 'minus Infinity',
      doubleValue: -Infinity,
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF)
    },
    {
      name: 'Number.MAX_SAFE_INTEGER',
      doubleValue: Number.MAX_SAFE_INTEGER,
      bufferDecoder:
          createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x43)
    },
    {
      name: 'Number.MIN_SAFE_INTEGER',
      doubleValue: Number.MIN_SAFE_INTEGER,
      bufferDecoder:
          createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0xC3)
    },
  ];
  return [...doublePairs];
}

exports = {getDoublePairs};
