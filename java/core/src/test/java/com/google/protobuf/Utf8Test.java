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

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Random;
import java.util.regex.Pattern;
import junit.framework.TestCase;

/** Unit tests for {@link Utf8}. */
public class Utf8Test extends TestCase {
  private static final int NUM_CHARS = 16384;

  private static final Utf8.Processor safeProcessor = new Utf8.SafeProcessor();
  private static final Utf8.Processor unsafeProcessor = new Utf8.UnsafeProcessor();

  public void testEncode() {
    assertEncoding(randomString(0x80));
    assertEncoding(randomString(0x90));
    assertEncoding(randomString(0x800));
    assertEncoding(randomString(0x10000));
    assertEncoding(randomString(0x10ffff));
  }

  public void testEncode_insufficientSpace() {
    assertEncoding_insufficientSpace(randomString(0x80));
    assertEncoding_insufficientSpace(randomString(0x90));
    assertEncoding_insufficientSpace(randomString(0x800));
    assertEncoding_insufficientSpace(randomString(0x10000));
    assertEncoding_insufficientSpace(randomString(0x10ffff));
  }

  public void testValid() {
    assertIsValid(new byte[] {(byte) 0xE0, (byte) 0xB9, (byte) 0x96}, true);
    assertIsValid(new byte[] {(byte) 0xF0, (byte) 0xB2, (byte) 0x83, (byte) 0xBC}, true);
  }

  public void testOverlongIsInvalid() {
    assertIsValid(new byte[] {(byte) 0xC0, (byte) 0x81}, false);
    assertIsValid(new byte[] {(byte) 0xE0, (byte) 0x81, (byte) 0x81}, false);
    assertIsValid(new byte[] {(byte) 0xF0, (byte) 0x81, (byte) 0x81, (byte) 0x81}, false);
  }

  public void testMaxCodepointExceeded() {
    // byte1 > 0xF4
    assertIsValid(new byte[] {(byte) 0xF5, (byte) 0x81, (byte) 0x81, (byte) 0x81}, false);
  }

  public void testInvalidSurrogateCodepoint() {
    assertIsValid(new byte[] {(byte) 0xED, (byte) 0xA1, (byte) 0x81}, false);

    // byte1 == 0xF0 && byte2 < 0x90
    assertIsValid(new byte[] {(byte) 0xF0, (byte) 0x81, (byte) 0x81, (byte) 0x81}, false);
    // byte1 == 0xF4 && byte2 > 0x8F
    assertIsValid(new byte[] {(byte) 0xF4, (byte) 0x90, (byte) 0x81, (byte) 0x81}, false);
  }

  private static String randomString(int maxCodePoint) {
    final long seed = 99;
    final Random rnd = new Random(seed);
    StringBuilder sb = new StringBuilder();
    for (int j = 0; j < NUM_CHARS; j++) {
      int codePoint;
      do {
        codePoint = rnd.nextInt(maxCodePoint);
      } while (Utf8Utils.isSurrogate(codePoint));
      sb.appendCodePoint(codePoint);
    }
    return sb.toString();
  }

  private static void assertIsValid(byte[] data, boolean valid) {
    assertEquals("isValidUtf8[ARRAY]", valid, safeProcessor.isValidUtf8(data, 0, data.length));
    assertEquals(
        "isValidUtf8[ARRAY_UNSAFE]", valid, unsafeProcessor.isValidUtf8(data, 0, data.length));

    ByteBuffer buffer = ByteBuffer.wrap(data);
    assertEquals(
        "isValidUtf8[NIO_HEAP]",
        valid,
        safeProcessor.isValidUtf8(buffer, buffer.position(), buffer.remaining()));

    // Direct buffers.
    buffer = ByteBuffer.allocateDirect(data.length);
    buffer.put(data);
    buffer.flip();
    assertEquals(
        "isValidUtf8[NIO_DEFAULT]",
        valid,
        safeProcessor.isValidUtf8(buffer, buffer.position(), buffer.remaining()));
    assertEquals(
        "isValidUtf8[NIO_UNSAFE]",
        valid,
        unsafeProcessor.isValidUtf8(buffer, buffer.position(), buffer.remaining()));
  }

