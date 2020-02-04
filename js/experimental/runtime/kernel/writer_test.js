/**
 * @fileoverview Tests for writer.js.
 */
goog.module('protobuf.binary.WriterTest');

goog.setTestOnly();

// Note to the reader:
// Since the writer behavior changes with the checking level some of the tests
// in this file have to know which checking level is enable to make correct
// assertions.
const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');
const WireType = goog.require('protobuf.binary.WireType');
const Writer = goog.require('protobuf.binary.Writer');
const {CHECK_BOUNDS, CHECK_TYPE, MAX_FIELD_NUMBER} = goog.require('protobuf.internal.checks');
const {arrayBufferSlice} = goog.require('protobuf.binary.typedArrays');
const {getDoublePairs} = goog.require('protobuf.binary.doubleTestPairs');
const {getFixed32Pairs} = goog.require('protobuf.binary.fixed32TestPairs');
const {getFloatPairs} = goog.require('protobuf.binary.floatTestPairs');
const {getInt32Pairs} = goog.require('protobuf.binary.int32TestPairs');
const {getInt64Pairs} = goog.require('protobuf.binary.int64TestPairs');
const {getPackedBoolPairs} = goog.require('protobuf.binary.packedBoolTestPairs');
const {getPackedDoublePairs} = goog.require('protobuf.binary.packedDoubleTestPairs');
const {getPackedFixed32Pairs} = goog.require('protobuf.binary.packedFixed32TestPairs');
const {getPackedFloatPairs} = goog.require('protobuf.binary.packedFloatTestPairs');
const {getPackedInt32Pairs} = goog.require('protobuf.binary.packedInt32TestPairs');
const {getPackedInt64Pairs} = goog.require('protobuf.binary.packedInt64TestPairs');
const {getPackedSfixed32Pairs} = goog.require('protobuf.binary.packedSfixed32TestPairs');
const {getPackedSfixed64Pairs} = goog.require('protobuf.binary.packedSfixed64TestPairs');
const {getPackedSint32Pairs} = goog.require('protobuf.binary.packedSint32TestPairs');
const {getPackedSint64Pairs} = goog.require('protobuf.binary.packedSint64TestPairs');
const {getPackedUint32Pairs} = goog.require('protobuf.binary.packedUint32TestPairs');
const {getSfixed32Pairs} = goog.require('protobuf.binary.sfixed32TestPairs');
const {getSfixed64Pairs} = goog.require('protobuf.binary.sfixed64TestPairs');
const {getSint32Pairs} = goog.require('protobuf.binary.sint32TestPairs');
const {getSint64Pairs} = goog.require('protobuf.binary.sint64TestPairs');
const {getUint32Pairs} = goog.require('protobuf.binary.uint32TestPairs');


/**
 * @param {...number} bytes
 * @return {!ArrayBuffer}
 */
function createArrayBuffer(...bytes) {
  return new Uint8Array(bytes).buffer;
}

/******************************************************************************
 *                        OPTIONAL FUNCTIONS
 ******************************************************************************/

describe('Writer does', () => {
  it('return an empty ArrayBuffer when nothing is encoded', () => {
    const writer = new Writer();
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  it('encode tag', () => {
    const writer = new Writer();
    writer.writeTag(1, WireType.VARINT);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer(0x08));
  });

  it('reset after calling getAndResetResultBuffer', () => {
    const writer = new Writer();
    writer.writeTag(1, WireType.VARINT);
    writer.getAndResetResultBuffer();
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  it('fail when field number is too large for writeTag', () => {
    const writer = new Writer();
    if (CHECK_TYPE) {
      expect(() => writer.writeTag(MAX_FIELD_NUMBER + 1, WireType.VARINT))
          .toThrowError('Field number is out of range: 536870912');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      writer.writeTag(MAX_FIELD_NUMBER + 1, WireType.VARINT);
      expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer(0));
    }
  });

  it('fail when field number is negative for writeTag', () => {
    const writer = new Writer();
    if (CHECK_TYPE) {
      expect(() => writer.writeTag(-1, WireType.VARINT))
          .toThrowError('Field number is out of range: -1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      writer.writeTag(-1, WireType.VARINT);
      expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer(0xF8));
    }
  });

  it('fail when wire type is invalid for writeTag', () => {
    const writer = new Writer();
    if (CHECK_TYPE) {
      expect(() => writer.writeTag(1, /** @type {!WireType} */ (0x08)))
          .toThrowError('Invalid wire type: 8');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      writer.writeTag(1, /** @type {!WireType} */ (0x08));
      expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer(0x08));
    }
  });

  it('encode singular boolean value', () => {
    const writer = new Writer();
    writer.writeBool(1, true);
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(0x08, 0x01));
  });

  it('encode length delimited', () => {
    const writer = new Writer();
    writer.writeDelimited(1, createArrayBuffer(0x01, 0x02));
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(0x0A, 0x02, 0x01, 0x02));
  });
});

