// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * @fileoverview This file contains helper code used by jspb.BinaryReader
 * and BinaryWriter.
 *
 * @author aappleby@google.com (Austin Appleby)
 */

goog.provide('jspb.utils');

goog.require('goog.asserts');
goog.require('goog.crypt');
goog.require('goog.crypt.base64');
goog.require('goog.string');
goog.require('jspb.BinaryConstants');


/**
 * Javascript can't natively handle 64-bit data types, so to manipulate them we
 * have to split them into two 32-bit halves and do the math manually.
 *
 * Instead of instantiating and passing small structures around to do this, we
 * instead just use two global temporary values. This one stores the low 32
 * bits of a split value - for example, if the original value was a 64-bit
 * integer, this temporary value will contain the low 32 bits of that integer.
 * If the original value was a double, this temporary value will contain the
 * low 32 bits of the binary representation of that double, etcetera.
 * @type {number}
 */
jspb.utils.split64Low = 0;


/**
 * And correspondingly, this temporary variable will contain the high 32 bits
 * of whatever value was split.
 * @type {number}
 */
jspb.utils.split64High = 0;


/**
 * Splits an unsigned Javascript integer into two 32-bit halves and stores it
 * in the temp values above.
 * @param {number} value The number to split.
 */
jspb.utils.splitUint64 = function(value) {
  // Extract low 32 bits and high 32 bits as unsigned integers.
  var lowBits = value >>> 0;
  var highBits = Math.floor((value - lowBits) /
                            jspb.BinaryConstants.TWO_TO_32) >>> 0;

  jspb.utils.split64Low = lowBits;
  jspb.utils.split64High = highBits;
};


/**
 * Splits a signed Javascript integer into two 32-bit halves and stores it in
 * the temp values above.
 * @param {number} value The number to split.
 */
jspb.utils.splitInt64 = function(value) {
  // Convert to sign-magnitude representation.
  var sign = (value < 0);
  value = Math.abs(value);

  // Extract low 32 bits and high 32 bits as unsigned integers.
  var lowBits = value >>> 0;
  var highBits = Math.floor((value - lowBits) /
                            jspb.BinaryConstants.TWO_TO_32);
  highBits = highBits >>> 0;

  // Perform two's complement conversion if the sign bit was set.
  if (sign) {
    highBits = ~highBits >>> 0;
    lowBits = ~lowBits >>> 0;
    lowBits += 1;
    if (lowBits > 0xFFFFFFFF) {
      lowBits = 0;
      highBits++;
      if (highBits > 0xFFFFFFFF) highBits = 0;
    }
  }

  jspb.utils.split64Low = lowBits;
  jspb.utils.split64High = highBits;
};


/**
 * Convers a signed Javascript integer into zigzag format, splits it into two
 * 32-bit halves, and stores it in the temp values above.
 * @param {number} value The number to split.
 */
jspb.utils.splitZigzag64 = function(value) {
  // Convert to sign-magnitude and scale by 2 before we split the value.
  var sign = (value < 0);
  value = Math.abs(value) * 2;

  jspb.utils.splitUint64(value);
  var lowBits = jspb.utils.split64Low;
  var highBits = jspb.utils.split64High;

  // If the value is negative, subtract 1 from the split representation so we
  // don't lose the sign bit due to precision issues.
  if (sign) {
    if (lowBits == 0) {
      if (highBits == 0) {
        lowBits = 0xFFFFFFFF;
        highBits = 0xFFFFFFFF;
      } else {
        highBits--;
        lowBits = 0xFFFFFFFF;
      }
    } else {
      lowBits--;
    }
  }

  jspb.utils.split64Low = lowBits;
  jspb.utils.split64High = highBits;
};


/**
 * Converts a floating-point number into 32-bit IEEE representation and stores
 * it in the temp values above.
 * @param {number} value
 */
