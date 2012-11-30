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

import java.io.IOException;
import java.util.Iterator;
import java.util.Map.Entry;

/**
 * LazyField encapsulates the logic of lazily parsing message fields. It stores
 * the message in a ByteString initially and then parse it on-demand.
 *
 * LazyField is thread-compatible e.g. concurrent read are safe, however,
 * synchronizations are needed under read/write situations.
 *
 * Now LazyField is only used to lazily load MessageSet.
 * TODO(xiangl): Use LazyField to lazily load all messages.
 *
 * @author xiangl@google.com (Xiang Li)
 */
class LazyField {

  final private MessageLite defaultInstance;
  final private ExtensionRegistryLite extensionRegistry;

  // Mutable because it is initialized lazily.
  private ByteString bytes;
  private volatile MessageLite value;
  private volatile boolean isDirty = false;

  public LazyField(MessageLite defaultInstance,
      ExtensionRegistryLite extensionRegistry, ByteString bytes) {
    this.defaultInstance = defaultInstance;
    this.extensionRegistry = extensionRegistry;
    this.bytes = bytes;
  }

  public MessageLite getValue() {
    ensureInitialized();
    return value;
  }

  /**
   * LazyField is not thread-safe for write access. Synchronizations are needed
   * under read/write situations.
   */
  public MessageLite setValue(MessageLite value) {
    MessageLite originalValue = this.value;
    this.value = value;
    bytes = null;
    isDirty = true;
    return originalValue;
  }

  /**
   * Due to the optional field can be duplicated at the end of serialized
   * bytes, which will make the serialized size changed after LazyField
   * parsed. Be careful when using this method.
   */
  public int getSerializedSize() {
    if (isDirty) {
      return value.getSerializedSize();
    }
    return bytes.size();
  }

  public ByteString toByteString() {
    if (!isDirty) {
      return bytes;
    }
    synchronized (this) {
      if (!isDirty) {
        return bytes;
      }
      bytes = value.toByteString();
      isDirty = false;
      return bytes;
    }
  }

  @Override
  public int hashCode() {
    ensureInitialized();
    return value.hashCode();
  }

  @Override
  public boolean equals(Object obj) {
    ensureInitialized();
    return value.equals(obj);
  }

  @Override
  public String toString() {
    ensureInitialized();
    return value.toString();
  }

  private void ensureInitialized() {
    if (value != null) {
      return;
    }
    synchronized (this) {
      if (value != null) {
        return;
      }
      try {
        if (bytes != null) {
          value = defaultInstance.getParserForType()
              .parseFrom(bytes, extensionRegistry);
        }
      } catch (IOException e) {
        // TODO(xiangl): Refactory the API to support the exception thrown from
        // lazily load messages.
      }
    }
  }

  // ====================================================

  /**
   * LazyEntry and LazyIterator are used to encapsulate the LazyField, when
   * users iterate all fields from FieldSet.
   */
  static class LazyEntry<K> implements Entry<K, Object> {
    private Entry<K, LazyField> entry;

    private LazyEntry(Entry<K, LazyField> entry) {
      this.entry = entry;
    }

    public K getKey() {
      return entry.getKey();
    }

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

    public boolean hasNext() {
      return iterator.hasNext();
    }

    @SuppressWarnings("unchecked")
    public Entry<K, Object> next() {
      Entry<K, ?> entry = iterator.next();
      if (entry.getValue() instanceof LazyField) {
        return new LazyEntry<K>((Entry<K, LazyField>) entry);
      }
      return (Entry<K, Object>) entry;
    }

    public void remove() {
      iterator.remove();
    }
  }
}
