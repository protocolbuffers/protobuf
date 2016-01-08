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

package com.google.protobuf;

/**
 * A set of low-level, high-performance static utility methods related
 * to the UTF-8 character encoding.  This class has no dependencies
 * outside of the core JDK libraries.
 *
 * <p>There are several variants of UTF-8.  The one implemented by
 * this class is the restricted definition of UTF-8 introduced in
 * Unicode 3.1, which mandates the rejection of "overlong" byte
 * sequences as well as rejection of 3-byte surrogate codepoint byte
 * sequences.  Note that the UTF-8 decoder included in Oracle's JDK
 * has been modified to also reject "overlong" byte sequences, but (as
 * of 2011) still accepts 3-byte surrogate codepoint byte sequences.
 *
 * <p>The byte sequences considered valid by this class are exactly
 * those that can be roundtrip converted to Strings and back to bytes
 * using the UTF-8 charset, without loss: <pre> {@code
 * Arrays.equals(bytes, new String(bytes, Internal.UTF_8).getBytes(Internal.UTF_8))
 * }</pre>
 *
 * <p>See the Unicode Standard,</br>
 * Table 3-6. <em>UTF-8 Bit Distribution</em>,</br>
 * Table 3-7. <em>Well Formed UTF-8 Byte Sequences</em>.
 *
 * <p>This class supports decoding of partial byte sequences, so that the
 * bytes in a complete UTF-8 byte sequences can be stored in multiple
 * segments.  Methods typically return {@link #MALFORMED} if the partial
 * byte sequence is definitely not well-formed, {@link #COMPLETE} if it is
 * well-formed in the absence of additional input, or if the byte sequence
 * apparently terminated in the middle of a character, an opaque integer
 * "state" value containing enough information to decode the character when
 * passed to a subsequent invocation of a partial decoding method.
 *
 * @author martinrb@google.com (Martin Buchholz)
 */
final class Utf8 {
  private Utf8() {}
  
  /**
   * Maximum number of bytes per Java UTF-16 char in UTF-8.
   * @see java.nio.charset.CharsetEncoder#maxBytesPerChar()
   */
  static final int MAX_BYTES_PER_CHAR = 3;

  /**
   * State value indicating that the byte sequence is well-formed and
   * complete (no further bytes are needed to complete a character).
   */
  public static final int COMPLETE = 0;

  /**
   * State value indicating that the byte sequence is definitely not
   * well-formed.
   */
  public static final int MALFORMED = -1;

  // Other state values include the partial bytes of the incomplete
  // character to be decoded in the simplest way: we pack the bytes
  // into the state int in little-endian order.  For example:
  //
  // int state = byte1 ^ (byte2 << 8) ^ (byte3 << 16);
  //
  // Such a state is unpacked thus (note the ~ operation for byte2 to
  // undo byte1's sign-extension bits):
  //
  // int byte1 = (byte) state;
  // int byte2 = (byte) ~(state >> 8);
  // int byte3 = (byte) (state >> 16);
  //
  // We cannot store a zero byte in the state because it would be
  // indistinguishable from the absence of a byte.  But we don't need
  // to, because partial bytes must always be negative.  When building
  // a state, we ensure that byte1 is negative and subsequent bytes
  // are valid trailing bytes.

  /**
   * Returns {@code true} if the given byte array is a well-formed
   * UTF-8 byte sequence.
   *
   * <p>This is a convenience method, equivalent to a call to {@code
   * isValidUtf8(bytes, 0, bytes.length)}.
   */
  public static boolean isValidUtf8(byte[] bytes) {
    return isValidUtf8(bytes, 0, bytes.length);
  }

  /**
   * Returns {@code true} if the given byte array slice is a
   * well-formed UTF-8 byte sequence.  The range of bytes to be
   * checked extends from index {@code index}, inclusive, to {@code
   * limit}, exclusive.
   *
   * <p>This is a convenience method, equivalent to {@code
   * partialIsValidUtf8(bytes, index, limit) == Utf8.COMPLETE}.
   */
  public static boolean isValidUtf8(byte[] bytes, int index, int limit) {
    return partialIsValidUtf8(bytes, index, limit) == COMPLETE;
  }

