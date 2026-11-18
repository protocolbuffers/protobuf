// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.common.truth.Truth.assertThat
import com.google.protobuf.ByteString
import java.lang.IndexOutOfBoundsException
import java.nio.Buffer
import java.nio.ByteBuffer
import kotlin.test.assertFailsWith
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for the extension functions in ByteStrings.kt. */
@RunWith(JUnit4::class)
class ByteStringsTest {
  @Test
  fun toByteStringUtf8() {
    assertThat("abc".toByteStringUtf8())
      .isEqualTo(ByteString.copyFrom("abc".toByteArray(Charsets.UTF_8)))
  }

  @Test
  fun plus() {
    assertThat("abc".toByteStringUtf8() + "def".toByteStringUtf8())
      .isEqualTo(ByteString.copyFrom("abcdef".toByteArray(Charsets.UTF_8)))
  }

  @Test
  fun byteAt() {
    val str = "abc".toByteStringUtf8()
    assertThat(str[0]).isEqualTo('a'.code)
    assertThat(str[2]).isEqualTo('c'.code)
  }

  @Test
  fun isNotEmpty_returnsTrue_whenNotEmpty() {
    assertThat("abc".toByteStringUtf8().isNotEmpty()).isTrue()
  }

  @Test
  fun isNotEmpty_returnsFalse_whenEmpty() {
    assertThat(ByteString.EMPTY.isNotEmpty()).isFalse()
  }

  @Test
  fun byteAtBelowZero() {
    val str = "abc".toByteStringUtf8()
    assertFailsWith<IndexOutOfBoundsException> { str[-1] }
    assertFailsWith<IndexOutOfBoundsException> { str[-2] }
  }

  @Test
  fun byteAtAboveLength() {
    val str = "abc".toByteStringUtf8()
    assertFailsWith<IndexOutOfBoundsException> { str[3] }
    assertFailsWith<IndexOutOfBoundsException> { str[4] }
  }

  @Test
  fun byteArrayToByteString() {
    assertThat("abc".toByteArray(Charsets.UTF_8).toByteString())
      .isEqualTo(ByteString.copyFromUtf8("abc"))
  }

  @Test
  fun byteBufferToByteString() {
    val buffer = ByteBuffer.wrap("abc".toByteArray(Charsets.UTF_8))
    assertThat(buffer.toByteString()).isEqualTo(ByteString.copyFromUtf8("abc"))
  }

  @Test
  fun byteBufferToByteStringRespectsPositionAndLimit() {
    val buffer = ByteBuffer.wrap("abc".toByteArray(Charsets.UTF_8))
    (buffer as java.nio.Buffer).position(1)
    (buffer as java.nio.Buffer).limit(2)
    assertThat(buffer.toByteString()).isEqualTo(ByteString.copyFromUtf8("b"))
  }
}
