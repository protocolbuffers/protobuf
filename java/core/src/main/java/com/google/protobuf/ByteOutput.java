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
 * An output target for raw bytes. This interface provides semantics that support two types of
 * writing:
 *
 * <p><b>Traditional write operations:</b> (as defined by {@link java.io.OutputStream}) where the
 * target method is responsible for either copying the data or completing the write before returning
 * from the method call.
 *
 * <p><b>Lazy write operations:</b> where the caller guarantees that it will never modify the
 * provided buffer and it can therefore be considered immutable. The target method is free to
 * maintain a reference to the buffer beyond the scope of the method call (e.g. until the write
 * operation completes).
 */
@ExperimentalApi
public abstract class ByteOutput {
  /**
   * Writes a single byte.
   *
   * @param value the byte to be written
   * @throws IOException thrown if an error occurred while writing
   */
  public abstract void write(byte value) throws IOException;

  /**
   * Writes a sequence of bytes. The {@link ByteOutput} must copy {@code value} if it will not be
   * processed prior to the return of this method call, since {@code value} may be reused/altered by
   * the caller.
   *
   * <p>NOTE: This method <strong>MUST NOT</strong> modify the {@code value}. Doing so is a
   * programming error and will lead to data corruption which will be difficult to debug.
   *
   * @param value the bytes to be written
   * @param offset the offset of the start of the writable range
   * @param length the number of bytes to write starting from {@code offset}
   * @throws IOException thrown if an error occurred while writing
   */
  public abstract void write(byte[] value, int offset, int length) throws IOException;

  /**
   * Writes a sequence of bytes. The {@link ByteOutput} is free to retain a reference to the value
   * beyond the scope of this method call (e.g. write later) since it is considered immutable and is
   * guaranteed not to change by the caller.
   *
   * <p>NOTE: This method <strong>MUST NOT</strong> modify the {@code value}. Doing so is a
   * programming error and will lead to data corruption which will be difficult to debug.
   *
   * @param value the bytes to be written
   * @param offset the offset of the start of the writable range
   * @param length the number of bytes to write starting from {@code offset}
   * @throws IOException thrown if an error occurred while writing
   */
  public abstract void writeLazy(byte[] value, int offset, int length) throws IOException;

  /**
   * Writes a sequence of bytes. The {@link ByteOutput} must copy {@code value} if it will not be
   * processed prior to the return of this method call, since {@code value} may be reused/altered by
   * the caller.
   *
   * <p>NOTE: This method <strong>MUST NOT</strong> modify the {@code value}. Doing so is a
   * programming error and will lead to data corruption which will be difficult to debug.
   *
   * @param value the bytes to be written. Upon returning from this call, the {@code position} of
   *     this buffer will be set to the {@code limit}
   * @throws IOException thrown if an error occurred while writing
   */
  public abstract void write(ByteBuffer value) throws IOException;

  /**
   * Writes a sequence of bytes. The {@link ByteOutput} is free to retain a reference to the value
   * beyond the scope of this method call (e.g. write later) since it is considered immutable and is
   * guaranteed not to change by the caller.
   *
   * <p>NOTE: This method <strong>MUST NOT</strong> modify the {@code value}. Doing so is a
   * programming error and will lead to data corruption which will be difficult to debug.
   *
   * @param value the bytes to be written. Upon returning from this call, the {@code position} of
   *     this buffer will be set to the {@code limit}
   * @throws IOException thrown if an error occurred while writing
   */
  public abstract void writeLazy(ByteBuffer value) throws IOException;
}
