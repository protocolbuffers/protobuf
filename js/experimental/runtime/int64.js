/**
 * @fileoverview Protobufs Int64 representation.
 */
goog.module('protobuf.Int64');

const Long = goog.require('goog.math.Long');
const {assert} = goog.require('goog.asserts');

/**
 * A container for protobufs Int64/Uint64 data type.
 * @final
 */
class Int64 {
  /** @return {!Int64} */
  static getZero() {
    return ZERO;
  }

  /** @return {!Int64} */
  static getMinValue() {
    return MIN_VALUE;
  }

  /** @return {!Int64} */
  static getMaxValue() {
    return MAX_VALUE;
  }

  /**
   * Constructs a Int64 given two 32 bit numbers
   * @param {number} lowBits
   * @param {number} highBits
   * @return {!Int64}
   */
  static fromBits(lowBits, highBits) {
    return new Int64(lowBits, highBits);
  }

  /**
   * Constructs an Int64 from a signed 32 bit number.
   * @param {number} value
   * @return {!Int64}
   */
  static fromInt(value) {
    // TODO: Use our own checking system here.
    assert(value === (value | 0), 'value should be a 32-bit integer');
    // Right shift 31 bits so all high bits are equal to the sign bit.
    // Note: cannot use >> 32, because (1 >> 32) = 1 (!).
    const signExtendedHighBits = value >> 31;
    return new Int64(value, signExtendedHighBits);
  }

  /**
   * Constructs an Int64 from a number (over 32 bits).
   * @param {number} value
   * @return {!Int64}
   */
  static fromNumber(value) {
    if (value > 0) {
      return new Int64(value, value / TWO_PWR_32_DBL);
    } else if (value < 0) {
      return negate(-value, -value / TWO_PWR_32_DBL);
    }
    return ZERO;
  }

  /**
   * Construct an Int64 from a signed decimal string.
   * @param {string} value
   * @return {!Int64}
   */
  static fromDecimalString(value) {
    // TODO: Use our own checking system here.
    assert(value.length > 0);
    // The basic Number conversion loses precision, but we can use it for
    // a quick validation that the format is correct and it is an integer.
    assert(Math.floor(Number(value)).toString().length == value.length);
    return decimalStringToInt64(value);
  }

  /**
   * Construct an Int64 from a signed hexadecimal string.
   * @param {string} value
   * @return {!Int64}
   */
  static fromHexString(value) {
    // TODO: Use our own checking system here.
    assert(value.length > 0);
    assert(value.slice(0, 2) == '0x' || value.slice(0, 3) == '-0x');
    const minus = value[0] === '-';
    // Strip the 0x or -0x prefix.
    value = value.slice(minus ? 3 : 2);
    const lowBits = parseInt(value.slice(-8), 16);
    const highBits = parseInt(value.slice(-16, -8) || '', 16);
    return (minus ? negate : Int64.fromBits)(lowBits, highBits);
  }

  // Note to the reader:
  // goog.math.Long suffers from a code size issue. JsCompiler almost always
  // considers toString methods to be alive in a program. So if you are
  // constructing a Long instance the toString method is assumed to be live.
  // Unfortunately Long's toString method makes a large chunk of code alive
  // of the entire class adding 1.3kB (gzip) of extra code size.
  // Callers that are sensitive to code size and are not using Long already
  // should avoid calling this method.
  /**
   * Creates an Int64 instance from a Long value.
   * @param {!Long} value
   * @return {!Int64}
   */
  static fromLong(value) {
    return new Int64(value.getLowBits(), value.getHighBits());
  }

  /**
   * @param {number} lowBits
   * @param {number} highBits
   * @private
   */
  constructor(lowBits, highBits) {
    /** @const @private {number} */
    this.lowBits_ = lowBits | 0;
    /** @const @private {number} */
    this.highBits_ = highBits | 0;
  }

  /**
   * Returns the int64 value as a JavaScript number. This will lose precision
   * if the number is outside of the safe range for JavaScript of 53 bits
   * precision.
   * @return {number}
   */
  asNumber() {
    const result = this.highBits_ * TWO_PWR_32_DBL + this.getLowBitsUnsigned();
    // TODO: Use our own checking system here.
    assert(
        Number.isSafeInteger(result), 'conversion to number loses precision.');
    return result;
  }

