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

import com.google.protobuf.Utf8.Processor;
import com.google.protobuf.Utf8.SafeProcessor;
import com.google.protobuf.Utf8.UnsafeProcessor;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;
import junit.framework.TestCase;

public class DecodeUtf8Test extends TestCase {
  private static Logger logger = Logger.getLogger(DecodeUtf8Test.class.getName());

  private static final Processor SAFE_PROCESSOR = new SafeProcessor();
  private static final Processor UNSAFE_PROCESSOR = new UnsafeProcessor();

  public void testRoundTripAllValidChars() throws Exception {
    for (int i = Character.MIN_CODE_POINT; i < Character.MAX_CODE_POINT; i++) {
      if (i < Character.MIN_SURROGATE || i > Character.MAX_SURROGATE) {
        String str = new String(Character.toChars(i));
        assertRoundTrips(str);
      }
    }
  }

  // Test all 1, 2, 3 invalid byte combinations. Valid ones would have been covered above.

  public void testOneByte() throws Exception {
    int valid = 0;
    ByteBuffer buffer = ByteBuffer.allocateDirect(1);
    for (int i = Byte.MIN_VALUE; i <= Byte.MAX_VALUE; i++) {
      ByteString bs = ByteString.copyFrom(new byte[] {(byte) i});
      if (bs.isValidUtf8()) {
        valid++;
      } else {
        assertInvalid(bs.toByteArray(), buffer);
      }
    }
    assertEquals(IsValidUtf8TestUtil.EXPECTED_ONE_BYTE_ROUNDTRIPPABLE_COUNT, valid);
  }

  public void testTwoBytes() throws Exception {
    int valid = 0;
    ByteBuffer buffer = ByteBuffer.allocateDirect(2);
    for (int i = Byte.MIN_VALUE; i <= Byte.MAX_VALUE; i++) {
      for (int j = Byte.MIN_VALUE; j <= Byte.MAX_VALUE; j++) {
        ByteString bs = ByteString.copyFrom(new byte[] {(byte) i, (byte) j});
        if (bs.isValidUtf8()) {
          valid++;
        } else {
          assertInvalid(bs.toByteArray(), buffer);
        }
      }
    }
    assertEquals(IsValidUtf8TestUtil.EXPECTED_TWO_BYTE_ROUNDTRIPPABLE_COUNT, valid);
  }

  public void testThreeBytes() throws Exception {
    int count = 0;
    int valid = 0;
    ByteBuffer buffer = ByteBuffer.allocateDirect(3);
    for (int i = Byte.MIN_VALUE; i <= Byte.MAX_VALUE; i++) {
      for (int j = Byte.MIN_VALUE; j <= Byte.MAX_VALUE; j++) {
        for (int k = Byte.MIN_VALUE; k <= Byte.MAX_VALUE; k++) {
          byte[] bytes = new byte[] {(byte) i, (byte) j, (byte) k};
          ByteString bs = ByteString.copyFrom(bytes);
          if (bs.isValidUtf8()) {
            valid++;
          } else {
            assertInvalid(bytes, buffer);
          }
          count++;
          if (count % 1000000L == 0) {
            logger.info("Processed " + (count / 1000000L) + " million characters");
          }
        }
      }
    }
    assertEquals(IsValidUtf8TestUtil.EXPECTED_THREE_BYTE_ROUNDTRIPPABLE_COUNT, valid);
  }

  /** Tests that round tripping of a sample of four byte permutations work. */
  public void testInvalid_4BytesSamples() throws Exception {
    // Bad trailing bytes
    assertInvalid(0xF0, 0xA4, 0xAD, 0x7F);
    assertInvalid(0xF0, 0xA4, 0xAD, 0xC0);

    // Special cases for byte2
    assertInvalid(0xF0, 0x8F, 0xAD, 0xA2);
    assertInvalid(0xF4, 0x90, 0xAD, 0xA2);
  }

