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

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InvalidObjectException;
import java.io.ObjectInputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.InvalidMarkException;
import java.nio.channels.Channels;
import java.nio.charset.Charset;
import java.util.Collections;
import java.util.List;

/**
 * A {@link ByteString} that wraps around a {@link ByteBuffer}.
 */
final class NioByteString extends ByteString.LeafByteString {
  private final ByteBuffer buffer;

  NioByteString(ByteBuffer buffer) {
    if (buffer == null) {
      throw new NullPointerException("buffer");
    }

    this.buffer = buffer.slice();
  }

  // =================================================================
  // Serializable

  /**
   * Magic method that lets us override serialization behavior.
   */
  private Object writeReplace() {
    return ByteString.copyFrom(buffer.slice());
  }

  /**
   * Magic method that lets us override deserialization behavior.
   */
  private void readObject(@SuppressWarnings("unused") ObjectInputStream in) throws IOException {
    throw new InvalidObjectException("NioByteString instances are not to be serialized directly");
  }

  // =================================================================

  @Override
  public byte byteAt(int index) {
    try {
      return buffer.get(index);
    } catch (ArrayIndexOutOfBoundsException e) {
      throw e;
    } catch (IndexOutOfBoundsException e) {
      throw new ArrayIndexOutOfBoundsException(e.getMessage());
    }
  }

  @Override
  public int size() {
    return buffer.remaining();
  }

  @Override
  public ByteString substring(int beginIndex, int endIndex) {
    try {
      ByteBuffer slice = slice(beginIndex, endIndex);
      return new NioByteString(slice);
    } catch (ArrayIndexOutOfBoundsException e) {
      throw e;
    } catch (IndexOutOfBoundsException e) {
      throw new ArrayIndexOutOfBoundsException(e.getMessage());
    }
  }

  @Override
  protected void copyToInternal(
      byte[] target, int sourceOffset, int targetOffset, int numberToCopy) {
    ByteBuffer slice = buffer.slice();
    slice.position(sourceOffset);
    slice.get(target, targetOffset, numberToCopy);
  }

  @Override
  public void copyTo(ByteBuffer target) {
    target.put(buffer.slice());
  }

  @Override
  public void writeTo(OutputStream out) throws IOException {
    writeToInternal(out, buffer.position(), buffer.remaining());
  }

  @Override
  boolean equalsRange(ByteString other, int offset, int length) {
    return substring(0, length).equals(other.substring(offset, offset + length));
  }

  @Override
  void writeToInternal(OutputStream out, int sourceOffset, int numberToWrite) throws IOException {
    if (buffer.hasArray()) {
      // Optimized write for array-backed buffers.
      // Note that we're taking the risk that a malicious OutputStream could modify the array.
      int bufferOffset = buffer.arrayOffset() + buffer.position() + sourceOffset;
      out.write(buffer.array(), bufferOffset, numberToWrite);
      return;
    }

    // Slow path
    if (out instanceof FileOutputStream || numberToWrite >= 8192) {
      // Use a channel to write out the ByteBuffer.
      Channels.newChannel(out).write(slice(sourceOffset, sourceOffset + numberToWrite));
    } else {
      // Just copy the data to an array and write it.
      out.write(toByteArray());
    }
  }

  @Override
  public ByteBuffer asReadOnlyByteBuffer() {
    return buffer.asReadOnlyBuffer();
  }

  @Override
  public List<ByteBuffer> asReadOnlyByteBufferList() {
    return Collections.singletonList(asReadOnlyByteBuffer());
  }

  @Override
  protected String toStringInternal(Charset charset) {
    byte[] bytes;
    int offset;
    if (buffer.hasArray()) {
      bytes = buffer.array();
      offset = buffer.arrayOffset() + buffer.position();
    } else {
      bytes = toByteArray();
      offset = 0;
    }
    return new String(bytes, offset, size(), charset);
  }

  @Override
  public boolean isValidUtf8() {
    // TODO(nathanmittler): add a ByteBuffer fork for Utf8.isValidUtf8 to avoid the copy
    byte[] bytes;
    int startIndex;
    if (buffer.hasArray()) {
      bytes = buffer.array();
      startIndex = buffer.arrayOffset() + buffer.position();
    } else {
      bytes = toByteArray();
      startIndex = 0;
    }
    return Utf8.isValidUtf8(bytes, startIndex, startIndex + size());
  }

  @Override
  protected int partialIsValidUtf8(int state, int offset, int length) {
    // TODO(nathanmittler): TODO add a ByteBuffer fork for Utf8.partialIsValidUtf8 to avoid the copy
    byte[] bytes;
    int startIndex;
    if (buffer.hasArray()) {
      bytes = buffer.array();
      startIndex = buffer.arrayOffset() + buffer.position();
    } else {
      bytes = toByteArray();
      startIndex = 0;
    }
    return Utf8.partialIsValidUtf8(state, bytes, startIndex, startIndex + size());
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
    if (other instanceof NioByteString) {
      return buffer.equals(((NioByteString) other).buffer);
    }
    if (other instanceof RopeByteString) {
      return other.equals(this);
    }
    return buffer.equals(otherString.asReadOnlyByteBuffer());
  }

  @Override
  protected int partialHash(int h, int offset, int length) {
    for (int i = offset; i < offset + length; i++) {
      h = h * 31 + buffer.get(i);
    }
    return h;
  }

  @Override
  public InputStream newInput() {
    return new InputStream() {
      private final ByteBuffer buf = buffer.slice();

      @Override
      public void mark(int readlimit) {
        buf.mark();
      }

      @Override
      public boolean markSupported() {
        return true;
      }

      @Override
      public void reset() throws IOException {
        try {
          buf.reset();
        } catch (InvalidMarkException e) {
          throw new IOException(e);
        }
      }

      @Override
      public int available() throws IOException {
        return buf.remaining();
      }

      @Override
      public int read() throws IOException {
        if (!buf.hasRemaining()) {
          return -1;
        }
        return buf.get() & 0xFF;
      }

      @Override
      public int read(byte[] bytes, int off, int len) throws IOException {
        if (!buf.hasRemaining()) {
          return -1;
        }

        len = Math.min(len, buf.remaining());
        buf.get(bytes, off, len);
        return len;
      }
    };
  }

  @Override
  public CodedInputStream newCodedInput() {
    return CodedInputStream.newInstance(buffer);
  }

  /**
   * Creates a slice of a range of this buffer.
   *
   * @param beginIndex the beginning index of the slice (inclusive).
   * @param endIndex the end index of the slice (exclusive).
   * @return the requested slice.
   */
  private ByteBuffer slice(int beginIndex, int endIndex) {
    if (beginIndex < buffer.position() || endIndex > buffer.limit() || beginIndex > endIndex) {
      throw new IllegalArgumentException(
          String.format("Invalid indices [%d, %d]", beginIndex, endIndex));
    }

    ByteBuffer slice = buffer.slice();
    slice.position(beginIndex - buffer.position());
    slice.limit(endIndex - buffer.position());
    return slice;
  }
}