jspb.utils.splitFloat32 = function(value) {
  var sign = (value < 0) ? 1 : 0;
  value = sign ? -value : value;
  var exp;
  var mant;

  // Handle zeros.
  if (value === 0) {
    if ((1 / value) > 0) {
      // Positive zero.
      jspb.utils.split64High = 0;
      jspb.utils.split64Low = 0x00000000;
    } else {
      // Negative zero.
      jspb.utils.split64High = 0;
      jspb.utils.split64Low = 0x80000000;
    }
    return;
  }

  // Handle nans.
  if (isNaN(value)) {
    jspb.utils.split64High = 0;
    jspb.utils.split64Low = 0x7FFFFFFF;
    return;
  }

  // Handle infinities.
  if (value > jspb.BinaryConstants.FLOAT32_MAX) {
    jspb.utils.split64High = 0;
    jspb.utils.split64Low = ((sign << 31) | (0x7F800000)) >>> 0;
    return;
  }

  // Handle denormals.
  if (value < jspb.BinaryConstants.FLOAT32_MIN) {
    // Number is a denormal.
    mant = Math.round(value / Math.pow(2, -149));
    jspb.utils.split64High = 0;
    jspb.utils.split64Low = ((sign << 31) | mant) >>> 0;
    return;
  }

  exp = Math.floor(Math.log(value) / Math.LN2);
  mant = value * Math.pow(2, -exp);
  mant = Math.round(mant * jspb.BinaryConstants.TWO_TO_23) & 0x7FFFFF;

  jspb.utils.split64High = 0;
  jspb.utils.split64Low = ((sign << 31) | ((exp + 127) << 23) | mant) >>> 0;
};


/**
 * Converts a floating-point number into 64-bit IEEE representation and stores
 * it in the temp values above.
 * @param {number} value
 */
jspb.utils.splitFloat64 = function(value) {
  var sign = (value < 0) ? 1 : 0;
  value = sign ? -value : value;

  // Handle zeros.
  if (value === 0) {
    if ((1 / value) > 0) {
      // Positive zero.
      jspb.utils.split64High = 0x00000000;
      jspb.utils.split64Low = 0x00000000;
    } else {
      // Negative zero.
      jspb.utils.split64High = 0x80000000;
      jspb.utils.split64Low = 0x00000000;
    }
    return;
  }

  // Handle nans.
  if (isNaN(value)) {
    jspb.utils.split64High = 0x7FFFFFFF;
    jspb.utils.split64Low = 0xFFFFFFFF;
    return;
  }

  // Handle infinities.
  if (value > jspb.BinaryConstants.FLOAT64_MAX) {
    jspb.utils.split64High = ((sign << 31) | (0x7FF00000)) >>> 0;
    jspb.utils.split64Low = 0;
    return;
  }

  // Handle denormals.
  if (value < jspb.BinaryConstants.FLOAT64_MIN) {
    // Number is a denormal.
    var mant = value / Math.pow(2, -1074);
    var mantHigh = (mant / jspb.BinaryConstants.TWO_TO_32);
    jspb.utils.split64High = ((sign << 31) | mantHigh) >>> 0;
    jspb.utils.split64Low = (mant >>> 0);
    return;
  }

  var exp = Math.floor(Math.log(value) / Math.LN2);
  if (exp == 1024) exp = 1023;
  var mant = value * Math.pow(2, -exp);

  var mantHigh = (mant * jspb.BinaryConstants.TWO_TO_20) & 0xFFFFF;
  var mantLow = (mant * jspb.BinaryConstants.TWO_TO_52) >>> 0;

  jspb.utils.split64High =
      ((sign << 31) | ((exp + 1023) << 20) | mantHigh) >>> 0;
  jspb.utils.split64Low = mantLow;
};


/**
 * Converts an 8-character hash string into two 32-bit numbers and stores them
 * in the temp values above.
 * @param {string} hash
 */
jspb.utils.splitHash64 = function(hash) {
  var a = hash.charCodeAt(0);
  var b = hash.charCodeAt(1);
  var c = hash.charCodeAt(2);
  var d = hash.charCodeAt(3);
  var e = hash.charCodeAt(4);
  var f = hash.charCodeAt(5);
  var g = hash.charCodeAt(6);
  var h = hash.charCodeAt(7);

  jspb.utils.split64Low = (a + (b << 8) + (c << 16) + (d << 24)) >>> 0;
  jspb.utils.split64High = (e + (f << 8) + (g << 16) + (h << 24)) >>> 0;
};