  public void testRealStrings() throws Exception {
    // English
    assertRoundTrips("The quick brown fox jumps over the lazy dog");
    // German
    assertRoundTrips("Quizdeltagerne spiste jordb\u00e6r med fl\u00f8de, mens cirkusklovnen");
    // Japanese
    assertRoundTrips("\u3044\u308d\u306f\u306b\u307b\u3078\u3068\u3061\u308a\u306c\u308b\u3092");
    // Hebrew
    assertRoundTrips(
        "\u05d3\u05d2 \u05e1\u05e7\u05e8\u05df \u05e9\u05d8 \u05d1\u05d9\u05dd "
            + "\u05de\u05d0\u05d5\u05db\u05d6\u05d1 \u05d5\u05dc\u05e4\u05ea\u05e2"
            + " \u05de\u05e6\u05d0 \u05dc\u05d5 \u05d7\u05d1\u05e8\u05d4 "
            + "\u05d0\u05d9\u05da \u05d4\u05e7\u05dc\u05d9\u05d8\u05d4");
    // Thai
    assertRoundTrips(
        " \u0e08\u0e07\u0e1d\u0e48\u0e32\u0e1f\u0e31\u0e19\u0e1e\u0e31\u0e12"
            + "\u0e19\u0e32\u0e27\u0e34\u0e0a\u0e32\u0e01\u0e32\u0e23");
    // Chinese
    assertRoundTrips(
        "\u8fd4\u56de\u94fe\u4e2d\u7684\u4e0b\u4e00\u4e2a\u4ee3\u7406\u9879\u9009\u62e9\u5668");
    // Chinese with 4-byte chars
    assertRoundTrips(
        "\uD841\uDF0E\uD841\uDF31\uD841\uDF79\uD843\uDC53\uD843\uDC78"
            + "\uD843\uDC96\uD843\uDCCF\uD843\uDCD5\uD843\uDD15\uD843\uDD7C\uD843\uDD7F"
            + "\uD843\uDE0E\uD843\uDE0F\uD843\uDE77\uD843\uDE9D\uD843\uDEA2");
    // Mixed
    assertRoundTrips(
        "The quick brown \u3044\u308d\u306f\u306b\u307b\u3078\u8fd4\u56de\u94fe"
            + "\u4e2d\u7684\u4e0b\u4e00");
  }

  public void testOverlong() throws Exception {
    assertInvalid(0xc0, 0xaf);
    assertInvalid(0xe0, 0x80, 0xaf);
    assertInvalid(0xf0, 0x80, 0x80, 0xaf);

    // Max overlong
    assertInvalid(0xc1, 0xbf);
    assertInvalid(0xe0, 0x9f, 0xbf);
    assertInvalid(0xf0, 0x8f, 0xbf, 0xbf);

    // null overlong
    assertInvalid(0xc0, 0x80);
    assertInvalid(0xe0, 0x80, 0x80);
    assertInvalid(0xf0, 0x80, 0x80, 0x80);
  }

  public void testIllegalCodepoints() throws Exception {
    // Single surrogate
    assertInvalid(0xed, 0xa0, 0x80);
    assertInvalid(0xed, 0xad, 0xbf);
    assertInvalid(0xed, 0xae, 0x80);
    assertInvalid(0xed, 0xaf, 0xbf);
    assertInvalid(0xed, 0xb0, 0x80);
    assertInvalid(0xed, 0xbe, 0x80);
    assertInvalid(0xed, 0xbf, 0xbf);

    // Paired surrogates
    assertInvalid(0xed, 0xa0, 0x80, 0xed, 0xb0, 0x80);
    assertInvalid(0xed, 0xa0, 0x80, 0xed, 0xbf, 0xbf);
    assertInvalid(0xed, 0xad, 0xbf, 0xed, 0xb0, 0x80);
    assertInvalid(0xed, 0xad, 0xbf, 0xed, 0xbf, 0xbf);
    assertInvalid(0xed, 0xae, 0x80, 0xed, 0xb0, 0x80);
    assertInvalid(0xed, 0xae, 0x80, 0xed, 0xbf, 0xbf);
    assertInvalid(0xed, 0xaf, 0xbf, 0xed, 0xb0, 0x80);
    assertInvalid(0xed, 0xaf, 0xbf, 0xed, 0xbf, 0xbf);
  }

  public void testBufferSlice() throws Exception {
    String str = "The quick brown fox jumps over the lazy dog";
    assertRoundTrips(str, 10, 4);
    assertRoundTrips(str, str.length(), 0);
  }

  public void testInvalidBufferSlice() throws Exception {
    byte[] bytes = "The quick brown fox jumps over the lazy dog".getBytes(Internal.UTF_8);
    assertInvalidSlice(bytes, bytes.length - 3, 4);
    assertInvalidSlice(bytes, bytes.length, 1);
    assertInvalidSlice(bytes, bytes.length + 1, 0);
    assertInvalidSlice(bytes, 0, bytes.length + 1);
  }

  private void assertInvalid(int... bytesAsInt) throws Exception {
    byte[] bytes = new byte[bytesAsInt.length];
    for (int i = 0; i < bytesAsInt.length; i++) {
      bytes[i] = (byte) bytesAsInt[i];
    }
    assertInvalid(bytes, null);
  }

