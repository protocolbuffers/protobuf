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

import java.util.AbstractCollection;
import java.util.AbstractSet;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;

/**
 * A hash map that uses primitive integers as keys with open addressing. To minimize the memory
 * footprint, this class uses open addressing rather than chaining. Collisions are resolved using
 * linear probing. Deletions implement compaction, so cost of remove can approach O(N) for full
 * maps, which makes a small loadFactor recommended.
 *
 * @param <V> The value type stored in the map.
 */
@ExperimentalApi
public final class Int2ObjectHashMap<V> implements Map<Integer, V> {
  /**
   * A primitive entry in the map, provided by the iterator from {@link #entries()}
   *
   * @param <V> the value type stored in the map.
   */
  public interface PrimitiveEntry<V> {
    /** Gets the key for this entry. */
    int key();

    /** Gets the value for this entry. */
    V value();

    /** Sets the value for this entry. */
    void setValue(V value);
  }

  /** Default initial capacity. Used if not specified in the constructor */
  public static final int DEFAULT_CAPACITY = 8;

  /** Default load factor. Used if not specified in the constructor */
  public static final float DEFAULT_LOAD_FACTOR = 0.5f;

  /**
   * Placeholder for null values, so we can use the actual null to mean available. (Better than
   * using a placeholder for available: less references for GC processing.)
   */
  private static final Object NULL_VALUE = new Object();

  /** The maximum number of elements allowed without allocating more space. */
  private int maxSize;

  /** The load factor for the map. Used to calculate {@link #maxSize}. */
  private final float loadFactor;

  private int[] keys;
  private V[] values;
  private int size;
  private int mask;

  private final Set<Integer> keySet = new KeySet();
  private final Set<Entry<Integer, V>> entrySet = new EntrySet();
  private final Iterable<PrimitiveEntry<V>> entries =
      new Iterable<PrimitiveEntry<V>>() {
        @Override
        public Iterator<PrimitiveEntry<V>> iterator() {
          return new PrimitiveIterator();
        }
      };

  public Int2ObjectHashMap() {
    this(DEFAULT_CAPACITY, DEFAULT_LOAD_FACTOR);
  }

  public Int2ObjectHashMap(int initialCapacity) {
    this(initialCapacity, DEFAULT_LOAD_FACTOR);
  }

  public Int2ObjectHashMap(int initialCapacity, float loadFactor) {
    if (initialCapacity < 1) {
      throw new IllegalArgumentException("initialCapacity must be >= 1");
    }
    if (loadFactor <= 0.0f || loadFactor > 1.0f) {
      // Cannot exceed 1 because we can never store more than capacity elements;
      // using a bigger loadFactor would trigger rehashing before the desired load is reached.
      throw new IllegalArgumentException("loadFactor must be > 0 and <= 1");
    }

    this.loadFactor = loadFactor;

    // Adjust the initial capacity if necessary.
    int capacity = findNextPositivePowerOfTwo(initialCapacity);
    mask = capacity - 1;

    // Allocate the arrays.
    keys = new int[capacity];
    @SuppressWarnings({"unchecked", "SuspiciousArrayCast"})
    V[] temp = (V[]) new Object[capacity];
    values = temp;

    // Initialize the maximum size value.
    maxSize = calcMaxSize(capacity);
  }

  private static <T> T toExternal(T value) {
    return value == NULL_VALUE ? null : value;
  }

  @SuppressWarnings("unchecked")
  private static <T> T toInternal(T value) {
    return value == null ? (T) NULL_VALUE : value;
  }

  public V get(int key) {
    int index = indexOf(key);
    return index == -1 ? null : toExternal(values[index]);
  }

  public V put(int key, V value) {
    int startIndex = hashIndex(key);
    int index = startIndex;

    for (; ; ) {
      if (values[index] == null) {
        // Found empty slot, use it.
        keys[index] = key;
        values[index] = toInternal(value);
        growSize();
        return null;
      }
      if (keys[index] == key) {
        // Found existing entry with this key, just replace the value.
        V previousValue = values[index];
        values[index] = toInternal(value);
        return toExternal(previousValue);
      }

      // Conflict, keep probing ...
      if ((index = probeNext(index)) == startIndex) {
        // Can only happen if the map was full at MAX_ARRAY_SIZE and couldn't grow.
        throw new IllegalStateException("Unable to insert");
      }
    }
  }

  @Override
  public void putAll(Map<? extends Integer, ? extends V> sourceMap) {
    if (sourceMap instanceof Int2ObjectHashMap) {
      // Optimization - iterate through the arrays.
      @SuppressWarnings("unchecked")
      Int2ObjectHashMap<V> source = (Int2ObjectHashMap<V>) sourceMap;
      for (int i = 0; i < source.values.length; ++i) {
        V sourceValue = source.values[i];
        if (sourceValue != null) {
          put(source.keys[i], sourceValue);
        }
      }
      return;
    }

    // Otherwise, just add each entry.
    for (Entry<? extends Integer, ? extends V> entry : sourceMap.entrySet()) {
      put(entry.getKey(), entry.getValue());
    }
  }

