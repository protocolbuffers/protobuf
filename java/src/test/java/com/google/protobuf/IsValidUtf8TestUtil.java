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

import static junit.framework.Assert.*;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.logging.Logger;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.Charset;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CoderResult;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;

/**
 * Shared testing code for {@link IsValidUtf8Test} and
 * {@link IsValidUtf8FourByteTest}.
 *
 * @author jonp@google.com (Jon Perlow)
 * @author martinrb@google.com (Martin Buchholz)
 */
class IsValidUtf8TestUtil {
  private static Logger logger = Logger.getLogger(
      IsValidUtf8TestUtil.class.getName());

  // 128 - [chars 0x0000 to 0x007f]
  static long ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS = 0x007f - 0x0000 + 1;

  // 128
  static long EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT =
      ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  // 1920 [chars 0x0080 to 0x07FF]
  static long TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS = 0x07FF - 0x0080 + 1;

  // 18,304
  static long EXPECTED_TWO_BYTE_ROUNDTRIPPABLE_COUNT =
      // Both bytes are one byte characters
      (long) Math.pow(EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT, 2) +
      // The possible number of two byte characters
      TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  // 2048
  static long THREE_BYTE_SURROGATES = 2 * 1024;

  // 61,440 [chars 0x0800 to 0xFFFF, minus surrogates]
  static long THREE_BYTE_ROUNDTRIPPABLE_CHARACTERS =
      0xFFFF - 0x0800 + 1 - THREE_BYTE_SURROGATES;

  // 2,650,112
  static long EXPECTED_THREE_BYTE_ROUNDTRIPPABLE_COUNT =
      // All one byte characters
      (long) Math.pow(EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT, 3) +
      // One two byte character and a one byte character
      2 * TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS *
          ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS +
       // Three byte characters
      THREE_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  // 1,048,576 [chars 0x10000L to 0x10FFFF]
  static long FOUR_BYTE_ROUNDTRIPPABLE_CHARACTERS = 0x10FFFF - 0x10000L + 1;

  // 289,571,839
  static long EXPECTED_FOUR_BYTE_ROUNDTRIPPABLE_COUNT =
      // All one byte characters
      (long) Math.pow(EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT, 4) +
      // One and three byte characters
      2 * THREE_BYTE_ROUNDTRIPPABLE_CHARACTERS *
          ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS +
      // Two two byte characters
      TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS * TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS +
      // Permutations of one and two byte characters
      3 * TWO_BYTE_ROUNDTRIPPABLE_CHARACTERS *
          ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS *
          ONE_BYTE_ROUNDTRIPPABLE_CHARACTERS +
      // Four byte characters
      FOUR_BYTE_ROUNDTRIPPABLE_CHARACTERS;

  static class Shard {
    final long index;
    final long start;
    final long lim;
    final long expected;


