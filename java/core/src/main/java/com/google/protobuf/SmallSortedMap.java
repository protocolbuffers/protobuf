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

import java.util.AbstractMap;
import java.util.AbstractSet;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;

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
class SmallSortedMap<K extends Comparable<K>, V> extends AbstractMap<K, V> {

  /**
   * Creates a new instance for mapping FieldDescriptors to their values. The {@link
   * #makeImmutable()} implementation will convert the List values of any repeated fields to
   * unmodifiable lists.
   *
   * @param arraySize The size of the entry array containing the lexicographically smallest
   *     mappings.
   */
  static <FieldDescriptorType extends FieldSet.FieldDescriptorLite<FieldDescriptorType>>
      SmallSortedMap<FieldDescriptorType, Object> newFieldMap(int arraySize) {
    return new SmallSortedMap<FieldDescriptorType, Object>(arraySize) {
      @Override
      @SuppressWarnings("unchecked")
      public void makeImmutable() {
        if (!isImmutable()) {
          for (int i = 0; i < getNumArrayEntries(); i++) {
            final Map.Entry<FieldDescriptorType, Object> entry = getArrayEntryAt(i);
            if (entry.getKey().isRepeated()) {
              final List value = (List) entry.getValue();
              entry.setValue(Collections.unmodifiableList(value));
            }
          }
          for (Map.Entry<FieldDescriptorType, Object> entry : getOverflowEntries()) {
            if (entry.getKey().isRepeated()) {
              final List value = (List) entry.getValue();
              entry.setValue(Collections.unmodifiableList(value));
            }
          }
        }
        super.makeImmutable();
      }
    };
  }

  /**
   * Creates a new instance for testing.
   *
   * @param arraySize The size of the entry array containing the lexicographically smallest
   *     mappings.
   */
  static <K extends Comparable<K>, V> SmallSortedMap<K, V> newInstanceForTest(int arraySize) {
    return new SmallSortedMap<K, V>(arraySize);
  }

  private final int maxArraySize;
  // The "entry array" is actually a List because generic arrays are not
  // allowed. ArrayList also nicely handles the entry shifting on inserts and
  // removes.
  private List<Entry> entryList;
  private Map<K, V> overflowEntries;
  private boolean isImmutable;
  // The EntrySet is a stateless view of the Map. It's initialized the first
  // time it is requested and reused henceforth.
  private volatile EntrySet lazyEntrySet;
  private Map<K, V> overflowEntriesDescending;
  private volatile DescendingEntrySet lazyDescendingEntrySet;

  /**
   * @code arraySize Size of the array in which the lexicographically smallest mappings are stored.
   *     (i.e. the {@code k} referred to in the class documentation).
   */
  private SmallSortedMap(int arraySize) {
    this.maxArraySize = arraySize;
    this.entryList = Collections.emptyList();
    this.overflowEntries = Collections.emptyMap();
    this.overflowEntriesDescending = Collections.emptyMap();
  }

  /** Make this map immutable from this point forward. */
  public void makeImmutable() {
    if (!isImmutable) {
      // Note: There's no need to wrap the entryList in an unmodifiableList
      // because none of the list's accessors are exposed. The iterator() of
      // overflowEntries, on the other hand, is exposed so it must be made
      // unmodifiable.
      overflowEntries =
          overflowEntries.isEmpty()
              ? Collections.<K, V>emptyMap()
              : Collections.unmodifiableMap(overflowEntries);
      overflowEntriesDescending =
          overflowEntriesDescending.isEmpty()
              ? Collections.<K, V>emptyMap()
              : Collections.unmodifiableMap(overflowEntriesDescending);
      isImmutable = true;
    }
  }

  /** @return Whether {@link #makeImmutable()} has been called. */
  public boolean isImmutable() {
    return isImmutable;
  }

  /** @return The number of entries in the entry array. */
  public int getNumArrayEntries() {
    return entryList.size();
  }

