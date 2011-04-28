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

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * @author darick@google.com Darick Tong
 */
public class SmallSortedMapTest extends TestCase {
  // java.util.AbstractMap.SimpleEntry is private in JDK 1.5. We re-implement it
  // here for JDK 1.5 users.
  private static class SimpleEntry<K, V> implements Map.Entry<K, V> {
    private final K key;
    private V value;

    SimpleEntry(K key, V value) {
      this.key = key;
      this.value = value;
    }

    public K getKey() {
      return key;
    }

    public V getValue() {
      return value;
    }

    public V setValue(V value) {
      V oldValue = this.value;
      this.value = value;
      return oldValue;
    }

    private static boolean eq(Object o1, Object o2) {
      return o1 == null ? o2 == null : o1.equals(o2);
    }

    @Override
    public boolean equals(Object o) {
      if (!(o instanceof Map.Entry))
        return false;
      Map.Entry e = (Map.Entry) o;
      return eq(key, e.getKey()) && eq(value, e.getValue());
    }

    @Override
    public int hashCode() {
      return ((key == null) ? 0 : key.hashCode()) ^
          ((value == null) ? 0 : value.hashCode());
    }
  }

  public void testPutAndGetArrayEntriesOnly() {
    runPutAndGetTest(3);
  }

  public void testPutAndGetOverflowEntries() {
    runPutAndGetTest(6);
  }

  private void runPutAndGetTest(int numElements) {
    // Test with even and odd arraySize
    SmallSortedMap<Integer, Integer> map1 =
        SmallSortedMap.newInstanceForTest(3);
    SmallSortedMap<Integer, Integer> map2 =
        SmallSortedMap.newInstanceForTest(4);
    SmallSortedMap<Integer, Integer> map3 =
        SmallSortedMap.newInstanceForTest(3);
    SmallSortedMap<Integer, Integer> map4 =
        SmallSortedMap.newInstanceForTest(4);

    // Test with puts in ascending order.
    for (int i = 0; i < numElements; i++) {
      assertNull(map1.put(i, i + 1));
      assertNull(map2.put(i, i + 1));
    }
    // Test with puts in descending order.
    for (int i = numElements - 1; i >= 0; i--) {
      assertNull(map3.put(i, i + 1));
      assertNull(map4.put(i, i + 1));
    }

    assertEquals(Math.min(3, numElements), map1.getNumArrayEntries());
    assertEquals(Math.min(4, numElements), map2.getNumArrayEntries());
    assertEquals(Math.min(3, numElements), map3.getNumArrayEntries());
    assertEquals(Math.min(4, numElements), map4.getNumArrayEntries());

    List<SmallSortedMap<Integer, Integer>> allMaps =
        new ArrayList<SmallSortedMap<Integer, Integer>>();
    allMaps.add(map1);
    allMaps.add(map2);
    allMaps.add(map3);
    allMaps.add(map4);

    for (SmallSortedMap<Integer, Integer> map : allMaps) {
      assertEquals(numElements, map.size());
      for (int i = 0; i < numElements; i++) {
        assertEquals(new Integer(i + 1), map.get(i));
      }
    }

    assertEquals(map1, map2);
    assertEquals(map2, map3);
    assertEquals(map3, map4);
  }

