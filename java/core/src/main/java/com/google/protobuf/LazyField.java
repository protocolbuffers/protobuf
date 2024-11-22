// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
  public boolean equals(
          Object obj) {
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