  public V remove(int key) {
    int index = indexOf(key);
    if (index == -1) {
      return null;
    }

    V prev = values[index];
    removeAt(index);
    return toExternal(prev);
  }

  @Override
  public int size() {
    return size;
  }

  @Override
  public boolean isEmpty() {
    return size == 0;
  }

  @Override
  public void clear() {
    Arrays.fill(keys, 0);
    Arrays.fill(values, null);
    size = 0;
  }

  public boolean containsKey(int key) {
    return indexOf(key) >= 0;
  }

  @Override
  public boolean containsValue(Object value) {
    @SuppressWarnings("unchecked")
    V v1 = toInternal((V) value);
    for (V v2 : values) {
      // The map supports null values; this will be matched as NULL_VALUE.equals(NULL_VALUE).
      if (v2 != null && v2.equals(v1)) {
        return true;
      }
    }
    return false;
  }

  public Iterable<PrimitiveEntry<V>> entries() {
    return entries;
  }

  @Override
  public Collection<V> values() {
    return new AbstractCollection<V>() {
      @Override
      public Iterator<V> iterator() {
        return new Iterator<V>() {
          final PrimitiveIterator iter = new PrimitiveIterator();

          @Override
          public boolean hasNext() {
            return iter.hasNext();
          }

          @Override
          public V next() {
            return iter.next().value();
          }

          @Override
          public void remove() {
            throw new UnsupportedOperationException();
          }
        };
      }

      @Override
      public int size() {
        return size;
      }
    };
  }

