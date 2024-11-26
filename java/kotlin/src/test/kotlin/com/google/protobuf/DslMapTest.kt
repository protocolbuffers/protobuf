// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.common.testing.EqualsTester
import com.google.common.truth.Truth.assertThat
import kotlin.test.assertFailsWith
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

@RunWith(JUnit4::class)
@OptIn(OnlyForUseByGeneratedProtoCode::class)
class DslMapTest {
  class DummyProxy private constructor() : DslProxy()

  @Test
  fun matchesMap() {
    assertThat(DslMap<Int, Int, DummyProxy>(mapOf(1 to -1, 2 to -2))).containsExactly(1, -1, 2, -2)
  }

  @Test
  fun reflectsChangesInMap() {
    val mutableMap = mutableMapOf(1 to -1, 2 to -2)
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMap)
    mutableMap[3] = -3
    assertThat(dslMap).containsExactly(1, -1, 2, -2, 3, -3).inOrder()
  }

  @Test
  fun dslMapIsNotMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    assertThat(dslMap is MutableMap<*, *>).isFalse()
  }

  @Test
  fun dslMapKeysAreNotMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    assertThat(dslMap.keys is MutableSet<*>).isFalse()
  }

  @Test
  fun dslMapValuesAreNotMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    assertThat(dslMap.values is MutableSet<*>).isFalse()
  }

  @Test
  fun dslMapEntriesAreNotMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    assertThat(dslMap.entries is MutableSet<*>).isFalse()
  }

  @Test
  fun dslMapEntryObjectsAreNotMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    assertThat(dslMap.entries.single() is MutableMap.MutableEntry<*, *>).isFalse()
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapIsNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapAsJavaUtilMap = dslMap as java.util.Map<Int, Int>
    assertFailsWith<UnsupportedOperationException> { dslMapAsJavaUtilMap.put(2, -2) }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapKeysAreNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapKeysAsJavaUtilSet = dslMap.keys as java.util.Set<Int>
    assertFailsWith<UnsupportedOperationException> { dslMapKeysAsJavaUtilSet.remove(1) }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapKeysIteratorIsNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapKeysAsJavaUtilSet = dslMap.keys as java.util.Set<Int>
    val itr = dslMapKeysAsJavaUtilSet.iterator()
    itr.next()
    assertFailsWith<UnsupportedOperationException> { itr.remove() }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapValuesAreNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapValuesAsJavaUtilCollection = dslMap.values as java.util.Collection<Int>
    assertFailsWith<UnsupportedOperationException> { dslMapValuesAsJavaUtilCollection.remove(1) }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapValuesIteratorIsNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapValuesAsJavaUtilCollection = dslMap.values as java.util.Collection<Int>
    val itr = dslMapValuesAsJavaUtilCollection.iterator()
    itr.next()
    assertFailsWith<UnsupportedOperationException> { itr.remove() }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapEntriesAreNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapEntriesAsJavaUtilSet = dslMap.entries as java.util.Set<Map.Entry<Int, Int>>
    val entry = dslMap.entries.single()
    assertFailsWith<UnsupportedOperationException> { dslMapEntriesAsJavaUtilSet.remove(entry) }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapEntriesIteratorIsNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapEntriesAsJavaUtilSet = dslMap.entries as java.util.Set<Map.Entry<Int, Int>>
    val itr = dslMapEntriesAsJavaUtilSet.iterator()
    itr.next()
    assertFailsWith<UnsupportedOperationException> { itr.remove() }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslMapEntryObjectsAreNotEvenSecretlyMutable() {
    val dslMap = DslMap<Int, Int, DummyProxy>(mutableMapOf(1 to -1))
    val dslMapEntryAsJavaUtilMapEntry = dslMap.entries.single() as java.util.Map.Entry<Int, Int>
    assertFailsWith<UnsupportedOperationException> { dslMapEntryAsJavaUtilMapEntry.value = 2 }
  }

  @Test
  fun expectedToString() {
    assertThat(DslMap<Int, Int, DummyProxy>(mapOf(1 to 2, 2 to 3)).toString())
      .isEqualTo("{1=2, 2=3}")
  }

  @Test
  fun equality() {
    EqualsTester()
      .addEqualityGroup(DslMap<Int, Int, DummyProxy>(mapOf(1 to 2, 2 to 3)), mapOf(1 to 2, 2 to 3))
      .addEqualityGroup(DslMap<Int, Int, DummyProxy>(mapOf(1 to 3, 2 to 3)), mapOf(1 to 3, 2 to 3))
      .addEqualityGroup(
        DslMap<Int, Int, DummyProxy>(emptyMap()),
        DslMap<String, String, DummyProxy>(emptyMap()),
        emptyMap<Int, Int>(),
      )
      .testEquals()
  }
}
