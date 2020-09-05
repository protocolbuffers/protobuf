goog.module('protobuf.binary.packedSfixed32TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of packed sfixed32 values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, sfixed32Values: !Array<number>,
 *                  bufferDecoder: !BufferDecoder, skip_writer: ?boolean}>}
 */
function getPackedSfixed32Pairs() {
  return [
    {
      name: 'empty value',
      sfixed32Values: [],
      bufferDecoder: createBufferDecoder(0x00),
      skip_writer: true,
    },
    {
      name: 'single value',
      sfixed32Values: [1],
      bufferDecoder: createBufferDecoder(0x04, 0x01, 0x00, 0x00, 0x00),
    },
    {
      name: 'multiple values',
      sfixed32Values: [1, 0],
      bufferDecoder: createBufferDecoder(
          0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
    },
  ];
}

exports = {getPackedSfixed32Pairs};
