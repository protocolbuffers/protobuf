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

import java.util.NoSuchElementException;

/**
 * This class is used to represent the substring of a {@link ByteString} over a
 * single byte array. In terms of the public API of {@link ByteString}, you end
 * up here by calling {@link ByteString#copyFrom(byte[])} followed by {@link
 * ByteString#substring(int, int)}.
 *
 * <p>This class contains most of the overhead involved in creating a substring
 * from a {@link LiteralByteString}.  The overhead involves some range-checking
 * and two extra fields.
 *
 * @author carlanton@google.com (Carl Haverl)
 */
class BoundedByteString extends LiteralByteString {

  private final int bytesOffset;
  private final int bytesLength;

  /**
   * Creates a {@code BoundedByteString} backed by the sub-range of given array,
   * without copying.
   *
   * @param bytes  array to wrap
   * @param offset index to first byte to use in bytes
   * @param length number of bytes to use from bytes
   * @throws IllegalArgumentException if {@code offset < 0}, {@code length < 0},
   *                                  or if {@code offset + length >
   *                                  bytes.length}.
   */
  BoundedByteString(byte[] bytes, int offset, int length) {
    super(bytes);
    if (offset < 0) {
      throw new IllegalArgumentException("Offset too small: " + offset);
    }
    if (length < 0) {
      throw new IllegalArgumentException("Length too small: " + offset);
    }
    if ((long) offset + length > bytes.length) {
      throw new IllegalArgumentException(
          "Offset+Length too large: " + offset + "+" + length);
    }

    this.bytesOffset = offset;
    this.bytesLength = length;
  }

  /**
   * Gets the byte at the given index.
   * Throws {@link ArrayIndexOutOfBoundsException}
   * for backwards-compatibility reasons although it would more properly be
   * {@link IndexOutOfBoundsException}.
   *
   * @param index index of byte
   * @return the value
   * @throws ArrayIndexOutOfBoundsException {@code index} is < 0 or >= size
   */
  @Override
  public byte byteAt(int index) {
    // We must check the index ourselves as we cannot rely on Java array index
    // checking for substrings.
    if (index < 0) {
      throw new ArrayIndexOutOfBoundsException("Index too small: " + index);
    }
    if (index >= size()) {
      throw new ArrayIndexOutOfBoundsException(
          "Index too large: " + index + ", " + size());
    }

    return bytes[bytesOffset + index];
  }

  @Override
  public int size() {
    return bytesLength;
  }

  @Override
  protected int getOffsetIntoBytes() {
    return bytesOffset;
  }

  // =================================================================
  // ByteString -> byte[]

  @Override
  protected void copyToInternal(byte[] target, int sourceOffset, 
      int targetOffset, int numberToCopy) {
    System.arraycopy(bytes, getOffsetIntoBytes() + sourceOffset, target,
        targetOffset, numberToCopy);
  }

  // =================================================================
  // ByteIterator

  @Override
  public ByteIterator iterator() {
    return new BoundedByteIterator();
  }

  private class BoundedByteIterator implements ByteIterator {

    private int position;
    private final int limit;

    private BoundedByteIterator() {
      position = getOffsetIntoBytes();
      limit = position + size();
    }

    public boolean hasNext() {
      return (position < limit);
    }

    public Byte next() {
      // Boxing calls Byte.valueOf(byte), which does not instantiate.
      return nextByte();
    }

    public byte nextByte() {
      if (position >= limit) {
        throw new NoSuchElementException();
      }
      return bytes[position++];
    }

    public void remove() {
      throw new UnsupportedOperationException();
    }
  }
}
