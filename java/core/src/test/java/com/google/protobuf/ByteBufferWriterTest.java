// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Random;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link ByteBufferWriter}. */
@RunWith(JUnit4.class)
public class ByteBufferWriterTest {

  @Test
  public void testHeapBuffer() throws IOException {
    // Test a small and large buffer.
    testWrite(ByteBuffer.allocate(100));
    testWrite(ByteBuffer.allocate(1024 * 100));
  }

  @Test
  public void testDirectBuffer() throws IOException {
    // Test a small and large buffer.
    testWrite(ByteBuffer.allocateDirect(100));
    testWrite(ByteBuffer.allocateDirect(1024 * 100));
  }

  private void testWrite(ByteBuffer buffer) throws IOException {
    fillRandom(buffer);
    ByteArrayOutputStream os = new ByteArrayOutputStream(buffer.remaining());
    ByteBufferWriter.write(buffer, os);
    assertThat(buffer.position()).isEqualTo(0);
    assertThat(Arrays.equals(toArray(buffer), os.toByteArray())).isTrue();
  }

  private void fillRandom(ByteBuffer buf) {
    byte[] bytes = new byte[buf.remaining()];
    new Random().nextBytes(bytes);
    buf.put(bytes);
    buf.flip();
    return;
  }

  private byte[] toArray(ByteBuffer buf) {
    int originalPosition = buf.position();
    byte[] bytes = new byte[buf.remaining()];
    buf.get(bytes);
    buf.position(originalPosition);
    return bytes;
  }
}
