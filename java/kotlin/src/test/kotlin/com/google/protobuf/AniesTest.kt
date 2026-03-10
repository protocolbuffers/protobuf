// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.common.truth.Truth.assertThat
import com.google.protobuf.Any as ProtoAny
import com.google.protobuf.InvalidProtocolBufferException
import proto2_unittest.UnittestProto.BoolMessage
import proto2_unittest.UnittestProto.Int32Message
import proto2_unittest.int32Message
import kotlin.test.assertFailsWith
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for extension methods on [ProtoAny]. */
@RunWith(JUnit4::class)
class AniesTest {
  companion object {
    val anAny = ProtoAny.pack(int32Message { data = 5 })
  }

  @Test
  fun isA_Positive() {
    assertThat(anAny.isA<Int32Message>()).isTrue()
  }

  @Test
  fun isA_Negative() {
    assertThat(anAny.isA<BoolMessage>()).isFalse()
  }

  @Test
  fun unpackValid() {
    assertThat(anAny.unpack<Int32Message>().data).isEqualTo(5)
  }

  @Test
  fun unpackInvalid() {
    assertFailsWith<InvalidProtocolBufferException> { anAny.unpack<BoolMessage>() }
  }
}