describe('Writer.writeBufferDecoder does', () => {
  it('encode BufferDecoder containing a varint value', () => {
    const writer = new Writer();
    const expected = createArrayBuffer(
        0x08, /* varint start= */ 0xFF, /* varint end= */ 0x01, 0x08, 0x01);
    writer.writeBufferDecoder(
        BufferDecoder.fromArrayBuffer(expected), 1, WireType.VARINT);
    const result = writer.getAndResetResultBuffer();
    expect(result).toEqual(arrayBufferSlice(expected, 1, 3));
  });

  it('encode BufferDecoder containing a fixed64 value', () => {
    const writer = new Writer();
    const expected = createArrayBuffer(
        0x09, /* fixed64 start= */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        /* fixed64 end= */ 0x08, 0x08, 0x01);
    writer.writeBufferDecoder(
        BufferDecoder.fromArrayBuffer(expected), 1, WireType.FIXED64);
    const result = writer.getAndResetResultBuffer();
    expect(result).toEqual(arrayBufferSlice(expected, 1, 9));
  });

  it('encode BufferDecoder containing a length delimited value', () => {
    const writer = new Writer();
    const expected = createArrayBuffer(
        0xA, /* length= */ 0x03, /* data start= */ 0x01, 0x02,
        /* data end= */ 0x03, 0x08, 0x01);
    writer.writeBufferDecoder(
        BufferDecoder.fromArrayBuffer(expected), 1, WireType.DELIMITED);
    const result = writer.getAndResetResultBuffer();
    expect(result).toEqual(arrayBufferSlice(expected, 1, 5));
  });

  it('encode BufferDecoder containing a group', () => {
    const writer = new Writer();
    const expected = createArrayBuffer(
        0xB, /* group start= */ 0x08, 0x01, /* nested group start= */ 0x0B,
        /* nested group end= */ 0x0C, /* group end= */ 0x0C, 0x08, 0x01);
    writer.writeBufferDecoder(
        BufferDecoder.fromArrayBuffer(expected), 1, WireType.START_GROUP);
    const result = writer.getAndResetResultBuffer();
    expect(result).toEqual(arrayBufferSlice(expected, 1, 6));
  });

  it('encode BufferDecoder containing a fixed32 value', () => {
    const writer = new Writer();
    const expected = createArrayBuffer(
        0x09, /* fixed64 start= */ 0x01, 0x02, 0x03, /* fixed64 end= */ 0x04,
        0x08, 0x01);
    writer.writeBufferDecoder(
        BufferDecoder.fromArrayBuffer(expected), 1, WireType.FIXED32);
    const result = writer.getAndResetResultBuffer();
    expect(result).toEqual(arrayBufferSlice(expected, 1, 5));
  });

  it('fail when encoding out of bound data', () => {
    const writer = new Writer();
    const buffer = createArrayBuffer(0x4, 0x0, 0x1, 0x2, 0x3);
    const subBuffer = arrayBufferSlice(buffer, 0, 2);
    expect(
        () => writer.writeBufferDecoder(
            BufferDecoder.fromArrayBuffer(subBuffer), 0, WireType.DELIMITED))
        .toThrow();
  });
});

describe('Writer.writeBytes does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encodes empty ByteString', () => {
    writer.writeBytes(1, ByteString.EMPTY);
    const buffer = writer.getAndResetResultBuffer();
    expect(buffer.byteLength).toBe(2);
  });

  it('encodes empty array', () => {
    writer.writeBytes(1, ByteString.fromArrayBuffer(new ArrayBuffer(0)));
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            1 << 3 | 0x02,  // tag (fieldnumber << 3 | (length delimited))
            0,              // length of the bytes
            ));
  });

  it('encodes ByteString', () => {
    const array = createArrayBuffer(1, 2, 3);
    writer.writeBytes(1, ByteString.fromArrayBuffer(array));
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            1 << 3 | 0x02,  // tag (fieldnumber << 3 | (length delimited))
            3,              // length of the bytes
            1,
            2,
            3,
            ));
  });
});

