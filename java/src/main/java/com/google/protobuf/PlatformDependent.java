// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

import sun.misc.Unsafe;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.charset.CoderResult;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Optimized platform-specific utilities.
 */
class PlatformDependent {
  private static final Logger logger = Logger.getLogger(PlatformDependent.class.getName());
  private static final Unsafe UNSAFE;

  static {
    UNSAFE = getUnsafe();
  }

  private static Unsafe getUnsafe() {
    Unsafe unsafe = null;
    try {
      java.lang.reflect.Field unsafeField = Unsafe.class.getDeclaredField("theUnsafe");
      unsafeField.setAccessible(true);
      unsafe = (Unsafe) unsafeField.get(null);
    } catch (Throwable cause) {
      // Do nothing.
    }

    log("sun.misc.Unsafe.theUnsafe: {}", unsafe != null ? "available" : "unavailable");
    return unsafe;
  }

  private static java.lang.reflect.Field field(Class clazz, String fieldName) {
    java.lang.reflect.Field field;
    try {
      field = clazz.getDeclaredField(fieldName);
      field.setAccessible(true);
    } catch (Throwable t) {
      // Failed to access the fields.
      field = null;
    }
    log("{0}.{1}: {2}", clazz.getName(), fieldName, field != null? "available" : "unavailable");
    return field;
  }

  private static long objectFieldOffset(java.lang.reflect.Field field) {
    return field == null || UNSAFE == null ? -1 : UNSAFE.objectFieldOffset(field);
  }

  private static void log(String format, Object... params) {
    logger.log(Level.FINEST, format, params);
  }

  /**
   * Encodes a {@code UTF-8} {@link String} to the given {@link ByteBuffer}. Implements the
   * same algorithm given by {@link Utf8#encode(CharSequence, byte[], int, int)}. Uses
   * optimizations provided by {@link Unsafe} where possible.
   */
  static CoderResult encode(String src, ByteBuffer target) {
    if (target.hasArray()) {
      return encodeToHeap(src, target);
    }
    if (UnsafeDirectByteBuffer.isAvailable()) {
      return unsafeEncodeToDirect(src, target);
    }
    return encodeToDirect(src, target);
  }

