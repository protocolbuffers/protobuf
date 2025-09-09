// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.common.truth.Truth.assertThat
import com.google.protobuf.kotlin.generator.`in`.EvilNamesProto3OuterClass.Class
import com.google.protobuf.kotlin.generator.`in`.EvilNamesProto3OuterClass.EvilNamesProto3
import com.google.protobuf.kotlin.generator.`in`.EvilNamesProto3OuterClass.HardKeywordsAllTypesProto3
import com.google.protobuf.kotlin.generator.`in`.HardKeywordsAllTypesProto3Kt
import com.google.protobuf.kotlin.generator.`in`.class_
import com.google.protobuf.kotlin.generator.`in`.evilNamesProto3
import com.google.protobuf.kotlin.generator.`in`.hardKeywordsAllTypesProto3
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
  @Suppress("CheckReturnValue")
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
      assertThat(optionalNestedEnumValue).isEqualTo(3)
      optionalNestedEnumValue = 1
      assertThat(optionalNestedEnumValue).isEqualTo(1)
      assertThat(optionalNestedEnum).isEqualTo(NestedEnum.FOO)

      oneofUint32 = 601
      assertThat(oneofUint32).isEqualTo(601)
    }
  }

  @Suppress("CheckReturnValue")
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
            nestedMessage { bb = 4 },
          )
        )
      repeatedNestedMessage[0] = nestedMessage { bb = 5 }
      assertThat(repeatedNestedMessage)
        .isEqualTo(
          listOf(
            nestedMessage { bb = 5 },
            nestedMessage { bb = 2 },
            nestedMessage { bb = 3 },
            nestedMessage { bb = 4 },
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
          dEPRECATEDFoo = "foo"
          iD = "foo"
          aBNotification = "foo"
          DEPRECATEDBar = "foo"
          notDEPRECATEDFoo = "foo"
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
          .setDEPRECATEDFoo("foo")
          .setID("foo")
          .setABNotification("foo")
          .setDEPRECATEDBar("foo")
          .setNotDEPRECATEDFoo("foo")
          .build()
      )

    assertThat(class_ {}).isEqualTo(Class.newBuilder().build())
  }

  @Suppress("CheckReturnValue")
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

  @Suppress("CheckReturnValue")
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

  @Suppress("CheckReturnValue")
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
