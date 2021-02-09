/**
 * @fileoverview Installs our custom equality matchers in Jasmine.
 */
goog.module('protobuf.testing.jasmineProtoBuf');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');
const {arrayBufferEqual} = goog.require('protobuf.binary.typedArrays');

/**
 * A function that ensures custom equality for ByteStrings.
 * Since Jasmine compare structure by default Bytestrings might be equal that
 * are not equal since ArrayBuffers still compare content in g3.
 * (Jasmine fix upstream: https://github.com/jasmine/jasmine/issues/1687)
 * Also ByteStrings that are equal might compare non equal in jasmine of the
 * base64 string has been initialized.
 * @param {*} first
 * @param {*} second
 * @return {boolean|undefined}
 */
const byteStringEquality = (first, second) => {
  if (second instanceof ByteString) {
    return second.equals(first);
  }

  // Intentionally not returning anything, this signals to jasmine that we
  // did not perform any equality on the given objects.
};

/**
 * A function that ensures custom equality for ArrayBuffers.
 * By default Jasmine does not compare the content of an ArrayBuffer and thus
 * will return true for buffers with the same length but different content.
 * @param {*} first
 * @param {*} second
 * @return {boolean|undefined}
 */
const arrayBufferCustomEquality = (first, second) => {
  if (first instanceof ArrayBuffer && second instanceof ArrayBuffer) {
    return arrayBufferEqual(first, second);
  }
  // Intentionally not returning anything, this signals to jasmine that we
  // did not perform any equality on the given objects.
};

/**
 * A function that ensures custom equality for ArrayBuffers.
 * By default Jasmine does not compare the content of an ArrayBuffer and thus
 * will return true for buffers with the same length but different content.
 * @param {*} first
 * @param {*} second
 * @return {boolean|undefined}
 */
const bufferDecoderCustomEquality = (first, second) => {
  if (first instanceof BufferDecoder && second instanceof BufferDecoder) {
    return first.asByteString().equals(second.asByteString());
  }
  // Intentionally not returning anything, this signals to jasmine that we
  // did not perform any equality on the given objects.
};

/**
 * Overrides the default ArrayBuffer toString method ([object ArrayBuffer]) with
 * a more readable representation.
 */
function overrideArrayBufferToString() {
  /**
   * Returns the hex values of the underlying bytes of the ArrayBuffer.
   *
   * @override
   * @return {string}
   */
  ArrayBuffer.prototype.toString = function() {
    const arr = Array.from(new Uint8Array(this));
    return 'ArrayBuffer[' +
        arr.map((b) => '0x' + (b & 0xFF).toString(16).toUpperCase())
            .join(', ') +
        ']';
  };
}

beforeEach(() => {
  jasmine.addCustomEqualityTester(arrayBufferCustomEquality);
  jasmine.addCustomEqualityTester(bufferDecoderCustomEquality);
  jasmine.addCustomEqualityTester(byteStringEquality);

  overrideArrayBufferToString();
});