/**
 * Joins two 32-bit values into a 64-bit unsigned integer. Precision will be
 * lost if the result is greater than 2^52.
 * @param {number} bitsLow
 * @param {number} bitsHigh
 * @return {number}
 */
jspb.utils.joinUint64 = function(bitsLow, bitsHigh) {
  return bitsHigh * jspb.BinaryConstants.TWO_TO_32 + (bitsLow >>> 0);
};


/**
 * Joins two 32-bit values into a 64-bit signed integer. Precision will be lost
 * if the result is greater than 2^52.
 * @param {number} bitsLow
 * @param {number} bitsHigh
 * @return {number}
 */
jspb.utils.joinInt64 = function(bitsLow, bitsHigh) {
  // If the high bit is set, do a manual two's complement conversion.
  var sign = (bitsHigh & 0x80000000);
  if (sign) {
    bitsLow = (~bitsLow + 1) >>> 0;
    bitsHigh = ~bitsHigh >>> 0;
    if (bitsLow == 0) {
      bitsHigh = (bitsHigh + 1) >>> 0;
    }
  }

  var result = jspb.utils.joinUint64(bitsLow, bitsHigh);
  return sign ? -result : result;
};

/**
 * Converts split 64-bit values from standard two's complement encoding to
 * zig-zag encoding. Invokes the provided function to produce final result.
 *
 * @param {number} bitsLow
 * @param {number} bitsHigh
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.utils.toZigzag64 = function(bitsLow, bitsHigh, convert) {
  // See
  // https://engdoc.corp.google.com/eng/howto/protocolbuffers/developerguide/encoding.shtml?cl=head#types
  // 64-bit math is: (n << 1) ^ (n >> 63)
  //
  // To do this in 32 bits, we can get a 32-bit sign-flipping mask from the
  // high word.
  // Then we can operate on each word individually, with the addition of the
  // "carry" to get the most significant bit from the low word into the high
  // word.
  var signFlipMask = bitsHigh >> 31;
  bitsHigh = (bitsHigh << 1 | bitsLow >>> 31) ^ signFlipMask;
  bitsLow = (bitsLow << 1) ^ signFlipMask;
  return convert(bitsLow, bitsHigh);
};


/**
 * Joins two 32-bit values into a 64-bit unsigned integer and applies zigzag
 * decoding. Precision will be lost if the result is greater than 2^52.
 * @param {number} bitsLow
 * @param {number} bitsHigh
 * @return {number}
 */
jspb.utils.joinZigzag64 = function(bitsLow, bitsHigh) {
  return jspb.utils.fromZigzag64(bitsLow, bitsHigh, jspb.utils.joinInt64);
};


/**
 * Converts split 64-bit values from zigzag encoding to standard two's
 * complement encoding. Invokes the provided function to produce final result.
 *
 * @param {number} bitsLow
 * @param {number} bitsHigh
 * @param {function(number, number): T} convert Conversion function to produce
 *     the result value, takes parameters (lowBits, highBits).
 * @return {T}
 * @template T
 */
jspb.utils.fromZigzag64 = function(bitsLow, bitsHigh, convert) {
  // 64 bit math is:
  //   signmask = (zigzag & 1) ? -1 : 0;
  //   twosComplement = (zigzag >> 1) ^ signmask;
  //
  // To work with 32 bit, we can operate on both but "carry" the lowest bit
  // from the high word by shifting it up 31 bits to be the most significant bit
  // of the low word.
  var signFlipMask = -(bitsLow & 1);
  bitsLow = ((bitsLow >>> 1) | (bitsHigh << 31)) ^ signFlipMask;
  bitsHigh = (bitsHigh >>> 1) ^ signFlipMask;
  return convert(bitsLow, bitsHigh);
};


/**
 * Joins two 32-bit values into a 32-bit IEEE floating point number and
 * converts it back into a Javascript number.
 * @param {number} bitsLow The low 32 bits of the binary number;
 * @param {number} bitsHigh The high 32 bits of the binary number.
 * @return {number}
 */