describe('Writer.writeDouble does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getDoublePairs()) {
    it(`encode ${pair.name}`, () => {
        writer.writeDouble(1, pair.doubleValue);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        expect(buffer.length).toBe(9);
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x09);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, 9))
            .toEqual(pair.bufferDecoder.asUint8Array());
    });
  }

  /**
   * NaN may have different value in different browsers. Thus, we need to make
   * the test lenient.
   */
  it('encode NaN', () => {
    writer.writeDouble(1, NaN);
    const buffer = new Uint8Array(writer.getAndResetResultBuffer());
    expect(buffer.length).toBe(9);
    // ensure we have a correct tag
    expect(buffer[0]).toEqual(0x09);
    // Encoded values are stored right after the tag
    const float64 = new DataView(buffer.buffer);
    expect(float64.getFloat64(1, true)).toBeNaN();
  });
});

describe('Writer.writeFixed32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getFixed32Pairs()) {
    it(`encode ${pair.name}`, () => {
      writer.writeFixed32(1, pair.intValue);
      const buffer = new Uint8Array(writer.getAndResetResultBuffer());
      expect(buffer.length).toBe(5);
      // ensure we have a correct tag
      expect(buffer[0]).toEqual(0x0D);
      // Encoded values are stored right after the tag
      expect(buffer.subarray(1, 5)).toEqual(pair.bufferDecoder.asUint8Array());
    });
  }
});

describe('Writer.writeFloat does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getFloatPairs()) {
    it(`encode ${pair.name}`, () => {
      writer.writeFloat(1, pair.floatValue);
      const buffer = new Uint8Array(writer.getAndResetResultBuffer());
      expect(buffer.length).toBe(5);
      // ensure we have a correct tag
      expect(buffer[0]).toEqual(0x0D);
      // Encoded values are stored right after the tag
      expect(buffer.subarray(1, 5)).toEqual(pair.bufferDecoder.asUint8Array());
    });
  }

  /**
   * NaN may have different value in different browsers. Thus, we need to make
   * the test lenient.
   */
  it('encode NaN', () => {
    writer.writeFloat(1, NaN);
    const buffer = new Uint8Array(writer.getAndResetResultBuffer());
    expect(buffer.length).toBe(5);
    // ensure we have a correct tag
    expect(buffer[0]).toEqual(0x0D);
    // Encoded values are stored right after the tag
    const float32 = new DataView(buffer.buffer);
    expect(float32.getFloat32(1, true)).toBeNaN();
  });
});

describe('Writer.writeInt32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getInt32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writeInt32(1, pair.intValue);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x08);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writeSfixed32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedSfixed32(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getSfixed32Pairs()) {
    it(`encode ${pair.name}`, () => {
      writer.writeSfixed32(1, pair.intValue);
      const buffer = new Uint8Array(writer.getAndResetResultBuffer());
      expect(buffer.length).toBe(5);
      // ensure we have a correct tag
      expect(buffer[0]).toEqual(0x0D);
      // Encoded values are stored right after the tag
      expect(buffer.subarray(1, 5)).toEqual(pair.bufferDecoder.asUint8Array());
    });
  }
});

describe('Writer.writeSfixed64 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getSfixed64Pairs()) {
    it(`encode ${pair.name}`, () => {
      writer.writeSfixed64(1, pair.longValue);
      const buffer = new Uint8Array(writer.getAndResetResultBuffer());
      expect(buffer.length).toBe(9);
      // ensure we have a correct tag
      expect(buffer[0]).toEqual(0x09);
      // Encoded values are stored right after the tag
      expect(buffer.subarray(1, 9)).toEqual(pair.bufferDecoder.asUint8Array());
    });
  }
});

