/**
 * @fileoverview Test data for int32 encoding and decoding.
 */
goog.module('protobuf.binary.uint32TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of float values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, intValue:number, bufferDecoder:
 *     !BufferDecoder, error: (boolean|undefined),
 *    skip_reader: (boolean|undefined), skip_writer: (boolean|undefined)}>}
 */
function getUint32Pairs() {
  const uint32Pairs = [
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
      name: 'max signed int 2^31 - 1',
      intValue: Math.pow(2, 31) - 1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0x07),
    },
    {
      name: 'max unsigned int 2^32 - 1',
      intValue: Math.pow(2, 32) - 1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0x0F),
    },
    {
      name: 'negative one',
      intValue: -1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0x0F),
      skip_reader: true,
    },
    {
      name: 'truncates more than 32 bits',
      intValue: Math.pow(2, 32) - 1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01),
      skip_writer: true,
    },
    {
      name: 'truncates more than 32 bits (bit 33 set)',
      intValue: Math.pow(2, 28) - 1,
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0x10),
      skip_writer: true,
    },
    {
      name: 'errors out for 11 bytes',
      intValue: Math.pow(2, 32) - 1,
      bufferDecoder: createBufferDecoder(
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF),
      error: true,
      skip_writer: true,
    },
  ];
  return [...uint32Pairs];
}

exports = {getUint32Pairs};
