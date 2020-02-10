/**
 * @fileoverview Tests for reader.js.
 */
goog.module('protobuf.binary.ReaderTest');

goog.setTestOnly();

// Note to the reader:
// Since the reader behavior changes with the checking level some of the
// tests in this file have to know which checking level is enable to make
// correct assertions.
const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const ByteString = goog.require('protobuf.ByteString');
const reader = goog.require('protobuf.binary.reader');
const {CHECK_STATE} = goog.require('protobuf.internal.checks');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');
const {encode} = goog.require('protobuf.binary.textencoding');
const {getBoolPairs} = goog.require('protobuf.binary.boolTestPairs');
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

/******************************************************************************
 *                        Optional FUNCTIONS
 ******************************************************************************/

describe('Read bool does', () => {
  for (const pair of getBoolPairs()) {
    it(`decode ${pair.name}`, () => {
      if (pair.error && CHECK_STATE) {
        expect(() => reader.readBool(pair.bufferDecoder, 0)).toThrow();
      } else {
        const d = reader.readBool(
            pair.bufferDecoder, pair.bufferDecoder.startIndex());
        expect(d).toEqual(pair.boolValue);
      }
    });
  }
});

describe('readBytes does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder();
    expect(() => reader.readBytes(bufferDecoder, 0)).toThrow();
  });

  it('read bytes by index', () => {
    const bufferDecoder = createBufferDecoder(3, 1, 2, 3);
    const byteString = reader.readBytes(bufferDecoder, 0);
    expect(ByteString.fromArrayBuffer(new Uint8Array([1, 2, 3]).buffer))
        .toEqual(byteString);
  });
});

describe('readDouble does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder();
    expect(() => reader.readDouble(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getDoublePairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readDouble(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.doubleValue);
    });
  }
});

describe('readFixed32 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder();
    expect(() => reader.readFixed32(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getFixed32Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readFixed32(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.intValue);
    });
  }
});

describe('readFloat does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder();
    expect(() => reader.readFloat(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getFloatPairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readFloat(pair.bufferDecoder, 0);
      expect(d).toEqual(Math.fround(pair.floatValue));
    });
  }
});

describe('readInt32 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder(0x80);
    expect(() => reader.readInt32(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getInt32Pairs()) {
    it(`decode ${pair.name}`, () => {
      if (pair.error && CHECK_STATE) {
        expect(() => reader.readInt32(pair.bufferDecoder, 0)).toThrow();
      } else {
        const d = reader.readInt32(pair.bufferDecoder, 0);
        expect(d).toEqual(pair.intValue);
      }
    });
  }
});

describe('readSfixed32 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder(0x80);
    expect(() => reader.readSfixed32(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getSfixed32Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readSfixed32(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.intValue);
    });
  }
});

describe('readSfixed64 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder(0x80);
    expect(() => reader.readSfixed64(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getSfixed64Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readSfixed64(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.longValue);
    });
  }
});

describe('readSint32 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder(0x80);
    expect(() => reader.readSint32(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getSint32Pairs()) {
    it(`decode ${pair.name}`, () => {
      if (pair.error && CHECK_STATE) {
        expect(() => reader.readSint32(pair.bufferDecoder, 0)).toThrow();
      } else {
        const d = reader.readSint32(pair.bufferDecoder, 0);
        expect(d).toEqual(pair.intValue);
      }
    });
  }
});

describe('readInt64 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder(0x80);
    expect(() => reader.readInt64(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getInt64Pairs()) {
    it(`decode ${pair.name}`, () => {
      if (pair.error && CHECK_STATE) {
        expect(() => reader.readInt64(pair.bufferDecoder, 0)).toThrow();
      } else {
        const d = reader.readInt64(pair.bufferDecoder, 0);
        expect(d).toEqual(pair.longValue);
      }
    });
  }
});

describe('readSint64 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder(0x80);
    expect(() => reader.readSint64(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getSint64Pairs()) {
    it(`decode ${pair.name}`, () => {
      if (pair.error && CHECK_STATE) {
        expect(() => reader.readSint64(pair.bufferDecoder, 0)).toThrow();
      } else {
        const d = reader.readSint64(pair.bufferDecoder, 0);
        expect(d).toEqual(pair.longValue);
      }
    });
  }
});

describe('readUint32 does', () => {
  it('throw exception if data is too short', () => {
    const bufferDecoder = createBufferDecoder(0x80);
    expect(() => reader.readUint32(bufferDecoder, 0)).toThrow();
  });

  for (const pair of getUint32Pairs()) {
    it(`decode ${pair.name}`, () => {
      if (pair.error && CHECK_STATE) {
        expect(() => reader.readUint32(pair.bufferDecoder, 0)).toThrow();
      } else {
        const d = reader.readUint32(pair.bufferDecoder, 0);
        expect(d).toEqual(pair.intValue);
      }
    });
  }
});

