// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.common.testing.EqualsTester
import com.google.common.truth.Truth.assertThat
import com.google.protobuf.kotlin.test.ExampleExtensibleMessage
import com.google.protobuf.kotlin.test.ExampleExtensibleMessageOuterClass as TestProto
import kotlin.test.assertFailsWith
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [DslList]. */
@RunWith(JUnit4::class)
@OptIn(OnlyForUseByGeneratedProtoCode::class)
class ExtensionListTest {
  class DummyProxy private constructor() : DslProxy()

  @Test
  fun matchesList() {
    assertThat(
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, listOf(1, 2, 3))
      )
      .containsExactly(1, 2, 3)
      .inOrder()
  }

  @Test
  fun reflectsChangesInList() {
    val mutableList = mutableListOf(1, 2, 3)
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, mutableList)
    mutableList.add(4)
    assertThat(extensionList).containsExactly(1, 2, 3, 4).inOrder()
  }

  @Test
  fun extensionListIsNotMutable() {
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(
        TestProto.repeatedExtension,
        mutableListOf(1, 2, 3),
      )
    assertThat((extensionList as Any) is MutableList<*>).isFalse()
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun extensionListIsNotEvenSecretlyMutable() {
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(
        TestProto.repeatedExtension,
        mutableListOf(1, 2, 3),
      )
    val extensionListAsJavaUtil = extensionList as java.util.List<Int>
    assertFailsWith<UnsupportedOperationException> { extensionListAsJavaUtil.add(4) }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun extensionList_IteratorIsNotEvenSecretlyMutable() {
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(
        TestProto.repeatedExtension,
        mutableListOf(1, 2, 3),
      )
    val iterator = extensionList.iterator() as java.util.Iterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> { iterator.remove() }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun extensionList_ListIteratorIsNotEvenSecretlyMutable() {
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(
        TestProto.repeatedExtension,
        mutableListOf(1, 2, 3),
      )
    val iterator = extensionList.listIterator() as java.util.ListIterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> { iterator.remove() }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun extensionList_ListIteratorIndexIsNotEvenSecretlyMutable() {
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(
        TestProto.repeatedExtension,
        mutableListOf(1, 2, 3),
      )
    val iterator = extensionList.listIterator(1) as java.util.ListIterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> { iterator.remove() }
  }

  @Test
  fun expectedToString() {
    assertThat(
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, listOf(1, 2))
          .toString()
      )
      .isEqualTo("[1, 2]")
  }

  @Test
  fun equality() {
    EqualsTester()
      .addEqualityGroup(
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, listOf(1, 2)),
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension) { listOf(1, 2) },
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.differentExtension, listOf(1, 2)),
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.differentExtension) { listOf(1, 2) },
        listOf(1, 2),
      )
      .addEqualityGroup(
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, listOf(2, 2)),
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension) { listOf(2, 2) },
        listOf(2, 2),
      )
      .addEqualityGroup(
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, emptyList()),
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension) { emptyList() },
        emptyList<Int>(),
      )
      .testEquals()
  }

  @Test
  fun supplierNotInvokedOnConstruction() {
    var supplierCalled = false
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension) {
        supplierCalled = true
        listOf(1, 2, 3)
      }
    assertThat(supplierCalled).isFalse()
    assertThat(extensionList).containsExactly(1, 2, 3).inOrder()
    assertThat(supplierCalled).isTrue()
  }

  @Test
  fun extensionPropertyAccessDoesNotInvokeSupplier() {
    var supplierCalled = false
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension) {
        supplierCalled = true
        listOf(1, 2, 3)
      }
    assertThat(extensionList.extension).isEqualTo(TestProto.repeatedExtension)
    assertThat(supplierCalled).isFalse()
  }

  @Test
  fun supplierEvaluatedOnlyOnFirstReadOperation() {
    var callCount = 0
    val extensionList =
      ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension) {
        callCount++
        listOf(1, 2, 1)
      }
    assertThat(callCount).isEqualTo(0)
    assertThat(extensionList.size).isEqualTo(3)
    assertThat(callCount).isEqualTo(1)
    assertThat(extensionList[0]).isEqualTo(1)
    assertThat(callCount).isEqualTo(1)
    assertThat(extensionList.isEmpty()).isFalse()
    assertThat(extensionList.contains(2)).isTrue()
    assertThat(extensionList.containsAll(listOf(1, 2))).isTrue()
    assertThat(extensionList.indexOf(1)).isEqualTo(0)
    assertThat(extensionList.lastIndexOf(1)).isEqualTo(2)
    assertThat(extensionList.subList(0, 2)).containsExactly(1, 2).inOrder()
    assertThat(extensionList.iterator().hasNext()).isTrue()
    assertThat(extensionList.listIterator().hasNext()).isTrue()
    assertThat(extensionList.listIterator(1).next()).isEqualTo(2)
    assertThat(callCount).isEqualTo(1)
  }
}