  @Override
  public int hashCode() {
    // Hashcode is based on all non-zero, valid keys. We have to scan the whole keys
    // array, which may have different lengths for two maps of same size(), so the
    // capacity cannot be used as input for hashing but the size can.
    int hash = size;
    for (int key : keys) {
      // 0 can be a valid key or unused slot, but won't impact the hashcode in either case.
      // This way we can use a cheap loop without conditionals, or hard-to-unroll operations,
      // or the devastatingly bad memory locality of visiting value objects.
      // Also, it's important to use a hash function that does not depend on the ordering
      // of terms, only their values; since the map is an unordered collection and
      // entries can end up in different positions in different maps that have the same
      // elements, but with different history of puts/removes, due to conflicts.
      hash ^= hashCode(key);
    }
    return hash;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }
    if (!(obj instanceof Int2ObjectHashMap)) {
      return false;
    }
    @SuppressWarnings("rawtypes")
    Int2ObjectHashMap other = (Int2ObjectHashMap) obj;
    if (size != other.size()) {
      return false;
    }
    for (int i = 0; i < values.length; ++i) {
      V value = values[i];
      if (value != null) {
        int key = keys[i];
        Object otherValue = other.get(key);
        if (value == NULL_VALUE) {
          if (otherValue != null) {
            return false;
          }
        } else if (!value.equals(otherValue)) {
          return false;
        }
      }
    }
    return true;
  }

  @Override
  public boolean containsKey(Object key) {
    return containsKey(objectToKey(key));
  }

  @Override
  public V get(Object key) {
    return get(objectToKey(key));
  }

  @Override
  public V put(Integer key, V value) {
    return put(objectToKey(key), value);
  }

  @Override
  public V remove(Object key) {
    return remove(objectToKey(key));
  }

  @Override
  public Set<Integer> keySet() {
    return keySet;
  }

  @Override
  public Set<Entry<Integer, V>> entrySet() {
    return entrySet;
  }

  private int objectToKey(Object key) {
    return ((Integer) key).intValue();
  }

  /**
   * Locates the index for the given key. This method probes using double hashing.
   *
   * @param key the key for an entry in the map.
   * @return the index where the key was found, or {@code -1} if no entry is found for that key.
   */
  private int indexOf(int key) {
    int startIndex = hashIndex(key);
    int index = startIndex;

    for (; ; ) {
      if (values[index] == null) {
        // It's available, so no chance that this value exists anywhere in the map.
        return -1;
      }
      if (key == keys[index]) {
        return index;
      }

      // Conflict, keep probing ...
      if ((index = probeNext(index)) == startIndex) {
        return -1;
      }
    }
  }

  /** Returns the hashed index for the given key. */
  private int hashIndex(int key) {
    // The array lengths are always a power of two, so we can use a bitmask to stay inside the
    // array bounds.
    return hashCode(key) & mask;
  }

  /** Returns the hash code for the key. */
  private static int hashCode(int key) {
    return key;
  }

  /** Get the next sequential index after {@code index} and wraps if necessary. */
  private int probeNext(int index) {
    // The array lengths are always a power of two, so we can use a bitmask to stay inside the
    // array bounds.
    return (index + 1) & mask;
  }

  /** Grows the map size after an insertion. If necessary, performs a rehash of the map. */
  private void growSize() {
    size++;

    if (size > maxSize) {
      if (keys.length == Integer.MAX_VALUE) {
        throw new IllegalStateException("Max capacity reached at size=" + size);
      }

      // Double the capacity.
      rehash(keys.length << 1);
    }
  }

  /**
   * Removes entry at the given index position. Also performs opportunistic, incremental rehashing
   * if necessary to not break conflict chains.
   *
   * @param index the index position of the element to remove.
   * @return {@code true} if the next item was moved back. {@code false} otherwise.
   */
  private boolean removeAt(final int index) {
    --size;
    // Clearing the key is not strictly necessary (for GC like in a regular collection),
    // but recommended for security. The memory location is still fresh in the cache anyway.
    keys[index] = 0;
    values[index] = null;

    // In the interval from index to the next available entry, the arrays may have entries
    // that are displaced from their base position due to prior conflicts. Iterate these
    // entries and move them back if possible, optimizing future lookups.
    // Knuth Section 6.4 Algorithm R, also used by the JDK's IdentityHashMap.

    boolean movedBack = false;
    int nextFree = index;
    for (int i = probeNext(index); values[i] != null; i = probeNext(i)) {
      int bucket = hashIndex(keys[i]);
      if ((i < bucket && (bucket <= nextFree || nextFree <= i))
          || (bucket <= nextFree && nextFree <= i)) {
        // Move the displaced entry "back" to the first available position.
        keys[nextFree] = keys[i];
        values[nextFree] = values[i];
        movedBack = true;
        // Put the first entry after the displaced entry
        keys[i] = 0;
        values[i] = null;
        nextFree = i;
      }
    }
    return movedBack;
  }

  /** Calculates the maximum size allowed before rehashing. */
  private int calcMaxSize(int capacity) {
    // Clip the upper bound so that there will always be at least one available slot.
    int upperBound = capacity - 1;
    return Math.min(upperBound, (int) (capacity * loadFactor));
  }

  /**
   * Rehashes the map for the given capacity.
   *
   * @param newCapacity the new capacity for the map.
   */
  private void rehash(int newCapacity) {
    int[] oldKeys = keys;
    V[] oldVals = values;

    keys = new int[newCapacity];
    @SuppressWarnings({"unchecked", "SuspiciousArrayCast"})
    V[] temp = (V[]) new Object[newCapacity];
    values = temp;

    maxSize = calcMaxSize(newCapacity);
    mask = newCapacity - 1;

    // Insert to the new arrays.
    for (int i = 0; i < oldVals.length; ++i) {
      V oldVal = oldVals[i];
      if (oldVal != null) {
        // Inlined put(), but much simpler: we don't need to worry about
        // duplicated keys, growing/rehashing, or failing to insert.
        int oldKey = oldKeys[i];
        int index = hashIndex(oldKey);

        for (; ; ) {
          if (values[index] == null) {
            keys[index] = oldKey;
            values[index] = oldVal;
            break;
          }

          // Conflict, keep probing. Can wrap around, but never reaches startIndex again.
          index = probeNext(index);
        }
      }
    }
  }

  @Override
  public String toString() {
    if (isEmpty()) {
      return "{}";
    }
    StringBuilder sb = new StringBuilder(4 * size);
    sb.append('{');
    boolean first = true;
    for (int i = 0; i < values.length; ++i) {
      V value = values[i];
      if (value != null) {
        if (!first) {
          sb.append(", ");
        }
        sb.append(keyToString(keys[i]))
            .append('=')
            .append(value == this ? "(this Map)" : toExternal(value));
        first = false;
      }
    }
    return sb.append('}').toString();
  }

  /**
   * Helper method called by {@link #toString()} in order to convert a single map key into a string.
   * This is protected to allow subclasses to override the appearance of a given key.
   */
  protected String keyToString(int key) {
    return Integer.toString(key);
  }

  /** Set implementation for iterating over the entries of the map. */
  private final class EntrySet extends AbstractSet<Entry<Integer, V>> {
    @Override
    public Iterator<Entry<Integer, V>> iterator() {
      return new MapIterator();
    }

    @Override
    public int size() {
      return Int2ObjectHashMap.this.size();
    }
  }

  /** Set implementation for iterating over the keys. */
  private final class KeySet extends AbstractSet<Integer> {
    @Override
    public int size() {
      return Int2ObjectHashMap.this.size();
    }

    @Override
    public boolean contains(Object o) {
      return Int2ObjectHashMap.this.containsKey(o);
    }

    @Override
    public boolean remove(Object o) {
      return Int2ObjectHashMap.this.remove(o) != null;
    }

    @Override
    public boolean retainAll(Collection<?> retainedKeys) {
      boolean changed = false;
      for (Iterator<PrimitiveEntry<V>> iter = entries().iterator(); iter.hasNext(); ) {
        PrimitiveEntry<V> entry = iter.next();
        if (!retainedKeys.contains(entry.key())) {
          changed = true;
          iter.remove();
        }
      }
      return changed;
    }

    @Override
    public void clear() {
      Int2ObjectHashMap.this.clear();
    }

    @Override
    public Iterator<Integer> iterator() {
      return new Iterator<Integer>() {
        private final Iterator<Entry<Integer, V>> iter = entrySet.iterator();

        @Override
        public boolean hasNext() {
          return iter.hasNext();
        }

        @Override
        public Integer next() {
          return iter.next().getKey();
        }

        @Override
        public void remove() {
          iter.remove();
        }
      };
    }
  }

  /**
   * Iterator over primitive entries. Entry key/values are overwritten by each call to {@link
   * #next()}.
   */
  private final class PrimitiveIterator implements Iterator<PrimitiveEntry<V>>, PrimitiveEntry<V> {
    private int prevIndex = -1;
    private int nextIndex = -1;
    private int entryIndex = -1;

    private void scanNext() {
      for (; ; ) {
        if (++nextIndex == values.length || values[nextIndex] != null) {
          break;
        }
      }
    }

    @Override
    public boolean hasNext() {
      if (nextIndex == -1) {
        scanNext();
      }
      return nextIndex < keys.length;
    }

    @Override
    public PrimitiveEntry<V> next() {
      if (!hasNext()) {
        throw new NoSuchElementException();
      }

      prevIndex = nextIndex;
      scanNext();

      // Always return the same Entry object, just change its index each time.
      entryIndex = prevIndex;
      return this;
    }

    @Override
    public void remove() {
      if (prevIndex < 0) {
        throw new IllegalStateException("next must be called before each remove.");
      }
      if (removeAt(prevIndex)) {
        // removeAt may move elements "back" in the array if they have been displaced because
        // their spot in the array was occupied when they were inserted. If this occurs then the
        // nextIndex is now invalid and should instead point to the prevIndex which now holds an
        // element which was "moved back".
        nextIndex = prevIndex;
      }
      prevIndex = -1;
    }

    // Entry implementation. Since this implementation uses a single Entry, we coalesce that
    // into the Iterator object (potentially making loop optimization much easier).

    @Override
    public int key() {
      return keys[entryIndex];
    }

    @Override
    public V value() {
      return toExternal(values[entryIndex]);
    }

    @Override
    public void setValue(V value) {
      values[entryIndex] = toInternal(value);
    }
  }

  /** Iterator used by the {@link Map} interface. */
  private final class MapIterator implements Iterator<Entry<Integer, V>> {
    private final PrimitiveIterator iter = new PrimitiveIterator();

    @Override
    public boolean hasNext() {
      return iter.hasNext();
    }

    @Override
    public Entry<Integer, V> next() {
      if (!hasNext()) {
        throw new NoSuchElementException();
      }

      iter.next();

      return new MapEntry(iter.entryIndex);
    }

    @Override
    public void remove() {
      iter.remove();
    }
  }

  /** A single entry in the map. */
  final class MapEntry implements Entry<Integer, V> {
    private final int entryIndex;

    MapEntry(int entryIndex) {
      this.entryIndex = entryIndex;
    }

    @Override
    public Integer getKey() {
      verifyExists();
      return keys[entryIndex];
    }

    @Override
    public V getValue() {
      verifyExists();
      return toExternal(values[entryIndex]);
    }

    @Override
    public V setValue(V value) {
      verifyExists();
      V prevValue = toExternal(values[entryIndex]);
      values[entryIndex] = toInternal(value);
      return prevValue;
    }

    private void verifyExists() {
      if (values[entryIndex] == null) {
        throw new IllegalStateException("The map entry has been removed");
      }
    }
  }

  /**
   * Fast method of finding the next power of 2 greater than or equal to the supplied value.
   *
   * <p>If the value is {@code <= 0} then 1 will be returned. This method is not suitable for {@link
   * Integer#MIN_VALUE} or numbers greater than 2^30.
   *
   * @param value from which to search for next power of 2
   * @return The next power of 2 or the value itself if it is a power of 2
   */
  private static int findNextPositivePowerOfTwo(final int value) {
    assert value > Integer.MIN_VALUE && value < 0x40000000;
    return 1 << (32 - Integer.numberOfLeadingZeros(value - 1));
  }
}
