// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FieldDescriptorArrayMapTest {

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
  public void testPutAndGet() {
    int numElements = 100;
    FieldDescriptorArrayMap<TestFieldDescriptor> map1 = new FieldDescriptorArrayMap<>();
    FieldDescriptorArrayMap<TestFieldDescriptor> map3 = new FieldDescriptorArrayMap<>();

    // Test with puts in ascending order.
    for (int i = 0; i < numElements; i++) {
      assertThat(map1.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    // Test with puts in descending order.
    for (int i = numElements - 1; i >= 0; i--) {
      assertThat(map3.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }

    List<FieldDescriptorArrayMap<TestFieldDescriptor>> allMaps = new ArrayList<>();
    allMaps.add(map1);
    allMaps.add(map3);

    for (FieldDescriptorArrayMap<TestFieldDescriptor> map : allMaps) {
      assertThat(map).hasSize(numElements);
      for (int i = 0; i < numElements; i++) {
        assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 1));
      }
    }

    assertThat(map1).isEqualTo(map3);
  }

  @Test
  public void testReplacingPut() {
    FieldDescriptorArrayMap<TestFieldDescriptor> map = new FieldDescriptorArrayMap<>();
    int numElements = 100;
    for (int i = 0; i < numElements; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    for (int i = 0; i < numElements; i++) {
      // put returns null in FieldDescriptorArrayMap even if it replaces.
      assertThat(map.put(new TestFieldDescriptor(i), i + 2)).isNull();
    }
    for (int i = 0; i < numElements; i++) {
      assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 2));
    }
  }

  @Test
  public void testRemove() {
    FieldDescriptorArrayMap<TestFieldDescriptor> map = new FieldDescriptorArrayMap<>();
    for (int i = 0; i < 100; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
      assertThat(map.remove(new TestFieldDescriptor(i + 1))).isNull();
    }

    assertThat(map).hasSize(100);

    assertThat(map.remove(new TestFieldDescriptor(1))).isEqualTo(Integer.valueOf(2));
    assertThat(map).hasSize(99);

    assertThat(map.remove(new TestFieldDescriptor(4))).isEqualTo(Integer.valueOf(5));
    assertThat(map).hasSize(98);

    assertThat(map.remove(new TestFieldDescriptor(3))).isEqualTo(Integer.valueOf(4));
    assertThat(map).hasSize(97);

    assertThat(map.remove(new TestFieldDescriptor(3))).isNull();
    assertThat(map).hasSize(97);

    assertThat(map.remove(new TestFieldDescriptor(0))).isEqualTo(Integer.valueOf(1));
    assertThat(map).hasSize(96);
  }

  @Test
  public void testClear() {
    FieldDescriptorArrayMap<TestFieldDescriptor> map = new FieldDescriptorArrayMap<>();
    for (int i = 0; i < 100; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    map.clear();
    assertThat(map).isEmpty();
  }

  @Test
  public void testEntrySetContains() {
    FieldDescriptorArrayMap<TestFieldDescriptor> map = new FieldDescriptorArrayMap<>();
    for (int i = 0; i < 100; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Set<Map.Entry<TestFieldDescriptor, Object>> entrySet = map.entrySet();
    for (int i = 0; i < 100; i++) {
      assertThat(entrySet)
          .contains(
              new AbstractMap.SimpleEntry<TestFieldDescriptor, Object>(
                  new TestFieldDescriptor(i), i + 1));
      assertThat(entrySet)
          .doesNotContain(
              new AbstractMap.SimpleEntry<TestFieldDescriptor, Object>(
                  new TestFieldDescriptor(i), i));
    }
  }

  @Test
  public void testEntrySetIteratorNext() {
    FieldDescriptorArrayMap<TestFieldDescriptor> map = new FieldDescriptorArrayMap<>();
    for (int i = 0; i < 100; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Iterator<Map.Entry<TestFieldDescriptor, Object>> it = map.entrySet().iterator();
    for (int i = 0; i < 100; i++) {
      assertThat(it.hasNext()).isTrue();
      Map.Entry<TestFieldDescriptor, Object> entry = it.next();
      assertThat(entry.getKey()).isEqualTo(new TestFieldDescriptor(i));
      assertThat(entry.getValue()).isEqualTo(Integer.valueOf(i + 1));
    }
    assertThat(it.hasNext()).isFalse();
  }

  @Test
  public void testMapEntryModification() {
    FieldDescriptorArrayMap<TestFieldDescriptor> map = new FieldDescriptorArrayMap<>();
    for (int i = 0; i < 100; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    Iterator<Map.Entry<TestFieldDescriptor, Object>> it = map.entrySet().iterator();
    for (int i = 0; i < 100; i++) {
      Map.Entry<TestFieldDescriptor, Object> entry = it.next();
      entry.setValue(i + 23);
    }
    for (int i = 0; i < 100; i++) {
      assertThat(map).containsEntry(new TestFieldDescriptor(i), Integer.valueOf(i + 23));
    }
  }

  @Test
  public void testMakeImmutable() {
    FieldDescriptorArrayMap<TestFieldDescriptor> map = new FieldDescriptorArrayMap<>();
    for (int i = 0; i < 100; i++) {
      assertThat(map.put(new TestFieldDescriptor(i), i + 1)).isNull();
    }
    map.makeImmutable();
    assertThat(map).containsEntry(new TestFieldDescriptor(0), Integer.valueOf(1));
    assertThat(map).hasSize(100);

    TestFieldDescriptor testFieldDescriptor = new TestFieldDescriptor(123);
    assertThrows(UnsupportedOperationException.class, () -> map.put(testFieldDescriptor, 123));

    Map<TestFieldDescriptor, Object> other = new HashMap<>();
    other.put(testFieldDescriptor, 123);
    assertThrows(UnsupportedOperationException.class, () -> map.putAll(other));

    assertThrows(UnsupportedOperationException.class, map::clear);
    assertThrows(UnsupportedOperationException.class, () -> map.remove(new TestFieldDescriptor(0)));

    Set<Map.Entry<TestFieldDescriptor, Object>> entrySet = map.entrySet();
    Iterator<Map.Entry<TestFieldDescriptor, Object>> it = entrySet.iterator();
    while (it.hasNext()) {
      Map.Entry<TestFieldDescriptor, Object> entry = it.next();
      assertThrows(UnsupportedOperationException.class, () -> entry.setValue(0));
      if (it.hasNext()) {
        assertThrows(UnsupportedOperationException.class, it::remove);
      }
    }
  }
}
