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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Random;
import junit.framework.TestCase;

/**
 * Tests for {@link ByteBufferWriter}.
 */
public class ByteBufferWriterTest extends TestCase {

  public void testHeapBuffer() throws IOException {
    // Test a small and large buffer.
    testWrite(ByteBuffer.allocate(100));
    testWrite(ByteBuffer.allocate(1024 * 100));
  }

  public void testDirectBuffer() throws IOException {
    // Test a small and large buffer.
    testWrite(ByteBuffer.allocateDirect(100));
    testWrite(ByteBuffer.allocateDirect(1024 * 100));
  }

  private void testWrite(ByteBuffer buffer) throws IOException {
    fillRandom(buffer);
    ByteArrayOutputStream os = new ByteArrayOutputStream(buffer.remaining());
    ByteBufferWriter.write(buffer, os);
    assertEquals(0, buffer.position());
    assertTrue(Arrays.equals(toArray(buffer), os.toByteArray()));
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
