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

import com.google.common.truth.Truth.assertThat
import com.google.protobuf.ByteString
import java.lang.IndexOutOfBoundsException
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
    assertThat(str[0]).isEqualTo('a'.toByte())
    assertThat(str[2]).isEqualTo('c'.toByte())
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
    buffer.position(1)
    buffer.limit(2)
    assertThat(buffer.toByteString()).isEqualTo(ByteString.copyFromUtf8("b"))
  }
}
