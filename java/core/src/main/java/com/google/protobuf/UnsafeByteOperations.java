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
 * Provides a number of unsafe byte operations to be used by advanced applications with high
 * performance requirements. These methods are referred to as "unsafe" due to the fact that they
 * potentially expose the backing buffer of a {@link ByteString} to the application.
 *
 * <p><strong>DISCLAIMER:</strong> The methods in this class should only be called if it is
 * guaranteed that the buffer backing the {@link ByteString} will never change! Mutation of a
 * {@link ByteString} can lead to unexpected and undesirable consequences in your application,
 * and will likely be difficult to debug. Proceed with caution!
 */
@ExperimentalApi
public final class UnsafeByteOperations {
  private UnsafeByteOperations() {}

  /**
   * An unsafe operation that returns a {@link ByteString} that is backed by the provided buffer.
   *
   * @param buffer the Java NIO buffer to be wrapped
   * @return a {@link ByteString} backed by the provided buffer
   */
  public static ByteString unsafeWrap(ByteBuffer buffer) {
    if (buffer.hasArray()) {
      final int offset = buffer.arrayOffset();
      return ByteString.wrap(buffer.array(), offset + buffer.position(), buffer.remaining());
    } else {
      return new NioByteString(buffer);
    }
  }

  /**
   * Writes the given {@link ByteString} to the provided {@link ByteOutput}. Calling this method may
   * result in multiple operations on the target {@link ByteOutput}
   * (i.e. for roped {@link ByteString}s).
   *
   * <p>This method exposes the internal backing buffer(s) of the {@link ByteString} to the {@link
   * ByteOutput} in order to avoid additional copying overhead. It would be possible for a malicious
   * {@link ByteOutput} to corrupt the {@link ByteString}. Use with caution!
   *
   * <p> NOTE: The {@link ByteOutput} <strong>MUST NOT</strong> modify the provided buffers. Doing
   * so may result in corrupted data, which would be difficult to debug.
   *
   * @param bytes the {@link ByteString} to be written
   * @param  output  the output to receive the bytes
   * @throws IOException  if an I/O error occurs
   */
  public static void unsafeWriteTo(ByteString bytes, ByteOutput output) throws IOException {
    bytes.writeTo(output);
  }
}
