// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.AbstractMap;
import java.util.AbstractSet;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;

/**
 * A custom map implementation from {@link FieldSet.FieldDescriptorLite} to Object.
 *
 * <p>This implementation is heavily optimized for insertion and iteration when the map is built
 * (via {@code put()}) with keys (FieldDescriptors) in ascending sorted order. When entries are
 * added in increasing order of the FieldDescriptor, no sorting overhead is incurred. The entries
 * are simply appended to a backing array, making memory allocation and insertion highly efficient.
 *
 * <p>Iteration over the entries is straightforward and avoids the creation of an {@code Iterator}
 * object. It should be done as follows:
 *
 * <pre>{@code
 * for (int i = 0; i < fieldMap.size(); i++) {
 *   process(fieldMap.getArrayEntryAt(i));
 * }
 * }</pre>
 *
 * The resulting iteration is in order of ascending field tag number. The object returned by {@link
 * #entrySet()} adheres to the same contract but is less efficient as it necessarily involves
 * creating an object for iteration.
 *
 * <p>If entries are added out of order, the map defers sorting until necessary (e.g., when
 * iterating, querying size, or retrieving elements). This lazily sorts and deduplicates the
 * underlying array.
 *
 * <p>Modifying operations (such as {@code put()}, {@code remove()}, or {@code clear()}) are not
 * thread-safe until {@link #makeImmutable()} is called, after which any modifying operation will
 * result in an {@link UnsupportedOperationException}. However, instances are thread-safe for
 * concurrent read-only operations (such as {@code get()} or {@code size()}) even before {@code
 * makeImmutable()} is called, as long as no modifying operations are performed concurrently.
 *
 * @author darick@google.com Darick Tong
 */
@SuppressWarnings("unchecked") // entries is known to contain Entry objects.)
class SmallSortedMap<K extends FieldSet.FieldDescriptorLite<K>> extends AbstractMap<K, Object> {

  static final int DEFAULT_FIELD_MAP_ARRAY_SIZE = 16;

  // Only has Entry elements inside.
  // Can't declare this as Entry[] because Entry is generic, so you get "generic array creation"
  // error. Instead, use an Object[], and cast to Entry on read.
  // null Object[] means 'empty'.
  // TODO: b/513203684 - Consider maintaining two arrays for keys and values.
  private Object[] entries;

  // Number of elements in entries that are valid, like ArrayList.size.
  private int size;

  // Whether {@link #makeImmutable()} has been called. If true, the map is immutable, and
  // guaranteed to be sorted and deduplicated.
  private boolean isImmutable;

  // The EntrySet is a stateless view of the Map. It's initialized the first
  // time it is requested and reused henceforth.
  private volatile EntrySet lazyEntrySet;

  private volatile int firstUnsortedIndex;

  SmallSortedMap() {
    isImmutable = false;
    entries = null;
    size = 0;
    firstUnsortedIndex = -1;
  }

  SmallSortedMap(int initialCapacity) {
    isImmutable = false;
    entries = new Object[initialCapacity];
    size = 0;
    firstUnsortedIndex = -1;
  }

  /**
   * Make this map immutable from this point forward. Immutable maps are guaranteed to be sorted and
   * deduplicated.
   */
  public void makeImmutable() {
    if (isImmutable) {
      return;
    }
    ensureSortedAndDeduplicated();
    if (entries != null) {
      for (int i = 0; i < size; i++) {
        Entry entry = (Entry) entries[i];
        if (entry.getKey().isRepeated()) {
          entry.setValue((Object) Collections.unmodifiableList((List<?>) entry.getValue()));
        }
      }
    }
    isImmutable = true;
  }

  /**
   * @return Whether {@link #makeImmutable()} has been called.
   */
  public boolean isImmutable() {
    return isImmutable;
  }

  /**
   * @return The array entry at the given {@code index}.
   */
  public Map.Entry<K, Object> getArrayEntryAt(int index) {
    ensureSortedAndDeduplicated();
    if (index >= size) {
      throw new ArrayIndexOutOfBoundsException(index);
    }
    Entry e = (Entry) entries[index];
    return e;
  }

  @Override
  public int size() {
    ensureSortedAndDeduplicated();
    return size;
  }

  @Override
  public boolean isEmpty() {
    return size == 0;
  }

  /**
   * The implementation throws a {@code ClassCastException} if o is not an object of type {@code K}.
   *
   * <p>{@inheritDoc}
   */
  @Override
  public boolean containsKey(Object o) {
    final K key = (K) o;
    ensureSortedAndDeduplicated();
    return binarySearch(key) >= 0;
  }

