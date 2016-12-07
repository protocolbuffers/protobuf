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

import java.io.IOException;
import java.io.InputStream;
import java.io.InvalidObjectException;
import java.io.ObjectInputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A {@link ByteString} that wraps around a {@link ByteInput}.
 */
final class ByteInputByteString extends ByteString.LeafByteString {
  private final ByteInput buffer;
  private final int offset, length;

  ByteInputByteString(ByteInput buffer, int offset, int length) {
    if (buffer == null) {
      throw new NullPointerException("buffer");
    }
    this.buffer = buffer;
    this.offset = offset;
    this.length = length;
  }

  // =================================================================
  // Serializable

  /**
   * Magic method that lets us override serialization behavior.
   */
  private Object writeReplace() {
    return ByteString.wrap(toByteArray());
  }

  /**
   * Magic method that lets us override deserialization behavior.
   */
  private void readObject(@SuppressWarnings("unused") ObjectInputStream in) throws IOException {
    throw new InvalidObjectException("ByteInputByteString instances are not to be serialized directly");
  }

  // =================================================================

  @Override
  public byte byteAt(int index) {
    return buffer.read(getAbsoluteOffset(index));
  }

  private int getAbsoluteOffset(int relativeOffset) {
    return this.offset + relativeOffset;
  }

  @Override
  public int size() {
    return length;
  }

  @Override
  public ByteString substring(int beginIndex, int endIndex) {
    if (beginIndex < 0 || beginIndex >= size() || endIndex < beginIndex || endIndex >= size()) {
      throw new IllegalArgumentException(
          String.format("Invalid indices [%d, %d]", beginIndex, endIndex));
    }
    return new ByteInputByteString(this.buffer, getAbsoluteOffset(beginIndex), endIndex - beginIndex);
  }

  @Override
  protected void copyToInternal(
      byte[] target, int sourceOffset, int targetOffset, int numberToCopy) {
    this.buffer.read(getAbsoluteOffset(sourceOffset), target, targetOffset, numberToCopy);
  }

  @Override
  public void copyTo(ByteBuffer target) {
    this.buffer.read(this.offset, target);
  }

  @Override
  public void writeTo(OutputStream out) throws IOException {
    out.write(toByteArray());
  }

  @Override
  boolean equalsRange(ByteString other, int offset, int length) {
    return substring(0, length).equals(other.substring(offset, offset + length));
  }

  @Override
  void writeToInternal(OutputStream out, int sourceOffset, int numberToWrite) throws IOException {
    byte[] buf = ByteBufferWriter.getOrCreateBuffer(numberToWrite);
    this.buffer.read(getAbsoluteOffset(sourceOffset), buf, 0, numberToWrite);
    out.write(buf, 0, numberToWrite);
  }

  @Override
  void writeTo(ByteOutput output) throws IOException {
    output.writeLazy(toByteArray(), 0, length);
  }

  @Override
  public ByteBuffer asReadOnlyByteBuffer() {
    return ByteBuffer.wrap(toByteArray()).asReadOnlyBuffer();
  }

  @Override
  public List<ByteBuffer> asReadOnlyByteBufferList() {
    return Collections.singletonList(asReadOnlyByteBuffer());
  }

  @Override
  protected String toStringInternal(Charset charset) {
    byte[] bytes = toByteArray();
    return new String(bytes, 0, bytes.length, charset);
  }

  @Override
  public boolean isValidUtf8() {
    return Utf8.isValidUtf8(buffer, offset, offset + length);
  }

  @Override
  protected int partialIsValidUtf8(int state, int offset, int length) {
    int off = getAbsoluteOffset(offset);
    return Utf8.partialIsValidUtf8(state, buffer, off, off + length);
  }

  @Override
  public boolean equals(Object other) {
    if (other == this) {
      return true;
    }
    if (!(other instanceof ByteString)) {
      return false;
    }
    ByteString otherString = ((ByteString) other);
    if (size() != otherString.size()) {
      return false;
    }
    if (size() == 0) {
      return true;
    }
    if (other instanceof RopeByteString) {
      return other.equals(this);
    }
    return Arrays.equals(this.toByteArray(), otherString.toByteArray());
  }

  @Override
  protected int partialHash(int h, int offset, int length) {
    offset = getAbsoluteOffset(offset);
    int end = offset + length;
    for (int i = offset; i < end; i++) {
      h = h * 31 + buffer.read(i);
    }
    return h;
  }

  @Override
  public InputStream newInput() {
    return new InputStream() {
      private final ByteInput buf = buffer;
      private int pos = offset;
      private int limit = pos + length;
      private int mark = pos;

      @Override
      public void mark(int readlimit) {
        this.mark = readlimit;
      }

      @Override
      public boolean markSupported() {
        return true;
      }

      @Override
      public void reset() throws IOException {
        this.pos = this.mark;
      }

      @Override
      public int available() throws IOException {
        return this.limit - this.pos;
      }

      @Override
      public int read() throws IOException {
        if (available() <= 0) {
          return -1;
        }
        return this.buf.read(pos++) & 0xFF;
      }

      @Override
      public int read(byte[] bytes, int off, int len) throws IOException {
        int remain = available();
        if (remain <= 0) {
          return -1;
        }
        len = Math.min(len, remain);
        buf.read(pos, bytes, off, len);
        pos += len;
        return len;
      }
    };
  }

  @Override
  public CodedInputStream newCodedInput() {
    // We trust CodedInputStream not to modify the bytes, or to give anyone
    // else access to them.
    CodedInputStream cis = CodedInputStream.newInstance(buffer, offset, length, true);
    cis.enableAliasing(true);
    return cis;
  }
}