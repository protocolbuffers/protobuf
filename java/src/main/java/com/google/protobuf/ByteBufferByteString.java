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

import java.io.Externalizable;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInput;
import java.io.ObjectOutput;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.charset.Charset;
import java.util.Collections;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * A {@link ByteString} that wraps around a {@link ByteBuffer}.
 */
final class ByteBufferByteString extends ByteString implements Externalizable {
  private static final long serialVersionUID = -5543703061195105843L;

  /**
   * The underlying buffer. This is generally final, but is non-final to support serialization.
   */
  private ByteBuffer buffer;

  /**
   * This is an optimization to support quick access to a read-only version of the buffer.
   */
  private List<ByteBuffer> readOnlyBufferList;

  /**
   * Has is lazy-calculated.
   */
  private int hash;

  /**
   * Public no-arg constructor is needed by {@link Externalizable}. Do not use directly.
   */
  public ByteBufferByteString() {
  }

  ByteBufferByteString(ByteBuffer buffer) {
    if (buffer == null) {
      throw new NullPointerException("buffer");
    }

    setBuffer(buffer);
  }

  private void setBuffer(ByteBuffer buffer) {
    this.buffer = buffer;
    this.readOnlyBufferList = Collections.singletonList(buffer.asReadOnlyBuffer());
  }

  /**
   * Provides direct access to the underlying {@code ByteBuffer}. Use at your own risk.
   */
  public ByteBuffer buffer() {
    return buffer;
  }

  @Override
  public void writeExternal(ObjectOutput out) throws IOException {
    byte[] bytes;
    int offset;
    int length;

    if (buffer.hasArray()) {
      bytes = buffer.array();
      offset = buffer.arrayOffset() + buffer.position();
      length = buffer.remaining();
    } else {
      bytes = new byte[buffer.remaining()];
      offset = buffer.position();
      length = buffer.remaining();

      int pos = buffer.position();
      try {
        buffer.get(bytes);
      } finally {
        buffer.position(pos);
      }
    }

    out.writeInt(length);
    out.write(bytes, offset, length);
  }

  @Override
  public void readExternal(ObjectInput in) throws IOException, ClassNotFoundException {
    int length = in.readInt();
    byte[] bytes = new byte[length];
    in.readFully(bytes);

    setBuffer(ByteBuffer.wrap(bytes));
  }

  @Override
  public byte byteAt(int index) {
    return buffer.get(index);
  }

  @Override
  public ByteIterator iterator() {
    return new ByteIterator() {
      private final ByteBuffer slice = buffer.slice();

      @Override
      public boolean hasNext() {
        return slice.hasRemaining();
      }

      @Override
      public Byte next() {
        return nextByte();
      }

      @Override
      public byte nextByte() {
        if (!hasNext()) {
          throw new NoSuchElementException();
        }

        return slice.get();
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }

  @Override
  public int size() {
    return buffer.remaining();
  }

  @Override
  public ByteString substring(int beginIndex, int endIndex) {
    ByteBuffer slice = slice(beginIndex, endIndex);
    return new ByteBufferByteString(slice);
  }

  @Override
  protected void copyToInternal(byte[] target, int sourceOffset, int targetOffset, int numberToCopy) {
    int oldPos = buffer.position();
    try {
      buffer.position(sourceOffset);
      buffer.get(target, targetOffset, numberToCopy);
    } finally {
      buffer.position(oldPos);
    }
  }

  @Override
  public void copyTo(ByteBuffer target, int sourceOffset, int length) {
    int oldPos = buffer.position();
    try {
      ByteBuffer slice = slice(sourceOffset, sourceOffset + length);
      target.put(slice);
    } finally {
      buffer.position(oldPos);
    }
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
      int bufferOffset = buffer.arrayOffset() + buffer.position() + sourceOffset;
      out.write(buffer.array(), bufferOffset, numberToWrite);
      return;
    }

    // Slow path. Creating a channel allocates an 8KB buffer each time it's called.
    if (out instanceof FileOutputStream || numberToWrite >= 8192) {
      Channels.newChannel(out).write(slice(sourceOffset, sourceOffset + numberToWrite));
    } else {
      byte[] bytes = new byte[numberToWrite];
      this.copyToInternal(bytes, sourceOffset, 0, numberToWrite);
      out.write(bytes);
    }
  }

  @Override
  public ByteBuffer asReadOnlyByteBuffer() {
    return readOnlyBufferList.get(0);
  }

  @Override
  public List<ByteBuffer> asReadOnlyByteBufferList() {
    return readOnlyBufferList;
  }

  @Override
  protected String toStringInternal(Charset charset) {
    byte[] bytes;
    int offset;
    if(buffer.hasArray()) {
      bytes = buffer.array();
      offset = buffer.arrayOffset();
    } else {
      bytes = new byte[buffer.remaining()];
      offset = 0;
      copyToInternal(bytes, buffer.position(), offset, size());
    }
    return new String(bytes, offset, size(), charset);
  }

  @Override
  public boolean isValidUtf8() {
    byte[] bytes;
    int startIndex;
    if (buffer.hasArray()) {
      bytes = buffer.array();
      startIndex = buffer.arrayOffset() + buffer.position();
    } else {
      bytes = new byte[buffer.remaining()];
      startIndex = 0;
      copyToInternal(bytes, 0, 0, size());
    }
    return Utf8.isValidUtf8(bytes, startIndex, startIndex + size());
  }

  @Override
  protected int partialIsValidUtf8(int state, int offset, int length) {
    byte[] bytes;
    int startIndex;
    if (buffer.hasArray()) {
      bytes = buffer.array();
      startIndex = buffer.arrayOffset() + buffer.position();
    } else {
      bytes = new byte[buffer.remaining()];
      startIndex = 0;
      copyToInternal(bytes, 0, 0, size());
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
    if (other instanceof ByteBufferByteString) {
      return buffer.equals(((ByteBufferByteString) other).buffer);
    }
    if (other instanceof RopeByteString) {
      return other.equals(this);
    }
    if (other instanceof LiteralByteString) {
      LiteralByteString lbs = (LiteralByteString) other;
      ByteBuffer otherByteBuffer = ByteBuffer.wrap(lbs.bytes, lbs.getOffsetIntoBytes(), lbs.size());
      return buffer.equals(otherByteBuffer);
    }
    for(int index=0; index < size(); ++index) {
      if (byteAt(index) != otherString.byteAt(index)) {
        return false;
      }
    }
    return false;
  }

  @Override
  public int hashCode() {
    int h = hash;

    if (h == 0) {
      int size = size();
      h = partialHash(size, 0, size);
      if (h == 0) {
        h = 1;
      }
      hash = h;
    }
    return h;
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
      private int mark;

      @Override
      public synchronized void mark(int readlimit) {
        mark = buf.position();
      }

      @Override
      public boolean markSupported() {
        return true;
      }

      @Override
      public synchronized void reset() throws IOException {
        buf.position(mark);
      }

      @Override
      public int available() throws IOException {
        return buf.remaining();
      }

      public int read() throws IOException {
        if (!buf.hasRemaining()) {
          return -1;
        }
        return buf.get() & 0xFF;
      }

      public int read(byte[] bytes, int off, int len)
          throws IOException {
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

  @Override
  protected int getTreeDepth() {
    return 0;
  }

  @Override
  protected boolean isBalanced() {
    return true;
  }

  @Override
  protected int peekCachedHashCode() {
    return hash;
  }

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