  /**
   * Tells whether the given byte array slice is a well-formed,
   * malformed, or incomplete UTF-8 byte sequence.  The range of bytes
   * to be checked extends from index {@code index}, inclusive, to
   * {@code limit}, exclusive.
   *
   * @param state either {@link Utf8#COMPLETE} (if this is the initial decoding
   * operation) or the value returned from a call to a partial decoding method
   * for the previous bytes
   *
   * @return {@link #MALFORMED} if the partial byte sequence is
   * definitely not well-formed, {@link #COMPLETE} if it is well-formed
   * (no additional input needed), or if the byte sequence is
   * "incomplete", i.e. apparently terminated in the middle of a character,
   * an opaque integer "state" value containing enough information to
   * decode the character when passed to a subsequent invocation of a
   * partial decoding method.
   */
  public static int partialIsValidUtf8(
      int state, byte[] bytes, int index, int limit) {
    if (state != COMPLETE) {
      // The previous decoding operation was incomplete (or malformed).
      // We look for a well-formed sequence consisting of bytes from
      // the previous decoding operation (stored in state) together
      // with bytes from the array slice.
      //
      // We expect such "straddler characters" to be rare.

      if (index >= limit) {  // No bytes? No progress.
        return state;
      }
      int byte1 = (byte) state;
      // byte1 is never ASCII.
      if (byte1 < (byte) 0xE0) {
        // two-byte form

        // Simultaneously checks for illegal trailing-byte in
        // leading position and overlong 2-byte form.
        if (byte1 < (byte) 0xC2 ||
            // byte2 trailing-byte test
            bytes[index++] > (byte) 0xBF) {
          return MALFORMED;
        }
      } else if (byte1 < (byte) 0xF0) {
        // three-byte form

        // Get byte2 from saved state or array
        int byte2 = (byte) ~(state >> 8);
        if (byte2 == 0) {
          byte2 = bytes[index++];
          if (index >= limit) {
            return incompleteStateFor(byte1, byte2);
          }
        }
        if (byte2 > (byte) 0xBF ||
            // overlong? 5 most significant bits must not all be zero
            (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0) ||
            // illegal surrogate codepoint?
            (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0) ||
            // byte3 trailing-byte test
            bytes[index++] > (byte) 0xBF) {
          return MALFORMED;
        }
      } else {
        // four-byte form

        // Get byte2 and byte3 from saved state or array
        int byte2 = (byte) ~(state >> 8);
        int byte3 = 0;
        if (byte2 == 0) {
          byte2 = bytes[index++];
          if (index >= limit) {
            return incompleteStateFor(byte1, byte2);
          }
        } else {
          byte3 = (byte) (state >> 16);
        }
        if (byte3 == 0) {
          byte3 = bytes[index++];
          if (index >= limit) {
            return incompleteStateFor(byte1, byte2, byte3);
          }
        }

        // If we were called with state == MALFORMED, then byte1 is 0xFF,
        // which never occurs in well-formed UTF-8, and so we will return
        // MALFORMED again below.

        if (byte2 > (byte) 0xBF ||
            // Check that 1 <= plane <= 16.  Tricky optimized form of:
            // if (byte1 > (byte) 0xF4 ||
            //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
            //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
            (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0 ||
            // byte3 trailing-byte test
            byte3 > (byte) 0xBF ||
            // byte4 trailing-byte test
             bytes[index++] > (byte) 0xBF) {
          return MALFORMED;
        }
      }
    }

    return partialIsValidUtf8(bytes, index, limit);
  }

  /**
   * Tells whether the given byte array slice is a well-formed,
   * malformed, or incomplete UTF-8 byte sequence.  The range of bytes
   * to be checked extends from index {@code index}, inclusive, to
   * {@code limit}, exclusive.
   *
   * <p>This is a convenience method, equivalent to a call to {@code
   * partialIsValidUtf8(Utf8.COMPLETE, bytes, index, limit)}.
   *
   * @return {@link #MALFORMED} if the partial byte sequence is
   * definitely not well-formed, {@link #COMPLETE} if it is well-formed
   * (no additional input needed), or if the byte sequence is
   * "incomplete", i.e. apparently terminated in the middle of a character,
   * an opaque integer "state" value containing enough information to
   * decode the character when passed to a subsequent invocation of a
   * partial decoding method.
   */
  public static int partialIsValidUtf8(
      byte[] bytes, int index, int limit) {
    // Optimize for 100% ASCII.
    // Hotspot loves small simple top-level loops like this.
    while (index < limit && bytes[index] >= 0) {
      index++;
    }

    return (index >= limit) ? COMPLETE :
        partialIsValidUtf8NonAscii(bytes, index, limit);
  }

  private static int partialIsValidUtf8NonAscii(
      byte[] bytes, int index, int limit) {
    for (;;) {
      int byte1, byte2;

      // Optimize for interior runs of ASCII bytes.
      do {
        if (index >= limit) {
          return COMPLETE;
        }
      } while ((byte1 = bytes[index++]) >= 0);

      if (byte1 < (byte) 0xE0) {
        // two-byte form

        if (index >= limit) {
          return byte1;
        }

        // Simultaneously checks for illegal trailing-byte in
        // leading position and overlong 2-byte form.
        if (byte1 < (byte) 0xC2 ||
            bytes[index++] > (byte) 0xBF) {
          return MALFORMED;
        }
      } else if (byte1 < (byte) 0xF0) {
        // three-byte form

        if (index >= limit - 1) { // incomplete sequence
          return incompleteStateFor(bytes, index, limit);
        }
        if ((byte2 = bytes[index++]) > (byte) 0xBF ||
            // overlong? 5 most significant bits must not all be zero
            (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0) ||
            // check for illegal surrogate codepoints
            (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0) ||
            // byte3 trailing-byte test
            bytes[index++] > (byte) 0xBF) {
          return MALFORMED;
        }
      } else {
        // four-byte form

        if (index >= limit - 2) {  // incomplete sequence
          return incompleteStateFor(bytes, index, limit);
        }
        if ((byte2 = bytes[index++]) > (byte) 0xBF ||
            // Check that 1 <= plane <= 16.  Tricky optimized form of:
            // if (byte1 > (byte) 0xF4 ||
            //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
            //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
            (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0 ||
            // byte3 trailing-byte test
            bytes[index++] > (byte) 0xBF ||
            // byte4 trailing-byte test
            bytes[index++] > (byte) 0xBF) {
          return MALFORMED;
        }
      }
    }
  }

  private static int incompleteStateFor(int byte1) {
    return (byte1 > (byte) 0xF4) ?
        MALFORMED : byte1;
  }

  private static int incompleteStateFor(int byte1, int byte2) {
    return (byte1 > (byte) 0xF4 ||
            byte2 > (byte) 0xBF) ?
        MALFORMED : byte1 ^ (byte2 << 8);
  }

  private static int incompleteStateFor(int byte1, int byte2, int byte3) {
    return (byte1 > (byte) 0xF4 ||
            byte2 > (byte) 0xBF ||
            byte3 > (byte) 0xBF) ?
        MALFORMED : byte1 ^ (byte2 << 8) ^ (byte3 << 16);
  }

  private static int incompleteStateFor(byte[] bytes, int index, int limit) {
    int byte1 = bytes[index - 1];
    switch (limit - index) {
      case 0: return incompleteStateFor(byte1);
      case 1: return incompleteStateFor(byte1, bytes[index]);
      case 2: return incompleteStateFor(byte1, bytes[index], bytes[index + 1]);
      default: throw new AssertionError();
    }
  }
  

  // These UTF-8 handling methods are copied from Guava's Utf8 class with a modification to throw
  // a protocol buffer local exception. This exception is then caught in CodedOutputStream so it can
  // fallback to more lenient behavior.
  
  static class UnpairedSurrogateException extends IllegalArgumentException {
    
    private UnpairedSurrogateException(int index, int length) {
      super("Unpaired surrogate at index " + index + " of " + length);
    }
  }
  
  /**
   * Returns the number of bytes in the UTF-8-encoded form of {@code sequence}. For a string,
   * this method is equivalent to {@code string.getBytes(UTF_8).length}, but is more efficient in
   * both time and space.
   *
   * @throws IllegalArgumentException if {@code sequence} contains ill-formed UTF-16 (unpaired
   *     surrogates)
   */
  static int encodedLength(CharSequence sequence) {
    // Warning to maintainers: this implementation is highly optimized.
    int utf16Length = sequence.length();
    int utf8Length = utf16Length;
    int i = 0;

    // This loop optimizes for pure ASCII.
    while (i < utf16Length && sequence.charAt(i) < 0x80) {
      i++;
    }

    // This loop optimizes for chars less than 0x800.
    for (; i < utf16Length; i++) {
      char c = sequence.charAt(i);
      if (c < 0x800) {
        utf8Length += ((0x7f - c) >>> 31);  // branch free!
      } else {
        utf8Length += encodedLengthGeneral(sequence, i);
        break;
      }
    }

    if (utf8Length < utf16Length) {
      // Necessary and sufficient condition for overflow because of maximum 3x expansion
      throw new IllegalArgumentException("UTF-8 length does not fit in int: "
              + (utf8Length + (1L << 32)));
    }
    return utf8Length;
  }

  private static int encodedLengthGeneral(CharSequence sequence, int start) {
    int utf16Length = sequence.length();
    int utf8Length = 0;
    for (int i = start; i < utf16Length; i++) {
      char c = sequence.charAt(i);
      if (c < 0x800) {
        utf8Length += (0x7f - c) >>> 31; // branch free!
      } else {
        utf8Length += 2;
        // jdk7+: if (Character.isSurrogate(c)) {
        if (Character.MIN_SURROGATE <= c && c <= Character.MAX_SURROGATE) {
          // Check that we have a well-formed surrogate pair.
          int cp = Character.codePointAt(sequence, i);
          if (cp < Character.MIN_SUPPLEMENTARY_CODE_POINT) {
            throw new UnpairedSurrogateException(i, utf16Length);
          }
          i++;
        }
      }
    }
    return utf8Length;
  }

  static int encode(CharSequence sequence, byte[] bytes, int offset, int length) {
    int utf16Length = sequence.length();
    int j = offset;
    int i = 0;
    int limit = offset + length;
    // Designed to take advantage of
    // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
    for (char c; i < utf16Length && i + j < limit && (c = sequence.charAt(i)) < 0x80; i++) {
      bytes[j + i] = (byte) c;
    }
    if (i == utf16Length) {
      return j + utf16Length;
    }
    j += i;
    for (char c; i < utf16Length; i++) {
      c = sequence.charAt(i);
      if (c < 0x80 && j < limit) {
        bytes[j++] = (byte) c;
      } else if (c < 0x800 && j <= limit - 2) { // 11 bits, two UTF-8 bytes
        bytes[j++] = (byte) ((0xF << 6) | (c >>> 6));
        bytes[j++] = (byte) (0x80 | (0x3F & c));
      } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c) && j <= limit - 3) {
        // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
        bytes[j++] = (byte) ((0xF << 5) | (c >>> 12));
        bytes[j++] = (byte) (0x80 | (0x3F & (c >>> 6)));
        bytes[j++] = (byte) (0x80 | (0x3F & c));
      } else if (j <= limit - 4) {
        // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8 bytes
        final char low;
        if (i + 1 == sequence.length()
                || !Character.isSurrogatePair(c, (low = sequence.charAt(++i)))) {
          throw new UnpairedSurrogateException((i - 1), utf16Length);
        }
        int codePoint = Character.toCodePoint(c, low);
        bytes[j++] = (byte) ((0xF << 4) | (codePoint >>> 18));
        bytes[j++] = (byte) (0x80 | (0x3F & (codePoint >>> 12)));
        bytes[j++] = (byte) (0x80 | (0x3F & (codePoint >>> 6)));
        bytes[j++] = (byte) (0x80 | (0x3F & codePoint));
      } else {
        // If we are surrogates and we're not a surrogate pair, always throw an
        // IllegalArgumentException instead of an ArrayOutOfBoundsException.
        if ((Character.MIN_SURROGATE <= c && c <= Character.MAX_SURROGATE)
            && (i + 1 == sequence.length()
                || !Character.isSurrogatePair(c, sequence.charAt(i + 1)))) {
          throw new UnpairedSurrogateException(i, utf16Length);
        }
        throw new ArrayIndexOutOfBoundsException("Failed writing " + c + " at index " + j);
      }
    }
    return j;
  }
  // End Guava UTF-8 methods.
}
