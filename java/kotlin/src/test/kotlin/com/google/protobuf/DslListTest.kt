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
    assertThat((dslList as Any) is MutableList<*>).isFalse()
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
      .addEqualityGroup(
        DslList<Int, DummyProxy>(listOf(1, 2)),
        DslList<Int, DummyProxy> { listOf(1, 2) },
        listOf(1, 2),
      )
      .addEqualityGroup(
        DslList<Int, DummyProxy>(listOf(2, 2)),
        DslList<Int, DummyProxy> { listOf(2, 2) },
        listOf(2, 2),
      )
      .addEqualityGroup(
        DslList<Int, DummyProxy>(emptyList()),
        DslList<String, DummyProxy>(emptyList()),
        DslList<Int, DummyProxy> { emptyList() },
        DslList<String, DummyProxy> { emptyList() },
        emptyList<Int>(),
      )
      .testEquals()
  }

  @Test
  fun supplierNotInvokedOnConstruction() {
    var supplierCalled = false
    val dslList = DslList<Int, DummyProxy> {
      supplierCalled = true
      listOf(1, 2, 3)
    }
    assertThat(supplierCalled).isFalse()
    assertThat(dslList).containsExactly(1, 2, 3).inOrder()
    assertThat(supplierCalled).isTrue()
  }

  @Test
  fun supplierEvaluatedOnlyOnFirstReadOperation() {
    var callCount = 0
    val dslList = DslList<Int, DummyProxy> {
      callCount++
      listOf(1, 2, 1)
    }
    assertThat(callCount).isEqualTo(0)
    assertThat(dslList.size).isEqualTo(3)
    assertThat(callCount).isEqualTo(1)
    assertThat(dslList[0]).isEqualTo(1)
    assertThat(callCount).isEqualTo(1)
    assertThat(dslList.isEmpty()).isFalse()
    assertThat(dslList.contains(2)).isTrue()
    assertThat(dslList.containsAll(listOf(1, 2))).isTrue()
    assertThat(dslList.indexOf(1)).isEqualTo(0)
    assertThat(dslList.lastIndexOf(1)).isEqualTo(2)
    assertThat(dslList.subList(0, 2)).containsExactly(1, 2).inOrder()
    assertThat(dslList.iterator().hasNext()).isTrue()
    assertThat(dslList.listIterator().hasNext()).isTrue()
    assertThat(dslList.listIterator(1).next()).isEqualTo(2)
    assertThat(callCount).isEqualTo(1)
  }
}
