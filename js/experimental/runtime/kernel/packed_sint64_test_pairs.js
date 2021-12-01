goog.module('protobuf.binary.packedSint64TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const Int64 = goog.require('protobuf.Int64');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of packed sint64 values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, sint64Values: !Array<number>,
 *                  bufferDecoder: !BufferDecoder, skip_writer: ?boolean}>}
 */
function getPackedSint64Pairs() {
  return [
    {
      name: 'empty value',
      sint64Values: [],
      bufferDecoder: createBufferDecoder(0x00),
      skip_writer: true,
    },
    {
      name: 'single value',
      sint64Values: [Int64.fromInt(-1)],
      bufferDecoder: createBufferDecoder(0x01, 0x01),
    },
    {
      name: 'multiple values',
      sint64Values: [Int64.fromInt(-1), Int64.fromInt(0)],
      bufferDecoder: createBufferDecoder(0x02, 0x01, 0x00),
    },
  ];
}

exports = {getPackedSint64Pairs};