  // Attempts to decode the byte array in several ways and asserts that it always generates an
  // exception. Allocating a direct ByteBuffer is slow, so the caller can optionally provide a
  // buffer to reuse. If buffer is non-null, it must be a direct-allocated ByteBuffer of the
  // appropriate size.
  private void assertInvalid(byte[] bytes, ByteBuffer buffer) throws Exception {
    try {
      UNSAFE_PROCESSOR.decodeUtf8(bytes, 0, bytes.length);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
    try {
      SAFE_PROCESSOR.decodeUtf8(bytes, 0, bytes.length);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }

    if (buffer == null) {
      buffer = ByteBuffer.allocateDirect(bytes.length);
    }
    buffer.put(bytes);
    buffer.flip();
    try {
      UNSAFE_PROCESSOR.decodeUtf8(buffer, 0, bytes.length);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
    try {
      SAFE_PROCESSOR.decodeUtf8(buffer, 0, bytes.length);
      fail();
    } catch (InvalidProtocolBufferException e) {
      // Expected.
    }
    buffer.clear();
  }

  private void assertInvalidSlice(byte[] bytes, int index, int size) throws Exception {
    try {
      UNSAFE_PROCESSOR.decodeUtf8(bytes, index, size);
      fail();
    } catch (ArrayIndexOutOfBoundsException e) {
      // Expected.
    }
    try {
      SAFE_PROCESSOR.decodeUtf8(bytes, index, size);
      fail();
    } catch (ArrayIndexOutOfBoundsException e) {
      // Expected.
    }

    ByteBuffer direct = ByteBuffer.allocateDirect(bytes.length);
    direct.put(bytes);
    direct.flip();
    try {
      UNSAFE_PROCESSOR.decodeUtf8(direct, index, size);
      fail();
    } catch (ArrayIndexOutOfBoundsException e) {
      // Expected.
    }
    try {
      SAFE_PROCESSOR.decodeUtf8(direct, index, size);
      fail();
    } catch (ArrayIndexOutOfBoundsException e) {
      // Expected.
    }

    ByteBuffer heap = ByteBuffer.allocate(bytes.length);
    heap.put(bytes);
    heap.flip();
    try {
      UNSAFE_PROCESSOR.decodeUtf8(heap, index, size);
      fail();
    } catch (ArrayIndexOutOfBoundsException e) {
      // Expected.
    }
    try {
      SAFE_PROCESSOR.decodeUtf8(heap, index, size);
      fail();
    } catch (ArrayIndexOutOfBoundsException e) {
      // Expected.
    }
  }

  private void assertRoundTrips(String str) throws Exception {
    assertRoundTrips(str, 0, -1);
  }

  private void assertRoundTrips(String str, int index, int size) throws Exception {
    byte[] bytes = str.getBytes(Internal.UTF_8);
    if (size == -1) {
      size = bytes.length;
    }
    assertDecode(
        new String(bytes, index, size, Internal.UTF_8),
        UNSAFE_PROCESSOR.decodeUtf8(bytes, index, size));
    assertDecode(
        new String(bytes, index, size, Internal.UTF_8),
        SAFE_PROCESSOR.decodeUtf8(bytes, index, size));

    ByteBuffer direct = ByteBuffer.allocateDirect(bytes.length);
    direct.put(bytes);
    direct.flip();
    assertDecode(
        new String(bytes, index, size, Internal.UTF_8),
        UNSAFE_PROCESSOR.decodeUtf8(direct, index, size));
    assertDecode(
        new String(bytes, index, size, Internal.UTF_8),
        SAFE_PROCESSOR.decodeUtf8(direct, index, size));

    ByteBuffer heap = ByteBuffer.allocate(bytes.length);
    heap.put(bytes);
    heap.flip();
    assertDecode(
        new String(bytes, index, size, Internal.UTF_8),
        UNSAFE_PROCESSOR.decodeUtf8(heap, index, size));
    assertDecode(
        new String(bytes, index, size, Internal.UTF_8),
        SAFE_PROCESSOR.decodeUtf8(heap, index, size));
  }

  private void assertDecode(String expected, String actual) {
    if (!expected.equals(actual)) {
      fail("Failure: Expected (" + codepoints(expected) + ") Actual (" + codepoints(actual) + ")");
    }
  }

  private List<String> codepoints(String str) {
    List<String> codepoints = new ArrayList<String>();
    for (int i = 0; i < str.length(); i++) {
      codepoints.add(Long.toHexString(str.charAt(i)));
    }
    return codepoints;
  }
}
