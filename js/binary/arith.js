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
 * @fileoverview This file contains helper code used by jspb.utils to
 * handle 64-bit integer conversion to/from strings.
 *
 * @author cfallin@google.com (Chris Fallin)
 *
 * TODO(haberman): move this to javascript/closure/math?
 */

goog.provide('jspb.arith.Int64');
goog.provide('jspb.arith.UInt64');

/**
 * UInt64 implements some 64-bit arithmetic routines necessary for properly
 * handling 64-bit integer fields. It implements lossless integer arithmetic on
 * top of JavaScript's number type, which has only 53 bits of precision, by
 * representing 64-bit integers as two 32-bit halves.
 *
 * @param {number} lo The low 32 bits.
 * @param {number} hi The high 32 bits.
 * @constructor
 */
jspb.arith.UInt64 = function(lo, hi) {
  /**
   * The low 32 bits.
   * @public {number}
   */
  this.lo = lo;
  /**
   * The high 32 bits.
   * @public {number}
   */
  this.hi = hi;
};


/**
 * Compare two 64-bit numbers. Returns -1 if the first is
 * less, +1 if the first is greater, or 0 if both are equal.
 * @param {!jspb.arith.UInt64} other
 * @return {number}
 */
jspb.arith.UInt64.prototype.cmp = function(other) {
  if (this.hi < other.hi || (this.hi == other.hi && this.lo < other.lo)) {
    return -1;
  } else if (this.hi == other.hi && this.lo == other.lo) {
    return 0;
  } else {
    return 1;
  }
};


/**
 * Right-shift this number by one bit.
 * @return {!jspb.arith.UInt64}
 */
jspb.arith.UInt64.prototype.rightShift = function() {
  var hi = this.hi >>> 1;
  var lo = (this.lo >>> 1) | ((this.hi & 1) << 31);
  return new jspb.arith.UInt64(lo >>> 0, hi >>> 0);
};


/**
 * Left-shift this number by one bit.
 * @return {!jspb.arith.UInt64}
 */
jspb.arith.UInt64.prototype.leftShift = function() {
  var lo = this.lo << 1;
  var hi = (this.hi << 1) | (this.lo >>> 31);
  return new jspb.arith.UInt64(lo >>> 0, hi >>> 0);
};


/**
 * Test the MSB.
 * @return {boolean}
 */
jspb.arith.UInt64.prototype.msb = function() {
  return !!(this.hi & 0x80000000);
};


/**
 * Test the LSB.
 * @return {boolean}
 */
jspb.arith.UInt64.prototype.lsb = function() {
  return !!(this.lo & 1);
};


/**
 * Test whether this number is zero.
 * @return {boolean}
 */
jspb.arith.UInt64.prototype.zero = function() {
  return this.lo == 0 && this.hi == 0;
};


/**
 * Add two 64-bit numbers to produce a 64-bit number.
 * @param {!jspb.arith.UInt64} other
 * @return {!jspb.arith.UInt64}
 */
jspb.arith.UInt64.prototype.add = function(other) {
  var lo = ((this.lo + other.lo) & 0xffffffff) >>> 0;
  var hi =
      (((this.hi + other.hi) & 0xffffffff) >>> 0) +
      (((this.lo + other.lo) >= 0x100000000) ? 1 : 0);
  return new jspb.arith.UInt64(lo >>> 0, hi >>> 0);
};


/**
 * Subtract two 64-bit numbers to produce a 64-bit number.
 * @param {!jspb.arith.UInt64} other
 * @return {!jspb.arith.UInt64}
 */
jspb.arith.UInt64.prototype.sub = function(other) {
  var lo = ((this.lo - other.lo) & 0xffffffff) >>> 0;
  var hi =
      (((this.hi - other.hi) & 0xffffffff) >>> 0) -
      (((this.lo - other.lo) < 0) ? 1 : 0);
  return new jspb.arith.UInt64(lo >>> 0, hi >>> 0);
};


/**
 * Multiply two 32-bit numbers to produce a 64-bit number.
 * @param {number} a The first integer:  must be in [0, 2^32-1).
 * @param {number} b The second integer: must be in [0, 2^32-1).
 * @return {!jspb.arith.UInt64}
 */
