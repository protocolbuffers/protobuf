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
    assertFailsWith<UnsupportedOperationException> {
      dslListAsJavaUtil.add(4)
    }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslList_IteratorIsNotEvenSecretlyMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    val iterator = dslList.iterator() as java.util.Iterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> {
      iterator.remove()
    }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslList_ListIteratorIsNotEvenSecretlyMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    val iterator = dslList.listIterator() as java.util.ListIterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> {
      iterator.remove()
    }
  }

  @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN", "UNCHECKED_CAST")
  @Test
  fun dslList_ListIteratorIndexIsNotEvenSecretlyMutable() {
    val dslList = DslList<Int, DummyProxy>(mutableListOf(1, 2, 3))
    val iterator = dslList.listIterator(1) as java.util.ListIterator<Int>
    iterator.next()

    assertFailsWith<UnsupportedOperationException> {
      iterator.remove()
    }
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
        emptyList<Int>()
      )
      .testEquals()
  }
}
