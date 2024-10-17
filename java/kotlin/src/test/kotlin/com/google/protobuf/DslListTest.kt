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

/** Tests for [DslList]. */
@RunWith(JUnit4::class)
@OptIn(OnlyForUseByGeneratedProtoCode::class)
class DslListTest {
  class DummyProxy private constructor() : DslProxy()

  @Test
  fun matchesList() {
    assertThat(DslList<Int, DummyProxy>(listOf(1, 2, 3))).containsExactly(1, 2, 3).inOrder()
  }

  @Test
  fun reflectsChangesInList() {
    val mutableList = mutableListOf(1, 2, 3)
    val dslList = DslList<Int, DummyProxy>(mutableList)
    mutableList.add(4)
    assertThat(dslList).containsExactly(1, 2, 3, 4).inOrder()
  }

  @Test
  fun dslListIsNotMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    assertThat(dslList is MutableList<*>).isFalse()
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslListIsNotEvenSecretlyMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    val dslListAsJavaUtil = dslList as java.util.List<Int>
    assertFailsWith<UnsupportedOperationException> { dslListAsJavaUtil.add(4) }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslList_IteratorIsNotEvenSecretlyMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    val iterator = dslList.iterator() as java.util.Iterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> { iterator.remove() }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslList_ListIteratorIsNotEvenSecretlyMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    val iterator = dslList.listIterator() as java.util.ListIterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> { iterator.remove() }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslList_ListIteratorIndexIsNotEvenSecretlyMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    val iterator = dslList.listIterator(1) as java.util.ListIterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> { iterator.remove() }
  }

  @Test
  fun expectedToString() {
    assertThat(DslList<Int, DummyProxy>(listOf(1, 2)).toString()).isEqualTo("[1, 2]")
  }

  @Test
  fun equality() {
    EqualsTester()
      .addEqualityGroup(DslList<Int, DummyProxy>(listOf(1, 2)), listOf(1, 2))
      .addEqualityGroup(DslList<Int, DummyProxy>(listOf(2, 2)), listOf(2, 2))
      .addEqualityGroup(
        DslList<Int, DummyProxy>(emptyList()),
        DslList<String, DummyProxy>(emptyList()),
        emptyList<Int>(),
      )
      .testEquals()
  }
}