  public void testReplacingPut() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
      assertNull(map.remove(i + 1));
    }
    for (int i = 0; i < 6; i++) {
      assertEquals(new Integer(i + 1), map.put(i, i + 2));
    }
  }

  public void testRemove() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
      assertNull(map.remove(i + 1));
    }

    assertEquals(3, map.getNumArrayEntries());
    assertEquals(3, map.getNumOverflowEntries());
    assertEquals(6,  map.size());
    assertEquals(makeSortedKeySet(0, 1, 2, 3, 4, 5), map.keySet());

    assertEquals(new Integer(2), map.remove(1));
    assertEquals(3, map.getNumArrayEntries());
    assertEquals(2, map.getNumOverflowEntries());
    assertEquals(5,  map.size());
    assertEquals(makeSortedKeySet(0, 2, 3, 4, 5), map.keySet());

    assertEquals(new Integer(5), map.remove(4));
    assertEquals(3, map.getNumArrayEntries());
    assertEquals(1, map.getNumOverflowEntries());
    assertEquals(4,  map.size());
    assertEquals(makeSortedKeySet(0, 2, 3, 5), map.keySet());

    assertEquals(new Integer(4), map.remove(3));
    assertEquals(3, map.getNumArrayEntries());
    assertEquals(0, map.getNumOverflowEntries());
    assertEquals(3, map.size());
    assertEquals(makeSortedKeySet(0, 2, 5), map.keySet());

    assertNull(map.remove(3));
    assertEquals(3, map.getNumArrayEntries());
    assertEquals(0, map.getNumOverflowEntries());
    assertEquals(3, map.size());

    assertEquals(new Integer(1), map.remove(0));
    assertEquals(2, map.getNumArrayEntries());
    assertEquals(0, map.getNumOverflowEntries());
    assertEquals(2, map.size());
  }

  public void testClear() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    map.clear();
    assertEquals(0, map.getNumArrayEntries());
    assertEquals(0, map.getNumOverflowEntries());
    assertEquals(0, map.size());
  }

  public void testGetArrayEntryAndOverflowEntries() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    assertEquals(3, map.getNumArrayEntries());
    for (int i = 0; i < 3; i++) {
      Map.Entry<Integer, Integer> entry = map.getArrayEntryAt(i);
      assertEquals(new Integer(i), entry.getKey());
      assertEquals(new Integer(i + 1), entry.getValue());
    }
    Iterator<Map.Entry<Integer, Integer>> it =
        map.getOverflowEntries().iterator();
    for (int i = 3; i < 6; i++) {
      assertTrue(it.hasNext());
      Map.Entry<Integer, Integer> entry = it.next();
      assertEquals(new Integer(i), entry.getKey());
      assertEquals(new Integer(i + 1), entry.getValue());
    }
    assertFalse(it.hasNext());
  }

  public void testEntrySetContains() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < 6; i++) {
      assertTrue(
          entrySet.contains(new SimpleEntry<Integer, Integer>(i, i + 1)));
      assertFalse(
          entrySet.contains(new SimpleEntry<Integer, Integer>(i, i)));
    }
  }

  public void testEntrySetAdd() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < 6; i++) {
      Map.Entry<Integer, Integer> entry =
          new SimpleEntry<Integer, Integer>(i, i + 1);
      assertTrue(entrySet.add(entry));
      assertFalse(entrySet.add(entry));
    }
    for (int i = 0; i < 6; i++) {
      assertEquals(new Integer(i + 1), map.get(i));
    }
    assertEquals(3, map.getNumArrayEntries());
    assertEquals(3, map.getNumOverflowEntries());
    assertEquals(6, map.size());
  }

  public void testEntrySetRemove() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    for (int i = 0; i < 6; i++) {
      Map.Entry<Integer, Integer> entry =
          new SimpleEntry<Integer, Integer>(i, i + 1);
      assertTrue(entrySet.remove(entry));
      assertFalse(entrySet.remove(entry));
    }
    assertTrue(map.isEmpty());
    assertEquals(0, map.getNumArrayEntries());
    assertEquals(0, map.getNumOverflowEntries());
    assertEquals(0, map.size());
  }

  public void testEntrySetClear() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    map.entrySet().clear();
    assertTrue(map.isEmpty());
    assertEquals(0, map.getNumArrayEntries());
    assertEquals(0, map.getNumOverflowEntries());
    assertEquals(0, map.size());
  }

  public void testEntrySetIteratorNext() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    Iterator<Map.Entry<Integer, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < 6; i++) {
      assertTrue(it.hasNext());
      Map.Entry<Integer, Integer> entry = it.next();
      assertEquals(new Integer(i), entry.getKey());
      assertEquals(new Integer(i + 1), entry.getValue());
    }
    assertFalse(it.hasNext());
  }

  public void testEntrySetIteratorRemove() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    Iterator<Map.Entry<Integer, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < 6; i++) {
      assertTrue(map.containsKey(i));
      it.next();
      it.remove();
      assertFalse(map.containsKey(i));
      assertEquals(6 - i - 1, map.size());
    }
  }

  public void testMapEntryModification() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    Iterator<Map.Entry<Integer, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < 6; i++) {
      Map.Entry<Integer, Integer> entry = it.next();
      entry.setValue(i + 23);
    }
    for (int i = 0; i < 6; i++) {
      assertEquals(new Integer(i + 23), map.get(i));
    }
  }

  public void testMakeImmutable() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest(3);
    for (int i = 0; i < 6; i++) {
      assertNull(map.put(i, i + 1));
    }
    map.makeImmutable();
    assertEquals(new Integer(1), map.get(0));
    assertEquals(6, map.size());

    try {
      map.put(23, 23);
      fail("Expected UnsupportedOperationException");
    } catch (UnsupportedOperationException expected) {
    }

    Map<Integer, Integer> other = new HashMap<Integer, Integer>();
    other.put(23, 23);
    try {
      map.putAll(other);
      fail("Expected UnsupportedOperationException");
    } catch (UnsupportedOperationException expected) {
    }

    try {
      map.remove(0);
      fail("Expected UnsupportedOperationException");
    } catch (UnsupportedOperationException expected) {
    }

    try {
      map.clear();
      fail("Expected UnsupportedOperationException");
    } catch (UnsupportedOperationException expected) {
    }

    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    try {
      entrySet.clear();
      fail("Expected UnsupportedOperationException");
    } catch (UnsupportedOperationException expected) {
    }

    Iterator<Map.Entry<Integer, Integer>> it = entrySet.iterator();
    while (it.hasNext()) {
      Map.Entry<Integer, Integer> entry = it.next();
      try {
        entry.setValue(0);
        fail("Expected UnsupportedOperationException");
      } catch (UnsupportedOperationException expected) {
      }
      try {
        it.remove();
        fail("Expected UnsupportedOperationException");
      } catch (UnsupportedOperationException expected) {
      }
    }

    Set<Integer> keySet = map.keySet();
    try {
      keySet.clear();
      fail("Expected UnsupportedOperationException");
    } catch (UnsupportedOperationException expected) {
    }

    Iterator<Integer> keys = keySet.iterator();
    while (keys.hasNext()) {
      Integer key = keys.next();
      try {
        keySet.remove(key);
        fail("Expected UnsupportedOperationException");
      } catch (UnsupportedOperationException expected) {
      }
      try {
        keys.remove();
        fail("Expected UnsupportedOperationException");
      } catch (UnsupportedOperationException expected) {
      }
    }
  }

  private Set<Integer> makeSortedKeySet(Integer... keys) {
    return new TreeSet<Integer>(Arrays.<Integer>asList(keys));
  }
}
