goog.module('protobuf.binary.packedDoubleTestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of packed double values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, doubleValues: !Array<number>,
 *                  bufferDecoder: !BufferDecoder, skip_writer: ?boolean}>}
 */
function getPackedDoublePairs() {
  return [
    {
      name: 'empty value',
      doubleValues: [],
      bufferDecoder: createBufferDecoder(0x00),
      skip_writer: true,
    },
    {
      name: 'single value',
      doubleValues: [1],
      bufferDecoder: createBufferDecoder(
          0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F),
    },
    {
      name: 'multiple values',
      doubleValues: [1, 0],
      bufferDecoder: createBufferDecoder(
          0x10,  // length
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0xF0,
          0x3F,  // 1
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,  // 0
          ),
    },
  ];
}

exports = {getPackedDoublePairs};