/**
 *
 * @param {string} s
 * @return {!Uint8Array}
 */
function encodeString(s) {
  if (typeof TextEncoder !== 'undefined') {
    const textEncoder = new TextEncoder('utf-8');
    return textEncoder.encode(s);
  } else {
    return encode(s);
  }
}

/** @param {string} s */
function expectEncodedStringToMatch(s) {
  const array = encodeString(s);
  const length = array.length;
  if (length > 127) {
    throw new Error('Test only works for strings shorter than 128');
  }
  const encodedArray = new Uint8Array(length + 1);
  encodedArray[0] = length;
  encodedArray.set(array, 1);
  const bufferDecoder = BufferDecoder.fromArrayBuffer(encodedArray.buffer);
  expect(reader.readString(bufferDecoder, 0)).toEqual(s);
}

describe('readString does', () => {
  it('return empty string for zero length string', () => {
    const s = reader.readString(createBufferDecoder(0x00), 0);
    expect(s).toEqual('');
  });

  it('decode random strings', () => {
    // 1 byte strings
    expectEncodedStringToMatch('hello');
    expectEncodedStringToMatch('HELLO1!');

    // 2 byte String
    expectEncodedStringToMatch('©');

    // 3 byte string
    expectEncodedStringToMatch('❄');

    // 4 byte string
    expectEncodedStringToMatch('😁');
  });

  it('decode 1 byte strings', () => {
    for (let i = 0; i < 0x80; i++) {
      const s = String.fromCharCode(i);
      expectEncodedStringToMatch(s);
    }
  });

  it('decode 2 byte strings', () => {
    for (let i = 0xC0; i < 0x7FF; i++) {
      const s = String.fromCharCode(i);
      expectEncodedStringToMatch(s);
    }
  });

  it('decode 3 byte strings', () => {
    for (let i = 0x7FF; i < 0x8FFF; i++) {
      const s = String.fromCharCode(i);
      expectEncodedStringToMatch(s);
    }
  });

  it('throw exception on invalid bytes', () => {
    // This test will only succeed with the native TextDecoder since
    // our polyfill does not do any validation. IE10 and IE11 don't support
    // TextDecoder.
    // TODO: Remove this check once we no longer need to support IE
    if (typeof TextDecoder !== 'undefined') {
      expect(
          () => reader.readString(
              createBufferDecoder(0x01, /* invalid utf data point*/ 0xFF), 0))
          .toThrow();
    }
  });

  it('throw exception if data is too short', () => {
    const array = createBufferDecoder(0x02, '?'.charCodeAt(0));
    expect(() => reader.readString(array, 0)).toThrow();
  });
});

/******************************************************************************
 *                        REPEATED FUNCTIONS
 ******************************************************************************/

describe('readPackedBool does', () => {
  for (const pair of getPackedBoolPairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedBool(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.boolValues);
    });
  }
});

describe('readPackedDouble does', () => {
  for (const pair of getPackedDoublePairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedDouble(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.doubleValues);
    });
  }
});

describe('readPackedFixed32 does', () => {
  for (const pair of getPackedFixed32Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedFixed32(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.fixed32Values);
    });
  }
});

describe('readPackedFloat does', () => {
  for (const pair of getPackedFloatPairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedFloat(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.floatValues);
    });
  }
});

describe('readPackedInt32 does', () => {
  for (const pair of getPackedInt32Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedInt32(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.int32Values);
    });
  }
});

describe('readPackedInt64 does', () => {
  for (const pair of getPackedInt64Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedInt64(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.int64Values);
    });
  }
});

describe('readPackedSfixed32 does', () => {
  for (const pair of getPackedSfixed32Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedSfixed32(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.sfixed32Values);
    });
  }
});

describe('readPackedSfixed64 does', () => {
  for (const pair of getPackedSfixed64Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedSfixed64(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.sfixed64Values);
    });
  }
});

describe('readPackedSint32 does', () => {
  for (const pair of getPackedSint32Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedSint32(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.sint32Values);
    });
  }
});

describe('readPackedSint64 does', () => {
  for (const pair of getPackedSint64Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedSint64(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.sint64Values);
    });
  }
});

describe('readPackedUint32 does', () => {
  for (const pair of getPackedUint32Pairs()) {
    it(`decode ${pair.name}`, () => {
      const d = reader.readPackedUint32(pair.bufferDecoder, 0);
      expect(d).toEqual(pair.uint32Values);
    });
  }
});
