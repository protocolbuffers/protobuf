/**
 * @fileoverview Tests for uint8arrays.js.
 */
goog.module('protobuf.binary.Uint8ArraysTest');

goog.setTestOnly();

const {concatenateByteArrays} = goog.require('protobuf.binary.uint8arrays');

describe('concatenateByteArrays does', () => {
  it('concatenate empty array', () => {
    const byteArrays = [];
    expect(concatenateByteArrays(byteArrays)).toEqual(new Uint8Array(0));
  });

  it('concatenate Uint8Arrays', () => {
    const byteArrays = [new Uint8Array([0x01]), new Uint8Array([0x02])];
    expect(concatenateByteArrays(byteArrays)).toEqual(new Uint8Array([
      0x01, 0x02
    ]));
  });

  it('concatenate array of bytes', () => {
    const byteArrays = [[0x01], [0x02]];
    expect(concatenateByteArrays(byteArrays)).toEqual(new Uint8Array([
      0x01, 0x02
    ]));
  });

  it('concatenate array of non-bytes', () => {
    // Note in unchecked mode we produce invalid output for invalid inputs.
    // This test just documents our behavior in those cases.
    // These values might change at any point and are not considered
    // what the implementation should be doing here.
    const byteArrays = [[40.0], [256]];
    expect(concatenateByteArrays(byteArrays)).toEqual(new Uint8Array([
      0x28, 0x00
    ]));
  });

  it('throw for null array', () => {
    expect(
        () => concatenateByteArrays(
            /** @type {!Array<!Uint8Array>} */ (/** @type {*} */ (null))))
        .toThrow();
  });
});
