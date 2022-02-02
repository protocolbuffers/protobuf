/**
 * @fileoverview Exposes internal only functions for ByteString. The
 * corresponding BUILD rule restricts access to this file to only the binary
 * kernel and APIs directly using the binary kernel.
 */
goog.module('protobuf.byteStringInternal');

const ByteString = goog.require('protobuf.ByteString');

/**
 * Constructs a ByteString from an Uint8Array. DON'T MODIFY the underlying
 * ArrayBuffer, since the ByteString directly uses it without making a copy.
 * @param {!Uint8Array} bytes
 * @return {!ByteString}
 */
function byteStringFromUint8ArrayUnsafe(bytes) {
  return ByteString.fromUint8ArrayUnsafe(bytes);
}

/**
 * Returns this ByteString as an Uint8Array. DON'T MODIFY the returned array,
 * since the ByteString holds the reference to the same array.
 * @param {!ByteString} bytes
 * @return {!Uint8Array}
 */
function byteStringToUint8ArrayUnsafe(bytes) {
  return bytes.toUint8ArrayUnsafe();
}

exports = {
  byteStringFromUint8ArrayUnsafe,
  byteStringToUint8ArrayUnsafe,
};
