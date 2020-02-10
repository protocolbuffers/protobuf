goog.module('protobuf.binary.packedSfixed64TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const Int64 = goog.require('protobuf.Int64');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of packed sfixed64 values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, sfixed64Values: !Array<!Int64>,
 *                  bufferDecoder: !BufferDecoder, skip_writer: ?boolean}>}
 */
function getPackedSfixed64Pairs() {
  return [
    {
      name: 'empty value',
      sfixed64Values: [],
      bufferDecoder: createBufferDecoder(0x00),
      skip_writer: true,
    },
    {
      name: 'single value',
      sfixed64Values: [Int64.fromInt(1)],
      bufferDecoder: createBufferDecoder(
          0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
    },
    {
      name: 'multiple values',
      sfixed64Values: [Int64.fromInt(1), Int64.fromInt(0)],
      bufferDecoder: createBufferDecoder(
          0x10,  // length
          0x01,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,  // 1
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,  // 2
          ),
    },
  ];
}

exports = {getPackedSfixed64Pairs};
