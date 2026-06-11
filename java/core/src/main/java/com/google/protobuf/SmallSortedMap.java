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
 * A custom map implementation from FieldDescriptor to Object optimized to minimize the number of
 * memory allocations for instances with a small number of mappings. The implementation stores the
 * first {@code k} mappings in an array for a configurable value of {@code k}, allowing direct
 * access to the corresponding {@code Entry}s without the need to create an Iterator. The remaining
 * entries are stored in an overflow map. Iteration over the entries in the map should be done as
 * follows:
 *
 * <pre>{@code
 * for (int i = 0; i < fieldMap.getNumArrayEntries(); i++) {
 *   process(fieldMap.getArrayEntryAt(i));
 * }
 * for (Map.Entry<K, V> entry : fieldMap.getOverflowEntries()) {
 *   process(entry);
 * }
 * }</pre>
 *
 * The resulting iteration is in order of ascending field tag number. The object returned by {@link
 * #entrySet()} adheres to the same contract but is less efficient as it necessarily involves
 * creating an object for iteration.
 *
 * <p>The tradeoff for this memory efficiency is that the worst case running time of the {@code
 * put()} operation is {@code O(k + lg n)}, which happens when entries are added in descending
 * order. {@code k} should be chosen such that it covers enough common cases without adversely
 * affecting larger maps. In practice, the worst case scenario does not happen for extensions
 * because extension fields are serialized and deserialized in order of ascending tag number, but
 * the worst case scenario can happen for DynamicMessages.
 *
 * <p>The running time for all other operations is similar to that of {@code TreeMap}.
 *
 * <p>Instances are not thread-safe until {@link #makeImmutable()} is called, after which any
 * modifying operation will result in an {@link UnsupportedOperationException}.
 *
 * @author darick@google.com Darick Tong
 */
// This class is final for all intents and purposes because the constructor is
// private. However, the FieldDescriptor-specific logic is encapsulated in
// a subclass to aid testability of the core logic.
class SmallSortedMap<K extends FieldSet.FieldDescriptorLite<K>, V> extends AbstractMap<K, V> {

  static final int DEFAULT_FIELD_MAP_ARRAY_SIZE = 16;

  /** Creates a new instance for testing. */
  static <K extends FieldSet.FieldDescriptorLite<K>, V> SmallSortedMap<K, V> newInstanceForTest() {
    return new SmallSortedMap<>();
  }

  // Only has Entry elements inside.
  // Can't declare this as Entry[] because Entry is generic, so you get "generic array creation"
  // error. Instead, use an Object[], and cast to Entry on read.
  // null Object[] means 'empty'.
  private Object[] entries;
  // Number of elements in entries that are valid, like ArrayList.size.
  private int size;

  private boolean isImmutable;
  // The EntrySet is a stateless view of the Map. It's initialized the first
  // time it is requested and reused henceforth.
  private volatile EntrySet lazyEntrySet;

  private int firstUnsortedIndex;

  SmallSortedMap() {
    isImmutable = false;
    entries = null;
    size = 0;
    firstUnsortedIndex = -1;
  }

  /** Make this map immutable from this point forward. */
  public void makeImmutable() {
    if (isImmutable) {
      return;
    }
    ensureSortedAndDeduplicated();
    if (entries != null) {
      for (int i = 0; i < size; i++) {
        Entry entry = (Entry) entries[i];
        if (entry.getKey().isRepeated()) {
          entry.setValue((V) Collections.unmodifiableList((List<?>) entry.getValue()));
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
  public Map.Entry<K, V> getArrayEntryAt(int index) {
    if (index >= size) {
      throw new ArrayIndexOutOfBoundsException(index);
    }
    @SuppressWarnings("unchecked")
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
    ensureSortedAndDeduplicated();
    return size == 0;
  }

  /**
   * The implementation throws a {@code ClassCastException} if o is not an object of type {@code K}.
   *
   * <p>{@inheritDoc}
   */
  @Override
  public boolean containsKey(Object o) {
    @SuppressWarnings("unchecked")
    final K key = (K) o;
    ensureSortedAndDeduplicated();
    return binarySearch((K) key) >= 0;
  }

  /**
   * The implementation throws a {@code ClassCastException} if o is not an object of type {@code K}.
   *
   * <p>{@inheritDoc}
   */
  @Override
  public V get(Object o) {
    @SuppressWarnings("unchecked")
    final K key = (K) o;
    ensureSortedAndDeduplicated();
    final int index = binarySearch(key);
    if (index >= 0) {
      @SuppressWarnings("unchecked")
      Entry e = (Entry) entries[index];
      return e.getValue();
    }
    return null;
  }

  @Override
  public void putAll(Map<? extends K, ? extends V> map) {
    checkMutable();
    ensureCapacity(size + map.size());
    for (Map.Entry<? extends K, ?> entry : map.entrySet()) {
      put((K) entry.getKey(), (V) entry.getValue());
    }
  }

  @Override
  @CanIgnoreReturnValue
  public V put(K key, V value) {
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
    } else if (entries == null) {
      entries = new Object[DEFAULT_FIELD_MAP_ARRAY_SIZE];
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
  public V remove(Object o) {
    checkMutable();
    ensureSortedAndDeduplicated();
    final K key = (K) o;
    int index = binarySearch(key);
    if (index < 0) {
      return null;
    }
    V oldValue = ((Entry) entries[index]).getValue();
    System.arraycopy(entries, index + 1, entries, index, size - index - 1);
    // Clear out the unused entries to allow garbage collection.
    entries[size] = null;
    size--;
    return oldValue;
  }

  private void ensureCapacity(int minCapacity) {
    if (minCapacity > entries.length) {
      int newCapacity = Math.max(entries.length * 2, minCapacity);
      entries = Arrays.copyOf(entries, newCapacity);
    }
  }

  private void ensureSortedAndDeduplicated() {
    if (firstUnsortedIndex != -1) {
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
  public Set<Map.Entry<K, V>> entrySet() {
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
  private class Entry implements Map.Entry<K, V>, Comparable<Entry> {

    private final K key;
    private V value;

    Entry(Map.Entry<K, V> copy) {
      this(copy.getKey(), copy.getValue());
    }

    Entry(K key, V value) {
      this.key = key;
      this.value = value;
    }

    @Override
    public K getKey() {
      return key;
    }

    @Override
    public V getValue() {
      return value;
    }

    @Override
    public int compareTo(Entry other) {
      return getKey().compareTo(other.getKey());
    }

    @Override
    @CanIgnoreReturnValue
    public V setValue(V newValue) {
      checkMutable();
      final V oldValue = this.value;
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
  private class EntrySet extends AbstractSet<Map.Entry<K, V>> {

    @Override
    public Iterator<Map.Entry<K, V>> iterator() {
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
      @SuppressWarnings("unchecked")
      final Map.Entry<K, V> entry = (Map.Entry<K, V>) o;
      final V existing = get(entry.getKey());
      final V value = entry.getValue();
      return existing == value || (existing != null && existing.equals(value));
    }
  }

  /**
   * Iterator implementation that switches from the entry array to the overflow entries
   * appropriately.
   */
  private class EntryIterator implements Iterator<Map.Entry<K, V>> {

    private int pos = -1;

    @Override
    public boolean hasNext() {
      return (pos + 1) < size;
    }

    @Override
    @SuppressWarnings("unchecked")
    public Map.Entry<K, V> next() {
      if (!hasNext()) {
        throw new NoSuchElementException();
      }
      return (Entry) entries[++pos];
    }
  }

  // @Override
  // public boolean equals(
  //         Object o) {
  //   if (this == o) {
  //     return true;
  //   }

  //   if (!(o instanceof SmallSortedMap)) {
  //     return super.equals(o);
  //   }

  //   SmallSortedMap<?, ?> other = (SmallSortedMap<?, ?>) o;
  //   final int size = size();
  //   if (size != other.size()) {
  //     return false;
  //   }

  //   // Best effort try to avoid allocating an entry set.
  //   final int numArrayEntries = getNumArrayEntries();
  //   if (numArrayEntries != other.getNumArrayEntries()) {
  //     return entrySet().equals(other.entrySet());
  //   }

  //   for (int i = 0; i < numArrayEntries; i++) {
  //     if (!getArrayEntryAt(i).equals(other.getArrayEntryAt(i))) {
  //       return false;
  //     }
  //   }

  //   if (numArrayEntries != size) {
  //     return overflowEntries.equals(other.overflowEntries);
  //   }

  //   return true;
  // }

  // @Override
  // public int hashCode() {
  //   int h = 0;
  //   final int listSize = getNumArrayEntries();
  //   for (int i = 0; i < listSize; i++) {
  //     h += entries[i].hashCode();
  //   }
  //   // Avoid the iterator allocation if possible.
  //   if (getNumOverflowEntries() > 0) {
  //     h += overflowEntries.hashCode();
  //   }
  //   return h;
  // }
}
