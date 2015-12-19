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

import junit.framework.TestCase;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Random;

/**
 * Tests for {@link PlatformDependent}.
 */
public class PlatformDependentTest  extends TestCase {
  private static final Charset UTF_8 = Charset.forName("UTF-8");

  public void testUtfEncoding() {
    testUtf8Encoding(0x80);
    testUtf8Encoding(0x90);
    testUtf8Encoding(0x800);
    testUtf8Encoding(0x10000);
    testUtf8Encoding(0x10ffff);
  }

  private void testUtf8Encoding(int maxCodePoint) {
    String input = randomString(maxCodePoint);
    byte[] expected = input.getBytes(UTF_8);

    byte[] actualToHeap = bytes(encodeHeapByteBuf(input));
    assertTrue(Arrays.equals(expected, actualToHeap));

    byte[] actualToDirect = bytes(encodeDirectByteBuf(input));
    assertTrue(Arrays.equals(expected, actualToDirect));

    if (PlatformDependent.UnsafeDirectByteBuffer.isAvailable()) {
      byte[] actualToUnsafeDirect = bytes(encodeUnsafeDirectByteBuf(input));
      assertTrue(Arrays.equals(expected, actualToUnsafeDirect));
    }
  }

  private ByteBuffer encodeHeapByteBuf(String input) {
    ByteBuffer buffer = ByteBuffer.allocate(input.length() * Utf8.MAX_BYTES_PER_CHAR);
    PlatformDependent.encodeToHeap(input, buffer);
    buffer.flip();
    return buffer;
  }

  private ByteBuffer encodeDirectByteBuf(String input) {
    ByteBuffer buffer = ByteBuffer.allocateDirect(input.length() * Utf8.MAX_BYTES_PER_CHAR);
    PlatformDependent.encodeToDirect(input, buffer);
    buffer.flip();
    return buffer;
  }

  private ByteBuffer encodeUnsafeDirectByteBuf(String input) {
    ByteBuffer buffer = ByteBuffer.allocateDirect(input.length() * Utf8.MAX_BYTES_PER_CHAR);
    PlatformDependent.unsafeEncodeToDirect(input, buffer);
    buffer.flip();
    return buffer;
  }

  private byte[] bytes(ByteBuffer buffer) {
    byte[] bytes = new byte[buffer.remaining()];
    buffer.get(bytes);
    return bytes;
  }

  private String randomString(int maxCodePoint) {
    final long seed = 99;
    final Random rnd = new Random(seed);
    StringBuilder sb = new StringBuilder();
    for (int j = 0; j < 100; j++) {
      int codePoint;
      do {
        codePoint = rnd.nextInt(maxCodePoint);
      } while (isSurrogate(codePoint));
      sb.appendCodePoint(codePoint);
    }
    return sb.toString();
  }

  /** Character.isSurrogate was added in Java SE 7. */
  private static boolean isSurrogate(int c) {
    return Character.MIN_HIGH_SURROGATE <= c && c <= Character.MAX_LOW_SURROGATE;
  }
}
