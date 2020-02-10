/**
 * @fileoverview Tests for typed_arrays.js.
 */
goog.module('protobuf.binary.typedArraysTest');

const {arrayBufferEqual, arrayBufferSlice, cloneArrayBufferView, hashUint8Array, uint8ArrayEqual} = goog.require('protobuf.binary.typedArrays');

describe('arrayBufferEqual', () => {
  it('returns true for empty buffers', () => {
    const buffer1 = new ArrayBuffer(0);
    const buffer2 = new ArrayBuffer(0);
    expect(arrayBufferEqual(buffer1, buffer2)).toBe(true);
  });

  it('throws for first null buffers', () => {
    const buffer = new ArrayBuffer(0);
    expect(
        () => arrayBufferEqual(
            /** @type {!ArrayBuffer} */ (/** @type {*} */ (null)), buffer))
        .toThrow();
  });

  it('throws for second null buffers', () => {
    const buffer = new ArrayBuffer(0);
    expect(
        () => arrayBufferEqual(
            buffer, /** @type {!ArrayBuffer} */ (/** @type {*} */ (null))))
        .toThrow();
  });

  it('returns true for arrays with same values', () => {
    const array1 = new Uint8Array(4);
    array1[0] = 1;
    const array2 = new Uint8Array(4);
    array2[0] = 1;
    expect(arrayBufferEqual(array1.buffer, array2.buffer)).toBe(true);
  });

  it('returns false for arrays with different values', () => {
    const array1 = new Uint8Array(4);
    array1[0] = 1;
    const array2 = new Uint8Array(4);
    array2[0] = 2;
    expect(arrayBufferEqual(array1.buffer, array2.buffer)).toBe(false);
  });

  it('returns true same instance', () => {
    const array1 = new Uint8Array(4);
    array1[0] = 1;
    expect(arrayBufferEqual(array1.buffer, array1.buffer)).toBe(true);
  });
});

describe('uint8ArrayEqual', () => {
  it('returns true for empty arrays', () => {
    const array1 = new Uint8Array(0);
    const array2 = new Uint8Array(0);
    expect(uint8ArrayEqual(array1, array2)).toBe(true);
  });

  it('throws for first Uint8Array array', () => {
    const array = new Uint8Array(0);
    expect(
        () => uint8ArrayEqual(
            /** @type {!Uint8Array} */ (/** @type {*} */ (null)), array))
        .toThrow();
  });

  it('throws for second null array', () => {
    const array = new Uint8Array(0);
    expect(
        () => uint8ArrayEqual(
            array, /** @type {!Uint8Array} */ (/** @type {*} */ (null))))
        .toThrow();
  });

  it('returns true for arrays with same values', () => {
    const buffer1 = new Uint8Array([0, 1, 2, 3]).buffer;
    const buffer2 = new Uint8Array([1, 2, 3, 4]).buffer;
    const array1 = new Uint8Array(buffer1, 1, 3);
    const array2 = new Uint8Array(buffer2, 0, 3);
    expect(uint8ArrayEqual(array1, array2)).toBe(true);
  });

  it('returns false for arrays with different values', () => {
    const array1 = new Uint8Array(4);
    array1[0] = 1;
    const array2 = new Uint8Array(4);
    array2[0] = 2;
    expect(uint8ArrayEqual(array1, array2)).toBe(false);
  });

  it('returns true same instance', () => {
    const array1 = new Uint8Array(4);
    array1[0] = 1;
    expect(uint8ArrayEqual(array1, array1)).toBe(true);
  });
});

describe('arrayBufferSlice', () => {
  it('Returns a new instance.', () => {
    const buffer1 = new ArrayBuffer(0);
    const buffer2 = arrayBufferSlice(buffer1, 0);
    expect(buffer2).not.toBe(buffer1);
    expect(arrayBufferEqual(buffer1, buffer2)).toBe(true);
  });

  it('Copies data with positive start/end.', () => {
    const buffer1 = new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).buffer;
    expect(buffer1.byteLength).toEqual(10);

    const buffer2 = arrayBufferSlice(buffer1, 2, 6);
    expect(buffer2.byteLength).toEqual(4);
    expect(buffer2).not.toBe(buffer1);
    const expected = new Uint8Array([2, 3, 4, 5]).buffer;
    expect(arrayBufferEqual(expected, buffer2)).toBe(true);
  });

  it('Copies all data without end.', () => {
    const buffer1 = new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).buffer;
    expect(buffer1.byteLength).toEqual(10);

    const buffer2 = arrayBufferSlice(buffer1, 0);
    expect(buffer2.byteLength).toEqual(10);
    expect(arrayBufferEqual(buffer1, buffer2)).toBe(true);
  });

  if (goog.DEBUG) {
    it('Fails with negative end.', () => {
      const buffer1 = new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).buffer;
      expect(() => void arrayBufferSlice(buffer1, 2, -1)).toThrow();
    });

    it('Fails with negative start.', () => {
      const buffer1 = new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).buffer;
      expect(() => void arrayBufferSlice(buffer1, 2, -1)).toThrow();
    });

    it('Fails when start > end.', () => {
      const buffer1 = new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).buffer;
      expect(() => void arrayBufferSlice(buffer1, 2, 1)).toThrow();
    });
  }
});

describe('cloneArrayBufferView', () => {
  it('Returns a new instance.', () => {
    const array1 = new Uint8Array(0);
    const array2 = cloneArrayBufferView(new Uint8Array(array1));
    expect(array2).not.toBe(array1);
    expect(uint8ArrayEqual(array1, array2)).toBe(true);
  });

  it('Returns an array of the exact size.', () => {
    const array1 =
        new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).subarray(2, 5);
    expect(array1.length).toEqual(3);
    expect(array1.buffer.byteLength).toEqual(10);
    const array2 = cloneArrayBufferView(array1);
    expect(array2.byteLength).toEqual(3);
    expect(array2).toEqual(new Uint8Array([2, 3, 4]));
  });
});

describe('hashUint8Array', () => {
  it('returns same hashcode for empty Uint8Arrays', () => {
    const array1 = new Uint8Array(0);
    const array2 = new Uint8Array(0);
    expect(hashUint8Array(array1)).toBe(hashUint8Array(array2));
  });

  it('returns same hashcode for Uint8Arrays with same values', () => {
    const array1 = new Uint8Array(4);
    array1[0] = 1;
    const array2 = new Uint8Array(4);
    array2[0] = 1;
    expect(hashUint8Array(array1)).toBe(hashUint8Array(array2));
  });

  it('returns different hashcode for Uint8Arrays with different values', () => {
    // This test might fail in the future if the hashing algorithm is updated
    // and we end up with a collision here.
    // We still need this test to make sure that we are not just returning
    // the same number for all buffers.
    const array1 = new Uint8Array(4);
    array1[0] = 1;
    const array2 = new Uint8Array(4);
    array2[0] = 2;
    expect(hashUint8Array(array1)).not.toBe(hashUint8Array(array2));
  });
});
