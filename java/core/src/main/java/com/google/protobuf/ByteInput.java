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
import java.nio.ByteBuffer;

/**
 * An input for raw bytes. This is similar to an InputStream but it is offset addressable. All the
 * read APIs are relative.
 */
@ExperimentalApi
public abstract class ByteInput {

  /**
   * Reads a single byte from the given offset.
   * @param offset The offset from where byte to be read
   * @return The byte of data at given offset
   */
  public abstract byte read(int offset);

  /**
   * Reads bytes of data from the given offset into an array of bytes.
   * @param offset src offset within this ByteInput from where data to be read.
   * @param out Destination byte array to read data into.
   * @return The number of bytes read from ByteInput
   */
  public int read(int offset, byte b[]) throws IOException {
    return read(offset, b, 0, b.length);
  }

  /**
   * Reads up to <code>len</code> bytes of data from the given offset into an array of bytes.
   * @param offset src offset within this ByteInput from where data to be read.
   * @param out Destination byte array to read data into.
   * @param outOffset Offset within the the out byte[] where data to be read into.
   * @param len The number of bytes to read.
   * @return The number of bytes read from ByteInput
   */
  public abstract int read(int offset, byte[] out, int outOffset, int len);

  /**
   * Reads bytes of data from the given offset into given {@link ByteBuffer}.
   * @param offset src offset within this ByteInput from where data to be read.
   * @param out Destination {@link ByteBuffer} to read data into.
   * @return The number of bytes read from ByteInput
   */
  public abstract void read(int offset, ByteBuffer out);

  /**
   * @return Total number of bytes in this ByteInput.
   */
  public abstract int size();
}