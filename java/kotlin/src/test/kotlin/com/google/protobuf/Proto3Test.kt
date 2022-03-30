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
import com.google.protobuf.kotlin.generator.EvilNamesProto3OuterClass.Class
import com.google.protobuf.kotlin.generator.EvilNamesProto3OuterClass.EvilNamesProto3
import com.google.protobuf.kotlin.generator.EvilNamesProto3OuterClass.HardKeywordsAllTypesProto3
import com.google.protobuf.kotlin.generator.HardKeywordsAllTypesProto3Kt
import com.google.protobuf.kotlin.generator.class_
import com.google.protobuf.kotlin.generator.evilNamesProto3
import com.google.protobuf.kotlin.generator.hardKeywordsAllTypesProto3
import proto3_unittest.TestAllTypesKt
import proto3_unittest.TestAllTypesKt.nestedMessage
import proto3_unittest.UnittestProto3.TestAllTypes
import proto3_unittest.UnittestProto3.TestAllTypes.NestedEnum
import proto3_unittest.UnittestProto3.TestEmptyMessage
import proto3_unittest.copy
import proto3_unittest.optionalForeignMessageOrNull
import proto3_unittest.optionalNestedMessageOrNull
import proto3_unittest.testAllTypes
import proto3_unittest.testEmptyMessage
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

@RunWith(JUnit4::class)
class Proto3Test {
  @Test
  fun testGettersAndSetters() {
    testAllTypes {
      optionalInt32 = 101
      assertThat(optionalInt32).isEqualTo(101)
      optionalString = "115"
      assertThat(optionalString).isEqualTo("115")
      optionalNestedMessage = TestAllTypesKt.nestedMessage { bb = 118 }
      assertThat(optionalNestedMessage).isEqualTo(TestAllTypesKt.nestedMessage { bb = 118 })
      optionalNestedEnum = NestedEnum.BAZ
      assertThat(optionalNestedEnum).isEqualTo(NestedEnum.BAZ)
      oneofUint32 = 601
      assertThat(oneofUint32).isEqualTo(601)
    }
  }

  @Test
  fun testRepeatedGettersAndSetters() {
    testAllTypes {
      repeatedInt32.addAll(listOf(1, 2))
      assertThat(repeatedInt32).isEqualTo(listOf(1, 2))
      repeatedInt32 += listOf(3, 4)
      assertThat(repeatedInt32).isEqualTo(listOf(1, 2, 3, 4))
      repeatedInt32[0] = 5
      assertThat(repeatedInt32).isEqualTo(listOf(5, 2, 3, 4))

      repeatedString.addAll(listOf("1", "2"))
      assertThat(repeatedString).isEqualTo(listOf("1", "2"))
      repeatedString += listOf("3", "4")
      assertThat(repeatedString).isEqualTo(listOf("1", "2", "3", "4"))
      repeatedString[0] = "5"
      assertThat(repeatedString).isEqualTo(listOf("5", "2", "3", "4"))

      repeatedNestedMessage.addAll(listOf(nestedMessage { bb = 1 }, nestedMessage { bb = 2 }))
      assertThat(repeatedNestedMessage)
        .isEqualTo(listOf(nestedMessage { bb = 1 }, nestedMessage { bb = 2 }))
      repeatedNestedMessage += listOf(nestedMessage { bb = 3 }, nestedMessage { bb = 4 })
      assertThat(repeatedNestedMessage)
        .isEqualTo(
          listOf(
            nestedMessage { bb = 1 },
            nestedMessage { bb = 2 },
            nestedMessage { bb = 3 },
            nestedMessage { bb = 4 }
          )
        )
      repeatedNestedMessage[0] = nestedMessage { bb = 5 }
      assertThat(repeatedNestedMessage)
        .isEqualTo(
          listOf(
            nestedMessage { bb = 5 },
            nestedMessage { bb = 2 },
            nestedMessage { bb = 3 },
            nestedMessage { bb = 4 }
          )
        )

      repeatedNestedEnum.addAll(listOf(NestedEnum.FOO, NestedEnum.BAR))
      assertThat(repeatedNestedEnum).isEqualTo(listOf(NestedEnum.FOO, NestedEnum.BAR))
      repeatedNestedEnum += listOf(NestedEnum.BAZ, NestedEnum.FOO)
      assertThat(repeatedNestedEnum)
        .isEqualTo(listOf(NestedEnum.FOO, NestedEnum.BAR, NestedEnum.BAZ, NestedEnum.FOO))
      repeatedNestedEnum[0] = NestedEnum.BAR
      assertThat(repeatedNestedEnum)
        .isEqualTo(listOf(NestedEnum.BAR, NestedEnum.BAR, NestedEnum.BAZ, NestedEnum.FOO))
    }
  }