  // Note to the reader:
  // goog.math.Long suffers from a code size issue. JsCompiler almost always
  // considers toString methods to be alive in a program. So if you are
  // constructing a Long instance the toString method is assumed to be live.
  // Unfortunately Long's toString method makes a large chunk of code alive
  // of the entire class adding 1.3kB (gzip) of extra code size.
  // Callers that are sensitive to code size and are not using Long already
  // should avoid calling this method.
  /** @return {!Long} */
  asLong() {
    return Long.fromBits(this.lowBits_, this.highBits_);
  }

  /** @return {number} Signed 32-bit integer value. */
  getLowBits() {
    return this.lowBits_;
  }

  /** @return {number} Signed 32-bit integer value. */
  getHighBits() {
    return this.highBits_;
  }

  /** @return {number} Unsigned 32-bit integer. */
  getLowBitsUnsigned() {
    return this.lowBits_ >>> 0;
  }

  /** @return {number} Unsigned 32-bit integer. */
  getHighBitsUnsigned() {
    return this.highBits_ >>> 0;
  }

  /** @return {string} */
  toSignedDecimalString() {
    return joinSignedDecimalString(this);
  }

  /** @return {string} */
  toUnsignedDecimalString() {
    return joinUnsignedDecimalString(this);
  }

  /**
   * Returns an unsigned hexadecimal string representation of the Int64.
   * @return {string}
   */
  toHexString() {
    let nibbles = new Array(16);
    let lowBits = this.lowBits_;
    let highBits = this.highBits_;
    for (let highIndex = 7, lowIndex = 15; lowIndex > 7;
         highIndex--, lowIndex--) {
      nibbles[highIndex] = HEX_DIGITS[highBits & 0xF];
      nibbles[lowIndex] = HEX_DIGITS[lowBits & 0xF];
      highBits = highBits >>> 4;
      lowBits = lowBits >>> 4;
    }
    // Always leave the least significant hex digit.
    while (nibbles.length > 1 && nibbles[0] == '0') {
      nibbles.shift();
    }
    return `0x${nibbles.join('')}`;
  }

  /**
   * @param {*} other object to compare against.
   * @return {boolean} Whether this Int64 equals the other.
   */
  equals(other) {
    if (this === other) {
      return true;
    }
    if (!(other instanceof Int64)) {
      return false;
    }
    // Compare low parts first as there is higher chance they are different.
    const otherInt64 = /** @type{!Int64} */ (other);
    return (this.lowBits_ === otherInt64.lowBits_) &&
        (this.highBits_ === otherInt64.highBits_);
  }

  /**
   * Returns a number (int32) that is suitable for using in hashed structures.
   * @return {number}
   */
  hashCode() {
    return (31 * this.lowBits_ + 17 * this.highBits_) | 0;
  }
}

/**
 * Losslessly converts a 64-bit unsigned integer in 32:32 split representation
 * into a decimal string.
 * @param {!Int64} int64
 * @return {string} The binary number represented as a string.
 */
const joinUnsignedDecimalString = (int64) => {
  const lowBits = int64.getLowBitsUnsigned();
  const highBits = int64.getHighBitsUnsigned();
  // Skip the expensive conversion if the number is small enough to use the
  // built-in conversions.
  // Number.MAX_SAFE_INTEGER = 0x001FFFFF FFFFFFFF, thus any number with
  // highBits <= 0x1FFFFF can be safely expressed with a double and retain
  // integer precision.
  // Proven by: Number.isSafeInteger(0x1FFFFF * 2**32 + 0xFFFFFFFF) == true.
  if (highBits <= 0x1FFFFF) {
    return String(TWO_PWR_32_DBL * highBits + lowBits);
  }

  // What this code is doing is essentially converting the input number from
  // base-2 to base-1e7, which allows us to represent the 64-bit range with
  // only 3 (very large) digits. Those digits are then trivial to convert to
  // a base-10 string.

  // The magic numbers used here are -
  // 2^24 = 16777216 = (1,6777216) in base-1e7.
  // 2^48 = 281474976710656 = (2,8147497,6710656) in base-1e7.

  // Split 32:32 representation into 16:24:24 representation so our
  // intermediate digits don't overflow.
  const low = lowBits & LOW_24_BITS;
  const mid = ((lowBits >>> 24) | (highBits << 8)) & LOW_24_BITS;
  const high = (highBits >> 16) & LOW_16_BITS;

  // Assemble our three base-1e7 digits, ignoring carries. The maximum
  // value in a digit at this step is representable as a 48-bit integer, which
  // can be stored in a 64-bit floating point number.
  let digitA = low + (mid * 6777216) + (high * 6710656);
  let digitB = mid + (high * 8147497);
  let digitC = (high * 2);

  // Apply carries from A to B and from B to C.
  const base = 10000000;
  if (digitA >= base) {
    digitB += Math.floor(digitA / base);
    digitA %= base;
  }

  if (digitB >= base) {
    digitC += Math.floor(digitB / base);
    digitB %= base;
  }

  // If digitC is 0, then we should have returned in the trivial code path
  // at the top for non-safe integers. Given this, we can assume both digitB
  // and digitA need leading zeros.
  // TODO: Use our own checking system here.
  assert(digitC);
  return digitC + decimalFrom1e7WithLeadingZeros(digitB) +
      decimalFrom1e7WithLeadingZeros(digitA);
};

