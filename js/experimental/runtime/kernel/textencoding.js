/**
 * @fileoverview A UTF8 decoder.
 */
goog.module('protobuf.binary.textencoding');

const {checkElementIndex} = goog.require('protobuf.internal.checks');

/**
 * Combines an array of codePoints into a string.
 * @param {!Array<number>} codePoints
 * @return {string}
 */
function codePointsToString(codePoints) {
  // Performance: http://jsperf.com/string-fromcharcode-test/13
  let s = '', i = 0;
  const length = codePoints.length;
  const BATCH_SIZE = 10000;
  while (i < length) {
    const end = Math.min(i + BATCH_SIZE, length);
    s += String.fromCharCode.apply(null, codePoints.slice(i, end));
    i = end;
  }
  return s;
}

/**
 * Decodes raw bytes into a string.
 * Supports codepoints from U+0000 up to U+10FFFF.
 * (http://en.wikipedia.org/wiki/UTF-8).
 * @param {!DataView} bytes
 * @return {string}
 */
function decode(bytes) {
  let cursor = 0;
  const codePoints = [];

  while (cursor < bytes.byteLength) {
    const c = bytes.getUint8(cursor++);
    if (c < 0x80) {  // Regular 7-bit ASCII.
      codePoints.push(c);
    } else if (c < 0xC0) {
      // UTF-8 continuation mark. We are out of sync. This
      // might happen if we attempted to read a character
      // with more than four bytes.
      continue;
    } else if (c < 0xE0) {  // UTF-8 with two bytes.
      checkElementIndex(cursor, bytes.byteLength);
      const c2 = bytes.getUint8(cursor++);
      codePoints.push(((c & 0x1F) << 6) | (c2 & 0x3F));
    } else if (c < 0xF0) {  // UTF-8 with three bytes.
      checkElementIndex(cursor + 1, bytes.byteLength);
      const c2 = bytes.getUint8(cursor++);
      const c3 = bytes.getUint8(cursor++);
      codePoints.push(((c & 0xF) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
    } else if (c < 0xF8) {  // UTF-8 with 4 bytes.
      checkElementIndex(cursor + 2, bytes.byteLength);
      const c2 = bytes.getUint8(cursor++);
      const c3 = bytes.getUint8(cursor++);
      const c4 = bytes.getUint8(cursor++);
      // Characters written on 4 bytes have 21 bits for a codepoint.
      // We can't fit that on 16bit characters, so we use surrogates.
      let codepoint = ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) |
          ((c3 & 0x3F) << 6) | (c4 & 0x3F);
      // Surrogates formula from wikipedia.
      // 1. Subtract 0x10000 from codepoint
      codepoint -= 0x10000;
      // 2. Split this into the high 10-bit value and the low 10-bit value
      // 3. Add 0xD800 to the high value to form the high surrogate
      // 4. Add 0xDC00 to the low value to form the low surrogate:
      const low = (codepoint & 0x3FF) + 0xDC00;
      const high = ((codepoint >> 10) & 0x3FF) + 0xD800;
      codePoints.push(high, low);
    }
  }
  return codePointsToString(codePoints);
}

/**
 * Writes a UTF16 JavaScript string to the buffer encoded as UTF8.
 * @param {string} value The string to write.
 * @return {!Uint8Array} An array containing the encoded bytes.
 */
function encode(value) {
  const buffer = [];

  for (let i = 0; i < value.length; i++) {
    const c1 = value.charCodeAt(i);

    if (c1 < 0x80) {
      buffer.push(c1);
    } else if (c1 < 0x800) {
      buffer.push((c1 >> 6) | 0xC0);
      buffer.push((c1 & 0x3F) | 0x80);
    } else if (c1 < 0xD800 || c1 >= 0xE000) {
      buffer.push((c1 >> 12) | 0xE0);
      buffer.push(((c1 >> 6) & 0x3F) | 0x80);
      buffer.push((c1 & 0x3F) | 0x80);
    } else {
      // surrogate pair
      i++;
      checkElementIndex(i, value.length);
      const c2 = value.charCodeAt(i);
      const paired = 0x10000 + (((c1 & 0x3FF) << 10) | (c2 & 0x3FF));
      buffer.push((paired >> 18) | 0xF0);
      buffer.push(((paired >> 12) & 0x3F) | 0x80);
      buffer.push(((paired >> 6) & 0x3F) | 0x80);
      buffer.push((paired & 0x3F) | 0x80);
    }
  }
  return new Uint8Array(buffer);
}

exports = {
  decode,
  encode,
};
