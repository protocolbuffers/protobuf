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

  private static class TestFieldDescriptor
      implements FieldSet.FieldDescriptorLite<TestFieldDescriptor> {
    private final int number;

    TestFieldDescriptor(int number) {
      this.number = number;
    }

    @Override
    public int getNumber() {
      return number;
    }

    @Override
    public WireFormat.FieldType getLiteType() {
      return WireFormat.FieldType.INT32;
    }

    @Override
    public WireFormat.JavaType getLiteJavaType() {
      return WireFormat.JavaType.INT;
    }

    @Override
    public boolean isRepeated() {
      return false;
    }

    @Override
    public boolean isPacked() {
      return false;
    }

    @Override
    public Internal.EnumLiteMap<?> getEnumType() {
      return null;
    }

    @Override
    public boolean internalMessageIsImmutable(Object message) {
      return false;
    }

    @Override
    public void internalMergeFrom(Object to, Object from) {}

    @Override
    public int compareTo(TestFieldDescriptor other) {
      return Integer.compare(number, other.number);
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) {
        return true;
      }
      if (!(o instanceof TestFieldDescriptor)) {
        return false;
      }
      return number == ((TestFieldDescriptor) o).number;
    }

    @Override
    public int hashCode() {
      return number;
    }

    @Override
    public String toString() {
      return String.valueOf(number);
    }
  }

  @Test
  public void testPutAndGetArrayEntriesOnly() {
    runPutAndGetTest(DEFAULT_FIELD_MAP_ARRAY_SIZE);
  }

  @Test
  public void testPutAndGetOverflowEntries() {
    runPutAndGetTest(DEFAULT_FIELD_MAP_ARRAY_SIZE * 2);
  }

  private void runPutAndGetTest(int numElements) {
    SmallSortedMap<TestFieldDescriptor, Integer> map1 = SmallSortedMap.newInstanceForTest();
    SmallSortedMap<TestFieldDescriptor, Integer> map3 = SmallSortedMap.newInstanceForTest();

    // Test with puts in ascending order.
    for (int i = 0; i < numElements; i++) {
      assertThat(map1.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    // Test with puts in descending order.
    for (int i = numElements - 1; i >= 0; i--) {
      assertThat(map3.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }

    assertThat(map1.getNumArrayEntries()).isEqualTo(min(16, numElements));
    assertThat(map3.getNumArrayEntries()).isEqualTo(min(16, numElements));

    List<SmallSortedMap<TestFieldDescriptor, Integer>> allMaps = new ArrayList<>();
    allMaps.add(map1);
    allMaps.add(map3);

    for (SmallSortedMap<TestFieldDescriptor, Integer> map : allMaps) {
      assertThat(map).hasSize(numElements);
      for (int i = 0; i < numElements; i++) {
        assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 1));
      }
    }

    assertThat(map1).isEqualTo(map3);
  }

  @Test
  public void testReplacingPut() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
      assertThat(map.remove(new TestFieldDescriptor(i + 1))).isNull();
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 2)).isEqualTo(Integer.valueOf(i + 1));
    }
  }

  @Test
  public void testRemove() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE + 3; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
      assertThat(map.remove(new TestFieldDescriptor(i + 1))).isNull();
    }

    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(3);
    assertThat(map).hasSize(19);
    assertThat(map.keySet())
        .isEqualTo(
            makeSortedKeySet(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(1))).isEqualTo(Integer.valueOf(2));
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(2);
    assertThat(map).hasSize(18);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(4))).isEqualTo(Integer.valueOf(5));
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(1);
    assertThat(map).hasSize(17);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(3))).isEqualTo(Integer.valueOf(4));
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).hasSize(16);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(3))).isNull();
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).hasSize(16);

    assertThat(map.remove(new TestFieldDescriptor(0))).isEqualTo(Integer.valueOf(1));
    assertThat(map.getNumArrayEntries()).isEqualTo(15);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).hasSize(15);
  }

  @Test
  public void testClear() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    map.clear();
    assertThat(map.getNumArrayEntries()).isEqualTo(0);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).isEmpty();
  }

  @Test
  public void testGetArrayEntryAndOverflowEntries() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    assertThat(map.getNumArrayEntries()).isEqualTo(DEFAULT_FIELD_MAP_ARRAY_SIZE);
    for (int i = 0; i < map.getNumArrayEntries(); i++) {
      Map.Entry<TestFieldDescriptor, Integer> entry = map.getArrayEntryAt(i);
      assertThat(entry.getKey()).isEqualTo(new TestFieldDescriptor(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    Iterator<Map.Entry<TestFieldDescriptor, Integer>> it = map.getOverflowEntries().iterator();
    assertThat(map.getNumOverflowEntries()).isEqualTo(DEFAULT_FIELD_MAP_ARRAY_SIZE);
    for (int i = DEFAULT_FIELD_MAP_ARRAY_SIZE; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(it.hasNext()).isTrue();
      Map.Entry<TestFieldDescriptor, Integer> entry = it.next();
      assertThat(entry.getKey()).isEqualTo(new TestFieldDescriptor(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    assertThat(it.hasNext()).isFalse();
  }

  @Test
  public void testEntrySetContains() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Set<Map.Entry<TestFieldDescriptor, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(entrySet)
          .contains(
              new AbstractMap.SimpleEntry<TestFieldDescriptor, Integer>(
                  new TestFieldDescriptor(i), i + 1));
      assertThat(entrySet)
          .doesNotContain(
              new AbstractMap.SimpleEntry<TestFieldDescriptor, Integer>(
                  new TestFieldDescriptor(i), i));
    }
  }

  @Test
  public void testEntrySetAdd() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    Set<Map.Entry<TestFieldDescriptor, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      Map.Entry<TestFieldDescriptor, Integer> entry =
          new AbstractMap.SimpleEntry<>(new TestFieldDescriptor(i), i + 1);
      assertThat(entrySet.add(entry)).isTrue();
      assertThat(entrySet.add(entry)).isFalse();
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 1));
    }
    assertThat(map.getNumArrayEntries()).isEqualTo(16);
    assertThat(map.getNumOverflowEntries()).isEqualTo(16);
    assertThat(map).hasSize(32);
  }

  @Test
  public void testEntrySetRemove() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    Set<Map.Entry<TestFieldDescriptor, Integer>> entrySet = map.entrySet();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      Map.Entry<TestFieldDescriptor, Integer> entry =
          new AbstractMap.SimpleEntry<>(new TestFieldDescriptor(i), i + 1);
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
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    map.clear();
    assertThat(map).isEmpty();
    assertThat(map.getNumArrayEntries()).isEqualTo(0);
    assertThat(map.getNumOverflowEntries()).isEqualTo(0);
    assertThat(map).isEmpty();
  }

  @Test
  public void testEntrySetIteratorNext() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Iterator<Map.Entry<TestFieldDescriptor, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(it.hasNext()).isTrue();
      Map.Entry<TestFieldDescriptor, Integer> entry = it.next();
      assertThat(entry.getKey()).isEqualTo(new TestFieldDescriptor(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    assertThat(it.hasNext()).isFalse();
  }

  @Test
  public void testEntrySetIteratorRemove() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Iterator<Map.Entry<TestFieldDescriptor, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map).containsKey(new TestFieldDescriptor(i));
      it.next();
      it.remove();
      assertThat(map).doesNotContainKey(new TestFieldDescriptor(i));
      assertThat(map).hasSize(32 - i - 1);
    }
  }

  @Test
  public void testMapEntryModification() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Iterator<Map.Entry<TestFieldDescriptor, Integer>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      Map.Entry<TestFieldDescriptor, Integer> entry = it.next();
      entry.setValue(i + 23);
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 23));
    }
  }

  @Test
  public void testMakeImmutable() {
    SmallSortedMap<TestFieldDescriptor, Integer> map = SmallSortedMap.newInstanceForTest();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    map.makeImmutable();
    assertThat(map).containsEntry(new TestFieldDescriptor(0), Integer.valueOf(1));
    assertThat(map).hasSize(DEFAULT_FIELD_MAP_ARRAY_SIZE * 2);

    try {
      map.put(new TestFieldDescriptor(23), 23);
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Map<TestFieldDescriptor, Integer> other = new HashMap<>();
    other.put(new TestFieldDescriptor(23), 23);
    try {
      map.putAll(other);
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      map.remove(new TestFieldDescriptor(0));
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    try {
      map.clear();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Set<Map.Entry<TestFieldDescriptor, Integer>> entrySet = map.entrySet();
    try {
      entrySet.clear();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Iterator<Map.Entry<TestFieldDescriptor, Integer>> it = entrySet.iterator();
    while (it.hasNext()) {
      Map.Entry<TestFieldDescriptor, Integer> entry = it.next();
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

    Set<TestFieldDescriptor> keySet = map.keySet();
    try {
      keySet.clear();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Iterator<TestFieldDescriptor> keys = keySet.iterator();
    while (keys.hasNext()) {
      TestFieldDescriptor key = keys.next();
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

  private Set<TestFieldDescriptor> makeSortedKeySet(Integer... keys) {
    Set<TestFieldDescriptor> set = new TreeSet<>();
    for (Integer key : keys) {
      set.add(new TestFieldDescriptor(key));
    }
    return set;
  }
}
