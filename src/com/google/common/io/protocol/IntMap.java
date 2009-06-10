// Copyright 2009 Google Inc. All Rights Reserved.

package com.google.common.io.protocol;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.NoSuchElementException;

/**
 * A Map from primitive integers to Object values. This stores values
 * for smaller keys in an Object array, and uses {@link Hashtable}
 * only for larger keys. This is specifically designed to be used by
 * J2me protocol buffer runtime ({@link ProtoBuf}, {@link ProtoBufType})
 * to support large tags that are commonly used in Extensions/MessageSet.
 *
 * This class is not thread safe, so the client has to provide
 * appropriate locking mechanism if the map is to be used from
 * multiple threads.
 */
public class IntMap {
  private static final int MAX_LOWER_BUFFER_SIZE = 64;
  private static final int INITIAL_LOWER_BUFFER_SIZE = 8;

  /**
   * An iterator that returns int keys of the IntMap. IntMap has its
   * own Iterator instead of Enumeration to avoid autoboxing.  This
   * uses the same buffer of the IntMap, so you should not update the
   * IntMap while the iterator is in use. Once the IntMap is changed,
   * the behavior of preiously obtained iterator is undefined, and a new
   * KeyIterator has to be used instead.
   */
  public class KeyIterator {
    private int oneAheadIndex = 0;
    private int currentKey = Integer.MIN_VALUE;
    private Enumeration  higherKeyEnumerator = null;

    /**
     * @returns true if there is more keys.
     */
    public boolean hasNext() {
      if (currentKey != Integer.MIN_VALUE) {
        return true;
      }
      if (oneAheadIndex <= maxLowerKey) {
        for (; oneAheadIndex <= maxLowerKey; oneAheadIndex++) {
          if (lower[oneAheadIndex] != null) {
            // record the key, then increment the oneAheadIndex.
            currentKey = oneAheadIndex++;
            return true;
          }
        }
      }
      if (higher != null) {
        if (higherKeyEnumerator == null) {
          higherKeyEnumerator = higher.keys();
        }
        if (higherKeyEnumerator.hasMoreElements()) {
          Integer key = (Integer) higherKeyEnumerator.nextElement();
          currentKey = key.intValue();
          return true;
        }
      }
      return false;
    }

    /**
     * @returns next key
     * @throws NoSuchElementException if there is no more keys.
     */
    public int next() {
      if (currentKey == Integer.MIN_VALUE && !hasNext()) {
        throw new NoSuchElementException();
      }
      int key = currentKey;
      currentKey = Integer.MIN_VALUE;
      return key;
    }
  }

  /** Stores values for lower keys */
  private Object[] lower;

  /** Hashtable for higher tags */
  private Hashtable higher;

  /** A maximum key that has been ever added to the lower buffer.*/
  private int maxLowerKey;

  /** A maximum key that has been ever added to the map.*/
  private int maxKey;

  /** the number of elements in lower buffer */
  private int lowerCount;

  /**
   * Constructs an {@link IntMap} with default lower buffer size.
   */
  public IntMap() {
    this(INITIAL_LOWER_BUFFER_SIZE); // can expand 3 times
  }

  /**
   * Constructs an {@link IntMap} with the suggested initial lower buffer size.
   * The argument is just a hint and may not be used. If its value is
   * larger than {@link MAX_LOWER_BUFFER_SIZE} or negative, it will use the
   * MAX_LOWER_BUFFER_SIZE instead.
   */
  IntMap(int initialLowerBufferSize) {
    int lowerBufferSize = INITIAL_LOWER_BUFFER_SIZE;
    if (initialLowerBufferSize > 0) {
      lowerBufferSize = Math.min(initialLowerBufferSize, MAX_LOWER_BUFFER_SIZE);
    }
    lower = new Object[lowerBufferSize];
    lowerCount = 0;
    maxKey = Integer.MIN_VALUE;
    maxLowerKey = Integer.MIN_VALUE;
  }

  /**
   * A factory method to constructs an {@link IntMap} with the same
   * lower buffer size.
   *
   * @return a new IntMap whose lower buffer size is same as
   * this instance.
   */
  public IntMap newIntMapWithSameBufferSize() {
    return new IntMap(maxLowerKey);
  }

  /**
   * @return the {@link KeyIterator} of the map.
   */
  public KeyIterator keys() {
    return new KeyIterator();
  }

  /**
   * Returns max key that ever added to the map. Note that this is not
   * max for current state. Removing the max key will not update the
   * max key value. If nothing is added, it will return {@link
   * Integer.MIN_VALUE}, which means you cannot tell if an value is
   * added with MIN_VALUE key.
   */
  public int maxKey() {
    return maxKey;
  }

  /**
   * Returns the number of key-value pairs in the map.
   */
  public int size() {
    return higher == null ? lowerCount : lowerCount + higher.size();
  }

  /**
   * @return true if the map is empty.
   */
  public boolean isEmpty() {
    return size() == 0;
  }

  /**
   * Clears all key/value pairs. The map becomes empty after this
   * operation, but does not release memory.
   */
  public void clear() {
    for (int i = 0; i < lower.length; i++) {
      lower[i] = null;
    }
    if (higher != null) higher.clear();
    maxKey = Integer.MIN_VALUE;
    maxLowerKey = Integer.MIN_VALUE;
    lowerCount = 0;
  }

  /**
   * Returns the value associated with the given key.
   *
   * @param key a key
   * @return the value associated with the given key. null if the map has
   * no value for the key.
   */
  public Object get(int key) {
    if (key > maxKey) {
      return null;
    } else if (0 <= key && key <= maxLowerKey) {
      return lower[key];
    } else if (higher != null) {
      return higher.get(key);
    } else {
      return null;
    }
  }

