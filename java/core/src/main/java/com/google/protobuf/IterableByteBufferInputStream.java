// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.EMPTY_BYTE_BUFFER;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Iterator;

class IterableByteBufferInputStream extends InputStream {
  /** The {@link Iterator} with type {@link ByteBuffer} of {@code input} */
  private Iterator<ByteBuffer> iterator;
  /** The current ByteBuffer; */
  private ByteBuffer currentByteBuffer;
  /** The number of ByteBuffers in the input data. */
  private int dataSize;
  /**
   * Current {@code ByteBuffer}'s index
   *
   * <p>If index equals dataSize, then all the data in the InputStream has been consumed
   */
  private int currentIndex;
  /** The current position for current ByteBuffer */
  private int currentByteBufferPos;
  /** Whether current ByteBuffer has an array */
  private boolean hasArray;
  /**
   * If the current ByteBuffer is unsafe-direct based, currentArray is null; otherwise should be the
   * array inside ByteBuffer.
   */
  private byte[] currentArray;
  /** Current ByteBuffer's array offset */
  private int currentArrayOffset;
  /**
   * If the current ByteBuffer is unsafe-direct based, currentAddress is the start address of this
   * ByteBuffer; otherwise should be zero.
   */
  private long currentAddress;

  IterableByteBufferInputStream(Iterable<ByteBuffer> data) {
    iterator = data.iterator();
    dataSize = 0;
    for (ByteBuffer unused : data) {
      dataSize++;
    }
    currentIndex = -1;

    if (!getNextByteBuffer()) {
      currentByteBuffer = EMPTY_BYTE_BUFFER;
      currentIndex = 0;
      currentByteBufferPos = 0;
      currentAddress = 0;
    }
  }

  private boolean getNextByteBuffer() {
    // The loop ensures we skip empty buffers (see
    // https://github.com/protocolbuffers/protobuf/issues/17850)
    do {
      currentIndex++;
      if (!iterator.hasNext()) {
        return false;
      }
      currentByteBuffer = iterator.next();
    } while (!currentByteBuffer.hasRemaining());
    currentByteBufferPos = currentByteBuffer.position();
    if (currentByteBuffer.hasArray()) {
      hasArray = true;
      currentArray = currentByteBuffer.array();
      currentArrayOffset = currentByteBuffer.arrayOffset();
    } else {
      hasArray = false;
      currentAddress = UnsafeUtil.addressOffset(currentByteBuffer);
      currentArray = null;
    }
    return true;
  }

  private void updateCurrentByteBufferPos(int numberOfBytesRead) {
    currentByteBufferPos += numberOfBytesRead;
    if (currentByteBufferPos == currentByteBuffer.limit()) {
      getNextByteBuffer();
    }
  }

  @Override
  public int read() throws IOException {
    if (currentIndex == dataSize) {
      return -1;
    }
    if (hasArray) {
      int result = currentArray[currentByteBufferPos + currentArrayOffset] & 0xFF;
      updateCurrentByteBufferPos(1);
      return result;
    } else {
      int result = UnsafeUtil.getByte(currentByteBufferPos + currentAddress) & 0xFF;
      updateCurrentByteBufferPos(1);
      return result;
    }
  }

  @Override
  public int read(byte[] output, int offset, int length) throws IOException {
    if (currentIndex == dataSize) {
      return -1;
    }
    int remaining = currentByteBuffer.limit() - currentByteBufferPos;
    if (length > remaining) {
      length = remaining;
    }
    if (hasArray) {
      System.arraycopy(
          currentArray, currentByteBufferPos + currentArrayOffset, output, offset, length);
      updateCurrentByteBufferPos(length);
    } else {
      int prevPos = currentByteBuffer.position();
      Java8Compatibility.position(currentByteBuffer, currentByteBufferPos);
      currentByteBuffer.get(output, offset, length);
      Java8Compatibility.position(currentByteBuffer, prevPos);
      updateCurrentByteBufferPos(length);
    }
    return length;
  }
}
