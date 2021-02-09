/**
 * @fileoverview Helper methods for Uint8Arrays.
 */
goog.module('protobuf.binary.uint8arrays');

/**
 * Combines multiple bytes arrays (either Uint8Array or number array whose
 * values are bytes) into a single Uint8Array.
 * @param {!Array<!Uint8Array>|!Array<!Array<number>>} arrays
 * @return {!Uint8Array}
 */
function concatenateByteArrays(arrays) {
  let totalLength = 0;
  for (const array of arrays) {
    totalLength += array.length;
  }
  const result = new Uint8Array(totalLength);
  let offset = 0;
  for (const array of arrays) {
    result.set(array, offset);
    offset += array.length;
  }
  return result;
}

exports = {
  concatenateByteArrays,
};