jspb.utils.joinFloat32 = function(bitsLow, bitsHigh) {
  var sign = ((bitsLow >> 31) * 2 + 1);
  var exp = (bitsLow >>> 23) & 0xFF;
  var mant = bitsLow & 0x7FFFFF;

  if (exp == 0xFF) {
    if (mant) {
      return NaN;
    } else {
      return sign * Infinity;
    }
  }

  if (exp == 0) {
    // Denormal.
    return sign * Math.pow(2, -149) * mant;
  } else {
    return sign * Math.pow(2, exp - 150) *
           (mant + Math.pow(2, 23));
  }
};


/**
 * Joins two 32-bit values into a 64-bit IEEE floating point number and
 * converts it back into a Javascript number.
 * @param {number} bitsLow The low 32 bits of the binary number;
 * @param {number} bitsHigh The high 32 bits of the binary number.
 * @return {number}
 */
jspb.utils.joinFloat64 = function(bitsLow, bitsHigh) {
  var sign = ((bitsHigh >> 31) * 2 + 1);
  var exp = (bitsHigh >>> 20) & 0x7FF;
  var mant = jspb.BinaryConstants.TWO_TO_32 * (bitsHigh & 0xFFFFF) + bitsLow;

  if (exp == 0x7FF) {
    if (mant) {
      return NaN;
    } else {
      return sign * Infinity;
    }
  }

  if (exp == 0) {
    // Denormal.
    return sign * Math.pow(2, -1074) * mant;
  } else {
    return sign * Math.pow(2, exp - 1075) *
           (mant + jspb.BinaryConstants.TWO_TO_52);
  }
};


/**
 * Joins two 32-bit values into an 8-character hash string.
 * @param {number} bitsLow
 * @param {number} bitsHigh
 * @return {string}
 */
jspb.utils.joinHash64 = function(bitsLow, bitsHigh) {
  var a = (bitsLow >>> 0) & 0xFF;
  var b = (bitsLow >>> 8) & 0xFF;
  var c = (bitsLow >>> 16) & 0xFF;
  var d = (bitsLow >>> 24) & 0xFF;
  var e = (bitsHigh >>> 0) & 0xFF;
  var f = (bitsHigh >>> 8) & 0xFF;
  var g = (bitsHigh >>> 16) & 0xFF;
  var h = (bitsHigh >>> 24) & 0xFF;

  return String.fromCharCode(a, b, c, d, e, f, g, h);
};

/**
 * Individual digits for number->string conversion.
 * @const {!Array<string>}
 */
jspb.utils.DIGITS = [
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
];

/** @const @private {number} '0' */
jspb.utils.ZERO_CHAR_CODE_ = 48;

/** @const @private {number} 'a' */
jspb.utils.A_CHAR_CODE_ = 97;

/**
 * Losslessly converts a 64-bit unsigned integer in 32:32 split representation
 * into a decimal string.
 * @param {number} bitsLow The low 32 bits of the binary number;
 * @param {number} bitsHigh The high 32 bits of the binary number.
 * @return {string} The binary number represented as a string.
 */