  /**
   * Maps the specified key to the given value in the {@link IntMap}.
   * Caveat: Passing null value removes the value from the map. This is to
   * keep the semantics used in {@link ProtoBuf}.
   *
   * @param key a key
   * @param value the value to be added to the Map. If this is null,
   * the key will be removed from the map. This method does nothing if
   * the key does not exist and value is null.
   */
  public void put(int key, Object value) {
    if (value == null) {
      remove(key);
      return;
    }
    expandLowerIfNecessary(key);
    maxKey = Math.max(key, maxKey);
    if (0 <= key && key < lower.length) {
      maxLowerKey = Math.max(key, maxLowerKey);
      if (lower[key] == null) {
        lowerCount++;
      }
      lower[key] = value;
    } else {
      if (higher == null) {
        higher = new Hashtable();
      }
      higher.put(key, value);
    }
  }

  /**
   * Removes the key and corresponding value from the {@link IntMap}.
   *
   * @param key the key to remove. This method does nothing if the map does not
   * have the value corresponding for the key.
   * @return the removed value. null if the map does not have the value for
   * the key.
   */
  public Object remove(int key) {
    Object deleted = null;
    if (0 <= key && key < lower.length) {
      deleted = lower[key];
      if (deleted != null) {
        lowerCount--;
      }
      lower[key] = null;
    } else if (higher != null) {
      return higher.remove(key);
    }
    return deleted;
  }

  /**
   * Tests if given key has a corresponding value in this {@link IntMap}.
   *
   * @param key possible key.
   * @return <code>true</code> if the given key has the correspoding
   * value. <code>false</code> otherwise.
   */
  public boolean containsKey(int key) {
    if (0 <= key && key < lower.length) {
      return lower[key] != null;
    } else if (higher != null) {
      return higher.containsKey(key);
    }
    return false;
  }

  /**
   * Returns hashcode for {@link IntMap}.
   */
  public int hashCode() {
    int hashCode = 1;
    for (int i = 0; i < lower.length ; i++) {
      Object value = lower[i];
      if (value != null) {
        hashCode = 31 * hashCode + value.hashCode() + i;
      }
    }
    // Hashtable in J2me does not implement hashCode(), so we simply
    // use the size of hashtable.
    return higher == null ? hashCode : hashCode + higher.size();
  }

  /**
   * Compares the equality. Two IntMaps are considered equals iff
   * both map have the same key-value pairs.
   * Caveat: This assumes that the Class of each value object implements
   * equals correctly. This may not be the case in J2me.
   *
   * @param object an object to be compared with
   * @return true if the specified Object is equal to this IntMap
   */
  public boolean equals(Object object) {
    if (this == object) {
      return true;
    }
    if (object == null || !(object instanceof IntMap)) {
      return false;
    }
    IntMap peer = (IntMap) object;
    if (size() != peer.size()) {
      return false;
    }
    return compareLowerBuffer(lower, peer.lower) &&
        compareHashtable(higher, peer.higher);
  }

  private boolean compareLowerBuffer(Object[] lower1, Object[] lower2) {
    int min = Math.min(lower1.length, lower2.length);

    for (int i = 0; i < min; i++) {
      if ((lower1[i] == null && lower2[i] != null) ||
          (lower1[i] != null && !lower1[i].equals(lower2[i]))) {
        return false;
      }
    }
    // make sure there are no values in remaining fields.
    if (lower1.length > lower2.length) {
      for (int i = min; i < lower1.length; i++) {
        if (lower1[i] != null) return false;
      }
    } else if (lower1.length < lower2.length) {
      for (int i = min; i < lower2.length; i++) {
        if (lower2[i] != null) return false;
      }
    }
    return true;
  }

  /**
   * J2me's Hashtable does not implement equal, Bummer!
   */
  private static boolean compareHashtable(Hashtable h1, Hashtable h2) {
    if (h1 == h2) { // null == null is caught here
      return true;
    }
    if (h1 == null || h2 == null) {
      return false;
    }
    if (h1.size() != h2.size()) {
      return false;
    }
    // Ensure the values are the same.
    Enumeration h1Keys = h1.keys();
    while (h1Keys.hasMoreElements()) {
      Object key = h1Keys.nextElement();
      Object h1Value = h1.get(key);
      Object h2Value = h2.get(key);
      if (!h1Value.equals(h2Value)) {
        return false;
      }
    }
    return true;
  }

  /**
   * Expands lower buffer iff the key does not fit to current buffer size,
   * but will fit in MAX buffer size.
   */
  private void expandLowerIfNecessary(int key) {
    if (key <= MAX_LOWER_BUFFER_SIZE && key >= lower.length && key > 0) {
      int size = lower.length;
      do {
        size <<= 1;
      } while (size <= key);
      size = Math.min(size, MAX_LOWER_BUFFER_SIZE);
      Object[] newLower = new Object[size];
      System.arraycopy(lower, 0, newLower, 0, lower.length);
      lower = newLower;
    }
  }

  /* {@inheritDoc} */
  public String toString() {
    StringBuffer buffer = new StringBuffer("IntMap{lower:");
    for (int i = 0; i < lower.length; i++) {
      if (lower[i] != null) {
        buffer.append(i);
        buffer.append("=>");
        buffer.append(lower[i]);
        buffer.append(", ");
      }
    }
    buffer.append(", higher:" + higher + "}");
    return buffer.toString();
  }
}
