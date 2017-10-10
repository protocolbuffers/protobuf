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

import static com.google.protobuf.UnsafeUtil.addressOffset;
import static com.google.protobuf.UnsafeUtil.hasUnsafeArrayOperations;
import static com.google.protobuf.UnsafeUtil.hasUnsafeByteBufferOperations;
import static java.lang.Character.MAX_SURROGATE;
import static java.lang.Character.MIN_SURROGATE;
import static java.lang.Character.isSurrogatePair;
import static java.lang.Character.toCodePoint;

import java.nio.ByteBuffer;

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
// TODO(nathanmittler): Copy changes in this class back to Guava
final class Utf8 {

  /**
   * UTF-8 is a runtime hot spot so we attempt to provide heavily optimized implementations
   * depending on what is available on the platform. The processor is the platform-optimized
   * delegate for which all methods are delegated directly to.
   */
  private static final Processor processor =
      UnsafeProcessor.isAvailable() ? new UnsafeProcessor() : new SafeProcessor();

  /**
   * A mask used when performing unsafe reads to determine if a long value contains any non-ASCII
   * characters (i.e. any byte >= 0x80).
   */
  private static final long ASCII_MASK_LONG = 0x8080808080808080L;

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

  /**
   * Used by {@code Unsafe} UTF-8 string validation logic to determine the minimum string length
   * above which to employ an optimized algorithm for counting ASCII characters. The reason for this
   * threshold is that for small strings, the optimization may not be beneficial or may even
   * negatively impact performance since it requires additional logic to avoid unaligned reads
   * (when calling {@code Unsafe.getLong}). This threshold guarantees that even if the initial
   * offset is unaligned, we're guaranteed to make at least one call to {@code Unsafe.getLong()}
   * which provides a performance improvement that entirely subsumes the cost of the additional
   * logic.
   */
  private static final int UNSAFE_COUNT_ASCII_THRESHOLD = 16;

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
    return processor.isValidUtf8(bytes, 0, bytes.length);
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
    return processor.isValidUtf8(bytes, index, limit);
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
  public static int partialIsValidUtf8(int state, byte[] bytes, int index, int limit) {
    return processor.partialIsValidUtf8(state, bytes, index, limit);
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

  private static int incompleteStateFor(
      final ByteBuffer buffer, final int byte1, final int index, final int remaining) {
    switch (remaining) {
      case 0:
        return incompleteStateFor(byte1);
      case 1:
        return incompleteStateFor(byte1, buffer.get(index));
      case 2:
        return incompleteStateFor(byte1, buffer.get(index), buffer.get(index + 1));
      default:
        throw new AssertionError();
    }
  }

  // These UTF-8 handling methods are copied from Guava's Utf8 class with a modification to throw
  // a protocol buffer local exception. This exception is then caught in CodedOutputStream so it can
  // fallback to more lenient behavior.
  
  static class UnpairedSurrogateException extends IllegalArgumentException {
    UnpairedSurrogateException(int index, int length) {
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

  static int encode(CharSequence in, byte[] out, int offset, int length) {
    return processor.encodeUtf8(in, out, offset, length);
  }
  // End Guava UTF-8 methods.

  /**
   * Determines if the given {@link ByteBuffer} is a valid UTF-8 string.
   *
   * <p>Selects an optimal algorithm based on the type of {@link ByteBuffer} (i.e. heap or direct)
   * and the capabilities of the platform.
   *
   * @param buffer the buffer to check.
   * @see Utf8#isValidUtf8(byte[], int, int)
   */
  static boolean isValidUtf8(ByteBuffer buffer) {
    return processor.isValidUtf8(buffer, buffer.position(), buffer.remaining());
  }

  /**
   * Determines if the given {@link ByteBuffer} is a partially valid UTF-8 string.
   *
   * <p>Selects an optimal algorithm based on the type of {@link ByteBuffer} (i.e. heap or direct)
   * and the capabilities of the platform.
   *
   * @param buffer the buffer to check.
   * @see Utf8#partialIsValidUtf8(int, byte[], int, int)
   */
  static int partialIsValidUtf8(int state, ByteBuffer buffer, int index, int limit) {
    return processor.partialIsValidUtf8(state, buffer, index, limit);
  }

  /**
   * Encodes the given characters to the target {@link ByteBuffer} using UTF-8 encoding.
   *
   * <p>Selects an optimal algorithm based on the type of {@link ByteBuffer} (i.e. heap or direct)
   * and the capabilities of the platform.
   *
   * @param in the source string to be encoded
   * @param out the target buffer to receive the encoded string.
   * @see Utf8#encode(CharSequence, byte[], int, int)
   */
  static void encodeUtf8(CharSequence in, ByteBuffer out) {
    processor.encodeUtf8(in, out);
  }

  /**
   * Counts (approximately) the number of consecutive ASCII characters in the given buffer.
   * The byte order of the {@link ByteBuffer} does not matter, so performance can be improved if
   * native byte order is used (i.e. no byte-swapping in {@link ByteBuffer#getLong(int)}).
   *
   * @param buffer the buffer to be scanned for ASCII chars
   * @param index the starting index of the scan
   * @param limit the limit within buffer for the scan
   * @return the number of ASCII characters found. The stopping position will be at or
   * before the first non-ASCII byte.
   */
  private static int estimateConsecutiveAscii(ByteBuffer buffer, int index, int limit) {
    int i = index;
    final int lim = limit - 7;
    // This simple loop stops when we encounter a byte >= 0x80 (i.e. non-ASCII).
    // To speed things up further, we're reading longs instead of bytes so we use a mask to
    // determine if any byte in the current long is non-ASCII.
    for (; i < lim && (buffer.getLong(i) & ASCII_MASK_LONG) == 0; i += 8) {}
    return i - index;
  }

  /**
   * A processor of UTF-8 strings, providing methods for checking validity and encoding.
   */
  // TODO(nathanmittler): Add support for Memory/MemoryBlock on Android.
  abstract static class Processor {
    /**
     * Returns {@code true} if the given byte array slice is a
     * well-formed UTF-8 byte sequence.  The range of bytes to be
     * checked extends from index {@code index}, inclusive, to {@code
     * limit}, exclusive.
     *
     * <p>This is a convenience method, equivalent to {@code
     * partialIsValidUtf8(bytes, index, limit) == Utf8.COMPLETE}.
     */
    final boolean isValidUtf8(byte[] bytes, int index, int limit) {
      return partialIsValidUtf8(COMPLETE, bytes, index, limit) == COMPLETE;
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
    abstract int partialIsValidUtf8(int state, byte[] bytes, int index, int limit);

    /**
     * Returns {@code true} if the given portion of the {@link ByteBuffer} is a
     * well-formed UTF-8 byte sequence.  The range of bytes to be
     * checked extends from index {@code index}, inclusive, to {@code
     * limit}, exclusive.
     *
     * <p>This is a convenience method, equivalent to {@code
     * partialIsValidUtf8(bytes, index, limit) == Utf8.COMPLETE}.
     */
    final boolean isValidUtf8(ByteBuffer buffer, int index, int limit) {
      return partialIsValidUtf8(COMPLETE, buffer, index, limit) == COMPLETE;
    }

    /**
     * Indicates whether or not the given buffer contains a valid UTF-8 string.
     *
     * @param buffer the buffer to check.
     * @return {@code true} if the given buffer contains a valid UTF-8 string.
     */
    final int partialIsValidUtf8(
        final int state, final ByteBuffer buffer, int index, final int limit) {
      if (buffer.hasArray()) {
        final int offset = buffer.arrayOffset();
        return partialIsValidUtf8(state, buffer.array(), offset + index, offset + limit);
      } else if (buffer.isDirect()){
        return partialIsValidUtf8Direct(state, buffer, index, limit);
      }
      return partialIsValidUtf8Default(state, buffer, index, limit);
    }

    /**
     * Performs validation for direct {@link ByteBuffer} instances.
     */
    abstract int partialIsValidUtf8Direct(
        final int state, final ByteBuffer buffer, int index, final int limit);

    /**
     * Performs validation for {@link ByteBuffer} instances using the {@link ByteBuffer} API rather
     * than potentially faster approaches. This first completes validation for the current
     * character (provided by {@code state}) and then finishes validation for the sequence.
     */
    final int partialIsValidUtf8Default(
        final int state, final ByteBuffer buffer, int index, final int limit) {
      if (state != COMPLETE) {
        // The previous decoding operation was incomplete (or malformed).
        // We look for a well-formed sequence consisting of bytes from
        // the previous decoding operation (stored in state) together
        // with bytes from the array slice.
        //
        // We expect such "straddler characters" to be rare.

        if (index >= limit) { // No bytes? No progress.
          return state;
        }

        byte byte1 = (byte) state;
        // byte1 is never ASCII.
        if (byte1 < (byte) 0xE0) {
          // two-byte form

          // Simultaneously checks for illegal trailing-byte in
          // leading position and overlong 2-byte form.
          if (byte1 < (byte) 0xC2
              // byte2 trailing-byte test
              || buffer.get(index++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else if (byte1 < (byte) 0xF0) {
          // three-byte form

          // Get byte2 from saved state or array
          byte byte2 = (byte) ~(state >> 8);
          if (byte2 == 0) {
            byte2 = buffer.get(index++);
            if (index >= limit) {
              return incompleteStateFor(byte1, byte2);
            }
          }
          if (byte2 > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // illegal surrogate codepoint?
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || buffer.get(index++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else {
          // four-byte form

          // Get byte2 and byte3 from saved state or array
          byte byte2 = (byte) ~(state >> 8);
          byte byte3 = 0;
          if (byte2 == 0) {
            byte2 = buffer.get(index++);
            if (index >= limit) {
              return incompleteStateFor(byte1, byte2);
            }
          } else {
            byte3 = (byte) (state >> 16);
          }
          if (byte3 == 0) {
            byte3 = buffer.get(index++);
            if (index >= limit) {
              return incompleteStateFor(byte1, byte2, byte3);
            }
          }

          // If we were called with state == MALFORMED, then byte1 is 0xFF,
          // which never occurs in well-formed UTF-8, and so we will return
          // MALFORMED again below.

          if (byte2 > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || byte3 > (byte) 0xBF
              // byte4 trailing-byte test
              || buffer.get(index++) > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }

      // Finish validation for the sequence.
      return partialIsValidUtf8(buffer, index, limit);
    }

    /**
     * Performs validation for {@link ByteBuffer} instances using the {@link ByteBuffer} API rather
     * than potentially faster approaches.
     */
    private static int partialIsValidUtf8(final ByteBuffer buffer, int index, final int limit) {
      index += estimateConsecutiveAscii(buffer, index, limit);

      for (;;) {
        // Optimize for interior runs of ASCII bytes.
        // TODO(nathanmittler): Consider checking 8 bytes at a time after some threshold?
        // Maybe after seeing a few in a row that are ASCII, go back to fast mode?
        int byte1;
        do {
          if (index >= limit) {
            return COMPLETE;
          }
        } while ((byte1 = buffer.get(index++)) >= 0);

        // If we're here byte1 is not ASCII. Only need to handle 2-4 byte forms.
        if (byte1 < (byte) 0xE0) {
          // Two-byte form (110xxxxx 10xxxxxx)
          if (index >= limit) {
            // Incomplete sequence
            return byte1;
          }

          // Simultaneously checks for illegal trailing-byte in
          // leading position and overlong 2-byte form.
          if (byte1 < (byte) 0xC2 || buffer.get(index) > (byte) 0xBF) {
            return MALFORMED;
          }
          index++;
        } else if (byte1 < (byte) 0xF0) {
          // Three-byte form (1110xxxx 10xxxxxx 10xxxxxx)
          if (index >= limit - 1) {
            // Incomplete sequence
            return incompleteStateFor(buffer, byte1, index, limit - index);
          }

          final byte byte2 = buffer.get(index++);
          if (byte2 > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // check for illegal surrogate codepoints
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || buffer.get(index) > (byte) 0xBF) {
            return MALFORMED;
          }
          index++;
        } else {
          // Four-byte form (1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx)
          if (index >= limit - 2) {
            // Incomplete sequence
            return incompleteStateFor(buffer, byte1, index, limit - index);
          }

          // TODO(nathanmittler): Consider using getInt() to improve performance.
          final int byte2 = buffer.get(index++);
          if (byte2 > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || buffer.get(index++) > (byte) 0xBF
              // byte4 trailing-byte test
              || buffer.get(index++) > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }
    }

    /**
     * Encodes an input character sequence ({@code in}) to UTF-8 in the target array ({@code out}).
     * For a string, this method is similar to
     * <pre>{@code
     * byte[] a = string.getBytes(UTF_8);
     * System.arraycopy(a, 0, bytes, offset, a.length);
     * return offset + a.length;
     * }</pre>
     *
     * but is more efficient in both time and space. One key difference is that this method
     * requires paired surrogates, and therefore does not support chunking.
     * While {@code String.getBytes(UTF_8)} replaces unpaired surrogates with the default
     * replacement character, this method throws {@link UnpairedSurrogateException}.
     *
     * <p>To ensure sufficient space in the output buffer, either call {@link #encodedLength} to
     * compute the exact amount needed, or leave room for 
     * {@code Utf8.MAX_BYTES_PER_CHAR * sequence.length()}, which is the largest possible number
     * of bytes that any input can be encoded to.
     *
     * @param in the input character sequence to be encoded
     * @param out the target array
     * @param offset the starting offset in {@code bytes} to start writing at
     * @param length the length of the {@code bytes}, starting from {@code offset}
     * @throws UnpairedSurrogateException if {@code sequence} contains ill-formed UTF-16 (unpaired
     *     surrogates)
     * @throws ArrayIndexOutOfBoundsException if {@code sequence} encoded in UTF-8 is longer than
     *     {@code bytes.length - offset}
     * @return the new offset, equivalent to {@code offset + Utf8.encodedLength(sequence)}
     */
    abstract int encodeUtf8(CharSequence in, byte[] out, int offset, int length);

    /**
     * Encodes an input character sequence ({@code in}) to UTF-8 in the target buffer ({@code out}).
     * Upon returning from this method, the {@code out} position will point to the position after
     * the last encoded byte. This method requires paired surrogates, and therefore does not
     * support chunking.
     *
     * <p>To ensure sufficient space in the output buffer, either call {@link #encodedLength} to
     * compute the exact amount needed, or leave room for
     * {@code Utf8.MAX_BYTES_PER_CHAR * in.length()}, which is the largest possible number
     * of bytes that any input can be encoded to.
     *
     * @param in the source character sequence to be encoded
     * @param out the target buffer
     * @throws UnpairedSurrogateException if {@code in} contains ill-formed UTF-16 (unpaired
     *     surrogates)
     * @throws ArrayIndexOutOfBoundsException if {@code in} encoded in UTF-8 is longer than
     *     {@code out.remaining()}
     */
    final void encodeUtf8(CharSequence in, ByteBuffer out) {
      if (out.hasArray()) {
        final int offset = out.arrayOffset();
        int endIndex =
            Utf8.encode(in, out.array(), offset + out.position(), out.remaining());
        out.position(endIndex - offset);
      } else if (out.isDirect()) {
        encodeUtf8Direct(in, out);
      } else {
        encodeUtf8Default(in, out);
      }
    }

    /**
     * Encodes the input character sequence to a direct {@link ByteBuffer} instance.
     */
    abstract void encodeUtf8Direct(CharSequence in, ByteBuffer out);

    /**
     * Encodes the input character sequence to a {@link ByteBuffer} instance using the {@link
     * ByteBuffer} API, rather than potentially faster approaches.
     */
    final void encodeUtf8Default(CharSequence in, ByteBuffer out) {
      final int inLength = in.length();
      int outIx = out.position();
      int inIx = 0;

      // Since ByteBuffer.putXXX() already checks boundaries for us, no need to explicitly check
      // access. Assume the buffer is big enough and let it handle the out of bounds exception
      // if it occurs.
      try {
        // Designed to take advantage of
        // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
        for (char c; inIx < inLength && (c = in.charAt(inIx)) < 0x80; ++inIx) {
          out.put(outIx + inIx, (byte) c);
        }
        if (inIx == inLength) {
          // Successfully encoded the entire string.
          out.position(outIx + inIx);
          return;
        }

        outIx += inIx;
        for (char c; inIx < inLength; ++inIx, ++outIx) {
          c = in.charAt(inIx);
          if (c < 0x80) {
            // One byte (0xxx xxxx)
            out.put(outIx, (byte) c);
          } else if (c < 0x800) {
            // Two bytes (110x xxxx 10xx xxxx)

            // Benchmarks show put performs better than putShort here (for HotSpot).
            out.put(outIx++, (byte) (0xC0 | (c >>> 6)));
            out.put(outIx, (byte) (0x80 | (0x3F & c)));
          } else if (c < MIN_SURROGATE || MAX_SURROGATE < c) {
            // Three bytes (1110 xxxx 10xx xxxx 10xx xxxx)
            // Maximum single-char code point is 0xFFFF, 16 bits.

            // Benchmarks show put performs better than putShort here (for HotSpot).
            out.put(outIx++, (byte) (0xE0 | (c >>> 12)));
            out.put(outIx++, (byte) (0x80 | (0x3F & (c >>> 6))));
            out.put(outIx, (byte) (0x80 | (0x3F & c)));
          } else {
            // Four bytes (1111 xxxx 10xx xxxx 10xx xxxx 10xx xxxx)

            // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8
            // bytes
            final char low;
            if (inIx + 1 == inLength || !isSurrogatePair(c, (low = in.charAt(++inIx)))) {
              throw new UnpairedSurrogateException(inIx, inLength);
            }
            // TODO(nathanmittler): Consider using putInt() to improve performance.
            int codePoint = toCodePoint(c, low);
            out.put(outIx++, (byte) ((0xF << 4) | (codePoint >>> 18)));
            out.put(outIx++, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
            out.put(outIx++, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
            out.put(outIx, (byte) (0x80 | (0x3F & codePoint)));
          }
        }

        // Successfully encoded the entire string.
        out.position(outIx);
      } catch (IndexOutOfBoundsException e) {
        // TODO(nathanmittler): Consider making the API throw IndexOutOfBoundsException instead.

        // If we failed in the outer ASCII loop, outIx will not have been updated. In this case,
        // use inIx to determine the bad write index.
        int badWriteIndex = out.position() + Math.max(inIx, outIx - out.position() + 1);
        throw new ArrayIndexOutOfBoundsException(
            "Failed writing " + in.charAt(inIx) + " at index " + badWriteIndex);
      }
    }
  }

  /**
   * {@link Processor} implementation that does not use any {@code sun.misc.Unsafe} methods.
   */
  static final class SafeProcessor extends Processor {
    @Override
    int partialIsValidUtf8(int state, byte[] bytes, int index, int limit) {
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
          if (byte1 < (byte) 0xC2
              // byte2 trailing-byte test
              || bytes[index++] > (byte) 0xBF) {
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
          if (byte2 > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // illegal surrogate codepoint?
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || bytes[index++] > (byte) 0xBF) {
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

          if (byte2 > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || byte3 > (byte) 0xBF
              // byte4 trailing-byte test
              || bytes[index++] > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }

      return partialIsValidUtf8(bytes, index, limit);
    }

    @Override
    int partialIsValidUtf8Direct(int state, ByteBuffer buffer, int index, int limit) {
      // For safe processing, we have to use the ByteBuffer API.
      return partialIsValidUtf8Default(state, buffer, index, limit);
    }

    @Override
    int encodeUtf8(CharSequence in, byte[] out, int offset, int length) {
      int utf16Length = in.length();
      int j = offset;
      int i = 0;
      int limit = offset + length;
      // Designed to take advantage of
      // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
      for (char c; i < utf16Length && i + j < limit && (c = in.charAt(i)) < 0x80; i++) {
        out[j + i] = (byte) c;
      }
      if (i == utf16Length) {
        return j + utf16Length;
      }
      j += i;
      for (char c; i < utf16Length; i++) {
        c = in.charAt(i);
        if (c < 0x80 && j < limit) {
          out[j++] = (byte) c;
        } else if (c < 0x800 && j <= limit - 2) { // 11 bits, two UTF-8 bytes
          out[j++] = (byte) ((0xF << 6) | (c >>> 6));
          out[j++] = (byte) (0x80 | (0x3F & c));
        } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c) && j <= limit - 3) {
          // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
          out[j++] = (byte) ((0xF << 5) | (c >>> 12));
          out[j++] = (byte) (0x80 | (0x3F & (c >>> 6)));
          out[j++] = (byte) (0x80 | (0x3F & c));
        } else if (j <= limit - 4) {
          // Minimum code point represented by a surrogate pair is 0x10000, 17 bits,
          // four UTF-8 bytes
          final char low;
          if (i + 1 == in.length()
                  || !Character.isSurrogatePair(c, (low = in.charAt(++i)))) {
            throw new UnpairedSurrogateException((i - 1), utf16Length);
          }
          int codePoint = Character.toCodePoint(c, low);
          out[j++] = (byte) ((0xF << 4) | (codePoint >>> 18));
          out[j++] = (byte) (0x80 | (0x3F & (codePoint >>> 12)));
          out[j++] = (byte) (0x80 | (0x3F & (codePoint >>> 6)));
          out[j++] = (byte) (0x80 | (0x3F & codePoint));
        } else {
          // If we are surrogates and we're not a surrogate pair, always throw an
          // UnpairedSurrogateException instead of an ArrayOutOfBoundsException.
          if ((Character.MIN_SURROGATE <= c && c <= Character.MAX_SURROGATE)
              && (i + 1 == in.length()
                  || !Character.isSurrogatePair(c, in.charAt(i + 1)))) {
            throw new UnpairedSurrogateException(i, utf16Length);
          }
          throw new ArrayIndexOutOfBoundsException("Failed writing " + c + " at index " + j);
        }
      }
      return j;
    }

    @Override
    void encodeUtf8Direct(CharSequence in, ByteBuffer out) {
      // For safe processing, we have to use the ByteBuffer API.
      encodeUtf8Default(in, out);
    }

    private static int partialIsValidUtf8(byte[] bytes, int index, int limit) {
      // Optimize for 100% ASCII (Hotspot loves small simple top-level loops like this).
      // This simple loop stops when we encounter a byte >= 0x80 (i.e. non-ASCII).
      while (index < limit && bytes[index] >= 0) {
        index++;
      }

      return (index >= limit) ? COMPLETE : partialIsValidUtf8NonAscii(bytes, index, limit);
    }

    private static int partialIsValidUtf8NonAscii(byte[] bytes, int index, int limit) {
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
            // Incomplete sequence
            return byte1;
          }

          // Simultaneously checks for illegal trailing-byte in
          // leading position and overlong 2-byte form.
          if (byte1 < (byte) 0xC2
              || bytes[index++] > (byte) 0xBF) {
            return MALFORMED;
          }
        } else if (byte1 < (byte) 0xF0) {
          // three-byte form

          if (index >= limit - 1) { // incomplete sequence
            return incompleteStateFor(bytes, index, limit);
          }
          if ((byte2 = bytes[index++]) > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // check for illegal surrogate codepoints
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || bytes[index++] > (byte) 0xBF) {
            return MALFORMED;
          }
        } else {
          // four-byte form

          if (index >= limit - 2) {  // incomplete sequence
            return incompleteStateFor(bytes, index, limit);
          }
          if ((byte2 = bytes[index++]) > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || bytes[index++] > (byte) 0xBF
              // byte4 trailing-byte test
              || bytes[index++] > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }
    }
  }

  /**
   * {@link Processor} that uses {@code sun.misc.Unsafe} where possible to improve performance.
   */
  static final class UnsafeProcessor extends Processor {
    /**
     * Indicates whether or not all required unsafe operations are supported on this platform.
     */
    static boolean isAvailable() {
      return hasUnsafeArrayOperations() && hasUnsafeByteBufferOperations();
    }

    @Override
    int partialIsValidUtf8(int state, byte[] bytes, final int index, final int limit) {
      if ((index | limit | bytes.length - limit) < 0) {
        throw new ArrayIndexOutOfBoundsException(
            String.format("Array length=%d, index=%d, limit=%d", bytes.length, index, limit));
      }
      long offset = index;
      final long offsetLimit = limit;
      if (state != COMPLETE) {
        // The previous decoding operation was incomplete (or malformed).
        // We look for a well-formed sequence consisting of bytes from
        // the previous decoding operation (stored in state) together
        // with bytes from the array slice.
        //
        // We expect such "straddler characters" to be rare.

        if (offset >= offsetLimit) {  // No bytes? No progress.
          return state;
        }
        int byte1 = (byte) state;
        // byte1 is never ASCII.
        if (byte1 < (byte) 0xE0) {
          // two-byte form

          // Simultaneously checks for illegal trailing-byte in
          // leading position and overlong 2-byte form.
          if (byte1 < (byte) 0xC2
              // byte2 trailing-byte test
              || UnsafeUtil.getByte(bytes, offset++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else if (byte1 < (byte) 0xF0) {
          // three-byte form

          // Get byte2 from saved state or array
          int byte2 = (byte) ~(state >> 8);
          if (byte2 == 0) {
            byte2 = UnsafeUtil.getByte(bytes, offset++);
            if (offset >= offsetLimit) {
              return incompleteStateFor(byte1, byte2);
            }
          }
          if (byte2 > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // illegal surrogate codepoint?
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || UnsafeUtil.getByte(bytes, offset++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else {
          // four-byte form

          // Get byte2 and byte3 from saved state or array
          int byte2 = (byte) ~(state >> 8);
          int byte3 = 0;
          if (byte2 == 0) {
            byte2 = UnsafeUtil.getByte(bytes, offset++);
            if (offset >= offsetLimit) {
              return incompleteStateFor(byte1, byte2);
            }
          } else {
            byte3 = (byte) (state >> 16);
          }
          if (byte3 == 0) {
            byte3 = UnsafeUtil.getByte(bytes, offset++);
            if (offset >= offsetLimit) {
              return incompleteStateFor(byte1, byte2, byte3);
            }
          }

          // If we were called with state == MALFORMED, then byte1 is 0xFF,
          // which never occurs in well-formed UTF-8, and so we will return
          // MALFORMED again below.

          if (byte2 > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || byte3 > (byte) 0xBF
              // byte4 trailing-byte test
              || UnsafeUtil.getByte(bytes, offset++) > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }

      return partialIsValidUtf8(bytes, offset, (int) (offsetLimit - offset));
    }

    @Override
    int partialIsValidUtf8Direct(
        final int state, ByteBuffer buffer, final int index, final int limit) {
      if ((index | limit | buffer.limit() - limit) < 0) {
        throw new ArrayIndexOutOfBoundsException(
            String.format("buffer limit=%d, index=%d, limit=%d", buffer.limit(), index, limit));
      }
      long address = addressOffset(buffer) + index;
      final long addressLimit = address + (limit - index);
      if (state != COMPLETE) {
        // The previous decoding operation was incomplete (or malformed).
        // We look for a well-formed sequence consisting of bytes from
        // the previous decoding operation (stored in state) together
        // with bytes from the array slice.
        //
        // We expect such "straddler characters" to be rare.

        if (address >= addressLimit) { // No bytes? No progress.
          return state;
        }

        final int byte1 = (byte) state;
        // byte1 is never ASCII.
        if (byte1 < (byte) 0xE0) {
          // two-byte form

          // Simultaneously checks for illegal trailing-byte in
          // leading position and overlong 2-byte form.
          if (byte1 < (byte) 0xC2
              // byte2 trailing-byte test
              || UnsafeUtil.getByte(address++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else if (byte1 < (byte) 0xF0) {
          // three-byte form

          // Get byte2 from saved state or array
          int byte2 = (byte) ~(state >> 8);
          if (byte2 == 0) {
            byte2 = UnsafeUtil.getByte(address++);
            if (address >= addressLimit) {
              return incompleteStateFor(byte1, byte2);
            }
          }
          if (byte2 > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // illegal surrogate codepoint?
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || UnsafeUtil.getByte(address++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else {
          // four-byte form

          // Get byte2 and byte3 from saved state or array
          int byte2 = (byte) ~(state >> 8);
          int byte3 = 0;
          if (byte2 == 0) {
            byte2 = UnsafeUtil.getByte(address++);
            if (address >= addressLimit) {
              return incompleteStateFor(byte1, byte2);
            }
          } else {
            byte3 = (byte) (state >> 16);
          }
          if (byte3 == 0) {
            byte3 = UnsafeUtil.getByte(address++);
            if (address >= addressLimit) {
              return incompleteStateFor(byte1, byte2, byte3);
            }
          }

          // If we were called with state == MALFORMED, then byte1 is 0xFF,
          // which never occurs in well-formed UTF-8, and so we will return
          // MALFORMED again below.

          if (byte2 > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || byte3 > (byte) 0xBF
              // byte4 trailing-byte test
              || UnsafeUtil.getByte(address++) > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }

      return partialIsValidUtf8(address, (int) (addressLimit - address));
    }

    @Override
    int encodeUtf8(final CharSequence in, final byte[] out, final int offset, final int length) {
      long outIx = offset;
      final long outLimit = outIx + length;
      final int inLimit = in.length();
      if (inLimit > length || out.length - length < offset) {
        // Not even enough room for an ASCII-encoded string.
        throw new ArrayIndexOutOfBoundsException(
            "Failed writing " + in.charAt(inLimit - 1) + " at index " + (offset + length));
      }

      // Designed to take advantage of
      // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
      int inIx = 0;
      for (char c; inIx < inLimit && (c = in.charAt(inIx)) < 0x80; ++inIx) {
        UnsafeUtil.putByte(out, outIx++, (byte) c);
      }
      if (inIx == inLimit) {
        // We're done, it was ASCII encoded.
        return (int) outIx;
      }

      for (char c; inIx < inLimit; ++inIx) {
        c = in.charAt(inIx);
        if (c < 0x80 && outIx < outLimit) {
          UnsafeUtil.putByte(out, outIx++, (byte) c);
        } else if (c < 0x800 && outIx <= outLimit - 2L) { // 11 bits, two UTF-8 bytes
          UnsafeUtil.putByte(out, outIx++, (byte) ((0xF << 6) | (c >>> 6)));
          UnsafeUtil.putByte(out, outIx++, (byte) (0x80 | (0x3F & c)));
        } else if ((c < MIN_SURROGATE || MAX_SURROGATE < c) && outIx <= outLimit - 3L) {
          // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
          UnsafeUtil.putByte(out, outIx++, (byte) ((0xF << 5) | (c >>> 12)));
          UnsafeUtil.putByte(out, outIx++, (byte) (0x80 | (0x3F & (c >>> 6))));
          UnsafeUtil.putByte(out, outIx++, (byte) (0x80 | (0x3F & c)));
        } else if (outIx <= outLimit - 4L) {
          // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8
          // bytes
          final char low;
          if (inIx + 1 == inLimit || !isSurrogatePair(c, (low = in.charAt(++inIx)))) {
            throw new UnpairedSurrogateException((inIx - 1), inLimit);
          }
          int codePoint = toCodePoint(c, low);
          UnsafeUtil.putByte(out, outIx++, (byte) ((0xF << 4) | (codePoint >>> 18)));
          UnsafeUtil.putByte(out, outIx++, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
          UnsafeUtil.putByte(out, outIx++, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
          UnsafeUtil.putByte(out, outIx++, (byte) (0x80 | (0x3F & codePoint)));
        } else {
          if ((MIN_SURROGATE <= c && c <= MAX_SURROGATE)
              && (inIx + 1 == inLimit || !isSurrogatePair(c, in.charAt(inIx + 1)))) {
            // We are surrogates and we're not a surrogate pair.
            throw new UnpairedSurrogateException(inIx, inLimit);
          }
          // Not enough space in the output buffer.
          throw new ArrayIndexOutOfBoundsException("Failed writing " + c + " at index " + outIx);
        }
      }

      // All bytes have been encoded.
      return (int) outIx;
    }

    @Override
    void encodeUtf8Direct(CharSequence in, ByteBuffer out) {
      final long address = addressOffset(out);
      long outIx = address + out.position();
      final long outLimit = address + out.limit();
      final int inLimit = in.length();
      if (inLimit > outLimit - outIx) {
        // Not even enough room for an ASCII-encoded string.
        throw new ArrayIndexOutOfBoundsException(
            "Failed writing " + in.charAt(inLimit - 1) + " at index " + out.limit());
      }

      // Designed to take advantage of
      // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
      int inIx = 0;
      for (char c; inIx < inLimit && (c = in.charAt(inIx)) < 0x80; ++inIx) {
        UnsafeUtil.putByte(outIx++, (byte) c);
      }
      if (inIx == inLimit) {
        // We're done, it was ASCII encoded.
        out.position((int) (outIx - address));
        return;
      }

      for (char c; inIx < inLimit; ++inIx) {
        c = in.charAt(inIx);
        if (c < 0x80 && outIx < outLimit) {
          UnsafeUtil.putByte(outIx++, (byte) c);
        } else if (c < 0x800 && outIx <= outLimit - 2L) { // 11 bits, two UTF-8 bytes
          UnsafeUtil.putByte(outIx++, (byte) ((0xF << 6) | (c >>> 6)));
          UnsafeUtil.putByte(outIx++, (byte) (0x80 | (0x3F & c)));
        } else if ((c < MIN_SURROGATE || MAX_SURROGATE < c) && outIx <= outLimit - 3L) {
          // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
          UnsafeUtil.putByte(outIx++, (byte) ((0xF << 5) | (c >>> 12)));
          UnsafeUtil.putByte(outIx++, (byte) (0x80 | (0x3F & (c >>> 6))));
          UnsafeUtil.putByte(outIx++, (byte) (0x80 | (0x3F & c)));
        } else if (outIx <= outLimit - 4L) {
          // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8
          // bytes
          final char low;
          if (inIx + 1 == inLimit || !isSurrogatePair(c, (low = in.charAt(++inIx)))) {
            throw new UnpairedSurrogateException((inIx - 1), inLimit);
          }
          int codePoint = toCodePoint(c, low);
          UnsafeUtil.putByte(outIx++, (byte) ((0xF << 4) | (codePoint >>> 18)));
          UnsafeUtil.putByte(outIx++, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
          UnsafeUtil.putByte(outIx++, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
          UnsafeUtil.putByte(outIx++, (byte) (0x80 | (0x3F & codePoint)));
        } else {
          if ((MIN_SURROGATE <= c && c <= MAX_SURROGATE)
              && (inIx + 1 == inLimit || !isSurrogatePair(c, in.charAt(inIx + 1)))) {
            // We are surrogates and we're not a surrogate pair.
            throw new UnpairedSurrogateException(inIx, inLimit);
          }
          // Not enough space in the output buffer.
          throw new ArrayIndexOutOfBoundsException("Failed writing " + c + " at index " + outIx);
        }
      }

      // All bytes have been encoded.
      out.position((int) (outIx - address));
    }

    /**
     * Counts (approximately) the number of consecutive ASCII characters starting from the given
     * position, using the most efficient method available to the platform.
     *
     * @param bytes the array containing the character sequence
     * @param offset the offset position of the index (same as index + arrayBaseOffset)
     * @param maxChars the maximum number of characters to count
     * @return the number of ASCII characters found. The stopping position will be at or
     * before the first non-ASCII byte.
     */
    private static int unsafeEstimateConsecutiveAscii(
        byte[] bytes, long offset, final int maxChars) {
      if (maxChars < UNSAFE_COUNT_ASCII_THRESHOLD) {
        // Don't bother with small strings.
        return 0;
      }

      for (int i = 0; i < maxChars; i++) {
        if (UnsafeUtil.getByte(bytes, offset++) < 0) {
          return i;
        }
      }
      return maxChars;
    }

    /**
     * Same as {@link Utf8#estimateConsecutiveAscii(ByteBuffer, int, int)} except that it uses the
     * most efficient method available to the platform.
     */
    private static int unsafeEstimateConsecutiveAscii(long address, final int maxChars) {
      int remaining = maxChars;
      if (remaining < UNSAFE_COUNT_ASCII_THRESHOLD) {
        // Don't bother with small strings.
        return 0;
      }

      // Read bytes until 8-byte aligned so that we can read longs in the loop below.
      // We do this by ANDing the address with 7 to determine the number of bytes that need to
      // be read before we're 8-byte aligned.
      final int unaligned = (int) address & 7;
      for (int j = unaligned; j > 0; j--) {
        if (UnsafeUtil.getByte(address++) < 0) {
          return unaligned - j;
        }
      }

      // This simple loop stops when we encounter a byte >= 0x80 (i.e. non-ASCII).
      // To speed things up further, we're reading longs instead of bytes so we use a mask to
      // determine if any byte in the current long is non-ASCII.
      remaining -= unaligned;
      for (; remaining >= 8 && (UnsafeUtil.getLong(address) & ASCII_MASK_LONG) == 0;
          address += 8, remaining -= 8) {}
      return maxChars - remaining;
    }

    private static int partialIsValidUtf8(final byte[] bytes, long offset, int remaining) {
      // Skip past ASCII characters as quickly as possible. 
      final int skipped = unsafeEstimateConsecutiveAscii(bytes, offset, remaining);
      remaining -= skipped;
      offset += skipped;

      for (;;) {
        // Optimize for interior runs of ASCII bytes.
        // TODO(nathanmittler): Consider checking 8 bytes at a time after some threshold?
        // Maybe after seeing a few in a row that are ASCII, go back to fast mode?
        int byte1 = 0;
        for (; remaining > 0 && (byte1 = UnsafeUtil.getByte(bytes, offset++)) >= 0; --remaining) {
        }
        if (remaining == 0) {
          return COMPLETE;
        }
        remaining--;

        // If we're here byte1 is not ASCII. Only need to handle 2-4 byte forms.
        if (byte1 < (byte) 0xE0) {
          // Two-byte form (110xxxxx 10xxxxxx)
          if (remaining == 0) {
            // Incomplete sequence
            return byte1;
          }
          remaining--;

          // Simultaneously checks for illegal trailing-byte in
          // leading position and overlong 2-byte form.
          if (byte1 < (byte) 0xC2
              || UnsafeUtil.getByte(bytes, offset++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else if (byte1 < (byte) 0xF0) {
          // Three-byte form (1110xxxx 10xxxxxx 10xxxxxx)
          if (remaining < 2) {
            // Incomplete sequence
            return unsafeIncompleteStateFor(bytes, byte1, offset, remaining);
          }
          remaining -= 2;

          final int byte2;
          if ((byte2 = UnsafeUtil.getByte(bytes, offset++)) > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // check for illegal surrogate codepoints
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || UnsafeUtil.getByte(bytes, offset++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else {
          // Four-byte form (1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx)
          if (remaining < 3) {
            // Incomplete sequence
            return unsafeIncompleteStateFor(bytes, byte1, offset, remaining);
          }
          remaining -= 3;

          final int byte2;
          if ((byte2 = UnsafeUtil.getByte(bytes, offset++)) > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || UnsafeUtil.getByte(bytes, offset++) > (byte) 0xBF
              // byte4 trailing-byte test
              || UnsafeUtil.getByte(bytes, offset++) > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }
    }

    private static int partialIsValidUtf8(long address, int remaining) {
      // Skip past ASCII characters as quickly as possible.
      final int skipped = unsafeEstimateConsecutiveAscii(address, remaining);
      address += skipped;
      remaining -= skipped;

      for (;;) {
        // Optimize for interior runs of ASCII bytes.
        // TODO(nathanmittler): Consider checking 8 bytes at a time after some threshold?
        // Maybe after seeing a few in a row that are ASCII, go back to fast mode?
        int byte1 = 0;
        for (; remaining > 0 && (byte1 = UnsafeUtil.getByte(address++)) >= 0; --remaining) {
        }
        if (remaining == 0) {
          return COMPLETE;
        }
        remaining--;

        if (byte1 < (byte) 0xE0) {
          // Two-byte form

          if (remaining == 0) {
            // Incomplete sequence
            return byte1;
          }
          remaining--;

          // Simultaneously checks for illegal trailing-byte in
          // leading position and overlong 2-byte form.
          if (byte1 < (byte) 0xC2 || UnsafeUtil.getByte(address++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else if (byte1 < (byte) 0xF0) {
          // Three-byte form

          if (remaining < 2) {
            // Incomplete sequence
            return unsafeIncompleteStateFor(address, byte1, remaining);
          }
          remaining -= 2;

          final byte byte2 = UnsafeUtil.getByte(address++);
          if (byte2 > (byte) 0xBF
              // overlong? 5 most significant bits must not all be zero
              || (byte1 == (byte) 0xE0 && byte2 < (byte) 0xA0)
              // check for illegal surrogate codepoints
              || (byte1 == (byte) 0xED && byte2 >= (byte) 0xA0)
              // byte3 trailing-byte test
              || UnsafeUtil.getByte(address++) > (byte) 0xBF) {
            return MALFORMED;
          }
        } else {
          // Four-byte form

          if (remaining < 3) {
            // Incomplete sequence
            return unsafeIncompleteStateFor(address, byte1, remaining);
          }
          remaining -= 3;

          final byte byte2 = UnsafeUtil.getByte(address++);
          if (byte2 > (byte) 0xBF
              // Check that 1 <= plane <= 16.  Tricky optimized form of:
              // if (byte1 > (byte) 0xF4 ||
              //     byte1 == (byte) 0xF0 && byte2 < (byte) 0x90 ||
              //     byte1 == (byte) 0xF4 && byte2 > (byte) 0x8F)
              || (((byte1 << 28) + (byte2 - (byte) 0x90)) >> 30) != 0
              // byte3 trailing-byte test
              || UnsafeUtil.getByte(address++) > (byte) 0xBF
              // byte4 trailing-byte test
              || UnsafeUtil.getByte(address++) > (byte) 0xBF) {
            return MALFORMED;
          }
        }
      }
    }

    private static int unsafeIncompleteStateFor(byte[] bytes, int byte1, long offset,
        int remaining) {
      switch (remaining) {
        case 0: {
          return incompleteStateFor(byte1);
        }
        case 1: {
          return incompleteStateFor(byte1, UnsafeUtil.getByte(bytes, offset));
        }
        case 2: {
          return incompleteStateFor(byte1, UnsafeUtil.getByte(bytes, offset),
              UnsafeUtil.getByte(bytes, offset + 1));
        }
        default: {
          throw new AssertionError();
        }
      }
    }

    private static int unsafeIncompleteStateFor(long address, final int byte1, int remaining) {
      switch (remaining) {
        case 0: {
          return incompleteStateFor(byte1);
        }
        case 1: {
          return incompleteStateFor(byte1, UnsafeUtil.getByte(address));
        }
        case 2: {
          return incompleteStateFor(byte1, UnsafeUtil.getByte(address),
              UnsafeUtil.getByte(address + 1));
        }
        default: {
          throw new AssertionError();
        }
      }
    }
  }

  private Utf8() {}
}
