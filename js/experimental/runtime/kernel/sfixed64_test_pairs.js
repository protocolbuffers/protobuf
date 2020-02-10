/**
 * @fileoverview Test data for sfixed32 encoding and decoding.
 */
goog.module('protobuf.binary.sfixed64TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const Int64 = goog.require('protobuf.Int64');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of int values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, longValue: !Int64, bufferDecoder:
 *     !BufferDecoder}>}
 */
function getSfixed64Pairs() {
  const sfixed64Pairs = [
    {
      name: 'zero',
      longValue: Int64.fromInt(0),
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
    },
    {
      name: 'one',
      longValue: Int64.fromInt(1),
      bufferDecoder:
          createBufferDecoder(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
    },
    {
      name: 'minus one',
      longValue: Int64.fromInt(-1),
      bufferDecoder:
          createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF),
    },
    {
      name: 'max int 2^63 -1',
      longValue: Int64.getMaxValue(),
      bufferDecoder:
          createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F)
    },
    {
      name: 'min int -2^63',
      longValue: Int64.getMinValue(),
      bufferDecoder:
          createBufferDecoder(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80)
    },
  ];
  return [...sfixed64Pairs];
}

exports = {getSfixed64Pairs};
