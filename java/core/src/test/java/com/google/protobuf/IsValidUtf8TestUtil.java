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

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import java.lang.ref.SoftReference;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

/**
 * Shared testing code for {@link IsValidUtf8Test} and {@link IsValidUtf8FourByteTest}.
 *
 * @author jonp@google.com (Jon Perlow)
 * @author martinrb@google.com (Martin Buchholz)
 */
final class IsValidUtf8TestUtil {
  private IsValidUtf8TestUtil() {}

  static interface ByteStringFactory {
    ByteString newByteString(byte[] bytes);
  }

  static final ByteStringFactory LITERAL_FACTORY =
      new ByteStringFactory() {
        @Override
        public ByteString newByteString(byte[] bytes) {
          return ByteString.wrap(bytes);
        }
      };

  static final ByteStringFactory HEAP_NIO_FACTORY =
      new ByteStringFactory() {
        @Override
        public ByteString newByteString(byte[] bytes) {
          return new NioByteString(ByteBuffer.wrap(bytes));
        }
      };

  private static final ThreadLocal<SoftReference<ByteBuffer>> directBuffer = new ThreadLocal<>();

  /**
   * Factory for direct {@link ByteBuffer} instances. To reduce direct memory usage, this uses a
   * thread local direct buffer. This means that each call will overwrite the buffer's contents from
   * the previous call, so the calling code must be careful not to continue using a buffer returned
   * from a previous invocation.
   */
  static final ByteStringFactory DIRECT_NIO_FACTORY =
      new ByteStringFactory() {
        @Override
        public ByteString newByteString(byte[] bytes) {
          SoftReference<ByteBuffer> ref = directBuffer.get();
          ByteBuffer buffer = ref == null ? null : ref.get();
          if (buffer == null || buffer.capacity() < bytes.length) {
            buffer = ByteBuffer.allocateDirect(bytes.length);
            directBuffer.set(new SoftReference<ByteBuffer>(buffer));
          }
          buffer.clear();
          buffer.put(bytes);
          buffer.flip();
          return new NioByteString(buffer);
        }
      };

  static final ByteStringFactory ROPE_FACTORY =
      new ByteStringFactory() {
        // Seed the random number generator with 0 so that the tests are deterministic.
        private final Random random = new Random(0);

        @Override
        public ByteString newByteString(byte[] bytes) {
          // We split the byte array into three pieces (some possibly empty) by choosing two random
          // cut points i and j.
          int i = random.nextInt(bytes.length);
          int j = random.nextInt(bytes.length);
          if (j < i) {
            int tmp = i;
            i = j;
            j = tmp;
          }
          return RopeByteString.newInstanceForTest(
              ByteString.wrap(bytes, 0, i),
              RopeByteString.newInstanceForTest(
                  ByteString.wrap(bytes, i, j - i), ByteString.wrap(bytes, j, bytes.length - j)));
        }
      };

  // 128 - [chars 0x0000 to 0x007f]
  static final long ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS = 0x007f - 0x0000 + 1;

  // 128
  static final long EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT = ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  // 1920 [chars 0x0080 to 0x07FF]
  static final long TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS = 0x07FF - 0x0080 + 1;

  // 18,304
  static final long EXPECTED_TWO_BYTE_ROUNDTRIPPABLE_COUNT =
      // Both bytes are one byte characters
      (long) Math.pow(EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT, 2)
          +
          // The possible number of two byte characters
          TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  // 2048
  static final long THREE_BYTE_SURROGATES = 2 * 1024;

  // 61,440 [chars 0x0800 to 0xFFFF, minus surrogates]
  static final long THREE_BYTE_ROUNDTRIPPABLE_CHARACTERS =
      0xFFFF - 0x0800 + 1 - THREE_BYTE_SURROGATES;

  // 2,650,112
  static final long EXPECTED_THREE_BYTE_ROUNDTRIPPABLE_COUNT =
      // All one byte characters
      (long) Math.pow(EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT, 3)
          +
          // One two byte character and a one byte character
          2 * TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS * ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS
          +
          // Three byte characters
          THREE_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  // 1,048,576 [chars 0x10000L to 0x10FFFF]
  static final long FOUR_BYTE_ROUNDTRIPPABLE_CHARACTERS = 0x10FFFF - 0x10000L + 1;

  // 289,571,839
  static final long EXPECTED_FOUR_BYTE_ROUNDTRIPPABLE_COUNT =
      // All one byte characters
      (long) Math.pow(EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT, 4)
          +
          // One and three byte characters
          2 * THREE_BYTE_ROUNDTRIPPABLE_CHARACTERS * ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS
          +
          // Two two byte characters
          TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS * TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS
          +
          // Permutations of one and two byte characters
          3
              * TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS
              * ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS
              * ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS
          +
          // Four byte characters
          FOUR_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  static final class Shard {
    final long index;
    final long start;
    final long lim;
    final long expected;

    public Shard(long index, long start, long lim, long expected) {
      assertThat(start).isLessThan(lim);
      this.index = index;
      this.start = start;
      this.lim = lim;
      this.expected = expected;
    }
  }

