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
import com.google.protobuf.kotlin.test.ExampleExtensibleMessage
import com.google.protobuf.kotlin.test.ExampleExtensibleMessageOuterClass as TestProto
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

@RunWith(JUnit4::class)
class ExtendableMessageExtensionsTest {
  @Test
  fun setOnBuilder() {
    val builder = ExampleExtensibleMessage.newBuilder()
    builder[TestProto.int32Extension] = 5
    assertThat(builder.build().getExtension(TestProto.int32Extension)).isEqualTo(5)
  }

  @Test
  fun getOnBuilder() {
    val builder = ExampleExtensibleMessage.newBuilder().setExtension(TestProto.int32Extension, 6)
    assertThat(builder[TestProto.int32Extension]).isEqualTo(6)
  }

  @Test
  fun getOnMessage() {
    val message =
      ExampleExtensibleMessage.newBuilder().setExtension(TestProto.int32Extension, 6).build()
    assertThat(message[TestProto.int32Extension]).isEqualTo(6)
  }

  @Test
  fun containsPositiveOnMessage() {
    val message =
      ExampleExtensibleMessage.newBuilder().setExtension(TestProto.int32Extension, 6).build()
    assertThat(TestProto.int32Extension in message).isTrue()
  }

  @Test
  fun containsPositiveOnBuilder() {
    val builder = ExampleExtensibleMessage.newBuilder().setExtension(TestProto.int32Extension, 6)
    assertThat(TestProto.int32Extension in builder).isTrue()
  }

  @Test
  fun containsNegativeOnMessage() {
    val message = ExampleExtensibleMessage.newBuilder().build()
    assertThat(TestProto.int32Extension in message).isFalse()
  }

  @Test
  fun containsNegativeOnBuilder() {
    val builder = ExampleExtensibleMessage.newBuilder()
    assertThat(TestProto.int32Extension in builder).isFalse()
  }
}