jspb.utils.joinUnsignedDecimalString = function(bitsLow, bitsHigh) {
  // Skip the expensive conversion if the number is small enough to use the
  // built-in conversions.
  if (bitsHigh <= 0x1FFFFF) {
    return '' + (jspb.BinaryConstants.TWO_TO_32 * bitsHigh + bitsLow);
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
  var low = bitsLow & 0xFFFFFF;
  var mid = (((bitsLow >>> 24) | (bitsHigh << 8)) >>> 0) & 0xFFFFFF;
  var high = (bitsHigh >> 16) & 0xFFFF;

  // Assemble our three base-1e7 digits, ignoring carries. The maximum
  // value in a digit at this step is representable as a 48-bit integer, which
  // can be stored in a 64-bit floating point number.
  var digitA = low + (mid * 6777216) + (high * 6710656);
  var digitB = mid + (high * 8147497);
  var digitC = (high * 2);

  // Apply carries from A to B and from B to C.
  var base = 10000000;
  if (digitA >= base) {
    digitB += Math.floor(digitA / base);
    digitA %= base;
  }

  if (digitB >= base) {
    digitC += Math.floor(digitB / base);
    digitB %= base;
  }

  // Convert base-1e7 digits to base-10, with optional leading zeroes.
  function decimalFrom1e7(digit1e7, needLeadingZeros) {
    var partial = digit1e7 ? String(digit1e7) : '';
    if (needLeadingZeros) {
      return '0000000'.slice(partial.length) + partial;
    }
    return partial;
  }

  return decimalFrom1e7(digitC, /*needLeadingZeros=*/ 0) +
      decimalFrom1e7(digitB, /*needLeadingZeros=*/ digitC) +
      // If the final 1e7 digit didn't need leading zeros, we would have
      // returned via the trivial code path at the top.
      decimalFrom1e7(digitA, /*needLeadingZeros=*/ 1);
};


/**
 * Losslessly converts a 64-bit signed integer in 32:32 split representation
 * into a decimal string.
 * @param {number} bitsLow The low 32 bits of the binary number;
 * @param {number} bitsHigh The high 32 bits of the binary number.
 * @return {string} The binary number represented as a string.
 */
jspb.utils.joinSignedDecimalString = function(bitsLow, bitsHigh) {
  // If we're treating the input as a signed value and the high bit is set, do
  // a manual two's complement conversion before the decimal conversion.
  var negative = (bitsHigh & 0x80000000);
  if (negative) {
    bitsLow = (~bitsLow + 1) >>> 0;
    var carry = (bitsLow == 0) ? 1 : 0;
    bitsHigh = (~bitsHigh + carry) >>> 0;
  }

  var result = jspb.utils.joinUnsignedDecimalString(bitsLow, bitsHigh);
  return negative ? '-' + result : result;
};


/**
 * Convert an 8-character hash string representing either a signed or unsigned
 * 64-bit integer into its decimal representation without losing accuracy.
 * @param {string} hash The hash string to convert.
 * @param {boolean} signed True if we should treat the hash string as encoding
 *     a signed integer.
 * @return {string}
 */
jspb.utils.hash64ToDecimalString = function(hash, signed) {
  jspb.utils.splitHash64(hash);
  var bitsLow = jspb.utils.split64Low;
  var bitsHigh = jspb.utils.split64High;
  return signed ?
      jspb.utils.joinSignedDecimalString(bitsLow, bitsHigh) :
      jspb.utils.joinUnsignedDecimalString(bitsLow, bitsHigh);
};


/**
 * Converts an array of 8-character hash strings into their decimal
 * representations.
 * @param {!Array<string>} hashes The array of hash strings to convert.
 * @param {boolean} signed True if we should treat the hash string as encoding
 *     a signed integer.
 * @return {!Array<string>}
 */
jspb.utils.hash64ArrayToDecimalStrings = function(hashes, signed) {
  var result = new Array(hashes.length);
  for (var i = 0; i < hashes.length; i++) {
    result[i] = jspb.utils.hash64ToDecimalString(hashes[i], signed);
  }
  return result;
};


/**
 * Converts a signed or unsigned decimal string into its hash string
 * representation.
 * @param {string} dec
 * @return {string}
 */
jspb.utils.decimalStringToHash64 = function(dec) {
  goog.asserts.assert(dec.length > 0);

  // Check for minus sign.
  var minus = false;
  if (dec[0] === '-') {
    minus = true;
    dec = dec.slice(1);
  }

  // Store result as a byte array.
  var resultBytes = [0, 0, 0, 0, 0, 0, 0, 0];

  // Set result to m*result + c.
  function muladd(m, c) {
    for (var i = 0; i < 8 && (m !== 1 || c > 0); i++) {
      var r = m * resultBytes[i] + c;
      resultBytes[i] = r & 0xFF;
      c = r >>> 8;
    }
  }

  // Negate the result bits.
  function neg() {
    for (var i = 0; i < 8; i++) {
      resultBytes[i] = (~resultBytes[i]) & 0xFF;
    }
  }

  // For each decimal digit, set result to 10*result + digit.
  for (var i = 0; i < dec.length; i++) {
    muladd(10, dec.charCodeAt(i) - jspb.utils.ZERO_CHAR_CODE_);
  }

  // If there's a minus sign, convert into two's complement.
  if (minus) {
    neg();
    muladd(1, 1);
  }

  return goog.crypt.byteArrayToString(resultBytes);
};


/**
 * Converts a signed or unsigned decimal string into two 32-bit halves, and
 * stores them in the temp variables listed above.
 * @param {string} value The decimal string to convert.
 */
jspb.utils.splitDecimalString = function(value) {
  jspb.utils.splitHash64(jspb.utils.decimalStringToHash64(value));
};

/**
 * @param {number} nibble A 4-bit integer.
 * @return {string}
 * @private
 */
jspb.utils.toHexDigit_ = function(nibble) {
  return String.fromCharCode(
      nibble < 10 ? jspb.utils.ZERO_CHAR_CODE_ + nibble :
                    jspb.utils.A_CHAR_CODE_ - 10 + nibble);
};

/**
 * @param {number} hexCharCode
 * @return {number}
 * @private
 */
jspb.utils.fromHexCharCode_ = function(hexCharCode) {
  if (hexCharCode >= jspb.utils.A_CHAR_CODE_) {
    return hexCharCode - jspb.utils.A_CHAR_CODE_ + 10;
  }
  return hexCharCode - jspb.utils.ZERO_CHAR_CODE_;
};

/**
 * Converts an 8-character hash string into its hexadecimal representation.
 * @param {string} hash
 * @return {string}
 */
jspb.utils.hash64ToHexString = function(hash) {
  var temp = new Array(18);
  temp[0] = '0';
  temp[1] = 'x';

  for (var i = 0; i < 8; i++) {
    var c = hash.charCodeAt(7 - i);
    temp[i * 2 + 2] = jspb.utils.toHexDigit_(c >> 4);
    temp[i * 2 + 3] = jspb.utils.toHexDigit_(c & 0xF);
  }

  var result = temp.join('');
  return result;
};


/**
 * Converts a '0x<16 digits>' hex string into its hash string representation.
 * @param {string} hex
 * @return {string}
 */
jspb.utils.hexStringToHash64 = function(hex) {
  hex = hex.toLowerCase();
  goog.asserts.assert(hex.length == 18);
  goog.asserts.assert(hex[0] == '0');
  goog.asserts.assert(hex[1] == 'x');

  var result = '';
  for (var i = 0; i < 8; i++) {
    var hi = jspb.utils.fromHexCharCode_(hex.charCodeAt(i * 2 + 2));
    var lo = jspb.utils.fromHexCharCode_(hex.charCodeAt(i * 2 + 3));
    result = String.fromCharCode(hi * 16 + lo) + result;
  }

  return result;
};


/**
 * Convert an 8-character hash string representing either a signed or unsigned
 * 64-bit integer into a Javascript number. Will lose accuracy if the result is
 * larger than 2^52.
 * @param {string} hash The hash string to convert.
 * @param {boolean} signed True if the has should be interpreted as a signed
 *     number.
 * @return {number}
 */
jspb.utils.hash64ToNumber = function(hash, signed) {
  jspb.utils.splitHash64(hash);
  var bitsLow = jspb.utils.split64Low;
  var bitsHigh = jspb.utils.split64High;
  return signed ? jspb.utils.joinInt64(bitsLow, bitsHigh) :
                  jspb.utils.joinUint64(bitsLow, bitsHigh);
};


/**
 * Convert a Javascript number into an 8-character hash string. Will lose
 * precision if the value is non-integral or greater than 2^64.
 * @param {number} value The integer to convert.
 * @return {string}
 */
jspb.utils.numberToHash64 = function(value) {
  jspb.utils.splitInt64(value);
  return jspb.utils.joinHash64(jspb.utils.split64Low,
                                  jspb.utils.split64High);
};


/**
 * Counts the number of contiguous varints in a buffer.
 * @param {!Uint8Array} buffer The buffer to scan.
 * @param {number} start The starting point in the buffer to scan.
 * @param {number} end The end point in the buffer to scan.
 * @return {number} The number of varints in the buffer.
 */
jspb.utils.countVarints = function(buffer, start, end) {
  // Count how many high bits of each byte were set in the buffer.
  var count = 0;
  for (var i = start; i < end; i++) {
    count += buffer[i] >> 7;
  }

  // The number of varints in the buffer equals the size of the buffer minus
  // the number of non-terminal bytes in the buffer (those with the high bit
  // set).
  return (end - start) - count;
};


/**
 * Counts the number of contiguous varint fields with the given field number in
 * the buffer.
 * @param {!Uint8Array} buffer The buffer to scan.
 * @param {number} start The starting point in the buffer to scan.
 * @param {number} end The end point in the buffer to scan.
 * @param {number} field The field number to count.
 * @return {number} The number of matching fields in the buffer.
 */
jspb.utils.countVarintFields = function(buffer, start, end, field) {
  var count = 0;
  var cursor = start;
  var tag = field * 8 + jspb.BinaryConstants.WireType.VARINT;

  if (tag < 128) {
    // Single-byte field tag, we can use a slightly quicker count.
    while (cursor < end) {
      // Skip the field tag, or exit if we find a non-matching tag.
      if (buffer[cursor++] != tag) return count;

      // Field tag matches, we've found a valid field.
      count++;

      // Skip the varint.
      while (1) {
        var x = buffer[cursor++];
        if ((x & 0x80) == 0) break;
      }
    }
  } else {
    while (cursor < end) {
      // Skip the field tag, or exit if we find a non-matching tag.
      var temp = tag;
      while (temp > 128) {
        if (buffer[cursor] != ((temp & 0x7F) | 0x80)) return count;
        cursor++;
        temp >>= 7;
      }
      if (buffer[cursor++] != temp) return count;

      // Field tag matches, we've found a valid field.
      count++;

      // Skip the varint.
      while (1) {
        var x = buffer[cursor++];
        if ((x & 0x80) == 0) break;
      }
    }
  }
  return count;
};


/**
 * Counts the number of contiguous fixed32 fields with the given tag in the
 * buffer.
 * @param {!Uint8Array} buffer The buffer to scan.
 * @param {number} start The starting point in the buffer to scan.
 * @param {number} end The end point in the buffer to scan.
 * @param {number} tag The tag value to count.
 * @param {number} stride The number of bytes to skip per field.
 * @return {number} The number of fields with a matching tag in the buffer.
 * @private
 */
jspb.utils.countFixedFields_ =
    function(buffer, start, end, tag, stride) {
  var count = 0;
  var cursor = start;

  if (tag < 128) {
    // Single-byte field tag, we can use a slightly quicker count.
    while (cursor < end) {
      // Skip the field tag, or exit if we find a non-matching tag.
      if (buffer[cursor++] != tag) return count;

      // Field tag matches, we've found a valid field.
      count++;

      // Skip the value.
      cursor += stride;
    }
  } else {
    while (cursor < end) {
      // Skip the field tag, or exit if we find a non-matching tag.
      var temp = tag;
      while (temp > 128) {
        if (buffer[cursor++] != ((temp & 0x7F) | 0x80)) return count;
        temp >>= 7;
      }
      if (buffer[cursor++] != temp) return count;

      // Field tag matches, we've found a valid field.
      count++;

      // Skip the value.
      cursor += stride;
    }
  }
  return count;
};


/**
 * Counts the number of contiguous fixed32 fields with the given field number
 * in the buffer.
 * @param {!Uint8Array} buffer The buffer to scan.
 * @param {number} start The starting point in the buffer to scan.
 * @param {number} end The end point in the buffer to scan.
 * @param {number} field The field number to count.
 * @return {number} The number of matching fields in the buffer.
 */
jspb.utils.countFixed32Fields = function(buffer, start, end, field) {
  var tag = field * 8 + jspb.BinaryConstants.WireType.FIXED32;
  return jspb.utils.countFixedFields_(buffer, start, end, tag, 4);
};


/**
 * Counts the number of contiguous fixed64 fields with the given field number
 * in the buffer.
 * @param {!Uint8Array} buffer The buffer to scan.
 * @param {number} start The starting point in the buffer to scan.
 * @param {number} end The end point in the buffer to scan.
 * @param {number} field The field number to count
 * @return {number} The number of matching fields in the buffer.
 */
jspb.utils.countFixed64Fields = function(buffer, start, end, field) {
  var tag = field * 8 + jspb.BinaryConstants.WireType.FIXED64;
  return jspb.utils.countFixedFields_(buffer, start, end, tag, 8);
};


/**
 * Counts the number of contiguous delimited fields with the given field number
 * in the buffer.
 * @param {!Uint8Array} buffer The buffer to scan.
 * @param {number} start The starting point in the buffer to scan.
 * @param {number} end The end point in the buffer to scan.
 * @param {number} field The field number to count.
 * @return {number} The number of matching fields in the buffer.
 */
jspb.utils.countDelimitedFields = function(buffer, start, end, field) {
  var count = 0;
  var cursor = start;
  var tag = field * 8 + jspb.BinaryConstants.WireType.DELIMITED;

  while (cursor < end) {
    // Skip the field tag, or exit if we find a non-matching tag.
    var temp = tag;
    while (temp > 128) {
      if (buffer[cursor++] != ((temp & 0x7F) | 0x80)) return count;
      temp >>= 7;
    }
    if (buffer[cursor++] != temp) return count;

    // Field tag matches, we've found a valid field.
    count++;

    // Decode the length prefix.
    var length = 0;
    var shift = 1;
    while (1) {
      temp = buffer[cursor++];
      length += (temp & 0x7f) * shift;
      shift *= 128;
      if ((temp & 0x80) == 0) break;
    }

    // Advance the cursor past the blob.
    cursor += length;
  }
  return count;
};


/**
 * String-ify bytes for text format. Should be optimized away in non-debug.
 * The returned string uses \xXX escapes for all values and is itself quoted.
 * [1, 31] serializes to '"\x01\x1f"'.
 * @param {jspb.ByteSource} byteSource The bytes to serialize.
 * @return {string} Stringified bytes for text format.
 */
jspb.utils.debugBytesToTextFormat = function(byteSource) {
  var s = '"';
  if (byteSource) {
    var bytes = jspb.utils.byteSourceToUint8Array(byteSource);
    for (var i = 0; i < bytes.length; i++) {
      s += '\\x';
      if (bytes[i] < 16) s += '0';
      s += bytes[i].toString(16);
    }
  }
  return s + '"';
};


/**
 * String-ify a scalar for text format. Should be optimized away in non-debug.
 * @param {string|number|boolean} scalar The scalar to stringify.
 * @return {string} Stringified scalar for text format.
 */
jspb.utils.debugScalarToTextFormat = function(scalar) {
  if (goog.isString(scalar)) {
    return goog.string.quote(scalar);
  } else {
    return scalar.toString();
  }
};


/**
 * Utility function: convert a string with codepoints 0--255 inclusive to a
 * Uint8Array. If any codepoints greater than 255 exist in the string, throws an
 * exception.
 * @param {string} str
 * @return {!Uint8Array}
 */
jspb.utils.stringToByteArray = function(str) {
  var arr = new Uint8Array(str.length);
  for (var i = 0; i < str.length; i++) {
    var codepoint = str.charCodeAt(i);
    if (codepoint > 255) {
      throw new Error('Conversion error: string contains codepoint ' +
                      'outside of byte range');
    }
    arr[i] = codepoint;
  }
  return arr;
};


/**
 * Converts any type defined in jspb.ByteSource into a Uint8Array.
 * @param {!jspb.ByteSource} data
 * @return {!Uint8Array}
 * @suppress {invalidCasts}
 */
jspb.utils.byteSourceToUint8Array = function(data) {
  if (data.constructor === Uint8Array) {
    return /** @type {!Uint8Array} */(data);
  }

  if (data.constructor === ArrayBuffer) {
    data = /** @type {!ArrayBuffer} */(data);
    return /** @type {!Uint8Array} */(new Uint8Array(data));
  }

  if (typeof Buffer != 'undefined' && data.constructor === Buffer) {
    return /** @type {!Uint8Array} */ (
        new Uint8Array(/** @type {?} */ (data)));
  }

  if (data.constructor === Array) {
    data = /** @type {!Array<number>} */(data);
    return /** @type {!Uint8Array} */(new Uint8Array(data));
  }

  if (data.constructor === String) {
    data = /** @type {string} */(data);
    return goog.crypt.base64.decodeStringToUint8Array(data);
  }

  goog.asserts.fail('Type not convertible to Uint8Array.');
  return /** @type {!Uint8Array} */(new Uint8Array(0));
};