  static final long[] FOUR_BYTE_SHARDS_EXPECTED_ROUNTRIPPABLES =
      generateFourByteShardsExpectedRunnables();

  private static long[] generateFourByteShardsExpectedRunnables() {
    long[] expected = new long[128];

    // 0-63 are all 5300224
    for (int i = 0; i <= 63; i++) {
      expected[i] = 5300224;
    }

    // 97-111 are all 2342912
    for (int i = 97; i <= 111; i++) {
      expected[i] = 2342912;
    }

    // 113-117 are all 1048576
    for (int i = 113; i <= 117; i++) {
      expected[i] = 1048576;
    }

    // One offs
    expected[112] = 786432;
    expected[118] = 786432;
    expected[119] = 1048576;
    expected[120] = 458752;
    expected[121] = 524288;
    expected[122] = 65536;

    // Anything not assigned was the default 0.
    return expected;
  }

  static final List<Shard> FOUR_BYTE_SHARDS =
      generateFourByteShards(128, FOUR_BYTE_SHARDS_EXPECTED_ROUNTRIPPABLES);

  private static List<Shard> generateFourByteShards(int numShards, long[] expected) {
    assertThat(expected).hasLength(numShards);
    List<Shard> shards = new ArrayList<>(numShards);
    long lim = 1L << 32;
    long increment = lim / numShards;
    assertThat(lim % numShards).isEqualTo(0);
    for (int i = 0; i < numShards; i++) {
      shards.add(new Shard(i, increment * i, increment * (i + 1), expected[i]));
    }
    return shards;
  }

  /**
   * Helper to run the loop to test all the permutations for the number of bytes specified.
   *
   * @param factory the factory for {@link ByteString} instances.
   * @param numBytes the number of bytes in the byte array
   * @param expectedCount the expected number of roundtrippable permutations
   */
  static void testBytes(ByteStringFactory factory, int numBytes, long expectedCount) {
    testBytes(factory, numBytes, expectedCount, 0, -1);
  }

  /**
   * Helper to run the loop to test all the permutations for the number of bytes specified. This
   * overload is useful for debugging to get the loop to start at a certain character.
   *
   * @param factory the factory for {@link ByteString} instances.
   * @param numBytes the number of bytes in the byte array
   * @param expectedCount the expected number of roundtrippable permutations
   * @param start the starting bytes encoded as a long as big-endian
   * @param lim the limit of bytes to process encoded as a long as big-endian, or -1 to mean the max
   *     limit for numBytes
   */
  static void testBytes(
      ByteStringFactory factory, int numBytes, long expectedCount, long start, long lim) {
    byte[] bytes = new byte[numBytes];

    if (lim == -1) {
      lim = 1L << (numBytes * 8);
    }
    long countRoundTripped = 0;
    for (long byteChar = start; byteChar < lim; byteChar++) {
      long tmpByteChar = byteChar;
      for (int i = 0; i < numBytes; i++) {
        bytes[bytes.length - i - 1] = (byte) tmpByteChar;
        tmpByteChar = tmpByteChar >> 8;
      }
      ByteString bs = factory.newByteString(bytes);
      boolean isRoundTrippable = bs.isValidUtf8();
      String s = new String(bytes, Internal.UTF_8);
      byte[] bytesReencoded = s.getBytes(Internal.UTF_8);
      boolean bytesEqual = Arrays.equals(bytes, bytesReencoded);

      if (bytesEqual != isRoundTrippable) {
        outputFailure(byteChar, bytes, bytesReencoded);
      }

      // Check agreement with Utf8.isValidUtf8.
      assertThat(Utf8.isValidUtf8(bytes)).isEqualTo(isRoundTrippable);

      if (isRoundTrippable) {
        countRoundTripped++;
      }
    }
    assertThat(countRoundTripped).isEqualTo(expectedCount);
  }

  private static void outputFailure(long byteChar, byte[] bytes, byte[] after) {
    outputFailure(byteChar, bytes, after, after.length);
  }

  private static void outputFailure(long byteChar, byte[] bytes, byte[] after, int len) {
    assertWithMessage("Failure: (%s) %s => %s",
            Long.toHexString(byteChar), toHexString(bytes), toHexString(after, len)).fail();
  }

  private static String toHexString(byte[] b) {
    return toHexString(b, b.length);
  }

  private static String toHexString(byte[] b, int len) {
    StringBuilder s = new StringBuilder();
    s.append("\"");
    for (int i = 0; i < len; i++) {
      if (i > 0) {
        s.append(" ");
      }
      s.append(String.format("%02x", b[i] & 0xFF));
    }
    s.append("\"");
    return s.toString();
  }
}
