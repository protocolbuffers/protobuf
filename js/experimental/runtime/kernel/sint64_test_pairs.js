/**
 * @fileoverview Test data for sint64 encoding and decoding.
 */
goog.module('protobuf.binary.sint64TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const Int64 = goog.require('protobuf.Int64');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of float values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, longValue: !Int64, bufferDecoder:
 *     !BufferDecoder, error: ?boolean, skip_writer: ?boolean}>}
 */
function getSint64Pairs() {
  const sint64Pairs = [
    {
      name: 'zero',
      longValue: Int64.fromInt(0),
      bufferDecoder: createBufferDecoder(0x00),
    },
    {
      name: 'one ',
      longValue: Int64.fromInt(1),
      bufferDecoder: createBufferDecoder(0x02),
    },
    {
      name: 'minus one',
      longValue: Int64.fromInt(-1),
      bufferDecoder: createBufferDecoder(0x01),
    },
    {
      name: 'two',
      longValue: Int64.fromInt(2),
      bufferDecoder: createBufferDecoder(0x04),
    },
    {
      name: 'minus two',
      longValue: Int64.fromInt(-2),
      bufferDecoder: createBufferDecoder(0x03),
    },
    {
      name: 'min value',
      longValue: Int64.getMinValue(),
      bufferDecoder: createBufferDecoder(
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01),
    },

    {
      name: 'max value',
      longValue: Int64.getMaxValue(),
      bufferDecoder: createBufferDecoder(
          0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01),
    },
  ];
  return [...sint64Pairs];
}

exports = {getSint64Pairs};