  /**
   * The implementation throws a {@code ClassCastException} if o is not an object of type {@code K}.
   *
   * <p>{@inheritDoc}
   */
  @Override
  public Object get(Object o) {
    final K key = (K) o;
    ensureSortedAndDeduplicated();
    final int index = binarySearch(key);
    if (index >= 0) {
      Entry e = (Entry) entries[index];
      return e.getValue();
    }
    return null;
  }

  @Override
  public void putAll(Map<? extends K, ?> map) {
    checkMutable();
    ensureCapacity(size + map.size());
    for (Map.Entry<? extends K, ?> entry : map.entrySet()) {
      put((K) entry.getKey(), (Object) entry.getValue());
    }
  }

  @Override
  @CanIgnoreReturnValue
  @SuppressWarnings("PreferPreconditions") // unsupported
  public Object put(K key, Object value) {
    checkMutable();
    if (key == null) {
      throw new NullPointerException("key cannot be null");
    }

    if (size > 0) {
      int comp = key.compareTo(((Entry) entries[size - 1]).getKey());
      if (comp < 0) {
        if (firstUnsortedIndex == -1) {
          firstUnsortedIndex = size;
        }
      } else if (comp == 0) {
        // Overwrite existing value.
        ((Entry) entries[size - 1]).setValue(value);
        // We could choose to return the previous value here, but we don't for consistency with
        // the code below.
        return null;
      }
    }
    ensureCapacity(size + 1);
    entries[size++] = new Entry(key, value);
    return null; // Note: doesn't return previous value to optimize for insertion speed
  }

  @Override
  public void clear() {
    checkMutable();
    if (size != 0) {
      entries = null;
      size = 0;
      firstUnsortedIndex = -1;
    }
  }

  /**
   * The implementation throws a {@code ClassCastException} if o is not an object of type {@code K}.
   *
   * <p>{@inheritDoc}
   */
  @Override
  @CanIgnoreReturnValue
  public Object remove(Object o) {
    checkMutable();
    ensureSortedAndDeduplicated();
    final K key = (K) o;
    int index = binarySearch(key);
    if (index < 0) {
      return null;
    }
    Object oldValue = ((Entry) entries[index]).getValue();
    System.arraycopy(entries, index + 1, entries, index, size - index - 1);
    // Clear out the unused entry at the end to allow garbage collection.
    entries[--size] = null;
    return oldValue;
  }

  private void ensureCapacity(int minCapacity) {
    if (entries == null || entries.length == 0) {
      entries = new Object[Math.max(DEFAULT_FIELD_MAP_ARRAY_SIZE, minCapacity)];
      return;
    }
    if (minCapacity > entries.length) {
      int newCapacity = Math.max(entries.length + (entries.length >> 1), minCapacity);
      entries = Arrays.copyOf(entries, newCapacity);
    }
  }

  /**
   * Ensures that the entries in the map are sorted and deduplicated.
   *
   * <p>Immutable maps are guaranteed to be already sorted and deduplicated.
   */
  private void ensureSortedAndDeduplicated() {
    if (isImmutable) {
      return;
    }
    if (firstUnsortedIndex != -1) {
      // Synchronize on the map to support concurrent read-only access. When multiple threads
      // concurrently invoke read operations on a mutable map that has unsorted entries, they must
      // safely coordinate the lazy sorting and deduplication without data corruption.
      synchronized (this) {
        if (firstUnsortedIndex == -1) {
          return;
        }
        if (size <= 1) {
          return;
        }
        for (int i = firstUnsortedIndex; i < size; i++) {
          Entry entry = (Entry) entries[i];
          K key = entry.getKey();
          int j = i - 1;
          while (j >= 0) {
            Entry prevEntry = (Entry) entries[j];
            if (prevEntry.getKey().compareTo(key) <= 0) {
              break;
            }
            entries[j + 1] = entries[j];
            j--;
          }
          entries[j + 1] = entry;
        }

        // Resolve duplicates in-place (stable, preserving last write)
        int newSize = 0;
        for (int i = 0; i < size; i++) {
          Entry entry = (Entry) entries[i];
          if (newSize > 0 && ((Entry) entries[newSize - 1]).getKey().equals(entry.getKey())) {
            entries[newSize - 1] = entry;
          } else {
            entries[newSize] = entry;
            newSize++;
          }
        }
        if (newSize < this.size) {
          this.size = newSize;
          // Clear out the unused entries to allow garbage collection.
          Arrays.fill(entries, newSize, entries.length, null);
        }
        this.firstUnsortedIndex = -1;
      }
    }
  }