jspb.arith.UInt64.mul32x32 = function(a, b) {
  // Directly multiplying two 32-bit numbers may produce up to 64 bits of
  // precision, thus losing precision because of the 53-bit mantissa of
  // JavaScript numbers. So we multiply with 16-bit digits (radix 65536)
  // instead.
  var aLow = (a & 0xffff);
  var aHigh = (a >>> 16);
  var bLow = (b & 0xffff);
  var bHigh = (b >>> 16);
  var productLow =
      // 32-bit result, result bits 0-31, take all 32 bits
      (aLow * bLow) +
      // 32-bit result, result bits 16-47, take bottom 16 as our top 16
      ((aLow * bHigh) & 0xffff) * 0x10000 +
      // 32-bit result, result bits 16-47, take bottom 16 as our top 16
      ((aHigh * bLow) & 0xffff) * 0x10000;
  var productHigh =
      // 32-bit result, result bits 32-63, take all 32 bits
      (aHigh * bHigh) +
      // 32-bit result, result bits 16-47, take top 16 as our bottom 16
      ((aLow * bHigh) >>> 16) +
      // 32-bit result, result bits 16-47, take top 16 as our bottom 16
      ((aHigh * bLow) >>> 16);

  // Carry. Note that we actually have up to *two* carries due to addition of
  // three terms.
  while (productLow >= 0x100000000) {
    productLow -= 0x100000000;
    productHigh += 1;
  }

  return new jspb.arith.UInt64(productLow >>> 0, productHigh >>> 0);
};


/**
 * Multiply this number by a 32-bit number, producing a 96-bit number, then
 * truncate the top 32 bits.
 * @param {number} a The multiplier.
 * @return {!jspb.arith.UInt64}
 */
jspb.arith.UInt64.prototype.mul = function(a) {
  // Produce two parts: at bits 0-63, and 32-95.
  var lo = jspb.arith.UInt64.mul32x32(this.lo, a);
  var hi = jspb.arith.UInt64.mul32x32(this.hi, a);
  // Left-shift hi by 32 bits, truncating its top bits. The parts will then be
  // aligned for addition.
  hi.hi = hi.lo;
  hi.lo = 0;
  return lo.add(hi);
};


/**
 * Divide a 64-bit number by a 32-bit number to produce a
 * 64-bit quotient and a 32-bit remainder.
 * @param {number} _divisor
 * @return {Array<jspb.arith.UInt64>} array of [quotient, remainder],
 * unless divisor is 0, in which case an empty array is returned.
 */
jspb.arith.UInt64.prototype.div = function(_divisor) {
  if (_divisor == 0) {
    return [];
  }

  // We perform long division using a radix-2 algorithm, for simplicity (i.e.,
  // one bit at a time). TODO: optimize to a radix-2^32 algorithm, taking care
  // to get the variable shifts right.
  var quotient = new jspb.arith.UInt64(0, 0);
  var remainder = new jspb.arith.UInt64(this.lo, this.hi);
  var divisor = new jspb.arith.UInt64(_divisor, 0);
  var unit = new jspb.arith.UInt64(1, 0);

  // Left-shift the divisor and unit until the high bit of divisor is set.
  while (!divisor.msb()) {
    divisor = divisor.leftShift();
    unit = unit.leftShift();
  }

  // Perform long division one bit at a time.
  while (!unit.zero()) {
    // If divisor < remainder, add unit to quotient and subtract divisor from
    // remainder.
    if (divisor.cmp(remainder) <= 0) {
      quotient = quotient.add(unit);
      remainder = remainder.sub(divisor);
    }
    // Right-shift the divisor and unit.
    divisor = divisor.rightShift();
    unit = unit.rightShift();
  }

  return [quotient, remainder];
};


/**
 * Convert a 64-bit number to a string.
 * @return {string}
 * @override
 */
jspb.arith.UInt64.prototype.toString = function() {
  var result = '';
  var num = this;
  while (!num.zero()) {
    var divResult = num.div(10);
    var quotient = divResult[0], remainder = divResult[1];
    result = remainder.lo + result;
    num = quotient;
  }
  if (result == '') {
    result = '0';
  }
  return result;
};


