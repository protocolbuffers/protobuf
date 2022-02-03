goog.module('proto.im.integration.ByteStringFieldsTest');
goog.setTestOnly();

const ByteString = goog.require('protobuf.ByteString');
const {arrayBufferSlice} = goog.require('protobuf.binary.typedArrays');

const /** !ArrayBuffer */ TEST_BYTES = new Uint8Array([1, 2, 3, 4]).buffer;
const /** !ByteString */ TEST_STRING = ByteString.fromArrayBuffer(TEST_BYTES);
const /** !ArrayBuffer */ PREFIXED_TEST_BYTES =
    new Uint8Array([0, 1, 2, 3, 4]).buffer;
const /** string */ HALLO_IN_BASE64 = 'aGFsbG8=';
const /** string */ HALLO_IN_BASE64_WITH_SPACES = 'a G F s b G 8=';
const /** !ArrayBufferView */ BYTES_WITH_HALLO = new Uint8Array([
  'h'.charCodeAt(0),
  'a'.charCodeAt(0),
  'l'.charCodeAt(0),
  'l'.charCodeAt(0),
  'o'.charCodeAt(0),
]);

describe('ByteString does', () => {
  it('create bytestring from buffer', () => {
    const byteString =
        ByteString.fromArrayBuffer(arrayBufferSlice(TEST_BYTES, 0));
    expect(byteString.toArrayBuffer()).toEqual(TEST_BYTES);
    expect(byteString.toUint8ArrayUnsafe()).toEqual(new Uint8Array(TEST_BYTES));
  });

  it('create bytestring from ArrayBufferView', () => {
    const byteString =
        ByteString.fromArrayBufferView(new Uint8Array(TEST_BYTES));
    expect(byteString.toArrayBuffer()).toEqual(TEST_BYTES);
    expect(byteString.toUint8ArrayUnsafe()).toEqual(new Uint8Array(TEST_BYTES));
  });

  it('create bytestring from subarray', () => {
    const byteString = ByteString.fromArrayBufferView(
        new Uint8Array(TEST_BYTES, /* offset */ 1, /* length */ 2));
    const expected = new Uint8Array([2, 3]);
    expect(new Uint8Array(byteString.toArrayBuffer())).toEqual(expected);
    expect(byteString.toUint8ArrayUnsafe()).toEqual(expected);
  });

  it('create bytestring from Uint8Array (unsafe)', () => {
    const array = new Uint8Array(TEST_BYTES);
    const byteString = ByteString.fromUint8ArrayUnsafe(array);
    expect(byteString.toArrayBuffer()).toEqual(array.buffer);
    expect(byteString.toUint8ArrayUnsafe()).toBe(array);
  });

  it('create bytestring from base64 string', () => {
    const byteString = ByteString.fromBase64String(HALLO_IN_BASE64);
    expect(byteString.toBase64String()).toEqual(HALLO_IN_BASE64);
    expect(byteString.toArrayBuffer()).toEqual(BYTES_WITH_HALLO.buffer);
    expect(byteString.toUint8ArrayUnsafe()).toEqual(BYTES_WITH_HALLO);
  });

  it('preserve immutability if underlying buffer changes: from buffer', () => {
    const buffer = new ArrayBuffer(4);
    const array = new Uint8Array(buffer);
    array[0] = 1;
    array[1] = 2;
    array[2] = 3;
    array[3] = 4;

    const byteString = ByteString.fromArrayBuffer(buffer);
    const otherBuffer = byteString.toArrayBuffer();

    expect(otherBuffer).not.toBe(buffer);
    expect(new Uint8Array(otherBuffer)).toEqual(array);

    // modify the original buffer
    array[0] = 5;
    // Are we still returning the original bytes?
    expect(new Uint8Array(byteString.toArrayBuffer())).toEqual(new Uint8Array([
      1, 2, 3, 4
    ]));
  });

  it('preserve immutability if underlying buffer changes: from ArrayBufferView',
     () => {
       const buffer = new ArrayBuffer(4);
       const array = new Uint8Array(buffer);
       array[0] = 1;
       array[1] = 2;
       array[2] = 3;
       array[3] = 4;

       const byteString = ByteString.fromArrayBufferView(array);
       const otherBuffer = byteString.toArrayBuffer();

       expect(otherBuffer).not.toBe(buffer);
       expect(new Uint8Array(otherBuffer)).toEqual(array);

       // modify the original buffer
       array[0] = 5;
       // Are we still returning the original bytes?
       expect(new Uint8Array(byteString.toArrayBuffer()))
           .toEqual(new Uint8Array([1, 2, 3, 4]));
     });

  it('mutate if underlying buffer changes: from Uint8Array (unsafe)', () => {
    const buffer = new ArrayBuffer(4);
    const array = new Uint8Array(buffer);
    array[0] = 1;
    array[1] = 2;
    array[2] = 3;
    array[3] = 4;

    const byteString = ByteString.fromUint8ArrayUnsafe(array);
    const otherBuffer = byteString.toArrayBuffer();

    expect(otherBuffer).not.toBe(buffer);
    expect(new Uint8Array(otherBuffer)).toEqual(array);

    // modify the original buffer
    array[0] = 5;
    // We are no longer returning the original bytes
    expect(new Uint8Array(byteString.toArrayBuffer())).toEqual(new Uint8Array([
      5, 2, 3, 4
    ]));
  });

  it('preserve immutability for returned buffers: toArrayBuffer', () => {
    const byteString = ByteString.fromArrayBufferView(new Uint8Array(4));
    const buffer1 = byteString.toArrayBuffer();
    const buffer2 = byteString.toArrayBuffer();

    expect(buffer1).toEqual(buffer2);

    const array1 = new Uint8Array(buffer1);
    array1[0] = 1;

    expect(buffer1).not.toEqual(buffer2);
  });

  it('does not preserve immutability for returned buffers: toUint8ArrayUnsafe',
     () => {
       const byteString = ByteString.fromUint8ArrayUnsafe(new Uint8Array(4));
       const array1 = byteString.toUint8ArrayUnsafe();
       const array2 = byteString.toUint8ArrayUnsafe();

       expect(array1).toEqual(array2);
       array1[0] = 1;

       expect(array1).toEqual(array2);
     });

  it('throws when created with null ArrayBufferView', () => {
    expect(
        () => ByteString.fromArrayBufferView(
            /** @type {!ArrayBufferView} */ (/** @type{*} */ (null))))
        .toThrow();
  });

  it('throws when created with null buffer', () => {
    expect(
        () => ByteString.fromBase64String(
            /** @type {string} */ (/** @type{*} */ (null))))
        .toThrow();
  });

  it('convert base64 to ArrayBuffer', () => {
    const other = ByteString.fromBase64String(HALLO_IN_BASE64);
    expect(BYTES_WITH_HALLO).toEqual(new Uint8Array(other.toArrayBuffer()));
  });

  it('convert base64 with spaces to ArrayBuffer', () => {
    const other = ByteString.fromBase64String(HALLO_IN_BASE64_WITH_SPACES);
    expect(new Uint8Array(other.toArrayBuffer())).toEqual(BYTES_WITH_HALLO);
  });

  it('convert bytes to base64', () => {
    const other = ByteString.fromArrayBufferView(BYTES_WITH_HALLO);
    expect(HALLO_IN_BASE64).toEqual(other.toBase64String());
  });

  it('equal empty bytetring', () => {
    const empty = ByteString.fromArrayBuffer(new ArrayBuffer(0));
    expect(ByteString.EMPTY.equals(empty)).toEqual(true);
  });

  it('equal empty bytestring constructed from ArrayBufferView', () => {
    const empty = ByteString.fromArrayBufferView(new Uint8Array(0));
    expect(ByteString.EMPTY.equals(empty)).toEqual(true);
  });

  it('equal empty bytestring constructed from Uint8Array (unsafe)', () => {
    const empty = ByteString.fromUint8ArrayUnsafe(new Uint8Array(0));
    expect(ByteString.EMPTY.equals(empty)).toEqual(true);
  });

  it('equal empty bytestring constructed from base64', () => {
    const empty = ByteString.fromBase64String('');
    expect(ByteString.EMPTY.equals(empty)).toEqual(true);
  });

  it('equal other instance', () => {
    const other = ByteString.fromArrayBuffer(arrayBufferSlice(TEST_BYTES, 0));
    expect(TEST_STRING.equals(other)).toEqual(true);
  });

  it('not equal different instance', () => {
    const other =
        ByteString.fromArrayBuffer(new Uint8Array([1, 2, 3, 4, 5]).buffer);
    expect(TEST_STRING.equals(other)).toEqual(false);
  });

  it('equal other instance constructed from ArrayBufferView', () => {
    const other =
        ByteString.fromArrayBufferView(new Uint8Array(PREFIXED_TEST_BYTES, 1));
    expect(TEST_STRING.equals(other)).toEqual(true);
  });

  it('not equal different instance constructed from ArrayBufferView', () => {
    const other =
        ByteString.fromArrayBufferView(new Uint8Array([1, 2, 3, 4, 5]));
    expect(TEST_STRING.equals(other)).toEqual(false);
  });

  it('equal other instance constructed from Uint8Array (unsafe)', () => {
    const other =
        ByteString.fromUint8ArrayUnsafe(new Uint8Array(PREFIXED_TEST_BYTES, 1));
    expect(TEST_STRING.equals(other)).toEqual(true);
  });

  it('not equal different instance constructed from Uint8Array (unsafe)',
     () => {
       const other =
           ByteString.fromUint8ArrayUnsafe(new Uint8Array([1, 2, 3, 4, 5]));
       expect(TEST_STRING.equals(other)).toEqual(false);
     });

  it('have same hashcode for empty bytes', () => {
    const empty = ByteString.fromArrayBuffer(new ArrayBuffer(0));
    expect(ByteString.EMPTY.hashCode()).toEqual(empty.hashCode());
  });

  it('have same hashcode for test bytes', () => {
    const other = ByteString.fromArrayBuffer(arrayBufferSlice(TEST_BYTES, 0));
    expect(TEST_STRING.hashCode()).toEqual(other.hashCode());
  });

  it('have same hashcode for test bytes', () => {
    const other = ByteString.fromArrayBufferView(
        new Uint8Array(arrayBufferSlice(TEST_BYTES, 0)));
    expect(TEST_STRING.hashCode()).toEqual(other.hashCode());
  });

  it('have same hashcode for different instance constructed with base64',
     () => {
       const other = ByteString.fromBase64String(HALLO_IN_BASE64);
       expect(ByteString.fromArrayBufferView(BYTES_WITH_HALLO).hashCode())
           .toEqual(other.hashCode());
     });

  it('preserves the length of a Uint8Array', () => {
    const original = new Uint8Array([105, 183, 51, 251, 253, 118, 247]);
    const afterByteString = new Uint8Array(
        ByteString.fromArrayBufferView(original).toArrayBuffer());
    expect(afterByteString).toEqual(original);
  });

  it('preserves the length of a base64 value', () => {
    const expected = new Uint8Array([105, 183, 51, 251, 253, 118, 247]);
    const afterByteString = new Uint8Array(
        ByteString.fromBase64String('abcz+/129w').toArrayBuffer());
    expect(afterByteString).toEqual(expected);
  });

  it('preserves the length of a base64 value with padding', () => {
    const expected = new Uint8Array([105, 183, 51, 251, 253, 118, 247]);
    const afterByteString = new Uint8Array(
        ByteString.fromBase64String('abcz+/129w==').toArrayBuffer());
    expect(afterByteString).toEqual(expected);
  });
});
