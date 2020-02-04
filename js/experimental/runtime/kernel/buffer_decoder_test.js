/**
 * @fileoverview Tests for BufferDecoder.
 */

goog.module('protobuf.binary.varintsTest');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {CHECK_CRITICAL_BOUNDS, CHECK_STATE} = goog.require('protobuf.internal.checks');

goog.setTestOnly();

/**
 * @param {...number} bytes
 * @return {!ArrayBuffer}
 */
function createArrayBuffer(...bytes) {
  return new Uint8Array(bytes).buffer;
}

describe('Skip varint does', () => {
  it('skip a varint', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01));
    expect(bufferDecoder.skipVarint(0)).toBe(1);
  });

  it('fail when varint is larger than 10 bytes', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(createArrayBuffer(
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));

    if (CHECK_CRITICAL_BOUNDS) {
      expect(() => bufferDecoder.skipVarint(0)).toThrow();
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(bufferDecoder.skipVarint(0)).toBe(11);
    }
  });

  it('fail when varint is beyond end of underlying array', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x80, 0x80));
    expect(() => bufferDecoder.skipVarint(0)).toThrow();
  });
});

describe('readVarint64 does', () => {
  it('read zero', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x00));
    const {dataStart, lowBits, highBits} = bufferDecoder.getVarint(0);
    expect(dataStart).toBe(1);
    expect(lowBits).toBe(0);
    expect(highBits).toBe(0);
  });

  it('read one', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01));
    const {dataStart, lowBits, highBits} = bufferDecoder.getVarint(0);
    expect(dataStart).toBe(1);
    expect(lowBits).toBe(1);
    expect(highBits).toBe(0);
  });

  it('read max value', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(createArrayBuffer(
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01));
    const {dataStart, lowBits, highBits} = bufferDecoder.getVarint(0);
    expect(dataStart).toBe(10);
    expect(lowBits).toBe(-1);
    expect(highBits).toBe(-1);
  });
});

describe('subBufferDecoder does', () => {
  it('can create valid sub buffers', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x00, 0x01, 0x02));

    expect(bufferDecoder.subBufferDecoder(0, 0))
        .toEqual(BufferDecoder.fromArrayBuffer(createArrayBuffer()));
    expect(bufferDecoder.subBufferDecoder(0, 1))
        .toEqual(BufferDecoder.fromArrayBuffer(createArrayBuffer(0x00)));
    expect(bufferDecoder.subBufferDecoder(1, 0))
        .toEqual(BufferDecoder.fromArrayBuffer(createArrayBuffer()));
    expect(bufferDecoder.subBufferDecoder(1, 1))
        .toEqual(BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01)));
    expect(bufferDecoder.subBufferDecoder(1, 2))
        .toEqual(BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01, 0x02)));
  });

  it('can not create invalid', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x00, 0x01, 0x02));
    if (CHECK_STATE) {
      expect(() => bufferDecoder.subBufferDecoder(-1, 1)).toThrow();
      expect(() => bufferDecoder.subBufferDecoder(0, -4)).toThrow();
      expect(() => bufferDecoder.subBufferDecoder(0, 4)).toThrow();
    }
  });
});
