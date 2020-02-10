goog.module('protobuf.binary.packedFloatTestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of packed float values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, floatValues: !Array<number>,
 *                  bufferDecoder: !BufferDecoder, skip_writer: ?boolean}>}
 */
function getPackedFloatPairs() {
  return [
    {
      name: 'empty value',
      floatValues: [],
      bufferDecoder: createBufferDecoder(0x00),
      skip_writer: true,
    },
    {
      name: 'single value',
      floatValues: [1],
      bufferDecoder: createBufferDecoder(0x04, 0x00, 0x00, 0x80, 0x3F),
    },
    {
      name: 'multiple values',
      floatValues: [1, 0],
      bufferDecoder: createBufferDecoder(
          0x08, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00),
    },
  ];
}

exports = {getPackedFloatPairs};
