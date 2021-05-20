/**
 * @fileoverview Provides ByteString as a basic data type for protos.
 */
goog.module('protobuf.ByteString');

const base64 = goog.require('goog.crypt.base64');
const {arrayBufferSlice, cloneArrayBufferView, hashUint8Array, uint8ArrayEqual} = goog.require('protobuf.binary.typedArrays');

/**
 * Immutable sequence of bytes.
 *
 * Bytes can be obtained as an ArrayBuffer or a base64 encoded string.
 * @final
 */
class ByteString {
  /**
   * @param {?Uint8Array} bytes
   * @param {?string} base64
   * @private
   */
  constructor(bytes, base64) {
    /** @private {?Uint8Array}*/
    this.bytes_ = bytes;
    /** @private {?string} */
    this.base64_ = base64;
    /** @private {number} */
    this.hashCode_ = 0;
  }

  /**
   * Constructs a ByteString instance from a base64 string.
   * @param {string} value
   * @return {!ByteString}
   */
  static fromBase64String(value) {
    if (value == null) {
      throw new Error('value must not be null');
    }
    return new ByteString(/* bytes */ null, value);
  }

  /**
   * Constructs a ByteString from an array buffer.
   * @param {!ArrayBuffer} bytes
   * @param {number=} start
   * @param {number=} end
   * @return {!ByteString}
   */
  static fromArrayBuffer(bytes, start = 0, end = undefined) {
    return new ByteString(
        new Uint8Array(arrayBufferSlice(bytes, start, end)), /* base64 */ null);
  }

  /**
   * Constructs a ByteString from any ArrayBufferView (e.g. DataView,
   * TypedArray, Uint8Array, etc.).
   * @param {!ArrayBufferView} bytes
   * @return {!ByteString}
   */
  static fromArrayBufferView(bytes) {
    return new ByteString(cloneArrayBufferView(bytes), /* base64 */ null);
  }

  /**
   * Constructs a ByteString from an Uint8Array. DON'T MODIFY the underlying
   * ArrayBuffer, since the ByteString directly uses it without making a copy.
   *
   * This method exists so that internal APIs can construct a ByteString without
   * paying the penalty of copying an ArrayBuffer when that ArrayBuffer is not
   * supposed to change. It is exposed to a limited number of internal classes
   * through bytestring_internal.js.
   *
   * @param {!Uint8Array} bytes
   * @return {!ByteString}
   * @package
   */
  static fromUint8ArrayUnsafe(bytes) {
    return new ByteString(bytes, /* base64 */ null);
  }

  /**
   * Returns this ByteString as an ArrayBuffer.
   * @return {!ArrayBuffer}
   */
  toArrayBuffer() {
    const bytes = this.ensureBytes_();
    return arrayBufferSlice(
        bytes.buffer, bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
  }

  /**
   * Returns this ByteString as an Uint8Array. DON'T MODIFY the returned array,
   * since the ByteString holds the reference to the same array.
   *
   * This method exists so that internal APIs can get contents of a ByteString
   * without paying the penalty of copying an ArrayBuffer. It is exposed to a
   * limited number of internal classes through bytestring_internal.js.
   * @return {!Uint8Array}
   * @package
   */
  toUint8ArrayUnsafe() {
    return this.ensureBytes_();
  }

  /**
   * Returns this ByteString as a base64 encoded string.
   * @return {string}
   */
  toBase64String() {
    return this.ensureBase64String_();
  }

  /**
   * Returns true for Bytestrings that contain identical values.
   * @param {*} other
   * @return {boolean}
   */
  equals(other) {
    if (this === other) {
      return true;
    }

    if (!(other instanceof ByteString)) {
      return false;
    }

    const otherByteString = /** @type {!ByteString} */ (other);
    return uint8ArrayEqual(this.ensureBytes_(), otherByteString.ensureBytes_());
  }

  /**
   * Returns a number (int32) that is suitable for using in hashed structures.
   * @return {number}
   */
  hashCode() {
    if (this.hashCode_ == 0) {
      this.hashCode_ = hashUint8Array(this.ensureBytes_());
    }
    return this.hashCode_;
  }

  /**
   * Returns true if the bytestring is empty.
   * @return {boolean}
   */
  isEmpty() {
    if (this.bytes_ != null && this.bytes_.byteLength == 0) {
      return true;
    }
    if (this.base64_ != null && this.base64_.length == 0) {
      return true;
    }
    return false;
  }

  /**
   * @return {!Uint8Array}
   * @private
   */
  ensureBytes_() {
    if (this.bytes_) {
      return this.bytes_;
    }
    return this.bytes_ = base64.decodeStringToUint8Array(
               /** @type {string} */ (this.base64_));
  }

  /**
   * @return {string}
   * @private
   */
  ensureBase64String_() {
    if (this.base64_ == null) {
      this.base64_ = base64.encodeByteArray(this.bytes_);
    }
    return this.base64_;
  }
}

/** @const {!ByteString} */
ByteString.EMPTY = new ByteString(new Uint8Array(0), null);

exports = ByteString;