  private static void assertEncoding(String message) {
    byte[] expected = message.getBytes(Internal.UTF_8);
    byte[] output = encodeToByteArray(message, expected.length, safeProcessor);
    assertTrue("encodeUtf8[ARRAY]", Arrays.equals(expected, output));

    output = encodeToByteArray(message, expected.length, unsafeProcessor);
    assertTrue("encodeUtf8[ARRAY_UNSAFE]", Arrays.equals(expected, output));

    output = encodeToByteBuffer(message, expected.length, false, safeProcessor);
    assertTrue("encodeUtf8[NIO_HEAP]", Arrays.equals(expected, output));

    output = encodeToByteBuffer(message, expected.length, true, safeProcessor);
    assertTrue("encodeUtf8[NIO_DEFAULT]", Arrays.equals(expected, output));

    output = encodeToByteBuffer(message, expected.length, true, unsafeProcessor);
    assertTrue("encodeUtf8[NIO_UNSAFE]", Arrays.equals(expected, output));
  }

  private void assertEncoding_insufficientSpace(String message) {
    final int length = message.length() - 1;
    Class<ArrayIndexOutOfBoundsException> clazz = ArrayIndexOutOfBoundsException.class;

    try {
      encodeToByteArray(message, length, safeProcessor);
      fail("Expected " + clazz.getSimpleName());
    } catch (Throwable t) {
      // Expected
      assertExceptionType(t, clazz);
      // byte[] + safeProcessor will not exit early. We can't match the message since we don't
      // know which char/index due to random input.
    }

    try {
      encodeToByteArray(message, length, unsafeProcessor);
      fail("Expected " + clazz.getSimpleName());
    } catch (Throwable t) {
      assertExceptionType(t, clazz);
      // byte[] + unsafeProcessor will exit early, so we have can match the message.
      assertExceptionMessage(t, length);
    }

    try {
      encodeToByteBuffer(message, length, false, safeProcessor);
      fail("Expected " + clazz.getSimpleName());
    } catch (Throwable t) {
      // Expected
      assertExceptionType(t, clazz);
      // ByteBuffer + safeProcessor will not exit early. We can't match the message since we don't
      // know which char/index due to random input.
    }

    try {
      encodeToByteBuffer(message, length, true, safeProcessor);
      fail("Expected " + clazz.getSimpleName());
    } catch (Throwable t) {
      // Expected
      assertExceptionType(t, clazz);
      // ByteBuffer + safeProcessor will not exit early. We can't match the message since we don't
      // know which char/index due to random input.
    }

    try {
      encodeToByteBuffer(message, length, true, unsafeProcessor);
      fail("Expected " + clazz.getSimpleName());
    } catch (Throwable t) {
      // Expected
      assertExceptionType(t, clazz);
      // Direct ByteBuffer + unsafeProcessor will exit early if it's not on Android, so we can
      // match the message. On Android, a direct ByteBuffer will have hasArray() being true and
      // it will take a different code path and produces a different message.
      if (!Android.isOnAndroidDevice()) {
        assertExceptionMessage(t, length);
      }
    }
  }

  private static byte[] encodeToByteArray(String message, int length, Utf8.Processor processor) {
    byte[] output = new byte[length];
    processor.encodeUtf8(message, output, 0, output.length);
    return output;
  }

  private static byte[] encodeToByteBuffer(
      String message, int length, boolean direct, Utf8.Processor processor) {
    ByteBuffer buffer = direct ? ByteBuffer.allocateDirect(length) : ByteBuffer.allocate(length);

    processor.encodeUtf8(message, buffer);
    buffer.flip();

    byte[] output = new byte[buffer.remaining()];
    buffer.get(output);
    return output;
  }

  private <T extends Throwable> void assertExceptionType(Throwable t, Class<T> expected) {
    if (!expected.isAssignableFrom(t.getClass())) {
      fail("Expected " + expected.getSimpleName() + ", but found " + t.getClass().getSimpleName());
    }
  }

  private void assertExceptionMessage(Throwable t, int index) {
    String pattern = "Failed writing (.) at index " + index;
    assertTrue(
        t.getMessage() + " does not match pattern " + pattern,
        Pattern.matches(pattern, t.getMessage()));
  }
}
