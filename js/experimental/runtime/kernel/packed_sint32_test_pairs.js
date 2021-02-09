goog.module('protobuf.binary.packedSint32TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of packed sint32 values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, sint32Values: !Array<number>,
 *                  bufferDecoder: !BufferDecoder, skip_writer: ?boolean}>}
 */
function getPackedSint32Pairs() {
  return [
    {
      name: 'empty value',
      sint32Values: [],
      bufferDecoder: createBufferDecoder(0x00),
      skip_writer: true,
    },
    {
      name: 'single value',
      sint32Values: [-1],
      bufferDecoder: createBufferDecoder(0x01, 0x01),
    },
    {
      name: 'multiple values',
      sint32Values: [-1, 0],
      bufferDecoder: createBufferDecoder(0x02, 0x01, 0x00),
    },
  ];
}

exports = {getPackedSint32Pairs};
