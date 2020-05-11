/**
 * @fileoverview Tests for tag.js.
 */
goog.module('protobuf.binary.TagTests');

const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const WireType = goog.require('protobuf.binary.WireType');
const {CHECK_CRITICAL_STATE} = goog.require('protobuf.internal.checks');
const {createTag, get32BitVarintLength, skipField, tagToFieldNumber, tagToWireType} = goog.require('protobuf.binary.tag');


goog.setTestOnly();

/**
 * @param {...number} bytes
 * @return {!ArrayBuffer}
 */
function createArrayBuffer(...bytes) {
  return new Uint8Array(bytes).buffer;
}

describe('skipField', () => {
  it('skips varints', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x80, 0x00));
    skipField(bufferDecoder, WireType.VARINT, 1);
    expect(bufferDecoder.cursor()).toBe(2);
  });

  it('throws for out of bounds varints', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x80, 0x00));
    bufferDecoder.setCursor(2);
    if (CHECK_CRITICAL_STATE) {
      expect(() => skipField(bufferDecoder, WireType.VARINT, 1)).toThrowError();
    }
  });

  it('skips fixed64', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    skipField(bufferDecoder, WireType.FIXED64, 1);
    expect(bufferDecoder.cursor()).toBe(8);
  });

  it('throws for fixed64 if length is too short', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => skipField(bufferDecoder, WireType.FIXED64, 1))
          .toThrowError();
    }
  });

  it('skips fixed32', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0x80, 0x00, 0x00, 0x00));
    skipField(bufferDecoder, WireType.FIXED32, 1);
    expect(bufferDecoder.cursor()).toBe(4);
  });

  it('throws for fixed32 if length is too short', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => skipField(bufferDecoder, WireType.FIXED32, 1))
          .toThrowError();
    }
  });


  it('skips length delimited', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0x03, 0x00, 0x00, 0x00));
    skipField(bufferDecoder, WireType.DELIMITED, 1);
    expect(bufferDecoder.cursor()).toBe(4);
  });

  it('throws for length delimited if length is too short', () => {
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x03, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => skipField(bufferDecoder, WireType.DELIMITED, 1))
          .toThrowError();
    }
  });

  it('skips groups', () => {
    const bufferDecoder = BufferDecoder.fromArrayBuffer(
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C));
    bufferDecoder.setCursor(1);
    skipField(bufferDecoder, WireType.START_GROUP, 1);
    expect(bufferDecoder.cursor()).toBe(4);
  });

  it('skips group in group', () => {
    const buffer = createArrayBuffer(
        0x0B,        // start outter
        0x10, 0x01,  // field: 2, value: 1
        0x0B,        // start inner group
        0x10, 0x01,  // payload inner group
        0x0C,        // stop inner group
        0x0C         // end outter
    );
    const bufferDecoder = BufferDecoder.fromArrayBuffer(buffer);
    bufferDecoder.setCursor(1);
    skipField(bufferDecoder, WireType.START_GROUP, 1);
    expect(bufferDecoder.cursor()).toBe(8);
  });

  it('throws for group if length is too short', () => {
    // no closing group
    const bufferDecoder =
        BufferDecoder.fromArrayBuffer(createArrayBuffer(0x0B, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => skipField(bufferDecoder, WireType.START_GROUP, 1))
          .toThrowError();
    }
  });
});


describe('tagToWireType', () => {
  it('decodes numbers ', () => {
    // simple numbers
    expect(tagToWireType(0x00)).toBe(WireType.VARINT);
    expect(tagToWireType(0x01)).toBe(WireType.FIXED64);
    expect(tagToWireType(0x02)).toBe(WireType.DELIMITED);
    expect(tagToWireType(0x03)).toBe(WireType.START_GROUP);
    expect(tagToWireType(0x04)).toBe(WireType.END_GROUP);
    expect(tagToWireType(0x05)).toBe(WireType.FIXED32);

    // upper bits should not matter
    expect(tagToWireType(0x08)).toBe(WireType.VARINT);
    expect(tagToWireType(0x09)).toBe(WireType.FIXED64);
    expect(tagToWireType(0x0A)).toBe(WireType.DELIMITED);
    expect(tagToWireType(0x0B)).toBe(WireType.START_GROUP);
    expect(tagToWireType(0x0C)).toBe(WireType.END_GROUP);
    expect(tagToWireType(0x0D)).toBe(WireType.FIXED32);

    // upper bits should not matter
    expect(tagToWireType(0xF8)).toBe(WireType.VARINT);
    expect(tagToWireType(0xF9)).toBe(WireType.FIXED64);
    expect(tagToWireType(0xFA)).toBe(WireType.DELIMITED);
    expect(tagToWireType(0xFB)).toBe(WireType.START_GROUP);
    expect(tagToWireType(0xFC)).toBe(WireType.END_GROUP);
    expect(tagToWireType(0xFD)).toBe(WireType.FIXED32);

    // negative numbers work
    expect(tagToWireType(-8)).toBe(WireType.VARINT);
    expect(tagToWireType(-7)).toBe(WireType.FIXED64);
    expect(tagToWireType(-6)).toBe(WireType.DELIMITED);
    expect(tagToWireType(-5)).toBe(WireType.START_GROUP);
    expect(tagToWireType(-4)).toBe(WireType.END_GROUP);
    expect(tagToWireType(-3)).toBe(WireType.FIXED32);
  });
});

