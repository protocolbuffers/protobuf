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

import java.util.List;

/**
 * An interface extending {@code List<String>} that also provides access to the
 * items of the list as UTF8-encoded ByteString objects. This is used by the
 * protocol buffer implementation to support lazily converting bytes parsed
 * over the wire to String objects until needed and also increases the
 * efficiency of serialization if the String was never requested as the
 * ByteString is already cached.
 * <p>
 * This only adds additional methods that are required for the use in the
 * protocol buffer code in order to be able successfully round trip byte arrays
 * through parsing and serialization without conversion to strings. It's not
 * attempting to support the functionality of say {@code List<ByteString>}, hence
 * why only these two very specific methods are added.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public interface LazyStringList extends List<String> {

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
   * Appends the specified element to the end of this list (optional
   * operation).
   *
   * @param element element to be appended to this list
   * @throws UnsupportedOperationException if the <tt>add</tt> operation
   *         is not supported by this list
   */
  void add(ByteString element);

  /**
   * Returns an unmodifiable List of the underlying elements, each of
   * which is either a {@code String} or its equivalent UTF-8 encoded
   * {@code ByteString}. It is an error for the caller to modify the returned
   * List, and attempting to do so will result in an
   * {@link UnsupportedOperationException}.
   */
  List<?> getUnderlyingElements();
}