  @Test
  fun testClears() {
    assertThat(
        testAllTypes {
          optionalInt32 = 101
          clearOptionalInt32()

          optionalString = "115"
          clearOptionalString()

          optionalNestedMessage = TestAllTypesKt.nestedMessage { bb = 118 }
          clearOptionalNestedMessage()

          optionalNestedEnum = NestedEnum.BAZ
          clearOptionalNestedEnum()

          oneofUint32 = 601
          clearOneofUint32()
        }
      )
      .isEqualTo(TestAllTypes.newBuilder().build())
  }

  @Test
  fun testCopy() {
    val message = testAllTypes {
      optionalInt32 = 101
      optionalString = "115"
    }
    val modifiedMessage = message.copy { optionalInt32 = 201 }

    assertThat(message)
      .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(101).setOptionalString("115").build())
    assertThat(modifiedMessage)
      .isEqualTo(TestAllTypes.newBuilder().setOptionalInt32(201).setOptionalString("115").build())
  }

  @Test
  fun testOneof() {
    val message = testAllTypes {
      oneofString = "foo"
      assertThat(oneofFieldCase).isEqualTo(TestAllTypes.OneofFieldCase.ONEOF_STRING)
      assertThat(oneofString).isEqualTo("foo")
      clearOneofField()
      assertThat(oneofFieldCase).isEqualTo(TestAllTypes.OneofFieldCase.ONEOFFIELD_NOT_SET)
      oneofUint32 = 5
    }

    assertThat(message.getOneofFieldCase()).isEqualTo(TestAllTypes.OneofFieldCase.ONEOF_UINT32)
    assertThat(message.getOneofUint32()).isEqualTo(5)
  }

  @Test
  fun testEmptyMessages() {
    assertThat(testEmptyMessage {}).isEqualTo(TestEmptyMessage.newBuilder().build())
  }

  @Test
  fun testEvilNames() {
    assertThat(
        evilNamesProto3 {
          initialized = true
          hasFoo = true
          bar = "foo"
          isInitialized = true
          fooBar = "foo"
          aLLCAPS += "foo"
          aLLCAPSMAP[1] = true
          hasUnderbarPrecedingNumeric1Foo = true
          hasUnderbarPrecedingNumeric42Bar = true
          hasUnderbarPrecedingNumeric123Foo42BarBaz = true
          extension += "foo"
          class_ = "foo"
          int = 1.0
          long = true
          boolean = 1L
          sealed = "foo"
          interface_ = 1F
          in_ = 1
          object_ = "foo"
          cachedSize_ = "foo"
          serializedSize_ = true
          value = "foo"
          index = 1L
          values += "foo"
          newValues += "foo"
          builder = true
          k[1] = 1
          v["foo"] = "foo"
          key["foo"] = 1
          map[1] = "foo"
          pairs["foo"] = 1
          LeadingUnderscore = "foo"
          option = 1
        }
      )
      .isEqualTo(
        EvilNamesProto3.newBuilder()
          .setInitialized(true)
          .setHasFoo(true)
          .setBar("foo")
          .setIsInitialized(true)
          .setFooBar("foo")
          .addALLCAPS("foo")
          .putALLCAPSMAP(1, true)
          .setHasUnderbarPrecedingNumeric1Foo(true)
          .setHasUnderbarPrecedingNumeric42Bar(true)
          .setHasUnderbarPrecedingNumeric123Foo42BarBaz(true)
          .addExtension("foo")
          .setClass_("foo")
          .setInt(1.0)
          .setLong(true)
          .setBoolean(1L)
          .setSealed("foo")
          .setInterface(1F)
          .setIn(1)
          .setObject("foo")
          .setCachedSize_("foo")
          .setSerializedSize_(true)
          .setValue("foo")
          .setIndex(1L)
          .addValues("foo")
          .addNewValues("foo")
          .setBuilder(true)
          .putK(1, 1)
          .putV("foo", "foo")
          .putKey("foo", 1)
          .putMap(1, "foo")
          .putPairs("foo", 1)
          .setLeadingUnderscore("foo")
          .setOption(1)
          .build()
      )

    assertThat(class_ {}).isEqualTo(Class.newBuilder().build())
  }

  @Test
  fun testHardKeywordGettersAndSetters() {
    hardKeywordsAllTypesProto3 {
      as_ = 1
      assertThat(as_).isEqualTo(1)

      in_ = "foo"
      assertThat(in_).isEqualTo("foo")

      break_ = HardKeywordsAllTypesProto3.NestedEnum.FOO
      assertThat(break_).isEqualTo(HardKeywordsAllTypesProto3.NestedEnum.FOO)

      do_ = HardKeywordsAllTypesProto3Kt.nestedMessage { while_ = 1 }
      assertThat(do_).isEqualTo(HardKeywordsAllTypesProto3Kt.nestedMessage { while_ = 1 })

      continue_[1] = 1
      assertThat(continue_[1]).isEqualTo(1)

      else_ += 1
      assertThat(else_).isEqualTo(listOf(1))

      for_ += "foo"
      assertThat(for_).isEqualTo(listOf("foo"))

      fun_ += HardKeywordsAllTypesProto3.NestedEnum.FOO
      assertThat(fun_).isEqualTo(listOf(HardKeywordsAllTypesProto3.NestedEnum.FOO))

      if_ += HardKeywordsAllTypesProto3Kt.nestedMessage { while_ = 1 }
      assertThat(if_).isEqualTo(listOf(HardKeywordsAllTypesProto3Kt.nestedMessage { while_ = 1 }))
    }
  }

  @Test
  fun testHardKeywordHazzers() {
    hardKeywordsAllTypesProto3 {
      as_ = 1
      assertThat(hasAs_()).isTrue()

      in_ = "foo"
      assertThat(hasIn_()).isTrue()

      break_ = HardKeywordsAllTypesProto3.NestedEnum.FOO
      assertThat(hasBreak_()).isTrue()

      do_ = HardKeywordsAllTypesProto3Kt.nestedMessage { while_ = 1 }
      assertThat(hasDo_()).isTrue()
    }
  }

  @Test
  fun testHardKeywordClears() {
    hardKeywordsAllTypesProto3 {
      as_ = 1
      clearAs_()
      assertThat(hasAs_()).isFalse()

      in_ = "foo"
      clearIn_()
      assertThat(hasIn_()).isFalse()

      break_ = HardKeywordsAllTypesProto3.NestedEnum.FOO
      clearBreak_()
      assertThat(hasBreak_()).isFalse()

      do_ = HardKeywordsAllTypesProto3Kt.nestedMessage { while_ = 1 }
      clearDo_()
      assertThat(hasDo_()).isFalse()
    }
  }

  @Test
  fun testMultipleFiles() {
    assertThat(com.google.protobuf.kotlin.generator.multipleFilesMessageA {})
      .isEqualTo(com.google.protobuf.kotlin.generator.MultipleFilesMessageA.newBuilder().build())

    assertThat(com.google.protobuf.kotlin.generator.multipleFilesMessageB {})
      .isEqualTo(com.google.protobuf.kotlin.generator.MultipleFilesMessageB.newBuilder().build())
  }

  @Test
  fun testGetOrNull() {
    val noNestedMessage = testAllTypes {}
    assertThat(noNestedMessage.optionalNestedMessageOrNull).isEqualTo(null)

    val someNestedMessage = testAllTypes {
      optionalNestedMessage = TestAllTypesKt.nestedMessage { bb = 118 }
    }
    assertThat(someNestedMessage.optionalNestedMessageOrNull)
      .isEqualTo(TestAllTypesKt.nestedMessage { bb = 118 })

    // No optional keyword, OrNull should still be generated
    assertThat(someNestedMessage.optionalForeignMessageOrNull).isEqualTo(null)
  }
}
