/**
 * @fileoverview Test data for int32 encoding and decoding.
 */
goog.module('protobuf.binary.sint32TestPairs');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * An array of Pairs of float values and their bit representation.
 * This is used to test encoding and decoding from/to the protobuf wire format.
 * @return {!Array<{name: string, intValue:number, bufferDecoder:
 *     !BufferDecoder, error: ?boolean, skip_writer: ?boolean}>}
 */
function getSint32Pairs() {
  const sint32Pairs = [
    {
      name: 'zero',
      intValue: 0,
      bufferDecoder: createBufferDecoder(0x00),
    },
    {
      name: 'one ',
      intValue: 1,
      bufferDecoder: createBufferDecoder(0x02),
    },
    {
      name: 'minus one',
      intValue: -1,
      bufferDecoder: createBufferDecoder(0x01),
    },
    {
      name: 'two',
      intValue: 2,
      bufferDecoder: createBufferDecoder(0x04),
    },
    {
      name: 'minus two',
      intValue: -2,
      bufferDecoder: createBufferDecoder(0x03),
    },
    {
      name: 'int 2^31 - 1',
      intValue: Math.pow(2, 31) - 1,
      bufferDecoder: createBufferDecoder(0xFE, 0xFF, 0xFF, 0xFF, 0x0F),

    },
    {
      name: '-2^31',
      intValue: -Math.pow(2, 31),
      bufferDecoder: createBufferDecoder(0xFF, 0xFF, 0xFF, 0xFF, 0x0F),
    },
  ];
  return [...sint32Pairs];
}

exports = {getSint32Pairs};
