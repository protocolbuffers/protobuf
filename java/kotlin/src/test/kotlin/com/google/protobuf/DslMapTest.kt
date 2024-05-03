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
        emptyMap<Int, Int>()
      )
      .testEquals()
  }
}
