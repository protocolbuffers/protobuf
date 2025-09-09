// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Provides a number of unsafe byte operations to be used by advanced applications with high
 * performance requirements. These methods are referred to as "unsafe" because they potentially
 * expose the backing buffer of a {@link ByteString} to the application.
 *
 * <p><strong>DISCLAIMER:</strong> The methods in this class should only be called if it is
 * guaranteed that the buffer backing the {@link ByteString} will never change! Mutation of a {@link
 * ByteString} can lead to unexpected and undesirable consequences in your application, and will
 * likely be difficult to debug. Proceed with caution!
 *
 * <p>This can have a number of significant side effects that have spooky-action-at-a-distance-like
 * behavior. In particular, if the bytes value changes out from under a Protocol Buffer:
 *
 * <ul>
 *   <li>serialization may throw
 *   <li>serialization may succeed but the wrong bytes may be written out
 *   <li>messages are no longer threadsafe
 *   <li>hashCode may be incorrect
 *       <ul>
 *         <li>can result in a permanent memory leak when used as a key in a long-lived HashMap
 *         <li>the semantics of many programs may be violated if this is the case
 *       </ul>
 * </ul>
 *
 * Each of these issues will occur in parts of the code base that are entirely distinct from the
 * parts of the code base modifying the buffer. In fact, both parts of the code base may be correct
 * - it is the bridging with the unsafe operations that was in error!
 */
public final class UnsafeByteOperations {
  private UnsafeByteOperations() {}

  /**
   * An unsafe operation that returns a {@link ByteString} that is backed by the provided buffer.
   *
   * @param buffer the buffer to be wrapped
   * @return a {@link ByteString} backed by the provided buffer
   */
  public static ByteString unsafeWrap(byte[] buffer) {
    return ByteString.wrap(buffer);
  }

  /**
   * An unsafe operation that returns a {@link ByteString} that is backed by a subregion of the
   * provided buffer.
   *
   * @param buffer the buffer to be wrapped
   * @param offset the offset of the wrapped region
   * @param length the number of bytes of the wrapped region
   * @return a {@link ByteString} backed by the provided buffer
   */
  public static ByteString unsafeWrap(byte[] buffer, int offset, int length) {
    return ByteString.wrap(buffer, offset, length);
  }

  /**
   * An unsafe operation that returns a {@link ByteString} that is backed by the provided buffer.
   *
   * @param buffer the Java NIO buffer to be wrapped
   * @return a {@link ByteString} backed by the provided buffer
   */
  public static ByteString unsafeWrap(ByteBuffer buffer) {
    return ByteString.wrap(buffer);
  }

  /**
   * Writes the given {@link ByteString} to the provided {@link ByteOutput}. Calling this method may
   * result in multiple operations on the target {@link ByteOutput} (i.e. for roped {@link
   * ByteString}s).
   *
   * <p>This method exposes the internal backing buffer(s) of the {@link ByteString} to the {@link
   * ByteOutput} in order to avoid additional copying overhead. It would be possible for a malicious
   * {@link ByteOutput} to corrupt the {@link ByteString}. Use with caution!
   *
   * <p>NOTE: The {@link ByteOutput} <strong>MUST NOT</strong> modify the provided buffers. Doing so
   * may result in corrupted data, which would be difficult to debug.
   *
   * @param bytes the {@link ByteString} to be written
   * @param output the output to receive the bytes
   * @throws IOException if an I/O error occurs
   */
  public static void unsafeWriteTo(ByteString bytes, ByteOutput output) throws IOException {
    bytes.writeTo(output);
  }
}
