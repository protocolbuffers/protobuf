// Copyright 2008 Google Inc. All Rights Reserved.

package com.google.common.io.protocol;

import java.io.*;

/**
 * An input stream backed by another input stream, where reading from the
 * underlying input stream is limited to a fixed number of bytes. Also does
 * some buffering.
 *
 */
public class BoundInputStream extends InputStream {

  /** Buffer size */
  static final int BUF_SIZE = 4096;

  /** Number of bytes that may still be read from the underlying stream */
  private int remaining;

  /** Small buffer to avoid making OS calls for each byte read. */
  private byte[] buf;

  /** Current position in the buffer */
  private int bufPos;

  /** Filled size of the buffer */
  private int bufSize;

  /** Underlying stream to read from */
  private InputStream base;

  public BoundInputStream(InputStream base, int len) {
    this.base = base;
    this.remaining = len;

    buf = new byte[Math.min(len, BUF_SIZE)];
  }

  /**
   * Make sure there is at least one byte in the buffer. If not possible,
   * return false.
   */
  private boolean checkBuf() throws IOException {
    if (remaining <= 0) {
      return false;
    }

    if (bufPos >= bufSize) {
      bufSize = base.read(buf, 0, Math.min(remaining, buf.length));
      if (bufSize <= 0) {
        remaining = 0;
        return false;
      }
      bufPos = 0;
    }
    return true;
  }

  public int available() {
    return bufSize - bufPos;
  }

  public int read() throws IOException {
    if (!checkBuf()) {
      return -1;
    }
    remaining--;
    return buf[bufPos++] & 255;
  }

  public int read(byte[] data, int start, int count) throws IOException {
    if (!checkBuf()) {
      return -1;
    }
    count = Math.min(count, bufSize - bufPos);
    System.arraycopy(buf, bufPos, data, start, count);
    bufPos += count;
    remaining -= count;
    return count;
  }

  /**
   * How many bytes are remaining, based on the length provided to the
   * constructor. The underlying stream may terminate earlier. Provided mainly
   * for testing purposes.
   */
  public int getRemaining() {
    return remaining;
  }
}
