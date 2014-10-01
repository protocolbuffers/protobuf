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

import java.util.Collection;
import java.util.List;

/**
 * An interface extending {@code List<String>} that also provides access to the
 * items of the list as UTF8-encoded ByteString or byte[] objects. This is
 * used by the protocol buffer implementation to support lazily converting bytes
 * parsed over the wire to String objects until needed and also increases the
 * efficiency of serialization if the String was never requested as the
 * ByteString or byte[] is already cached. The ByteString methods are used in
 * immutable API only and byte[] methods used in mutable API only for they use
 * different representations for string/bytes fields.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public interface LazyStringList extends ProtocolStringList {

  /**
   * Returns the element at the specified position in this list as a ByteString.
   *
   * @param index index of the element to return
   * @return the element at the specified position in this list
   * @throws IndexOutOfBoundsException if the index is out of range
   *         ({@code index < 0 || index >= size()})
   */
  ByteString getByteString(int index);
  
  /**
   * Returns the element at the specified position in this list as byte[].
   *
   * @param index index of the element to return
   * @return the element at the specified position in this list
   * @throws IndexOutOfBoundsException if the index is out of range
   *         ({@code index < 0 || index >= size()})
   */
  byte[] getByteArray(int index);

  /**
   * Appends the specified element to the end of this list (optional
   * operation).
   *
   * @param element element to be appended to this list
   * @throws UnsupportedOperationException if the <tt>add</tt> operation
   *         is not supported by this list
   */
  void add(ByteString element);

  /**
   * Appends the specified element to the end of this list (optional
   * operation).
   *
   * @param element element to be appended to this list
   * @throws UnsupportedOperationException if the <tt>add</tt> operation
   *         is not supported by this list
   */
  void add(byte[] element);

  /**
   * Replaces the element at the specified position in this list with the
   * specified element (optional operation).
   *
   * @param index index of the element to replace
   * @param element the element to be stored at the specified position
   * @throws UnsupportedOperationException if the <tt>set</tt> operation
   *         is not supported by this list
   *         IndexOutOfBoundsException if the index is out of range
   *         ({@code index < 0 || index >= size()})
   */
  void set(int index, ByteString element);
  
  /**
   * Replaces the element at the specified position in this list with the
   * specified element (optional operation).
   *
   * @param index index of the element to replace
   * @param element the element to be stored at the specified position
   * @throws UnsupportedOperationException if the <tt>set</tt> operation
   *         is not supported by this list
   *         IndexOutOfBoundsException if the index is out of range
   *         ({@code index < 0 || index >= size()})
   */
  void set(int index, byte[] element);

  /**
   * Appends all elements in the specified ByteString collection to the end of
   * this list.
   *
   * @param c collection whose elements are to be added to this list
   * @return true if this list changed as a result of the call
   * @throws UnsupportedOperationException if the <tt>addAllByteString</tt>
   *         operation is not supported by this list
   */
  boolean addAllByteString(Collection<? extends ByteString> c);

  /**
   * Appends all elements in the specified byte[] collection to the end of
   * this list.
   *
   * @param c collection whose elements are to be added to this list
   * @return true if this list changed as a result of the call
   * @throws UnsupportedOperationException if the <tt>addAllByteArray</tt>
   *         operation is not supported by this list
   */
  boolean addAllByteArray(Collection<byte[]> c);

  /**
   * Returns an unmodifiable List of the underlying elements, each of which is
   * either a {@code String} or its equivalent UTF-8 encoded {@code ByteString}
   * or byte[]. It is an error for the caller to modify the returned
   * List, and attempting to do so will result in an
   * {@link UnsupportedOperationException}.
   */
  List<?> getUnderlyingElements();

  /**
   * Merges all elements from another LazyStringList into this one. This method
   * differs from {@link #addAll(Collection)} on that underlying byte arrays are
   * copied instead of reference shared. Immutable API doesn't need to use this
   * method as byte[] is not used there at all.
   */
  void mergeFrom(LazyStringList other);

  /**
   * Returns a mutable view of this list. Changes to the view will be made into
   * the original list. This method is used in mutable API only.
   */
  List<byte[]> asByteArrayList();

  /** Returns an unmodifiable view of the list. */
  LazyStringList getUnmodifiableView();
}