  /** @return The array entry at the given {@code index}. */
  public Map.Entry<K, V> getArrayEntryAt(int index) {
    return entryList.get(index);
  }

  /** @return There number of overflow entries. */
  public int getNumOverflowEntries() {
    return overflowEntries.size();
  }

  /** @return An iterable over the overflow entries. */
  public Iterable<Map.Entry<K, V>> getOverflowEntries() {
    return overflowEntries.isEmpty()
        ? EmptySet.<Map.Entry<K, V>>iterable()
        : overflowEntries.entrySet();
  }

  Iterable<Map.Entry<K, V>> getOverflowEntriesDescending() {
    return overflowEntriesDescending.isEmpty()
        ? EmptySet.<Map.Entry<K, V>>iterable()
        : overflowEntriesDescending.entrySet();
  }

  @Override
  public int size() {
    return entryList.size() + overflowEntries.size();
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
    return binarySearchInArray(key) >= 0 || overflowEntries.containsKey(key);
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
    final int index = binarySearchInArray(key);
    if (index >= 0) {
      return entryList.get(index).getValue();
    }
    return overflowEntries.get(key);
  }

  @Override
  public V put(K key, V value) {
    checkMutable();
    final int index = binarySearchInArray(key);
    if (index >= 0) {
      // Replace existing array entry.
      return entryList.get(index).setValue(value);
    }
    ensureEntryArrayMutable();
    final int insertionPoint = -(index + 1);
    if (insertionPoint >= maxArraySize) {
      // Put directly in overflow.
      return getOverflowEntriesMutable().put(key, value);
    }
    // Insert new Entry in array.
    if (entryList.size() == maxArraySize) {
      // Shift the last array entry into overflow.
      final Entry lastEntryInArray = entryList.remove(maxArraySize - 1);
      getOverflowEntriesMutable().put(lastEntryInArray.getKey(), lastEntryInArray.getValue());
    }
    entryList.add(insertionPoint, new Entry(key, value));
    return null;
  }

  @Override
  public void clear() {
    checkMutable();
    if (!entryList.isEmpty()) {
      entryList.clear();
    }
    if (!overflowEntries.isEmpty()) {
      overflowEntries.clear();
    }
  }

  /**
   * The implementation throws a {@code ClassCastException} if o is not an object of type {@code K}.
   *
   * <p>{@inheritDoc}
   */
  @Override
  public V remove(Object o) {
    checkMutable();
    @SuppressWarnings("unchecked")
    final K key = (K) o;
    final int index = binarySearchInArray(key);
    if (index >= 0) {
      return removeArrayEntryAt(index);
    }
    // overflowEntries might be Collections.unmodifiableMap(), so only
    // call remove() if it is non-empty.
    if (overflowEntries.isEmpty()) {
      return null;
    } else {
      return overflowEntries.remove(key);
    }
  }

  private V removeArrayEntryAt(int index) {
    checkMutable();
    final V removed = entryList.remove(index).getValue();
    if (!overflowEntries.isEmpty()) {
      // Shift the first entry in the overflow to be the last entry in the
      // array.
      final Iterator<Map.Entry<K, V>> iterator = getOverflowEntriesMutable().entrySet().iterator();
      entryList.add(new Entry(iterator.next()));
      iterator.remove();
    }
    return removed;
  }