describe('Writer.writeSint32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getSint32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writeSint32(1, pair.intValue);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x08);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writeSint64 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getSint64Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writeSint64(1, pair.longValue);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x08);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writeInt64 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getInt64Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writeInt64(1, pair.longValue);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x08);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writeUint32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  for (const pair of getUint32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writeUint32(1, pair.intValue);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x08);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writeString does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty string', () => {
    writer.writeString(1, '');
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            1 << 3 | 0x02,  // tag (fieldnumber << 3 | (length delimited))
            0,              // length of the string
            ));
  });

  it('encode simple string', () => {
    writer.writeString(1, 'hello');
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            1 << 3 | 0x02,  // tag (fieldnumber << 3 | (length delimited))
            5,              // length of the string
            'h'.charCodeAt(0),
            'e'.charCodeAt(0),
            'l'.charCodeAt(0),
            'l'.charCodeAt(0),
            'o'.charCodeAt(0),
            ));
  });

  it('throw for invalid fieldnumber', () => {
    if (CHECK_BOUNDS) {
      expect(() => writer.writeString(-1, 'a'))
          .toThrowError('Field number is out of range: -1');
    } else {
      writer.writeString(-1, 'a');
      expect(new Uint8Array(writer.getAndResetResultBuffer()))
          .toEqual(new Uint8Array(createArrayBuffer(
              -6,  // invalid tag
              1,   // string length
              'a'.charCodeAt(0),
              )));
    }
  });

  it('throw for null string value', () => {
    expect(
        () => writer.writeString(
            1, /** @type {string} */ (/** @type {*} */ (null))))
        .toThrow();
  });
});


/******************************************************************************
 *                        REPEATED FUNCTIONS
 ******************************************************************************/

describe('Writer.writePackedBool does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedBool(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedBoolPairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedBool(1, pair.boolValues);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writeRepeatedBool does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writeRepeatedBool(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  it('encode repeated unpacked boolean values', () => {
    const writer = new Writer();
    writer.writeRepeatedBool(1, [true, false]);
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            1 << 3 | 0x00,  // tag (fieldnumber << 3 | (varint))
            0x01,           // value[0]
            1 << 3 | 0x00,  // tag (fieldnumber << 3 | (varint))
            0x00,           // value[1]
            ));
  });
});

describe('Writer.writePackedDouble does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedDouble(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedDoublePairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedDouble(1, pair.doubleValues);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedFixed32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedFixed32(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedFixed32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedFixed32(1, pair.fixed32Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedFloat does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedFloat(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedFloatPairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedFloat(1, pair.floatValues);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedInt32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedInt32(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedInt32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedInt32(1, pair.int32Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedInt64 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedInt64(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedInt64Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedInt64(1, pair.int64Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedSfixed32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedSfixed32(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedSfixed32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedSfixed32(1, pair.sfixed32Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedSfixed64 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedSfixed64(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedSfixed64Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedSfixed64(1, pair.sfixed64Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedSint32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedSint32(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedSint32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedSint32(1, pair.sint32Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedSint64 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedSint64(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedSint64Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedSint64(1, pair.sint64Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writePackedUint32 does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writePackedUint32(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  for (const pair of getPackedUint32Pairs()) {
    if (!pair.skip_writer) {
      it(`encode ${pair.name}`, () => {
        writer.writePackedUint32(1, pair.uint32Values);
        const buffer = new Uint8Array(writer.getAndResetResultBuffer());
        // ensure we have a correct tag
        expect(buffer[0]).toEqual(0x0A);
        // Encoded values are stored right after the tag
        expect(buffer.subarray(1, buffer.length))
            .toEqual(pair.bufferDecoder.asUint8Array());
      });
    }
  }
});

describe('Writer.writeRepeatedBytes does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writeRepeatedBytes(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  it('encode single value', () => {
    const value = createArrayBuffer(0x61);
    writer.writeRepeatedBytes(1, [ByteString.fromArrayBuffer(value)]);
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            0x0A,
            0x01,
            0x61,  // a
            ));
  });

  it('encode multiple values', () => {
    const value1 = createArrayBuffer(0x61);
    const value2 = createArrayBuffer(0x62);
    writer.writeRepeatedBytes(1, [
      ByteString.fromArrayBuffer(value1),
      ByteString.fromArrayBuffer(value2),
    ]);
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            0x0A,
            0x01,
            0x61,  // a
            0x0A,
            0x01,
            0x62,  // b
            ));
  });
});

describe('Writer.writeRepeatedString does', () => {
  let writer;
  beforeEach(() => {
    writer = new Writer();
  });

  it('encode empty array', () => {
    writer.writeRepeatedString(1, []);
    expect(writer.getAndResetResultBuffer()).toEqual(createArrayBuffer());
  });

  it('encode single value', () => {
    writer.writeRepeatedString(1, ['a']);
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            0x0A,
            0x01,
            0x61,  // a
            ));
  });

  it('encode multiple values', () => {
    writer.writeRepeatedString(1, ['a', 'b']);
    expect(writer.getAndResetResultBuffer())
        .toEqual(createArrayBuffer(
            0x0A,
            0x01,
            0x61,  // a
            0x0A,
            0x01,
            0x62,  // b
            ));
  });
});
