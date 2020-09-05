/**
 * @fileoverview Helper methods for typed arrays.
 */
goog.module('protobuf.binary.typedArrays');

const {assert} = goog.require('goog.asserts');

/**
 * @param {!ArrayBuffer} buffer1
 * @param {!ArrayBuffer} buffer2
 * @return {boolean}
 */
function arrayBufferEqual(buffer1, buffer2) {
  if (!buffer1 || !buffer2) {
    throw new Error('Buffer shouldn\'t be empty');
  }

  const array1 = new Uint8Array(buffer1);
  const array2 = new Uint8Array(buffer2);

  return uint8ArrayEqual(array1, array2);
}

/**
 * @param {!Uint8Array} array1
 * @param {!Uint8Array} array2
 * @return {boolean}
 */
function uint8ArrayEqual(array1, array2) {
  if (array1 === array2) {
    return true;
  }

  if (array1.byteLength !== array2.byteLength) {
    return false;
  }

  for (let i = 0; i < array1.byteLength; i++) {
    if (array1[i] !== array2[i]) {
      return false;
    }
  }
  return true;
}

/**
 * ArrayBuffer.prototype.slice, but fallback to manual copy if missing.
 * @param {!ArrayBuffer} buffer
 * @param {number} start
 * @param {number=} end
 * @return {!ArrayBuffer} New array buffer with given the contents of `buffer`.
 */
function arrayBufferSlice(buffer, start, end = undefined) {
  // The fallback isn't fully compatible with ArrayBuffer.slice, enforce
  // strict requirements on start/end.
  // Spec:
  // https://www.ecma-international.org/ecma-262/6.0/#sec-arraybuffer.prototype.slice
  assert(start >= 0);
  assert(end === undefined || (end >= 0 && end <= buffer.byteLength));
  assert((end === undefined ? buffer.byteLength : end) >= start);
  if (buffer.slice) {
    const slicedBuffer = buffer.slice(start, end);
    // The ArrayBuffer.prototype.slice function was flawed before iOS 12.2. This
    // causes 0-length results when passing undefined or a number greater
    // than 32 bits for the optional end value. In these cases, we fall back to
    // using our own slice implementation.
    // More details: https://bugs.webkit.org/show_bug.cgi?id=185127
    if (slicedBuffer.byteLength !== 0 || (start === end)) {
      return slicedBuffer;
    }
  }
  const realEnd = end == null ? buffer.byteLength : end;
  const length = realEnd - start;
  assert(length >= 0);
  const view = new Uint8Array(buffer, start, length);
  // A TypedArray constructed from another Typed array copies the data.
  const clone = new Uint8Array(view);

  return clone.buffer;
}

/**
 * Returns a new Uint8Array with the size and contents of the given
 * ArrayBufferView. ArrayBufferView is an interface implemented by DataView,
 * Uint8Array and all typed arrays.
 * @param {!ArrayBufferView} view
 * @return {!Uint8Array}
 */
function cloneArrayBufferView(view) {
  return new Uint8Array(arrayBufferSlice(
      view.buffer, view.byteOffset, view.byteOffset + view.byteLength));
}

/**
 * Returns a 32 bit number for the corresponding Uint8Array.
 * @param {!Uint8Array} array
 * @return {number}
 */
function hashUint8Array(array) {
  const prime = 31;
  let result = 17;

  for (let i = 0; i < array.length; i++) {
    // '| 0' ensures signed 32 bits
    result = (result * prime + array[i]) | 0;
  }
  return result;
}

exports = {
  arrayBufferEqual,
  uint8ArrayEqual,
  arrayBufferSlice,
  cloneArrayBufferView,
  hashUint8Array,
};