  /**
   * @param key The key to find in the entry array.
   * @return The returned integer position follows the same semantics as the value returned by
   *     {@link java.util.Arrays#binarySearch()}.
   */
  private int binarySearchInArray(K key) {
    int left = 0;
    int right = entryList.size() - 1;

    // Optimization: For the common case in which entries are added in
    // ascending tag order, check the largest element in the array before
    // doing a full binary search.
    if (right >= 0) {
      int cmp = key.compareTo(entryList.get(right).getKey());
      if (cmp > 0) {
        return -(right + 2); // Insert point is after "right".
      } else if (cmp == 0) {
        return right;
      }
    }

    while (left <= right) {
      int mid = (left + right) / 2;
      int cmp = key.compareTo(entryList.get(mid).getKey());
      if (cmp < 0) {
        right = mid - 1;
      } else if (cmp > 0) {
        left = mid + 1;
      } else {
        return mid;
      }
    }
    return -(left + 1);
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

  Set<Map.Entry<K, V>> descendingEntrySet() {
    if (lazyDescendingEntrySet == null) {
      lazyDescendingEntrySet = new DescendingEntrySet();
    }
    return lazyDescendingEntrySet;
  }

  /** @throws UnsupportedOperationException if {@link #makeImmutable()} has has been called. */
  private void checkMutable() {
    if (isImmutable) {
      throw new UnsupportedOperationException();
    }
  }

  /**
   * @return a {@link SortedMap} to which overflow entries mappings can be added or removed.
   * @throws UnsupportedOperationException if {@link #makeImmutable()} has been called.
   */
  @SuppressWarnings("unchecked")
  private SortedMap<K, V> getOverflowEntriesMutable() {
    checkMutable();
    if (overflowEntries.isEmpty() && !(overflowEntries instanceof TreeMap)) {
      overflowEntries = new TreeMap<K, V>();
      overflowEntriesDescending = ((TreeMap<K, V>) overflowEntries).descendingMap();
    }
    return (SortedMap<K, V>) overflowEntries;
  }

  /** Lazily creates the entry list. Any code that adds to the list must first call this method. */
  private void ensureEntryArrayMutable() {
    checkMutable();
    if (entryList.isEmpty() && !(entryList instanceof ArrayList)) {
      entryList = new ArrayList<Entry>(maxArraySize);
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
    public V setValue(V newValue) {
      checkMutable();
      final V oldValue = this.value;
      this.value = newValue;
      return oldValue;
    }

    @Override
    public boolean equals(Object o) {
      if (o == this) {
        return true;
      }
      if (!(o instanceof Map.Entry)) {
        return false;
      }
      @SuppressWarnings("unchecked")
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
    private boolean equals(Object o1, Object o2) {
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

    @Override
    public boolean add(Map.Entry<K, V> entry) {
      if (!contains(entry)) {
        put(entry.getKey(), entry.getValue());
        return true;
      }
      return false;
    }

    /**
     * Throws a {@link ClassCastException} if o is not of the expected type.
     *
     * <p>{@inheritDoc}
     */
    @Override
    public boolean remove(Object o) {
      @SuppressWarnings("unchecked")
      final Map.Entry<K, V> entry = (Map.Entry<K, V>) o;
      if (contains(entry)) {
        SmallSortedMap.this.remove(entry.getKey());
        return true;
      }
      return false;
    }

    @Override
    public void clear() {
      SmallSortedMap.this.clear();
    }
  }

  private class DescendingEntrySet extends EntrySet {
    @Override
    public Iterator<java.util.Map.Entry<K, V>> iterator() {
      return new DescendingEntryIterator();
    }
  }

  /**
   * Iterator implementation that switches from the entry array to the overflow entries
   * appropriately.
   */
  private class EntryIterator implements Iterator<Map.Entry<K, V>> {

    private int pos = -1;
    private boolean nextCalledBeforeRemove;
    private Iterator<Map.Entry<K, V>> lazyOverflowIterator;

    @Override
    public boolean hasNext() {
      return (pos + 1) < entryList.size()
          || (!overflowEntries.isEmpty() && getOverflowIterator().hasNext());
    }

    @Override
    public Map.Entry<K, V> next() {
      nextCalledBeforeRemove = true;
      // Always increment pos so that we know whether the last returned value
      // was from the array or from overflow.
      if (++pos < entryList.size()) {
        return entryList.get(pos);
      }
      return getOverflowIterator().next();
    }

    @Override
    public void remove() {
      if (!nextCalledBeforeRemove) {
        throw new IllegalStateException("remove() was called before next()");
      }
      nextCalledBeforeRemove = false;
      checkMutable();

      if (pos < entryList.size()) {
        removeArrayEntryAt(pos--);
      } else {
        getOverflowIterator().remove();
      }
    }

    /**
     * It is important to create the overflow iterator only after the array entries have been
     * iterated over because the overflow entry set changes when the client calls remove() on the
     * array entries, which invalidates any existing iterators.
     */
    private Iterator<Map.Entry<K, V>> getOverflowIterator() {
      if (lazyOverflowIterator == null) {
        lazyOverflowIterator = overflowEntries.entrySet().iterator();
      }
      return lazyOverflowIterator;
    }
  }

  /**
   * Reverse Iterator implementation that switches from the entry array to the overflow entries
   * appropriately.
   */
  private class DescendingEntryIterator implements Iterator<Map.Entry<K, V>> {

    private int pos = entryList.size();
    private Iterator<Map.Entry<K, V>> lazyOverflowIterator;

    @Override
    public boolean hasNext() {
      return (pos > 0 && pos <= entryList.size()) || getOverflowIterator().hasNext();
    }

    @Override
    public Map.Entry<K, V> next() {
      if (getOverflowIterator().hasNext()) {
        return getOverflowIterator().next();
      }
      return entryList.get(--pos);
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException();
    }

    /**
     * It is important to create the overflow iterator only after the array entries have been
     * iterated over because the overflow entry set changes when the client calls remove() on the
     * array entries, which invalidates any existing iterators.
     */
    private Iterator<Map.Entry<K, V>> getOverflowIterator() {
      if (lazyOverflowIterator == null) {
        lazyOverflowIterator = overflowEntriesDescending.entrySet().iterator();
      }
      return lazyOverflowIterator;
    }
  }

  /**
   * Helper class that holds immutable instances of an Iterable/Iterator that we return when the
   * overflow entries is empty. This eliminates the creation of an Iterator object when there is
   * nothing to iterate over.
   */
  private static class EmptySet {

    private static final Iterator<Object> ITERATOR =
        new Iterator<Object>() {
          @Override
          public boolean hasNext() {
            return false;
          }

          @Override
          public Object next() {
            throw new NoSuchElementException();
          }

          @Override
          public void remove() {
            throw new UnsupportedOperationException();
          }
        };

    private static final Iterable<Object> ITERABLE =
        new Iterable<Object>() {
          @Override
          public Iterator<Object> iterator() {
            return ITERATOR;
          }
        };

    @SuppressWarnings("unchecked")
    static <T> Iterable<T> iterable() {
      return (Iterable<T>) ITERABLE;
    }
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }

    if (!(o instanceof SmallSortedMap)) {
      return super.equals(o);
    }

    SmallSortedMap<?, ?> other = (SmallSortedMap<?, ?>) o;
    final int size = size();
    if (size != other.size()) {
      return false;
    }

    // Best effort try to avoid allocating an entry set.
    final int numArrayEntries = getNumArrayEntries();
    if (numArrayEntries != other.getNumArrayEntries()) {
      return entrySet().equals(other.entrySet());
    }

    for (int i = 0; i < numArrayEntries; i++) {
      if (!getArrayEntryAt(i).equals(other.getArrayEntryAt(i))) {
        return false;
      }
    }

    if (numArrayEntries != size) {
      return overflowEntries.equals(other.overflowEntries);
    }

    return true;
  }

  @Override
  public int hashCode() {
    int h = 0;
    final int listSize = getNumArrayEntries();
    for (int i = 0; i < listSize; i++) {
      h += entryList.get(i).hashCode();
    }
    // Avoid the iterator allocation if possible.
    if (getNumOverflowEntries() > 0) {
      h += overflowEntries.hashCode();
    }
    return h;
  }
}
