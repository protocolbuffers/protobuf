/**
 * @fileoverview Helper methods to create BufferDecoders.
 */
goog.module('protobuf.binary.bufferDecoderHelper');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');

/**
 * @param {...number} bytes
 * @return {!BufferDecoder}
 */
function createBufferDecoder(...bytes) {
  return BufferDecoder.fromArrayBuffer(new Uint8Array(bytes).buffer);
}

exports = {
  createBufferDecoder,
};
