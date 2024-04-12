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

import com.google.protobuf.ArrayDecoders.Registers;
import java.io.IOException;

/**
 * A runtime schema for a single protobuf message. A schema provides operations on message instances
 * such as serialization/deserialization.
 */
@ExperimentalApi
@CheckReturnValue
interface Schema<T> {
  /** Writes the given message to the target {@link Writer}. */
  void writeTo(T message, Writer writer) throws IOException;

  /**
   * Reads fields from the given {@link Reader} and merges them into the message. It doesn't make
   * the message immutable after parsing is done. To make the message immutable, use {@link
   * #makeImmutable}.
   */
  void mergeFrom(T message, Reader reader, ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /**
   * Like the above but parses from a byte[] without extensions. Entry point of fast path. Note that
   * this method may throw IndexOutOfBoundsException if the input data is not valid protobuf wire
   * format. Protobuf public API methods should catch and convert that exception to
   * InvalidProtocolBufferException.
   */
  void mergeFrom(T message, byte[] data, int position, int limit, Registers registers)
      throws IOException;

  /** Marks repeated/map/extension/unknown fields as immutable. */
  void makeImmutable(T message);

  /** Checks whether all required fields are set. */
  boolean isInitialized(T message);

  /** Creates a new instance of the message class. */
  T newInstance();

  /** Determine of the two messages are equal. */
  boolean equals(T message, T other);

  /** Compute a hashCode for the message. */
  int hashCode(T message);

  /**
   * Merge values from {@code other} into {@code message}. This method doesn't make the message
   * immutable. To make the message immutable after merging, use {@link #makeImmutable}.
   */
  void mergeFrom(T message, T other);

  /** Compute the serialized size of the message. */
  int getSerializedSize(T message);
}
