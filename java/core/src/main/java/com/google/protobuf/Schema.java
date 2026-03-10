// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
