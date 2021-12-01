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

import java.util.Iterator;
import java.util.Map.Entry;

/**
 * LazyField encapsulates the logic of lazily parsing message fields. It stores the message in a
 * ByteString initially and then parses it on-demand.
 *
 * <p>Most methods are implemented in {@link LazyFieldLite} but this class can contain a
 * default instance of the message to provide {@code hashCode()}, {@code equals()}, and {@code
 * toString()}.
 *
 * @author xiangl@google.com (Xiang Li)
 */
public class LazyField extends LazyFieldLite {

  /**
   * Carry a message's default instance which is used by {@code hashCode()}, {@code equals()}, and
   * {@code toString()}.
   */
  private final MessageLite defaultInstance;

  public LazyField(
      MessageLite defaultInstance, ExtensionRegistryLite extensionRegistry, ByteString bytes) {
    super(extensionRegistry, bytes);

    this.defaultInstance = defaultInstance;
  }

  @Override
  public boolean containsDefaultInstance() {
    return super.containsDefaultInstance() || value == defaultInstance;
  }

  public MessageLite getValue() {
    return getValue(defaultInstance);
  }

  @Override
  public int hashCode() {
    return getValue().hashCode();
  }

  @Override
  public boolean equals(Object obj) {
    return getValue().equals(obj);
  }

  @Override
  public String toString() {
    return getValue().toString();
  }

  // ====================================================

  /**
   * LazyEntry and LazyIterator are used to encapsulate the LazyField, when users iterate all fields
   * from FieldSet.
   */
  static class LazyEntry<K> implements Entry<K, Object> {
    private Entry<K, LazyField> entry;

    private LazyEntry(Entry<K, LazyField> entry) {
      this.entry = entry;
    }

    @Override
    public K getKey() {
      return entry.getKey();
    }

    @Override
    public Object getValue() {
      LazyField field = entry.getValue();
      if (field == null) {
        return null;
      }
      return field.getValue();
    }

    public LazyField getField() {
      return entry.getValue();
    }

    @Override
    public Object setValue(Object value) {
      if (!(value instanceof MessageLite)) {
        throw new IllegalArgumentException(
            "LazyField now only used for MessageSet, "
                + "and the value of MessageSet must be an instance of MessageLite");
      }
      return entry.getValue().setValue((MessageLite) value);
    }
  }

  static class LazyIterator<K> implements Iterator<Entry<K, Object>> {
    private Iterator<Entry<K, Object>> iterator;

    public LazyIterator(Iterator<Entry<K, Object>> iterator) {
      this.iterator = iterator;
    }

    @Override
    public boolean hasNext() {
      return iterator.hasNext();
    }

    @Override
    @SuppressWarnings("unchecked")
    public Entry<K, Object> next() {
      Entry<K, ?> entry = iterator.next();
      if (entry.getValue() instanceof LazyField) {
        return new LazyEntry<K>((Entry<K, LazyField>) entry);
      }
      return (Entry<K, Object>) entry;
    }

    @Override
    public void remove() {
      iterator.remove();
    }
  }
}