  /**
   * Uses {@link Unsafe} to write to the direct buffer's underlying address space.
   */
  private static CoderResult unsafeEncodeToDirect(String src, ByteBuffer targetBuffer) {
    long targetOffset = UnsafeDirectByteBuffer.getAddress(targetBuffer);
    long targetLimit = targetOffset + targetBuffer.remaining();

    long targetIx = targetOffset;
    int srcIx = 0;
    int srcLimit = src.length();

    // Designed to take advantage of
    // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
    for (char c;
         srcIx < srcLimit && targetIx < targetLimit&& (c = src.charAt(srcIx)) < 0x80;
         srcIx++) {
      UnsafeDirectByteBuffer.putByte(targetIx++, (byte) c);
    }
    if (srcIx == srcLimit) {
      targetBuffer.position(targetBuffer.position() + (int) (targetIx - targetOffset));
      return CoderResult.UNDERFLOW;
    }

    for (char c; srcIx < srcLimit; srcIx++) {
      c = src.charAt(srcIx);
      if (c < 0x80 && targetIx < targetLimit) {
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) c);
      } else if (c < 0x800 && targetIx <= targetLimit - 2) { // 11 bits, two UTF-8 bytes
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) ((0xF << 6) | (c >>> 6)));
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) (0x80 | (0x3F & c)));
      } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c)
              && targetIx <= targetLimit - 3) {
        // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) ((0xF << 5) | (c >>> 12)));
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) (0x80 | (0x3F & (c >>> 6))));
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) (0x80 | (0x3F & c)));
      } else if (targetIx <= targetLimit - 4) {
        // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8 bytes
        final char low;
        if (srcIx + 1 == srcLimit
                || !Character.isSurrogatePair(c, (low = src.charAt(++srcIx)))) {
          return CoderResult.malformedForLength(srcIx - 1);
        }
        int codePoint = Character.toCodePoint(c, low);
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) ((0xF << 4) | (codePoint >>> 18)));
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
        UnsafeDirectByteBuffer.putByte(targetIx++, (byte) (0x80 | (0x3F & codePoint)));
      } else {
        if ((Character.MIN_SURROGATE <= c && c <= Character.MAX_SURROGATE)
                && (srcIx + 1 == srcLimit
                || !Character.isSurrogatePair(c, src.charAt(srcIx + 1)))) {
          // We are surrogates and we're not a surrogate pair.
          return CoderResult.malformedForLength(srcIx);
        }
        // Not enough space in the output buffer.
        return CoderResult.OVERFLOW;
      }
    }
    // All bytes have been encoded.
    targetBuffer.position(targetBuffer.position() + (int) (targetIx - targetOffset));
    return CoderResult.UNDERFLOW;
  }

  /**
   * Encodes the {@link String} to the heap buffer's underlying array.
   */
  private static CoderResult encodeToHeap(String src, ByteBuffer target) {
    byte[] bytes = target.array();
    int targetOffset = target.arrayOffset() + target.position();

    int srcLength = src.length();
    int targetIx = targetOffset;
    int srcIx = 0;
    int targetLimit = targetOffset + target.remaining();

    // Designed to take advantage of
    // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
    for (char c;
         srcIx < srcLength && targetIx < targetLimit && (c = src.charAt(srcIx)) < 0x80;
         srcIx++) {
      bytes[targetIx++] = (byte) c;
    }
    if (srcIx == srcLength) {
      // The entire source buffer has been consumed.
      target.position(targetIx - target.arrayOffset());
      return CoderResult.UNDERFLOW;
    }

    for (char c; srcIx < srcLength; srcIx++) {
      c = src.charAt(srcIx);
      if (c < 0x80 && targetIx < targetLimit) {
        bytes[targetIx++] = (byte) c;
      } else if (c < 0x800 && targetIx <= targetLimit - 2) { // 11 bits, two UTF-8 bytes
        bytes[targetIx++] = (byte) ((0xF << 6) | (c >>> 6));
        bytes[targetIx++] = (byte) (0x80 | (0x3F & c));
      } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c)
              && targetIx <= targetLimit - 3) {
        // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
        bytes[targetIx++] = (byte) ((0xF << 5) | (c >>> 12));
        bytes[targetIx++] = (byte) (0x80 | (0x3F & (c >>> 6)));
        bytes[targetIx++] = (byte) (0x80 | (0x3F & c));
      } else if (targetIx <= targetLimit - 4) {
        // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8 bytes
        final char low;
        if (srcIx + 1 == srcLength
                || !Character.isSurrogatePair(c, (low = src.charAt(++srcIx)))) {
          return CoderResult.malformedForLength(srcIx - 1);
        }
        int codePoint = Character.toCodePoint(c, low);
        bytes[targetIx++] = (byte) ((0xF << 4) | (codePoint >>> 18));
        bytes[targetIx++] = (byte) (0x80 | (0x3F & (codePoint >>> 12)));
        bytes[targetIx++] = (byte) (0x80 | (0x3F & (codePoint >>> 6)));
        bytes[targetIx++] = (byte) (0x80 | (0x3F & codePoint));
      } else {
        if ((Character.MIN_SURROGATE <= c && c <= Character.MAX_SURROGATE)
                && (srcIx + 1 == srcLength
                || !Character.isSurrogatePair(c, src.charAt(srcIx + 1)))) {
          // We are surrogates and we're not a surrogate pair.
          return CoderResult.malformedForLength(srcIx);
        }
        // Not enough space in the output buffer.
        return CoderResult.OVERFLOW;
      }
    }
    // All bytes have been encoded.
    target.position(targetIx - target.arrayOffset());
    return CoderResult.UNDERFLOW;
  }

  /**
   * Encodes the {@link String} to the direct buffer using the standard {@link ByteBuffer} API.
   */
  private static CoderResult encodeToDirect(String src, ByteBuffer target) {
    int srcLength = src.length();
    int targetOffset = target.position();
    int targetIx = targetOffset;
    int srcIx = 0;
    int targetLimit = targetOffset + target.remaining();

    // Designed to take advantage of
    // https://wikis.oracle.com/display/HotSpotInternals/RangeCheckElimination
    for (char c;
         srcIx < srcLength && targetIx < targetLimit && (c = src.charAt(srcIx)) < 0x80;
         srcIx++) {
      target.put(targetIx++, (byte) c);
    }
    if (srcIx == srcLength) {
      // The entire source buffer has been consumed.
      target.position(targetIx);
      return CoderResult.UNDERFLOW;
    }

    for (char c; srcIx < srcLength; srcIx++) {
      c = src.charAt(srcIx);
      if (c < 0x80 && targetIx < targetLimit) {
        target.put(targetIx++, (byte) c);
      } else if (c < 0x800 && targetIx <= targetLimit - 2) { // 11 bits, two UTF-8 bytes
        target.put(targetIx++, (byte) ((0xF << 6) | (c >>> 6)));
        target.put(targetIx++, (byte) (0x80 | (0x3F & c)));
      } else if ((c < Character.MIN_SURROGATE || Character.MAX_SURROGATE < c)
              && targetIx <= targetLimit - 3) {
        // Maximum single-char code point is 0xFFFF, 16 bits, three UTF-8 bytes
        target.put(targetIx++, (byte) ((0xF << 5) | (c >>> 12)));
        target.put(targetIx++, (byte) (0x80 | (0x3F & (c >>> 6))));
        target.put(targetIx++, (byte) (0x80 | (0x3F & c)));
      } else if (targetIx <= targetLimit - 4) {
        // Minimum code point represented by a surrogate pair is 0x10000, 17 bits, four UTF-8 bytes
        final char low;
        if (srcIx + 1 == srcLength
                || !Character.isSurrogatePair(c, (low = src.charAt(++srcIx)))) {
          return CoderResult.malformedForLength(srcIx - 1);
        }
        int codePoint = Character.toCodePoint(c, low);
        target.put(targetIx++, (byte) ((0xF << 4) | (codePoint >>> 18)));
        target.put(targetIx++, (byte) (0x80 | (0x3F & (codePoint >>> 12))));
        target.put(targetIx++, (byte) (0x80 | (0x3F & (codePoint >>> 6))));
        target.put(targetIx++, (byte) (0x80 | (0x3F & codePoint)));
      } else {
        if ((Character.MIN_SURROGATE <= c && c <= Character.MAX_SURROGATE)
                && (srcIx + 1 == srcLength
                || !Character.isSurrogatePair(c, src.charAt(srcIx + 1)))) {
          // We are surrogates and we're not a surrogate pair.
          return CoderResult.malformedForLength(srcIx);
        }
        // Not enough space in the output buffer.
        return CoderResult.OVERFLOW;
      }
    }
    // All bytes have been encoded.
    target.position(targetIx);
    return CoderResult.UNDERFLOW;
  }

  private static class UnsafeDirectByteBuffer {
    private static final long ADDRESS_OFFSET;

    static {
      ADDRESS_OFFSET = objectFieldOffset(field(Buffer.class, "address"));
    }

    static boolean isAvailable() {
      return ADDRESS_OFFSET != -1;
    }

    private static long getAddress(ByteBuffer buffer) {
      return UNSAFE.getLong(buffer, ADDRESS_OFFSET);
    }

    public static void putByte(long address, byte b) {
      UNSAFE.putByte(address, b);
    }
  }
}
