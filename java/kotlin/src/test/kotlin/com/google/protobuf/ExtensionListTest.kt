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
    assertThat(extensionList is MutableList<*>).isFalse()
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
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.differentExtension, listOf(1, 2)),
        listOf(1, 2),
      )
      .addEqualityGroup(
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, listOf(2, 2)),
        listOf(2, 2),
      )
      .addEqualityGroup(
        ExtensionList<Int, ExampleExtensibleMessage>(TestProto.repeatedExtension, emptyList()),
        emptyList<Int>(),
      )
      .testEquals()
  }
}
