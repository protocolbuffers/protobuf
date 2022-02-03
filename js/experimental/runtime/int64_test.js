/**
 * @fileoverview Tests for Int64.
 */
goog.module('protobuf.Int64Test');
goog.setTestOnly();

const Int64 = goog.require('protobuf.Int64');
const Long = goog.require('goog.math.Long');

describe('Int64', () => {
  it('can be constructed from bits', () => {
    const int64 = Int64.fromBits(0, 1);
    expect(int64.getLowBits()).toEqual(0);
    expect(int64.getHighBits()).toEqual(1);
  });

  it('zero is defined', () => {
    const int64 = Int64.getZero();
    expect(int64.getLowBits()).toEqual(0);
    expect(int64.getHighBits()).toEqual(0);
  });

  it('max value is defined', () => {
    const int64 = Int64.getMaxValue();
    expect(int64).toEqual(Int64.fromBits(0xFFFFFFFF, 0x7FFFFFFF));
    expect(int64.asLong()).toEqual(Long.getMaxValue());
  });

  it('min value is defined', () => {
    const int64 = Int64.getMinValue();
    expect(int64).toEqual(Int64.fromBits(0, 0x80000000));
    expect(int64.asLong()).toEqual(Long.getMinValue());
  });

  it('Can be converted to long', () => {
    const int64 = Int64.fromInt(1);
    expect(int64.asLong()).toEqual(Long.fromInt(1));
  });

  it('Negative value can be converted to long', () => {
    const int64 = Int64.fromInt(-1);
    expect(int64.getLowBits()).toEqual(0xFFFFFFFF | 0);
    expect(int64.getHighBits()).toEqual(0xFFFFFFFF | 0);
    expect(int64.asLong()).toEqual(Long.fromInt(-1));
  });

  it('Can be converted to number', () => {
    const int64 = Int64.fromInt(1);
    expect(int64.asNumber()).toEqual(1);
  });

  it('Can convert negative value to number', () => {
    const int64 = Int64.fromInt(-1);
    expect(int64.asNumber()).toEqual(-1);
  });

  it('MAX_SAFE_INTEGER can be used.', () => {
    const int64 = Int64.fromNumber(Number.MAX_SAFE_INTEGER);
    expect(int64.getLowBitsUnsigned()).toEqual(0xFFFFFFFF);
    expect(int64.getHighBits()).toEqual(0x1FFFFF);
    expect(int64.asNumber()).toEqual(Number.MAX_SAFE_INTEGER);
  });

  it('MIN_SAFE_INTEGER can be used.', () => {
    const int64 = Int64.fromNumber(Number.MIN_SAFE_INTEGER);
    expect(int64.asNumber()).toEqual(Number.MIN_SAFE_INTEGER);
  });

  it('constructs fromInt', () => {
    const int64 = Int64.fromInt(1);
    expect(int64.getLowBits()).toEqual(1);
    expect(int64.getHighBits()).toEqual(0);
  });

  it('constructs fromLong', () => {
    const int64 = Int64.fromLong(Long.fromInt(1));
    expect(int64.getLowBits()).toEqual(1);
    expect(int64.getHighBits()).toEqual(0);
  });

  // TODO: Use our own checking system here.
  if (goog.DEBUG) {
    it('asNumber throws for MAX_SAFE_INTEGER + 1', () => {
      expect(() => Int64.fromNumber(Number.MAX_SAFE_INTEGER + 1).asNumber())
          .toThrow();
    });

    it('fromInt(MAX_SAFE_INTEGER) throws', () => {
      expect(() => Int64.fromInt(Number.MAX_SAFE_INTEGER)).toThrow();
    });

    it('fromInt(1.5) throws', () => {
      expect(() => Int64.fromInt(1.5)).toThrow();
    });
  }

  const decimalHexPairs = {
    '0x0000000000000000': {signed: '0'},
    '0x0000000000000001': {signed: '1'},
    '0x00000000ffffffff': {signed: '4294967295'},
    '0x0000000100000000': {signed: '4294967296'},
    '0xffffffffffffffff': {signed: '-1', unsigned: '18446744073709551615'},
    '0x8000000000000000':
        {signed: '-9223372036854775808', unsigned: '9223372036854775808'},
    '0x8000000080000000':
        {signed: '-9223372034707292160', unsigned: '9223372039002259456'},
    '0x01b69b4bacd05f15': {signed: '123456789123456789'},
    '0xfe4964b4532fa0eb':
        {signed: '-123456789123456789', unsigned: '18323287284586094827'},
    '0xa5a5a5a5a5a5a5a5':
        {signed: '-6510615555426900571', unsigned: '11936128518282651045'},
    '0x5a5a5a5a5a5a5a5a': {signed: '6510615555426900570'},
    '0xffffffff00000000':
        {signed: '-4294967296', unsigned: '18446744069414584320'},
  };

  it('serializes to signed decimal strings', () => {
    for (const [hex, decimals] of Object.entries(decimalHexPairs)) {
      const int64 = hexToInt64(hex);
      expect(int64.toSignedDecimalString()).toEqual(decimals.signed);
    }
  });

  it('serializes to unsigned decimal strings', () => {
    for (const [hex, decimals] of Object.entries(decimalHexPairs)) {
      const int64 = hexToInt64(hex);
      expect(int64.toUnsignedDecimalString())
          .toEqual(decimals.unsigned || decimals.signed);
    }
  });

  it('serializes to unsigned hex strings', () => {
    for (const [hex, decimals] of Object.entries(decimalHexPairs)) {
      const int64 = hexToInt64(hex);
      let shortHex = hex.replace(/0x0*/, '0x');
      if (shortHex == '0x') {
        shortHex = '0x0';
      }
      expect(int64.toHexString()).toEqual(shortHex);
    }
  });

  it('parses decimal strings', () => {
    for (const [hex, decimals] of Object.entries(decimalHexPairs)) {
      const signed = Int64.fromDecimalString(decimals.signed);
      expect(int64ToHex(signed)).toEqual(hex);
      if (decimals.unsigned) {
        const unsigned = Int64.fromDecimalString(decimals.unsigned);
        expect(int64ToHex(unsigned)).toEqual(hex);
      }
    }
  });

  it('parses hex strings', () => {
    for (const [hex, decimals] of Object.entries(decimalHexPairs)) {
      expect(int64ToHex(Int64.fromHexString(hex))).toEqual(hex);
    }
    expect(int64ToHex(Int64.fromHexString('-0x1')))
        .toEqual('0xffffffffffffffff');
  });

  // TODO: Use our own checking system here.
  if (goog.DEBUG) {
    it('throws when parsing empty string', () => {
      expect(() => Int64.fromDecimalString('')).toThrow();
    });

    it('throws when parsing float string', () => {
      expect(() => Int64.fromDecimalString('1.5')).toThrow();
    });

    it('throws when parsing non-numeric string', () => {
      expect(() => Int64.fromDecimalString('0xa')).toThrow();
    });
  }

  it('checks if equal', () => {
    const low = Int64.fromInt(1);
    const high = Int64.getMaxValue();
    expect(low.equals(Int64.fromInt(1))).toEqual(true);
    expect(low.equals(high)).toEqual(false);
    expect(high.equals(Int64.getMaxValue())).toEqual(true);
  });

  it('returns unique hashcode', () => {
    expect(Int64.fromInt(1).hashCode()).toEqual(Int64.fromInt(1).hashCode());
    expect(Int64.fromInt(1).hashCode())
        .not.toEqual(Int64.fromInt(2).hashCode());
  });
});

/**
 * @param {string} hexString
 * @return {!Int64}
 */
function hexToInt64(hexString) {
  const high = hexString.slice(2, 10);
  const low = hexString.slice(10);
  return Int64.fromBits(parseInt(low, 16), parseInt(high, 16));
}

/**
 * @param {!Int64} int64
 * @return {string}
 */
function int64ToHex(int64) {
  const ZEROS_32_BIT = '00000000';
  const highPartialHex = int64.getHighBitsUnsigned().toString(16);
  const lowPartialHex = int64.getLowBitsUnsigned().toString(16);
  const highHex = ZEROS_32_BIT.slice(highPartialHex.length) + highPartialHex;
  const lowHex = ZEROS_32_BIT.slice(lowPartialHex.length) + lowPartialHex;
  return `0x${highHex}${lowHex}`;
}
