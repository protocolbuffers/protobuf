/**
 * @fileoverview Tests for textdecoder.js.
 */
goog.module('protobuf.binary.TextDecoderTest');

goog.setTestOnly();

const {decode, encode} = goog.require('protobuf.binary.textencoding');

describe('Decode does', () => {
  it('return empty string for empty array', () => {
    expect(decode(new DataView(new ArrayBuffer(0)))).toEqual('');
  });

  it('throw on null being passed', () => {
    expect(() => decode(/** @type {!DataView} */ (/** @type {*} */ (null))))
        .toThrow();
  });
});

describe('Encode does', () => {
  it('return empty array for empty string', () => {
    expect(encode('')).toEqual(new Uint8Array(0));
  });

  it('throw on null being passed', () => {
    expect(() => encode(/** @type {string} */ (/** @type {*} */ (null))))
        .toThrow();
  });
});

/** @const {!TextEncoder} */
const textEncoder = new TextEncoder('utf-8');

/**
 * A Pair of string and Uint8Array representing the same data.
 * Each pair has the string value and its utf-8 bytes.
 */
class Pair {
  /**
   * Constructs a pair from a given string.
   * @param {string} s
   * @return {!Pair}
   */
  static fromString(s) {
    return new Pair(s, textEncoder.encode(s).buffer);
  }
  /**
   * Constructs a pair from a given charCode.
   * @param {number} charCode
   * @return {!Pair}
   */
  static fromCharCode(charCode) {
    return Pair.fromString(String.fromCharCode(charCode));
  }

  /**
   * @param {string} stringValue
   * @param {!ArrayBuffer} bytes
   * @private
   */
  constructor(stringValue, bytes) {
    /** @const @private {string} */
    this.stringValue_ = stringValue;
    /** @const @private {!ArrayBuffer} */
    this.bytes_ = bytes;
  }

  /** Ensures that a given pair encodes and decodes round trip*/
  expectPairToMatch() {
    expect(decode(new DataView(this.bytes_))).toEqual(this.stringValue_);
    expect(encode(this.stringValue_)).toEqual(new Uint8Array(this.bytes_));
  }
}

describe('textencoding does', () => {
  it('works for empty string', () => {
    Pair.fromString('').expectPairToMatch();
  });

  it('decode and encode random strings', () => {
    // 1 byte strings
    Pair.fromString('hello').expectPairToMatch();
    Pair.fromString('HELLO1!');

    // 2 byte String
    Pair.fromString('©').expectPairToMatch();

    // 3 byte string
    Pair.fromString('❄').expectPairToMatch();

    // 4 byte string
    Pair.fromString('😁').expectPairToMatch();
  });

  it('decode and encode 1 byte strings', () => {
    for (let i = 0; i < 0x80; i++) {
      Pair.fromCharCode(i).expectPairToMatch();
    }
  });

  it('decode and encode 2 byte strings', () => {
    for (let i = 0xC0; i < 0x7FF; i++) {
      Pair.fromCharCode(i).expectPairToMatch();
    }
  });

  it('decode and encode 3 byte strings', () => {
    for (let i = 0x7FF; i < 0x8FFF; i++) {
      Pair.fromCharCode(i).expectPairToMatch();
    }
  });
});