describe('tagToFieldNumber', () => {
  it('returns fieldNumber', () => {
    expect(tagToFieldNumber(0x08)).toBe(1);
    expect(tagToFieldNumber(0x09)).toBe(1);
    expect(tagToFieldNumber(0x10)).toBe(2);
    expect(tagToFieldNumber(0x12)).toBe(2);
  });
});

describe('createTag', () => {
  it('combines fieldNumber and wireType', () => {
    expect(createTag(WireType.VARINT, 1)).toBe(0x08);
    expect(createTag(WireType.FIXED64, 1)).toBe(0x09);
    expect(createTag(WireType.DELIMITED, 1)).toBe(0x0A);
    expect(createTag(WireType.START_GROUP, 1)).toBe(0x0B);
    expect(createTag(WireType.END_GROUP, 1)).toBe(0x0C);
    expect(createTag(WireType.FIXED32, 1)).toBe(0x0D);

    expect(createTag(WireType.VARINT, 2)).toBe(0x10);
    expect(createTag(WireType.FIXED64, 2)).toBe(0x11);
    expect(createTag(WireType.DELIMITED, 2)).toBe(0x12);
    expect(createTag(WireType.START_GROUP, 2)).toBe(0x13);
    expect(createTag(WireType.END_GROUP, 2)).toBe(0x14);
    expect(createTag(WireType.FIXED32, 2)).toBe(0x15);

    expect(createTag(WireType.VARINT, 0x1FFFFFFF)).toBe(0xFFFFFFF8 >>> 0);
    expect(createTag(WireType.FIXED64, 0x1FFFFFFF)).toBe(0xFFFFFFF9 >>> 0);
    expect(createTag(WireType.DELIMITED, 0x1FFFFFFF)).toBe(0xFFFFFFFA >>> 0);
    expect(createTag(WireType.START_GROUP, 0x1FFFFFFF)).toBe(0xFFFFFFFB >>> 0);
    expect(createTag(WireType.END_GROUP, 0x1FFFFFFF)).toBe(0xFFFFFFFC >>> 0);
    expect(createTag(WireType.FIXED32, 0x1FFFFFFF)).toBe(0xFFFFFFFD >>> 0);
  });
});

describe('get32BitVarintLength', () => {
  it('length of tag', () => {
    expect(get32BitVarintLength(0)).toBe(1);
    expect(get32BitVarintLength(1)).toBe(1);
    expect(get32BitVarintLength(1)).toBe(1);

    expect(get32BitVarintLength(Math.pow(2, 7) - 1)).toBe(1);
    expect(get32BitVarintLength(Math.pow(2, 7))).toBe(2);

    expect(get32BitVarintLength(Math.pow(2, 14) - 1)).toBe(2);
    expect(get32BitVarintLength(Math.pow(2, 14))).toBe(3);

    expect(get32BitVarintLength(Math.pow(2, 21) - 1)).toBe(3);
    expect(get32BitVarintLength(Math.pow(2, 21))).toBe(4);

    expect(get32BitVarintLength(Math.pow(2, 28) - 1)).toBe(4);
    expect(get32BitVarintLength(Math.pow(2, 28))).toBe(5);

    expect(get32BitVarintLength(Math.pow(2, 31) - 1)).toBe(5);

    expect(get32BitVarintLength(-1)).toBe(5);
    expect(get32BitVarintLength(-Math.pow(2, 31))).toBe(5);

    expect(get32BitVarintLength(createTag(WireType.VARINT, 0x1fffffff)))
        .toBe(5);
    expect(get32BitVarintLength(createTag(WireType.FIXED32, 0x1fffffff)))
        .toBe(5);
  });
});
