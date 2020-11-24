/**
 * @fileoverview Tests for BufferDecoder.
 */

goog.module('protobuf.binary.varintsTest');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const {CHECK_CRITICAL_STATE, CHECK_STATE} = goog.require('protobuf.internal.checks');

goog.setTestOnly();

/**
 * @param {...number} bytes
 * @return {!ArrayBuffer}
 */
function createArrayBuffer(...bytes) {
  return new Uint8Array(bytes).buffer;
}

describe('setCursor does', () => {
  it('set the cursor at the position specified', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x0, 0x1));
    expect(bufferDecoder.cursor()).toBe(0);
    bufferDecoder.setCursor(1);
    expect(bufferDecoder.cursor()).toBe(1);
  });
});

describe('skip does', () => {
  it('advance the cursor', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x0, 0x1, 0x2));
    bufferDecoder.setCursor(1);
    bufferDecoder.skip(1);
    expect(bufferDecoder.cursor()).toBe(2);
  });
});

describe('Skip varint does', () => {
  it('skip a varint', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01));
    bufferDecoder.skipVarint();
    expect(bufferDecoder.cursor()).toBe(1);
  });

  it('fail when varint is larger than 10 bytes', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(createArrayBuffer(
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));

    if (CHECK_CRITICAL_STATE) {
      expect(() => bufferDecoder.skipVarint()).toThrow();
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      bufferDecoder.skipVarint();
      expect(bufferDecoder.cursor()).toBe(11);
    }
  });

  it('fail when varint is beyond end of underlying array', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x80, 0x80));
    expect(() => bufferDecoder.skipVarint()).toThrow();
  });
});

describe('readVarint64 does', () => {
  it('read zero', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x00));
    const {lowBits, highBits} = bufferDecoder.getVarint(0);
    expect(lowBits).toBe(0);
    expect(highBits).toBe(0);
    expect(bufferDecoder.cursor()).toBe(1);
  });

  it('read one', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01));
    const {lowBits, highBits} = bufferDecoder.getVarint(0);
    expect(lowBits).toBe(1);
    expect(highBits).toBe(0);
    expect(bufferDecoder.cursor()).toBe(1);
  });

  it('read max value', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(createArrayBuffer(
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01));
    const {lowBits, highBits} = bufferDecoder.getVarint(0);
    expect(lowBits).toBe(-1);
    expect(highBits).toBe(-1);
    expect(bufferDecoder.cursor()).toBe(10);
  });
});

describe('readUnsignedVarint32 does', () => {
  it('read zero', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x00));
    const result = bufferDecoder.getUnsignedVarint32();
    expect(result).toBe(0);
    expect(bufferDecoder.cursor()).toBe(1);
  });

  it('read one', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01));
    const result = bufferDecoder.getUnsignedVarint32();
    expect(result).toBe(1);
    expect(bufferDecoder.cursor()).toBe(1);
  });

  it('read max int32', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0xFF, 0xFF, 0xFF, 0xFF, 0x0F));
    const result = bufferDecoder.getUnsignedVarint32();
    expect(result).toBe(4294967295);
    expect(bufferDecoder.cursor()).toBe(5);
  });

  it('read max value', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(createArrayBuffer(
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01));
    const result = bufferDecoder.getUnsignedVarint32();
    expect(result).toBe(4294967295);
    expect(bufferDecoder.cursor()).toBe(10);
  });

  it('fail if data is longer than 10 bytes', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(createArrayBuffer(
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01));
    if (CHECK_CRITICAL_STATE) {
      expect(() => bufferDecoder.getUnsignedVarint32()).toThrow();
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const result = bufferDecoder.getUnsignedVarint32();
      expect(result).toBe(4294967295);
      expect(bufferDecoder.cursor()).toBe(10);
    }
  });
});

describe('readUnsignedVarint32At does', () => {
  it('reads from a specific index', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x1, 0x2));
    const result = bufferDecoder.getUnsignedVarint32At(1);
    expect(result).toBe(2);
    expect(bufferDecoder.cursor()).toBe(2);
  });
});

describe('getFloat32 does', () => {
  it('read one', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0x00, 0x00, 0x80, 0x3F));
    const result = bufferDecoder.getFloat32(0);
    expect(result).toBe(1);
    expect(bufferDecoder.cursor()).toBe(4);
  });
});

describe('getFloat64 does', () => {
  it('read one', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F));
    const result = bufferDecoder.getFloat64(0);
    expect(result).toBe(1);
    expect(bufferDecoder.cursor()).toBe(8);
  });
});

describe('getInt32 does', () => {
  it('read one', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0x01, 0x00, 0x00, 0x00));
    const result = bufferDecoder.getInt32(0);
    expect(result).toBe(1);
    expect(bufferDecoder.cursor()).toBe(4);
  });

  it('read minus one', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0xFF, 0xFF, 0xFF, 0xFF));
    const result = bufferDecoder.getInt32(0);
    expect(result).toBe(-1);
    expect(bufferDecoder.cursor()).toBe(4);
  });
});

describe('getUint32 does', () => {
  it('read one', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x01, 0x00, 0x00, 0x0));
    const result = bufferDecoder.getUint32(0);
    expect(result).toBe(1);
    expect(bufferDecoder.cursor()).toBe(4);
  });

  it('read max uint32', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0xFF, 0xFF, 0xFF, 0xFF));
    const result = bufferDecoder.getUint32(0);
    expect(result).toBe(4294967295);
    expect(bufferDecoder.cursor()).toBe(4);
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
