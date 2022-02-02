goog.module('protobuf.binary.packedBoolTestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of packed bool values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, boolValues: !Array<boolean>,
 *                  bufferDecoder: !BufferDecoder, skip_writer: ?boolean}>}
 */
function getPackedBoolPairs() {
  return [
    {
      name: 'empty value',
      boolValues: [],
      bufferDecoder: createBufferDecoder(0x00),
      skip_writer: true,
    },
    {
      name: 'single value',
      boolValues: [true],
      bufferDecoder: createBufferDecoder(0x01, 0x01),
    },
    {
      name: 'single multi-bytes value',
      boolValues: [true],
      bufferDecoder: createBufferDecoder(0x02, 0x80, 0x01),
      skip_writer: true,
    },
    {
      name: 'multiple values',
      boolValues: [true, false],
      bufferDecoder: createBufferDecoder(0x02, 0x01, 0x00),
    },
    {
      name: 'multiple multi-bytes values',
      boolValues: [true, false],
      bufferDecoder: createBufferDecoder(
          0x0C,  // length
          0x80,
          0x80,
          0x80,
          0x80,
          0x80,
          0x01,  // true
          0x80,
          0x80,
          0x80,
          0x80,
          0x80,
          0x00,  // false
          ),
      skip_writer: true,
    },
  ];
}

exports = {getPackedBoolPairs};