/**
 * @param {number} digit1e7 Number < 1e7
 * @return {string} Decimal representation of digit1e7 with leading zeros.
 */
const decimalFrom1e7WithLeadingZeros = (digit1e7) => {
  const partial = String(digit1e7);
  return '0000000'.slice(partial.length) + partial;
};

/**
 * Losslessly converts a 64-bit signed integer in 32:32 split representation
 * into a decimal string.
 * @param {!Int64} int64
 * @return {string} The binary number represented as a string.
 */
const joinSignedDecimalString = (int64) => {
  // If we're treating the input as a signed value and the high bit is set, do
  // a manual two's complement conversion before the decimal conversion.
  const negative = (int64.getHighBits() & 0x80000000);
  if (negative) {
    int64 = negate(int64.getLowBits(), int64.getHighBits());
  }

  const result = joinUnsignedDecimalString(int64);
  return negative ? '-' + result : result;
};

/**
 * @param {string} dec
 * @return {!Int64}
 */
const decimalStringToInt64 = (dec) => {
  // Check for minus sign.
  const minus = dec[0] === '-';
  if (minus) {
    dec = dec.slice(1);
  }

  // Work 6 decimal digits at a time, acting like we're converting base 1e6
  // digits to binary. This is safe to do with floating point math because
  // Number.isSafeInteger(ALL_32_BITS * 1e6) == true.
  const base = 1e6;
  let lowBits = 0;
  let highBits = 0;
  function add1e6digit(begin, end = undefined) {
    // Note: Number('') is 0.
    const digit1e6 = Number(dec.slice(begin, end));
    highBits *= base;
    lowBits = lowBits * base + digit1e6;
    // Carry bits from lowBits to
    if (lowBits >= TWO_PWR_32_DBL) {
      highBits = highBits + ((lowBits / TWO_PWR_32_DBL) | 0);
      lowBits = lowBits % TWO_PWR_32_DBL;
    }
  }
  add1e6digit(-24, -18);
  add1e6digit(-18, -12);
  add1e6digit(-12, -6);
  add1e6digit(-6);

  return (minus ? negate : Int64.fromBits)(lowBits, highBits);
};

/**
 * @param {number} lowBits
 * @param {number} highBits
 * @return {!Int64} Two's compliment negation of input.
 * @see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Bitwise_Operators#Signed_32-bit_integers
 */
const negate = (lowBits, highBits) => {
  highBits = ~highBits;
  if (lowBits) {
    lowBits = ~lowBits + 1;
  } else {
    // If lowBits is 0, then bitwise-not is 0xFFFFFFFF,
    // adding 1 to that, results in 0x100000000, which leaves
    // the low bits 0x0 and simply adds one to the high bits.
    highBits += 1;
  }
  return Int64.fromBits(lowBits, highBits);
};

/** @const {!Int64} */
const ZERO = new Int64(0, 0);

/** @const @private {number} */
const LOW_16_BITS = 0xFFFF;

/** @const @private {number} */
const LOW_24_BITS = 0xFFFFFF;

/** @const @private {number} */
const LOW_31_BITS = 0x7FFFFFFF;

/** @const @private {number} */
const ALL_32_BITS = 0xFFFFFFFF;

/** @const {!Int64} */
const MAX_VALUE = Int64.fromBits(ALL_32_BITS, LOW_31_BITS);

/** @const {!Int64} */
const MIN_VALUE = Int64.fromBits(0, 0x80000000);

/** @const {number} */
const TWO_PWR_32_DBL = 0x100000000;

/** @const {string} */
const HEX_DIGITS = '0123456789abcdef';

exports = Int64;