/**
 * Parse a string into a 64-bit number. Returns `null` on a parse error.
 * @param {string} s
 * @return {?jspb.arith.UInt64}
 */
jspb.arith.UInt64.fromString = function(s) {
  var result = new jspb.arith.UInt64(0, 0);
  // optimization: reuse this instance for each digit.
  var digit64 = new jspb.arith.UInt64(0, 0);
  for (var i = 0; i < s.length; i++) {
    if (s[i] < '0' || s[i] > '9') {
      return null;
    }
    var digit = parseInt(s[i], 10);
    digit64.lo = digit;
    result = result.mul(10).add(digit64);
  }
  return result;
};


/**
 * Make a copy of the uint64.
 * @return {!jspb.arith.UInt64}
 */
jspb.arith.UInt64.prototype.clone = function() {
  return new jspb.arith.UInt64(this.lo, this.hi);
};


/**
 * Int64 is like UInt64, but modifies string conversions to interpret the stored
 * 64-bit value as a twos-complement-signed integer. It does *not* support the
 * full range of operations that UInt64 does: only add, subtract, and string
 * conversions.
 *
 * N.B. that multiply and divide routines are *NOT* supported. They will throw
 * exceptions. (They are not necessary to implement string conversions, which
 * are the only operations we really need in jspb.)
 *
 * @param {number} lo The low 32 bits.
 * @param {number} hi The high 32 bits.
 * @constructor
 */
jspb.arith.Int64 = function(lo, hi) {
  /**
   * The low 32 bits.
   * @public {number}
   */
  this.lo = lo;
  /**
   * The high 32 bits.
   * @public {number}
   */
  this.hi = hi;
};


/**
 * Add two 64-bit numbers to produce a 64-bit number.
 * @param {!jspb.arith.Int64} other
 * @return {!jspb.arith.Int64}
 */
jspb.arith.Int64.prototype.add = function(other) {
  var lo = ((this.lo + other.lo) & 0xffffffff) >>> 0;
  var hi =
      (((this.hi + other.hi) & 0xffffffff) >>> 0) +
      (((this.lo + other.lo) >= 0x100000000) ? 1 : 0);
  return new jspb.arith.Int64(lo >>> 0, hi >>> 0);
};


/**
 * Subtract two 64-bit numbers to produce a 64-bit number.
 * @param {!jspb.arith.Int64} other
 * @return {!jspb.arith.Int64}
 */
jspb.arith.Int64.prototype.sub = function(other) {
  var lo = ((this.lo - other.lo) & 0xffffffff) >>> 0;
  var hi =
      (((this.hi - other.hi) & 0xffffffff) >>> 0) -
      (((this.lo - other.lo) < 0) ? 1 : 0);
  return new jspb.arith.Int64(lo >>> 0, hi >>> 0);
};


/**
 * Make a copy of the int64.
 * @return {!jspb.arith.Int64}
 */
jspb.arith.Int64.prototype.clone = function() {
  return new jspb.arith.Int64(this.lo, this.hi);
};


/**
 * Convert a 64-bit number to a string.
 * @return {string}
 * @override
 */
jspb.arith.Int64.prototype.toString = function() {
  // If the number is negative, find its twos-complement inverse.
  var sign = (this.hi & 0x80000000) != 0;
  var num = new jspb.arith.UInt64(this.lo, this.hi);
  if (sign) {
    num = new jspb.arith.UInt64(0, 0).sub(num);
  }
  return (sign ? '-' : '') + num.toString();
};


/**
 * Parse a string into a 64-bit number. Returns `null` on a parse error.
 * @param {string} s
 * @return {?jspb.arith.Int64}
 */
jspb.arith.Int64.fromString = function(s) {
  var hasNegative = (s.length > 0 && s[0] == '-');
  if (hasNegative) {
    s = s.substring(1);
  }
  var num = jspb.arith.UInt64.fromString(s);
  if (num === null) {
    return null;
  }
  if (hasNegative) {
    num = new jspb.arith.UInt64(0, 0).sub(num);
  }
  return new jspb.arith.Int64(num.lo, num.hi);
};