    public Shard(long index, long start, long lim, long expected) {
      assertTrue(start < lim);
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

  static final List<Shard> FOUR_BYTE_SHARDS = generateFourByteShards(
      128, FOUR_BYTE_SHARDS_EXPECTED_ROUNTRIPPABLES);


  private static List<Shard> generateFourByteShards(
      int numShards, long[] expected) {
    assertEquals(numShards, expected.length);
    List<Shard> shards = new ArrayList<Shard>(numShards);
    long LIM = 1L << 32;
    long increment = LIM / numShards;
    assertTrue(LIM % numShards == 0);
    for (int i = 0; i < numShards; i++) {
      shards.add(new Shard(i,
          increment * i,
          increment * (i + 1),
          expected[i]));
    }
    return shards;
  }

  /**
   * Helper to run the loop to test all the permutations for the number of bytes
   * specified.
   *
   * @param numBytes the number of bytes in the byte array
   * @param expectedCount the expected number of roundtrippable permutations
   */
  static void testBytes(int numBytes, long expectedCount)
      throws UnsupportedEncodingException {
    testBytes(numBytes, expectedCount, 0, -1);
  }

  /**
   * Helper to run the loop to test all the permutations for the number of bytes
   * specified. This overload is useful for debugging to get the loop to start
   * at a certain character.
   *
   * @param numBytes the number of bytes in the byte array
   * @param expectedCount the expected number of roundtrippable permutations
   * @param start the starting bytes encoded as a long as big-endian
   * @param lim the limit of bytes to process encoded as a long as big-endian,
   *     or -1 to mean the max limit for numBytes
   */
  static void testBytes(int numBytes, long expectedCount, long start, long lim)
      throws UnsupportedEncodingException {
    Random rnd = new Random();
    byte[] bytes = new byte[numBytes];

    if (lim == -1) {
      lim = 1L << (numBytes * 8);
    }
    long count = 0;
    long countRoundTripped = 0;
    for (long byteChar = start; byteChar < lim; byteChar++) {
      long tmpByteChar = byteChar;
      for (int i = 0; i < numBytes; i++) {
        bytes[bytes.length - i - 1] = (byte) tmpByteChar;
        tmpByteChar = tmpByteChar >> 8;
      }
      ByteString bs = ByteString.copyFrom(bytes);
      boolean isRoundTrippable = bs.isValidUtf8();
      String s = new String(bytes, "UTF-8");
      byte[] bytesReencoded = s.getBytes("UTF-8");
      boolean bytesEqual = Arrays.equals(bytes, bytesReencoded);

      if (bytesEqual != isRoundTrippable) {
        outputFailure(byteChar, bytes, bytesReencoded);
      }

      // Check agreement with static Utf8 methods.
      assertEquals(isRoundTrippable, Utf8.isValidUtf8(bytes));
      assertEquals(isRoundTrippable, Utf8.isValidUtf8(bytes, 0, numBytes));

      // Test partial sequences.
      // Partition numBytes into three segments (not necessarily non-empty).
      int i = rnd.nextInt(numBytes);
      int j = rnd.nextInt(numBytes);
      if (j < i) {
        int tmp = i; i = j; j = tmp;
      }
      int state1 = Utf8.partialIsValidUtf8(Utf8.COMPLETE, bytes, 0, i);
      int state2 = Utf8.partialIsValidUtf8(state1, bytes, i, j);
      int state3 = Utf8.partialIsValidUtf8(state2, bytes, j, numBytes);
      if (isRoundTrippable != (state3 == Utf8.COMPLETE)) {
        System.out.printf("state=%04x %04x %04x i=%d j=%d%n",
                          state1, state2, state3, i, j);
        outputFailure(byteChar, bytes, bytesReencoded);
      }
      assertEquals(isRoundTrippable, (state3 == Utf8.COMPLETE));

      // Test ropes built out of small partial sequences
      ByteString rope = RopeByteString.newInstanceForTest(
          bs.substring(0, i),
          RopeByteString.newInstanceForTest(
              bs.substring(i, j),
              bs.substring(j, numBytes)));
      assertSame(RopeByteString.class, rope.getClass());

      ByteString[] byteStrings = { bs, bs.substring(0, numBytes), rope };
      for (ByteString x : byteStrings) {
        assertEquals(isRoundTrippable,
                     x.isValidUtf8());
        assertEquals(state3,
                     x.partialIsValidUtf8(Utf8.COMPLETE, 0, numBytes));

        assertEquals(state1,
                     x.partialIsValidUtf8(Utf8.COMPLETE, 0, i));
        assertEquals(state1,
                     x.substring(0, i).partialIsValidUtf8(Utf8.COMPLETE, 0, i));
        assertEquals(state2,
                     x.partialIsValidUtf8(state1, i, j - i));
        assertEquals(state2,
                     x.substring(i, j).partialIsValidUtf8(state1, 0, j - i));
        assertEquals(state3,
                     x.partialIsValidUtf8(state2, j, numBytes - j));
        assertEquals(state3,
                     x.substring(j, numBytes)
                     .partialIsValidUtf8(state2, 0, numBytes - j));
      }

      // ByteString reduplication should not affect its UTF-8 validity.
      ByteString ropeADope =
          RopeByteString.newInstanceForTest(bs, bs.substring(0, numBytes));
      assertEquals(isRoundTrippable, ropeADope.isValidUtf8());

      if (isRoundTrippable) {
        countRoundTripped++;
      }
      count++;
      if (byteChar != 0 && byteChar % 1000000L == 0) {
        logger.info("Processed " + (byteChar / 1000000L) +
            " million characters");
      }
    }
    logger.info("Round tripped " + countRoundTripped + " of " + count);
    assertEquals(expectedCount, countRoundTripped);
  }

  /**
   * Variation of {@link #testBytes} that does less allocation using the
   * low-level encoders/decoders directly. Checked in because it's useful for
   * debugging when trying to process bytes faster, but since it doesn't use the
   * actual String class, it's possible for incompatibilities to develop
   * (although unlikely).
   *
   * @param numBytes the number of bytes in the byte array
   * @param expectedCount the expected number of roundtrippable permutations
   * @param start the starting bytes encoded as a long as big-endian
   * @param lim the limit of bytes to process encoded as a long as big-endian,
   *     or -1 to mean the max limit for numBytes
   */
  void testBytesUsingByteBuffers(
      int numBytes, long expectedCount, long start, long lim)
      throws UnsupportedEncodingException {
    CharsetDecoder decoder = Charset.forName("UTF-8").newDecoder()
        .onMalformedInput(CodingErrorAction.REPLACE)
        .onUnmappableCharacter(CodingErrorAction.REPLACE);
    CharsetEncoder encoder = Charset.forName("UTF-8").newEncoder()
        .onMalformedInput(CodingErrorAction.REPLACE)
        .onUnmappableCharacter(CodingErrorAction.REPLACE);
    byte[] bytes = new byte[numBytes];
    int maxChars = (int) (decoder.maxCharsPerByte() * numBytes) + 1;
    char[] charsDecoded =
        new char[(int) (decoder.maxCharsPerByte() * numBytes) + 1];
    int maxBytes = (int) (encoder.maxBytesPerChar() * maxChars) + 1;
    byte[] bytesReencoded = new byte[maxBytes];

    ByteBuffer bb = ByteBuffer.wrap(bytes);
    CharBuffer cb = CharBuffer.wrap(charsDecoded);
    ByteBuffer bbReencoded = ByteBuffer.wrap(bytesReencoded);
    if (lim == -1) {
      lim = 1L << (numBytes * 8);
    }
    long count = 0;
    long countRoundTripped = 0;
    for (long byteChar = start; byteChar < lim; byteChar++) {
      bb.rewind();
      bb.limit(bytes.length);
      cb.rewind();
      cb.limit(charsDecoded.length);
      bbReencoded.rewind();
      bbReencoded.limit(bytesReencoded.length);
      encoder.reset();
      decoder.reset();
      long tmpByteChar = byteChar;
      for (int i = 0; i < bytes.length; i++) {
        bytes[bytes.length - i - 1] = (byte) tmpByteChar;
        tmpByteChar = tmpByteChar >> 8;
      }
      boolean isRoundTrippable = ByteString.copyFrom(bytes).isValidUtf8();
      CoderResult result = decoder.decode(bb, cb, true);
      assertFalse(result.isError());
      result = decoder.flush(cb);
      assertFalse(result.isError());

      int charLen = cb.position();
      cb.rewind();
      cb.limit(charLen);
      result = encoder.encode(cb, bbReencoded, true);
      assertFalse(result.isError());
      result = encoder.flush(bbReencoded);
      assertFalse(result.isError());

      boolean bytesEqual = true;
      int bytesLen = bbReencoded.position();
      if (bytesLen != numBytes) {
        bytesEqual = false;
      } else {
        for (int i = 0; i < numBytes; i++) {
          if (bytes[i] != bytesReencoded[i]) {
            bytesEqual = false;
            break;
          }
        }
      }
      if (bytesEqual != isRoundTrippable) {
        outputFailure(byteChar, bytes, bytesReencoded, bytesLen);
      }

      count++;
      if (isRoundTrippable) {
        countRoundTripped++;
      }
      if (byteChar != 0 && byteChar % 1000000 == 0) {
        logger.info("Processed " + (byteChar / 1000000) +
            " million characters");
      }
    }
    logger.info("Round tripped " + countRoundTripped + " of " + count);
    assertEquals(expectedCount, countRoundTripped);
  }

  private static void outputFailure(long byteChar, byte[] bytes, byte[] after) {
    outputFailure(byteChar, bytes, after, after.length);
  }

  private static void outputFailure(long byteChar, byte[] bytes, byte[] after,
      int len) {
    fail("Failure: (" + Long.toHexString(byteChar) + ") " +
        toHexString(bytes) + " => " + toHexString(after, len));
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
