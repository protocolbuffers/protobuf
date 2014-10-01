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

import com.google.protobuf.IsValidUtf8TestUtil.Shard;

import junit.framework.TestCase;

import java.io.UnsupportedEncodingException;

/**
 * Tests cases for {@link ByteString#isValidUtf8()}. This includes three
 * brute force tests that actually test every permutation of one byte, two byte,
 * and three byte sequences to ensure that the method produces the right result
 * for every possible byte encoding where "right" means it's consistent with
 * java's UTF-8 string encoding/decoding such that the method returns true for
 * any sequence that will round trip when converted to a String and then back to
 * bytes and will return false for any sequence that will not round trip.
 * See also {@link IsValidUtf8FourByteTest}. It also includes some
 * other more targeted tests.
 *
 * @author jonp@google.com (Jon Perlow)
 * @author martinrb@google.com (Martin Buchholz)
 */
public class IsValidUtf8Test extends TestCase {

  /**
   * Tests that round tripping of all two byte permutations work.
   */
  public void testIsValidUtf8_1Byte() throws UnsupportedEncodingException {
    IsValidUtf8TestUtil.testBytes(1,
        IsValidUtf8TestUtil.EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT);
  }

  /**
   * Tests that round tripping of all two byte permutations work.
   */
  public void testIsValidUtf8_2Bytes() throws UnsupportedEncodingException {
    IsValidUtf8TestUtil.testBytes(2,
        IsValidUtf8TestUtil.EXPECTED_TWO_BYTE_ROUNDTRIPPABLE_COUNT);
  }

  /**
   * Tests that round tripping of all three byte permutations work.
   */
  public void testIsValidUtf8_3Bytes() throws UnsupportedEncodingException {
    IsValidUtf8TestUtil.testBytes(3,
        IsValidUtf8TestUtil.EXPECTED_THREE_BYTE_ROUNDTRIPPABLE_COUNT);
  }

  /**
   * Tests that round tripping of a sample of four byte permutations work.
   * All permutations are prohibitively expensive to test for automated runs;
   * {@link IsValidUtf8FourByteTest} is used for full coverage. This method
   * tests specific four-byte cases.
   */
  public void testIsValidUtf8_4BytesSamples()
      throws UnsupportedEncodingException {
    // Valid 4 byte.
    assertValidUtf8(0xF0, 0xA4, 0xAD, 0xA2);

    // Bad trailing bytes
    assertInvalidUtf8(0xF0, 0xA4, 0xAD, 0x7F);
    assertInvalidUtf8(0xF0, 0xA4, 0xAD, 0xC0);

    // Special cases for byte2
    assertInvalidUtf8(0xF0, 0x8F, 0xAD, 0xA2);
    assertInvalidUtf8(0xF4, 0x90, 0xAD, 0xA2);
  }

  /**
   * Tests some hard-coded test cases.
   */
  public void testSomeSequences() {
    // Empty
    assertTrue(asBytes("").isValidUtf8());

    // One-byte characters, including control characters
    assertTrue(asBytes("\u0000abc\u007f").isValidUtf8());

    // Two-byte characters
    assertTrue(asBytes("\u00a2\u00a2").isValidUtf8());

    // Three-byte characters
    assertTrue(asBytes("\u020ac\u020ac").isValidUtf8());

    // Four-byte characters
    assertTrue(asBytes("\u024B62\u024B62").isValidUtf8());

    // Mixed string
    assertTrue(
        asBytes("a\u020ac\u00a2b\\u024B62u020acc\u00a2de\u024B62")
        .isValidUtf8());

    // Not a valid string
    assertInvalidUtf8(-1, 0, -1, 0);
  }

  private byte[] toByteArray(int... bytes) {
    byte[] realBytes = new byte[bytes.length];
    for (int i = 0; i < bytes.length; i++) {
      realBytes[i] = (byte) bytes[i];
    }
    return realBytes;
  }

  private ByteString toByteString(int... bytes) {
    return ByteString.copyFrom(toByteArray(bytes));
  }

  private void assertValidUtf8(int[] bytes, boolean not) {
    byte[] realBytes = toByteArray(bytes);
    assertTrue(not ^ Utf8.isValidUtf8(realBytes));
    assertTrue(not ^ Utf8.isValidUtf8(realBytes, 0, bytes.length));
    ByteString lit = ByteString.copyFrom(realBytes);
    ByteString sub = lit.substring(0, bytes.length);
    assertTrue(not ^ lit.isValidUtf8());
    assertTrue(not ^ sub.isValidUtf8());
    ByteString[] ropes = {
      RopeByteString.newInstanceForTest(ByteString.EMPTY, lit),
      RopeByteString.newInstanceForTest(ByteString.EMPTY, sub),
      RopeByteString.newInstanceForTest(lit, ByteString.EMPTY),
      RopeByteString.newInstanceForTest(sub, ByteString.EMPTY),
      RopeByteString.newInstanceForTest(sub, lit)
    };
    for (ByteString rope : ropes) {
      assertTrue(not ^ rope.isValidUtf8());
    }
  }

  private void assertValidUtf8(int... bytes) {
    assertValidUtf8(bytes, false);
  }

  private void assertInvalidUtf8(int... bytes) {
    assertValidUtf8(bytes, true);
  }

  private static ByteString asBytes(String s) {
    return ByteString.copyFromUtf8(s);
  }

  public void testShardsHaveExpectedRoundTrippables() {
    // A sanity check.
    int actual = 0;
    for (Shard shard : IsValidUtf8TestUtil.FOUR_BYTE_SHARDS) {
      actual += shard.expected;
    }
    assertEquals(IsValidUtf8TestUtil.EXPECTED_FOUR_BYTE_ROUNDTRIPPABLE_COUNT,
        actual);
  }
}
