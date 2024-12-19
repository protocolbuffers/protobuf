// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static com.google.protobuf.SmallSortedMap.DEFAULT_FIELD_MAP_ARRAY_SIZE;
import static java.lang.Math.min;

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class SmallSortedMapTest {

  @Test
  public void testPutAndGetArrayEntriesOnly() {
    runPutAndGetTest(DEFAULT_FIELD_MAP_ARRAY_SIZE);
  }

  @Test
  public void testPutAndGetOverflowEntries() {
    runPutAndGetTest(DEFAULT_FIELD_MAP_ARRAY_SIZE * 2);
  }

  private void runPutAndGetTest(int numElements) {
    SmallSortedMap<Integer, Integer> map1 = SmallSortedMap.newInstanceForTest();
    SmallSortedMap<Integer, Integer> map3 = SmallSortedMap.newInstanceForTest();

    // Test with puts in ascending order.
    for (int i = 0; i < numElements; i++) {
      assertThat(map1.put(i, i + 1)).isNull();
    }
    // Test with puts in descending order.
    for (int i = numElements - 1; i >= 0; i--) {
      assertThat(map3.put(i, i + 1)).isNull();
    }

    assertThat(map1.getNumArrayEntries()).isEqualTo(min(16, numElements));
    assertThat(map3.getNumArrayEntries()).isEqualTo(min(16, numElements));

    List<SmallSortedMap<Integer, Integer>> allMaps = new ArrayList<>();
    allMaps.add(map1);
    allMaps.add(map3);

    for (SmallSortedMap<Integer, Integer> map : allMaps) {
      assertThat(map).hasSize(numElements);
      for (int i = 0; i < numElements; i++) {
        assertThat(map).containsEntry(i, Integer.valueOf(i + 1));
      }
    }

    assertThat(map1).isEqualTo(map3);
  }

  @Test
  public void testReplacingPut() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
      assertThat(map.remove(i + 1)).isNull();
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 2)).isEqualTo(Integer.valueOf(i + 1));
    }
  }

  @Test
  public void testRemove() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE + 3; i++) {
      assertThat(map.put(i, i + 1)).isNull();
      assertThat(map.remove(i + 1)).isNull();
    }

    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(3);
    assertThat(map).hasSize(19);
    assertThat(map.keySet())
        .isEqualTo(
            makeSortedKeySet(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(1)).isEqualTo(Integer.valueOf(2));
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(2);
    assertThat(map).hasSize(18);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(4)).isEqualTo(Integer.valueOf(5));
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(1);
    assertThat(map).hasSize(17);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(3)).isEqualTo(Integer.valueOf(4));
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).hasSize(16);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(3)).isNull();
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).hasSize(16);

    assertThat(map.remove(0)).isEqualTo(Integer.valueOf(1));
    assertThat(map.getNumArrayEntries()).isEqualTo(15);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).hasSize(15);
  }

  @Test
  public void testClear() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    map.clear();
    assertThat(map.getNumArrayEntries()).isEqualTo(0);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).isEmpty();
  }

  @Test
  public void testGetArrayEntryAndOverflowEntries() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    assertThat(map.getNumArrayEntries()).isEqualTo(DEFAULT_FIELD_MAP_ARRAY_SIZE);
    for (int i = 0; i < map.getNumArrayEntries(); i++) {
      Map.Entry<Integer, Integer> entry = map.getArrayEntryAt(i);
      assertThat(entry.getKey()).isEqualTo(Integer.valueOf(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    Iterator<Map.Entry<Integer, Integer>> it = map.getOverflowEntries().iterator();
    assertThat(map.getNumOverflowEntries()).isEqualTo(DEFAULT_FIELD_MAP_ARRAY_SIZE);
    for (int i = DEFAULT_FIELD_MAP_ARRAY_SIZE; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(it.hasNext()).isTrue();
      Map.Entry<Integer, Integer> entry = it.next();
      assertThat(entry.getKey()).isEqualTo(Integer.valueOf(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    assertThat(it.hasNext()).isFalse();
  }

  @Test
  public void testEntrySetContains() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(entrySet).contains(new AbstractMap.SimpleEntry<Integer, Integer>(i, i + 1));
      assertThat(entrySet).doesNotContain(new AbstractMap.SimpleEntry<Integer, Integer>(i, i));
    }
  }

  @Test
  public void testEntrySetAdd() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      Map.Entry<Integer, Integer> entry = new AbstractMap.SimpleEntry<>(i, i + 1);
      assertThat(entrySet.add(entry)).isTrue();
      assertThat(entrySet.add(entry)).isFalse();
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map).containsEntry(i, Integer.valueOf(i + 1));
    }
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(16);
    assertThat(map).hasSize(32);
  }

  @Test
  public void testEntrySetRemove() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      Map.Entry<Integer, Integer> entry = new AbstractMap.SimpleEntry<>(i, i + 1);
      assertThat(entrySet.remove(entry)).isTrue();
      assertThat(entrySet.remove(entry)).isFalse();
    }
    assertThat(map).isEmpty();
    assertThat(map.getNumArrayEntries()).isEqualTo(0);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).isEmpty();
  }

  @Test
  public void testEntrySetClear() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    map.clear();
    assertThat(map).isEmpty();
    assertThat(map.getNumArrayEntries()).isEqualTo(0);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).isEmpty();
  }

  @Test
  public void testEntrySetIteratorNext() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    Iterator<Map.Entry<Integer, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(it.hasNext()).isTrue();
      Map.Entry<Integer, Integer> entry = it.next();
      assertThat(entry.getKey()).isEqualTo(Integer.valueOf(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    assertThat(it.hasNext()).isFalse();
  }

  @Test
  public void testEntrySetIteratorRemove() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    Iterator<Map.Entry<Integer, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map).containsKey(i);
      it.next();
      it.remove();
      assertThat(map).doesNotContainKey(i);
      assertThat(map).hasSize(32 - i - 1);
    }
  }

  @Test
  public void testMapEntryModification() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    Iterator<Map.Entry<Integer, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      Map.Entry<Integer, Integer> entry = it.next();
      entry.setValue(i + 23);
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map).containsEntry(i, Integer.valueOf(i + 23));
    }
  }

  @Test
  public void testMakeImmutable() {
    SmallSortedMap<Integer, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(i, i + 1)).isNull();
    }
    map.makeImmutable();
    assertThat(map).containsEntry(0, Integer.valueOf(1));
    assertThat(map).hasSize(DEFAULT_FIELD_MAP_ARRAY_SIZE * 2);

    try {
      map.put(23, 23);
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Map<Integer, Integer> other = new HashMap<>();
    other.put(23, 23);
    try {
      map.putAll(other);
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      map.remove(0);
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      map.clear();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Set<Map.Entry<Integer, Integer>> entrySet = map.entrySet();
    try {
      entrySet.clear();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Iterator<Map.Entry<Integer, Integer>> it = entrySet.iterator();
    while (it.hasNext()) {
      Map.Entry<Integer, Integer> entry = it.next();
      try {
        entry.setValue(0);
        assertWithMessage("Expected UnsupportedOperationException").fail();
      } catch (UnsupportedOperationException expected) {
      }
      try {
        it.remove();
        assertWithMessage("Expected UnsupportedOperationException").fail();
      } catch (UnsupportedOperationException expected) {
      }
    }

    Set<Integer> keySet = map.keySet();
    try {
      keySet.clear();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Iterator<Integer> keys = keySet.iterator();
    while (keys.hasNext()) {
      Integer key = keys.next();
      try {
        keySet.remove(key);
        assertWithMessage("Expected UnsupportedOperationException").fail();
      } catch (UnsupportedOperationException expected) {
      }
      try {
        keys.remove();
        assertWithMessage("Expected UnsupportedOperationException").fail();
      } catch (UnsupportedOperationException expected) {
      }
    }
  }

  private Set<Integer> makeSortedKeySet(Integer... keys) {
    return new TreeSet<>(Arrays.<Integer>asList(keys));
  }
}