  /**
   * @param key The key to find in the entry array.
   * @return The returned integer position follows the same semantics as the value returned by
   *     {@link java.util.Arrays#binarySearch()}.
   */
  private int binarySearch(K key) {
    int low = 0;
    int high = size - 1;
    while (low <= high) {
      int mid = (low + high) >>> 1;
      K midKey = ((Entry) entries[mid]).getKey();
      int cmp = midKey.compareTo(key);
      if (cmp < 0) {
        low = mid + 1;
      } else if (cmp > 0) {
        high = mid - 1;
      } else {
        return mid; // key found
      }
    }
    return -(low + 1); // key not found.
  }

  /**
   * Similar to the AbstractMap implementation of {@code keySet()} and {@code values()}, the entry
   * set is created the first time this method is called, and returned in response to all subsequent
   * calls.
   *
   * <p>{@inheritDoc}
   */
  @Override
  public Set<Map.Entry<K, Object>> entrySet() {
    ensureSortedAndDeduplicated();
    if (lazyEntrySet == null) {
      lazyEntrySet = new EntrySet();
    }
    return lazyEntrySet;
  }

  /**
   * @throws UnsupportedOperationException if {@link #makeImmutable()} has has been called.
   */
  private void checkMutable() {
    if (isImmutable) {
      throw new UnsupportedOperationException();
    }
  }

  /**
   * Entry implementation that implements Comparable in order to support binary search within the
   * entry array. Also checks mutability in {@link #setValue()}.
   */
  private class Entry implements Map.Entry<K, Object>, Comparable<Entry> {

    private final K key;
    private Object value;

    Entry(K key, Object value) {
      this.key = key;
      this.value = value;
    }

    @Override
    public K getKey() {
      return key;
    }

    @Override
    public Object getValue() {
      return value;
    }

    @Override
    public int compareTo(Entry other) {
      return getKey().compareTo(other.getKey());
    }

    @Override
    @CanIgnoreReturnValue
    public Object setValue(Object newValue) {
      checkMutable();
      final Object oldValue = this.value;
      this.value = newValue;
      return oldValue;
    }

    @Override
    public boolean equals(
            Object o) {
      if (o == this) {
        return true;
      }
      if (!(o instanceof Map.Entry)) {
        return false;
      }
      Map.Entry<?, ?> other = (Map.Entry<?, ?>) o;
      return equals(key, other.getKey()) && equals(value, other.getValue());
    }

    @Override
    public int hashCode() {
      return (key == null ? 0 : key.hashCode()) ^ (value == null ? 0 : value.hashCode());
    }

    @Override
    public String toString() {
      return key + "=" + value;
    }

    /** equals() that handles null values. */
    private boolean equals(
            Object o1,
        Object o2) {
      return o1 == null ? o2 == null : o1.equals(o2);
    }
  }

  /** Stateless view of the entries in the field map. */
  private class EntrySet extends AbstractSet<Map.Entry<K, Object>> {

    @Override
    public Iterator<Map.Entry<K, Object>> iterator() {
      return new EntryIterator();
    }

    @Override
    public int size() {
      return SmallSortedMap.this.size();
    }

    /**
     * Throws a {@link ClassCastException} if o is not of the expected type.
     *
     * <p>{@inheritDoc}
     */
    @Override
    public boolean contains(Object o) {
      final Map.Entry<K, Object> entry = (Map.Entry<K, Object>) o;
      final Object existing = get(entry.getKey());
      final Object value = entry.getValue();
      return existing == value || (existing != null && existing.equals(value));
    }
  }

  /**
   * Iterator implementation that switches from the entry array to the overflow entries
   * appropriately.
   */
  private class EntryIterator implements Iterator<Map.Entry<K, Object>> {

    private int pos = -1;

    @Override
    public boolean hasNext() {
      return (pos + 1) < size;
    }

    @Override
    @SuppressWarnings("unchecked") // entries is known to contain Entry objects.
    public Map.Entry<K, Object> next() {
      if (!hasNext()) {
        throw new NoSuchElementException();
      }
      return (Entry) entries[++pos];
    }
  }

  @Override
  public boolean equals(
          Object o) {
    if (this == o) {
      return true;
    }

    if (!(o instanceof SmallSortedMap)) {
      return super.equals(o);
    }

    SmallSortedMap<?> other = (SmallSortedMap<?>) o;
    final int size = size();
    if (size != other.size()) {
      return false;
    }

    final Object[] thisEntries = entries;
    final Object[] otherEntries = other.entries;
    for (int i = 0; i < size; i++) {
      if (!thisEntries[i].equals(otherEntries[i])) {
        return false;
      }
    }
    return true;
  }

  @Override
  public int hashCode() {
    int h = 0;
    final int listSize = size();
    for (int i = 0; i < listSize; i++) {
      h += entries[i].hashCode();
    }
    return h;
  }
}
