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

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicInteger;
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

  private void runPutAndGetTest(int numElements) {
    SmallSortedMap<TestFieldDescriptor> map1 = new SmallSortedMap<>();
    SmallSortedMap<TestFieldDescriptor> map3 = new SmallSortedMap<>();

    // Test with puts in ascending order.
    for (int i = 0; i < numElements; i++) {
      assertThat(map1.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    // Test with puts in descending order.
    for (int i = numElements - 1; i >= 0; i--) {
      assertThat(map3.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }

    assertThat(map1.size()).isEqualTo(numElements);
    assertThat(map3.size()).isEqualTo(numElements);

    List<SmallSortedMap<TestFieldDescriptor>> allMaps = new ArrayList<>();
    allMaps.add(map1);
    allMaps.add(map3);

    for (SmallSortedMap<TestFieldDescriptor> map : allMaps) {
      assertThat(map).hasSize(numElements);
      for (int i = 0; i < numElements; i++) {
        assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 1));
      }
    }

    assertThat(map1).isEqualTo(map3);
  }

  @Test
  public void testReplacingPut() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
      assertThat(map.remove(new TestFieldDescriptor(i + 1))).isNull();
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 2)).isNull();
    }
  }

  @Test
  public void testRemove() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE + 3; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
      assertThat(map.remove(new TestFieldDescriptor(i + 1))).isNull();
    }

    assertThat(map).hasSize(19);
    assertThat(map.keySet())
        .isEqualTo(
            makeSortedKeySet(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(1))).isEqualTo(Integer.valueOf(2));
    assertThat(map).hasSize(18);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(4))).isEqualTo(Integer.valueOf(5));
    assertThat(map).hasSize(17);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(3))).isEqualTo(Integer.valueOf(4));
    assertThat(map).hasSize(16);
    assertThat(map.keySet())
        .isEqualTo(makeSortedKeySet(0, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));

    assertThat(map.remove(new TestFieldDescriptor(3))).isNull();
    assertThat(map).hasSize(16);

    assertThat(map.remove(new TestFieldDescriptor(0))).isEqualTo(Integer.valueOf(1));
    assertThat(map).hasSize(15);
  }

  @Test
  public void testRemoveAtArrayEnd() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    assertThat(map.remove(new TestFieldDescriptor(DEFAULT_FIELD_MAP_ARRAY_SIZE - 1)))
        .isEqualTo(Integer.valueOf(DEFAULT_FIELD_MAP_ARRAY_SIZE));
    assertThat(map).hasSize(DEFAULT_FIELD_MAP_ARRAY_SIZE - 1);
  }

  @Test
  public void testClear() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    map.clear();
    assertThat(map.size()).isEqualTo(0);
    assertThat(map).isEmpty();
  }

  @Test
  public void testGetArrayEntry() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    assertThat(map.size()).isEqualTo(DEFAULT_FIELD_MAP_ARRAY_SIZE * 2);
    for (int i = 0; i < map.size(); i++) {
      Map.Entry<TestFieldDescriptor, Object> entry = map.getArrayEntryAt(i);
      assertThat(entry.getKey()).isEqualTo(new TestFieldDescriptor(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
  }

  @Test
  public void testEntrySetContains() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Set<Map.Entry<TestFieldDescriptor, Object>> entrySet = map.entrySet();
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
  public void testEntrySetClear() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    map.clear();
    assertThat(map).isEmpty();
    assertThat(map.size()).isEqualTo(0);
    assertThat(map).isEmpty();
  }

  @Test
  public void testEntrySetIteratorNext() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Iterator<Map.Entry<TestFieldDescriptor, Object>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(it.hasNext()).isTrue();
      Map.Entry<TestFieldDescriptor, Object> entry = it.next();
      assertThat(entry.getKey()).isEqualTo(new TestFieldDescriptor(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    assertThat(it.hasNext()).isFalse();
  }

  @Test
  public void testMapEntryModification() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Iterator<Map.Entry<TestFieldDescriptor, Object>> it = map.entrySet().iterator();
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      Map.Entry<TestFieldDescriptor, Object> entry = it.next();
      entry.setValue(i + 23);
    }
    for (int i = 0; i < DEFAULT_FIELD_MAP_ARRAY_SIZE * 2; i++) {
      assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 23));
    }
  }

  @Test
  public void testMakeImmutable() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
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

    Set<Map.Entry<TestFieldDescriptor, Object>> entrySet = map.entrySet();
    try {
      entrySet.clear();
      assertWithMessage("Expected UnsupportedOperationException").fail();
    } catch (UnsupportedOperationException expected) {
    }

    Iterator<Map.Entry<TestFieldDescriptor, Object>> it = entrySet.iterator();
    while (it.hasNext()) {
      Map.Entry<TestFieldDescriptor, Object> entry = it.next();
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

  @Test
  public void testConcurrentPutsEnsureSortedAndDeduplicated() throws Exception {
    final SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    map.put(new TestFieldDescriptor(10), 100);
    map.put(new TestFieldDescriptor(20), 200);
    map.put(new TestFieldDescriptor(30), 300);

    int numThreads = 10;
    Thread[] threads = new Thread[numThreads];
    final AtomicInteger errors = new AtomicInteger(0);
    final CountDownLatch latch = new CountDownLatch(1);

    for (int i = 0; i < numThreads; i++) {
      final int index = i;
      threads[i] =
          new Thread(
              new Runnable() {
                @Override
                public void run() {
                  try {
                    latch.await();
                    synchronized (map) {
                      map.put(new TestFieldDescriptor(index), index * 10);
                    }
                  } catch (Exception e) {
                    errors.incrementAndGet();
                  }
                }
              });
      threads[i].start();
    }

    latch.countDown();
    for (int i = 0; i < numThreads; i++) {
      threads[i].join();
    }

    assertThat(errors.get()).isEqualTo(0);

    assertThat(map.size()).isEqualTo(13);
    int[] expectedKeys = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30};
    int i = 0;
    for (Map.Entry<TestFieldDescriptor, Object> entry : map.entrySet()) {
      assertThat(entry.getKey().getNumber()).isEqualTo(expectedKeys[i++]);
    }
  }

  @Test
  public void testConcurrentReadsWithUnsortedKeys() throws Exception {
    final SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    map.put(new TestFieldDescriptor(50), 500);
    map.put(new TestFieldDescriptor(10), 100);
    map.put(new TestFieldDescriptor(30), 300);
    map.put(new TestFieldDescriptor(20), 200);
    map.put(new TestFieldDescriptor(40), 400);

    int numThreads = 10;
    Thread[] threads = new Thread[numThreads];
    final AtomicInteger errors = new AtomicInteger(0);
    final CountDownLatch latch = new CountDownLatch(1);

    for (int i = 0; i < numThreads; i++) {
      threads[i] =
          new Thread(
              new Runnable() {
                @Override
                public void run() {
                  try {
                    latch.await();
                    assertThat(map.get(new TestFieldDescriptor(30)))
                        .isEqualTo(Integer.valueOf(300));
                    assertThat(map.containsKey(new TestFieldDescriptor(20))).isTrue();
                    assertThat(map.size()).isEqualTo(5);
                  } catch (Exception e) {
                    errors.incrementAndGet();
                  }
                }
              });
      threads[i].start();
    }

    latch.countDown();
    for (int i = 0; i < numThreads; i++) {
      threads[i].join();
    }

    assertThat(errors.get()).isEqualTo(0);

    int[] expectedKeys = {10, 20, 30, 40, 50};
    int index = 0;
    for (Map.Entry<TestFieldDescriptor, Object> entry : map.entrySet()) {
      assertThat(entry.getKey().getNumber()).isEqualTo(expectedKeys[index++]);
    }
  }

  @Test
  @SuppressWarnings("ModifyCollectionInEnhancedForLoop") // For testing
  public void testReplacingPutOutOfOrderDuringIteration() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    map.put(new TestFieldDescriptor(10), 100);
    map.put(new TestFieldDescriptor(20), 200);
    map.put(new TestFieldDescriptor(30), 300);

    List<Object> values = new ArrayList<>();
    int iterations = 0;
    for (Map.Entry<TestFieldDescriptor, Object> entry : map.entrySet()) {
      iterations++;
      if (entry.getKey().getNumber() == 10) {
        // Replaces preexisting entry 20, but is out of order relative to the last key (30).
        map.put(new TestFieldDescriptor(20), 201);
      }
      values.add(entry.getValue());
    }

    assertThat(iterations).isEqualTo(3);
    assertThat(values).containsExactly(100, 201, 300).inOrder();
  }

  @Test
  @SuppressWarnings("ModifyCollectionInEnhancedForLoop") // For testing
  public void testRemoveAndPutDuringIteration() {
    SmallSortedMap<TestFieldDescriptor> map = new SmallSortedMap<>();
    map.put(new TestFieldDescriptor(10), 100);
    map.put(new TestFieldDescriptor(20), 200);
    map.put(new TestFieldDescriptor(30), 300);

    List<Integer> visitedKeys = new ArrayList<>();
    for (Map.Entry<TestFieldDescriptor, Object> entry : map.entrySet()) {
      int keyNumber = entry.getKey().getNumber();
      visitedKeys.add(keyNumber);
      if (keyNumber == 20) {
        map.remove(new TestFieldDescriptor(20));
        map.put(new TestFieldDescriptor(20), 201);
      }
    }

    assertThat(visitedKeys).containsExactly(10, 20, 30).inOrder();
    assertThat(map.get(new TestFieldDescriptor(20))).isEqualTo(Integer.valueOf(201));
  }

  private Set<TestFieldDescriptor> makeSortedKeySet(Integer... keys) {
    Set<TestFieldDescriptor> set = new TreeSet<>();
    for (Integer key : keys) {
      set.add(new TestFieldDescriptor(key));
    }
    return set;
  }
}
