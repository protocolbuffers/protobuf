// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
 * Arrays.equals(bytes, new String(bytes, "UTF-8").getBytes("UTF-8"))
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
}
