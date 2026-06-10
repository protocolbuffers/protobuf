package com.google.protobuf;

import java.util.AbstractMap;
import java.util.AbstractSet;
import java.util.Arrays;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;

/**
 * A field map implementation backed by two parallel arrays and optimized for insertion, iteration
 * and memory allocation.
 */
@SuppressWarnings({"unchecked", "ReturnMissingNullable"})
final class FieldDescriptorArrayMap<K extends FieldSet.FieldDescriptorLite<K>>
    extends AbstractMap<K, Object> {
  private K[] keys;
  private Object[] values;
  private int size;
  private volatile int firstUnsortedIndex = -1;
  private volatile boolean isImmutable = false;

  private static final int DEFAULT_INITIAL_CAPACITY = 16;

  /** Constructs a new map. */
  FieldDescriptorArrayMap() {
    this(DEFAULT_INITIAL_CAPACITY);
  }

  FieldDescriptorArrayMap(int initialCapacity) {
    this.keys = (K[]) new FieldSet.FieldDescriptorLite<?>[initialCapacity];
    this.values = new Object[initialCapacity];
    this.size = 0;
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
  public Object get(Object key) {
    if (!(key instanceof FieldSet.FieldDescriptorLite)) {
      return null;
    }
    ensureSortedAndDeduplicated();
    int index = binarySearch((K) key);
    return index >= 0 ? values[index] : null;
  }

  @Override
  public boolean containsKey(Object key) {
    if (!(key instanceof FieldSet.FieldDescriptorLite)) {
      return false;
    }
    ensureSortedAndDeduplicated();
    return binarySearch((K) key) >= 0;
  }

  @Override
  public Object put(K key, Object value) {
    checkMutable();
    if (key == null) {
      throw new NullPointerException("key cannot be null");
    }
    if (size > 0) {
      int comp = key.compareTo(keys[size - 1]);
      if (comp < 0) {
        if (firstUnsortedIndex == -1) {
          firstUnsortedIndex = size;
        }
      } else if (comp == 0) {
        // Overwrite existing value.
        values[size - 1] = value;
        // We could choose to return the previous value here, but we don't for consistency with
        // the code below.
        return null;
      }
    }
    ensureCapacity(size + 1);
    keys[size] = key;
    values[size] = value;
    size++;
    return null; // Note: doesn't return previous value to optimize for insertion speed
  }

  @Override
  public void putAll(Map<? extends K, ?> map) {
    checkMutable();
    ensureCapacity(size + map.size());
    for (Map.Entry<? extends K, ?> entry : map.entrySet()) {
      put(entry.getKey(), entry.getValue());
    }
  }

  @Override
  public void clear() {
    checkMutable();
    this.keys = (K[]) new FieldSet.FieldDescriptorLite<?>[DEFAULT_INITIAL_CAPACITY];
    this.values = new Object[DEFAULT_INITIAL_CAPACITY];
    size = 0;
    firstUnsortedIndex = -1;
  }

  @Override
  public Object remove(Object key) {
    checkMutable();
    ensureSortedAndDeduplicated();
    int index = binarySearch((K) key);
    if (index < 0) {
      return null;
    }
    Object oldValue = values[index];
    System.arraycopy(keys, index + 1, keys, index, size - index - 1);
    System.arraycopy(values, index + 1, values, index, size - index - 1);
    // Clear out the unused entries to allow garbage collection.
    keys[size] = null;
    values[size] = null;
    size--;
    return oldValue;
  }

  private void ensureCapacity(int minCapacity) {
    if (minCapacity > keys.length) {
      int newCapacity = Math.max(keys.length * 2, minCapacity);
      keys = Arrays.copyOf(keys, newCapacity);
      values = Arrays.copyOf(values, newCapacity);
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
          K key = keys[i];
          Object val = values[i];
          int j = i - 1;
          while (j >= 0 && keys[j].compareTo(key) > 0) {
            keys[j + 1] = keys[j];
            values[j + 1] = values[j];
            j--;
          }
          keys[j + 1] = key;
          values[j + 1] = val;
        }

        // Resolve duplicates in-place (stable, preserving last write)
        int newSize = 0;
        for (int i = 0; i < size; i++) {
          if (newSize > 0 && keys[newSize - 1].equals(keys[i])) {
            values[newSize - 1] = values[i];
          } else {
            keys[newSize] = keys[i];
            values[newSize] = values[i];
            newSize++;
          }
        }
        if (newSize < this.size) {
          this.size = newSize;
          // Clear out the unused entries to allow garbage collection.
          Arrays.fill(keys, newSize, keys.length, null);
          Arrays.fill(values, newSize, values.length, null);
        }
        this.firstUnsortedIndex = -1;
      }
    }
  }

  private int binarySearch(K key) {
    int low = 0;
    int high = size - 1;
    while (low <= high) {
      int mid = (low + high) >>> 1;
      K midVal = keys[mid];
      int cmp = midVal.compareTo(key);
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

  @Override
  public Set<Entry<K, Object>> entrySet() {
    ensureSortedAndDeduplicated();
    return new EntrySet();
  }

  public void makeImmutable() {
    if (isImmutable) {
      return;
    }
    synchronized (this) {
      if (isImmutable) {
        return;
      }
      ensureSortedAndDeduplicated();
      keys = Arrays.copyOf(keys, size);
      values = Arrays.copyOf(values, size);
      for (int i = 0; i < size; i++) {
        if (keys[i].isRepeated()) {
          values[i] = java.util.Collections.unmodifiableList((java.util.List<?>) values[i]);
        }
      }
      isImmutable = true;
    }
  }

  public boolean isImmutable() {
    return isImmutable;
  }

  private void checkMutable() {
    if (isImmutable) {
      throw new UnsupportedOperationException();
    }
  }

  private final class EntrySet extends AbstractSet<Entry<K, Object>> {
    @Override
    public int size() {
      return size;
    }

    @Override
    public Iterator<Entry<K, Object>> iterator() {
      return new Iterator<Entry<K, Object>>() {
        private int index = 0;

        @Override
        public boolean hasNext() {
          return index < size;
        }

        @Override
        public Entry<K, Object> next() {
          if (!hasNext()) {
            throw new NoSuchElementException();
          }
          final int curIndex = index++;
          return new Entry<K, Object>() {
            @Override
            public K getKey() {
              return keys[curIndex];
            }

            @Override
            public Object getValue() {
              return values[curIndex];
            }

            @Override
            public Object setValue(Object value) {
              if (isImmutable) {
                throw new UnsupportedOperationException("Map is immutable");
              }
              Object oldValue = values[curIndex];
              values[curIndex] = value;
              return oldValue;
            }
          };
        }

        @Override
        public void remove() {
          FieldDescriptorArrayMap.this.remove(keys[index]);
        }
      };
    }
  }
}
