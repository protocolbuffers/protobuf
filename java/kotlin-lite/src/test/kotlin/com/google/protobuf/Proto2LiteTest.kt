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
import com.google.protobuf.TestAllTypesLiteKt
import com.google.protobuf.TestAllTypesLiteKt.nestedMessage
import com.google.protobuf.TestUtilLite
import com.google.protobuf.TestUtilLite.toBytes
import com.google.protobuf.UnittestImportLite.ImportEnumLite
import com.google.protobuf.UnittestImportLite.ImportMessageLite
import com.google.protobuf.UnittestImportPublicLite.PublicImportMessageLite
import com.google.protobuf.UnittestLite
import com.google.protobuf.UnittestLite.ForeignEnumLite
import com.google.protobuf.UnittestLite.TestAllTypesLite
import com.google.protobuf.UnittestLite.TestAllTypesLite.NestedEnum
import com.google.protobuf.UnittestLite.TestEmptyMessageLite
import com.google.protobuf.UnittestLite.TestEmptyMessageWithExtensionsLite
import com.google.protobuf.copy
import com.google.protobuf.foreignMessageLite
import com.google.protobuf.kotlin.generator.EvilNamesProto2OuterClass.EvilNamesProto2
import com.google.protobuf.kotlin.generator.EvilNamesProto2OuterClass.HardKeywordsAllTypesProto2
import com.google.protobuf.kotlin.generator.EvilNamesProto2OuterClass.Interface
import com.google.protobuf.kotlin.generator.HardKeywordsAllTypesProto2Kt
import com.google.protobuf.kotlin.generator.evilNamesProto2
import com.google.protobuf.kotlin.generator.hardKeywordsAllTypesProto2
import com.google.protobuf.kotlin.generator.interface_
import com.google.protobuf.optionalGroupExtensionLite
import com.google.protobuf.repeatedGroupExtensionLite
import com.google.protobuf.testAllExtensionsLite
import com.google.protobuf.testAllTypesLite
import com.google.protobuf.testEmptyMessageLite
import com.google.protobuf.testEmptyMessageWithExtensionsLite
import protobuf_unittest.MapLiteUnittest.MapEnumLite
import protobuf_unittest.MapLiteUnittest.TestMapLite
import protobuf_unittest.testMapLite
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

@RunWith(JUnit4::class)
class Proto2LiteTest {
  @Test
  fun testSetters() {
    assertThat(
        testAllTypesLite {
          optionalInt32 = 101
          optionalInt64 = 102
          optionalUint32 = 103
          optionalUint64 = 104
          optionalSint32 = 105
          optionalSint64 = 106
          optionalFixed32 = 107
          optionalFixed64 = 108
          optionalSfixed32 = 109
          optionalSfixed64 = 110
          optionalFloat = 111.0f
          optionalDouble = 112.0
          optionalBool = true
          optionalString = "115"
          optionalBytes = toBytes("116")
          optionalGroup = TestAllTypesLiteKt.optionalGroup { a = 117 }
          optionalNestedMessage = nestedMessage { bb = 118 }
          optionalForeignMessage = foreignMessageLite { c = 119 }
          optionalImportMessage = ImportMessageLite.newBuilder().setD(120).build()
          optionalPublicImportMessage = PublicImportMessageLite.newBuilder().setE(126).build()
          optionalLazyMessage = nestedMessage { bb = 127 }
          optionalUnverifiedLazyMessage = nestedMessage { bb = 128 }
          optionalNestedEnum = NestedEnum.BAZ
          optionalForeignEnum = ForeignEnumLite.FOREIGN_LITE_BAZ
          optionalImportEnum = ImportEnumLite.IMPORT_LITE_BAZ
          optionalStringPiece = "124"
          optionalCord = "125"
          repeatedInt32.add(201)
          repeatedInt64.add(202)
          repeatedUint32.add(203)
          repeatedUint64.add(204)
          repeatedSint32.add(205)
          repeatedSint64.add(206)
          repeatedFixed32.add(207)
          repeatedFixed64.add(208)
          repeatedSfixed32.add(209)
          repeatedSfixed64.add(210)
          repeatedFloat.add(211f)
          repeatedDouble.add(212.0)
          repeatedBool.add(true)
          repeatedString.add("215")
          repeatedBytes.add(toBytes("216"))
          repeatedGroup.add(TestAllTypesLiteKt.repeatedGroup { a = 217 })
          repeatedNestedMessage.add(nestedMessage { bb = 218 })
          repeatedForeignMessage.add(foreignMessageLite { c = 219 })
          repeatedImportMessage.add(ImportMessageLite.newBuilder().setD(220).build())
          repeatedLazyMessage.add(nestedMessage { bb = 227 })
          repeatedNestedEnum.add(NestedEnum.BAR)
          repeatedForeignEnum.add(ForeignEnumLite.FOREIGN_LITE_BAR)
          repeatedImportEnum.add(ImportEnumLite.IMPORT_LITE_BAR)
          repeatedStringPiece.add("224")
          repeatedCord.add("225")
          repeatedInt32 += 301
          repeatedInt64 += 302
          repeatedUint32 += 303
          repeatedUint64 += 304
          repeatedSint32 += 305
          repeatedSint64 += 306
          repeatedFixed32 += 307
          repeatedFixed64 += 308
          repeatedSfixed32 += 309
          repeatedSfixed64 += 310
          repeatedFloat += 311f
          repeatedDouble += 312.0
          repeatedBool += false
          repeatedString += "315"
          repeatedBytes += toBytes("316")
          repeatedGroup += TestAllTypesLiteKt.repeatedGroup { a = 317 }
          repeatedNestedMessage += nestedMessage { bb = 318 }
          repeatedForeignMessage += foreignMessageLite { c = 319 }
          repeatedImportMessage += ImportMessageLite.newBuilder().setD(320).build()
          repeatedLazyMessage += TestAllTypesLiteKt.nestedMessage { bb = 327 }
          repeatedNestedEnum += NestedEnum.BAZ
          repeatedForeignEnum += ForeignEnumLite.FOREIGN_LITE_BAZ
          repeatedImportEnum += ImportEnumLite.IMPORT_LITE_BAZ
          repeatedStringPiece += "324"
          repeatedCord += "325"
          defaultInt32 = 401
          defaultInt64 = 402
          defaultUint32 = 403
          defaultUint64 = 404
          defaultSint32 = 405
          defaultSint64 = 406
          defaultFixed32 = 407
          defaultFixed64 = 408
          defaultSfixed32 = 409
          defaultSfixed64 = 410
          defaultFloat = 411f
          defaultDouble = 412.0
          defaultBool = false
          defaultString = "415"
          defaultBytes = toBytes("416")
          defaultNestedEnum = NestedEnum.FOO
          defaultForeignEnum = ForeignEnumLite.FOREIGN_LITE_FOO
          defaultImportEnum = ImportEnumLite.IMPORT_LITE_FOO
          defaultStringPiece = "424"
          defaultCord = "425"
          oneofUint32 = 601
          oneofNestedMessage = TestAllTypesLiteKt.nestedMessage { bb = 602 }
          oneofString = "603"
          oneofBytes = toBytes("604")
        }
      )
      .isEqualTo(TestUtilLite.getAllLiteSetBuilder().build())
  }

  @Test
  fun testGetters() {
    testAllTypesLite {
      optionalInt32 = 101
      assertThat(optionalInt32).isEqualTo(101)
      optionalString = "115"
      assertThat(optionalString).isEqualTo("115")
      optionalGroup = TestAllTypesLiteKt.optionalGroup { a = 117 }
      assertThat(optionalGroup).isEqualTo(TestAllTypesLiteKt.optionalGroup { a = 117 })
      optionalNestedMessage = TestAllTypesLiteKt.nestedMessage { bb = 118 }
      assertThat(optionalNestedMessage).isEqualTo(TestAllTypesLiteKt.nestedMessage { bb = 118 })
      optionalNestedEnum = NestedEnum.BAZ
      assertThat(optionalNestedEnum).isEqualTo(NestedEnum.BAZ)
      defaultInt32 = 401
      assertThat(defaultInt32).isEqualTo(401)
      oneofUint32 = 601
      assertThat(oneofUint32).isEqualTo(601)
    }
  }

  @Test
  fun testDefaultGetters() {
    testAllTypesLite {
      assertThat(defaultInt32).isEqualTo(41)
      assertThat(defaultString).isEqualTo("hello")
      assertThat(defaultNestedEnum).isEqualTo(NestedEnum.BAR)
      assertThat(defaultStringPiece).isEqualTo("abc")
    }
  }

  @Test
  fun testRepeatedGettersAndSetters() {
    testAllTypesLite {
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

      repeatedGroup.addAll(
        listOf(
          TestAllTypesLiteKt.repeatedGroup { a = 1 },
          TestAllTypesLiteKt.repeatedGroup { a = 2 }
        )
      )
      assertThat(repeatedGroup)
        .isEqualTo(
          listOf(
            TestAllTypesLiteKt.repeatedGroup { a = 1 },
            TestAllTypesLiteKt.repeatedGroup { a = 2 }
          )
        )
      repeatedGroup +=
        listOf(
          TestAllTypesLiteKt.repeatedGroup { a = 3 },
          TestAllTypesLiteKt.repeatedGroup { a = 4 }
        )
      assertThat(repeatedGroup)
        .isEqualTo(
          listOf(
            TestAllTypesLiteKt.repeatedGroup { a = 1 },
            TestAllTypesLiteKt.repeatedGroup { a = 2 },
            TestAllTypesLiteKt.repeatedGroup { a = 3 },
            TestAllTypesLiteKt.repeatedGroup { a = 4 }
          )
        )
      repeatedGroup[0] = TestAllTypesLiteKt.repeatedGroup { a = 5 }
      assertThat(repeatedGroup)
        .isEqualTo(
          listOf(
            TestAllTypesLiteKt.repeatedGroup { a = 5 },
            TestAllTypesLiteKt.repeatedGroup { a = 2 },
            TestAllTypesLiteKt.repeatedGroup { a = 3 },
            TestAllTypesLiteKt.repeatedGroup { a = 4 }
          )
        )

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
  fun testHazzers() {
    testAllTypesLite {
      optionalInt32 = 101
      assertThat(hasOptionalInt32()).isTrue()
      assertThat(hasOptionalString()).isFalse()
      optionalGroup = TestAllTypesLiteKt.optionalGroup { a = 117 }
      assertThat(hasOptionalGroup()).isTrue()
      assertThat(hasOptionalNestedMessage()).isFalse()
      optionalNestedEnum = NestedEnum.BAZ
      assertThat(hasOptionalNestedEnum()).isTrue()
      assertThat(hasDefaultInt32()).isFalse()
      oneofUint32 = 601
      assertThat(hasOneofUint32()).isTrue()
    }

    testAllTypesLite {
      assertThat(hasOptionalInt32()).isFalse()
      optionalString = "115"
      assertThat(hasOptionalString()).isTrue()
      assertThat(hasOptionalGroup()).isFalse()
      optionalNestedMessage = TestAllTypesLiteKt.nestedMessage { bb = 118 }
      assertThat(hasOptionalNestedMessage()).isTrue()
      assertThat(hasOptionalNestedEnum()).isFalse()
      defaultInt32 = 401
      assertThat(hasDefaultInt32()).isTrue()
      assertThat(hasOneofUint32()).isFalse()
    }
  }

  @Test
  fun testClears() {
    testAllTypesLite {
      optionalInt32 = 101
      clearOptionalInt32()
      assertThat(hasOptionalInt32()).isFalse()

      optionalString = "115"
      clearOptionalString()
      assertThat(hasOptionalString()).isFalse()

      optionalGroup = TestAllTypesLiteKt.optionalGroup { a = 117 }
      clearOptionalGroup()
      assertThat(hasOptionalGroup()).isFalse()

      optionalNestedMessage = TestAllTypesLiteKt.nestedMessage { bb = 118 }
      clearOptionalNestedMessage()
      assertThat(hasOptionalNestedMessage()).isFalse()

      optionalNestedEnum = NestedEnum.BAZ
      clearOptionalNestedEnum()
      assertThat(hasOptionalNestedEnum()).isFalse()

      defaultInt32 = 401
      clearDefaultInt32()
      assertThat(hasDefaultInt32()).isFalse()

      oneofUint32 = 601
      clearOneofUint32()
      assertThat(hasOneofUint32()).isFalse()
    }
  }

  @Test
  fun testCopy() {
    val message = testAllTypesLite {
      optionalInt32 = 101
      optionalString = "115"
    }
    val modifiedMessage = message.copy { optionalInt32 = 201 }

    assertThat(message)
      .isEqualTo(
        TestAllTypesLite.newBuilder().setOptionalInt32(101).setOptionalString("115").build()
      )
    assertThat(modifiedMessage)
      .isEqualTo(
        TestAllTypesLite.newBuilder().setOptionalInt32(201).setOptionalString("115").build()
      )
  }

  @Test
  fun testOneof() {
    val message = testAllTypesLite {
      oneofString = "foo"
      assertThat(oneofFieldCase).isEqualTo(TestAllTypesLite.OneofFieldCase.ONEOF_STRING)
      assertThat(oneofString).isEqualTo("foo")
      clearOneofField()
      assertThat(hasOneofUint32()).isFalse()
      assertThat(oneofFieldCase).isEqualTo(TestAllTypesLite.OneofFieldCase.ONEOFFIELD_NOT_SET)
      oneofUint32 = 5
    }

    assertThat(message.getOneofFieldCase()).isEqualTo(TestAllTypesLite.OneofFieldCase.ONEOF_UINT32)
    assertThat(message.getOneofUint32()).isEqualTo(5)
  }

  @Test
  fun testExtensionsSet() {
    assertThat(
        testAllExtensionsLite {
          this[UnittestLite.optionalInt32ExtensionLite] = 101
          this[UnittestLite.optionalInt64ExtensionLite] = 102L
          this[UnittestLite.optionalUint32ExtensionLite] = 103
          this[UnittestLite.optionalUint64ExtensionLite] = 104L
          this[UnittestLite.optionalSint32ExtensionLite] = 105
          this[UnittestLite.optionalSint64ExtensionLite] = 106L
          this[UnittestLite.optionalFixed32ExtensionLite] = 107
          this[UnittestLite.optionalFixed64ExtensionLite] = 108L
          this[UnittestLite.optionalSfixed32ExtensionLite] = 109
          this[UnittestLite.optionalSfixed64ExtensionLite] = 110L
          this[UnittestLite.optionalFloatExtensionLite] = 111F
          this[UnittestLite.optionalDoubleExtensionLite] = 112.0
          this[UnittestLite.optionalBoolExtensionLite] = true
          this[UnittestLite.optionalStringExtensionLite] = "115"
          this[UnittestLite.optionalBytesExtensionLite] = toBytes("116")
          this[UnittestLite.optionalGroupExtensionLite] = optionalGroupExtensionLite { a = 117 }
          this[UnittestLite.optionalNestedMessageExtensionLite] =
            TestAllTypesLiteKt.nestedMessage { bb = 118 }
          this[UnittestLite.optionalForeignMessageExtensionLite] = foreignMessageLite { c = 119 }
          this[UnittestLite.optionalImportMessageExtensionLite] =
            ImportMessageLite.newBuilder().setD(120).build()
          this[UnittestLite.optionalPublicImportMessageExtensionLite] =
            PublicImportMessageLite.newBuilder().setE(126).build()
          this[UnittestLite.optionalLazyMessageExtensionLite] =
            TestAllTypesLiteKt.nestedMessage { bb = 127 }
          this[UnittestLite.optionalUnverifiedLazyMessageExtensionLite] =
            TestAllTypesLiteKt.nestedMessage { bb = 128 }
          this[UnittestLite.optionalNestedEnumExtensionLite] = NestedEnum.BAZ
          this[UnittestLite.optionalForeignEnumExtensionLite] = ForeignEnumLite.FOREIGN_LITE_BAZ
          this[UnittestLite.optionalImportEnumExtensionLite] = ImportEnumLite.IMPORT_LITE_BAZ
          this[UnittestLite.optionalStringPieceExtensionLite] = "124"
          this[UnittestLite.optionalCordExtensionLite] = "125"
          this[UnittestLite.repeatedInt32ExtensionLite].add(201)
          this[UnittestLite.repeatedInt64ExtensionLite].add(202L)
          this[UnittestLite.repeatedUint32ExtensionLite].add(203)
          this[UnittestLite.repeatedUint64ExtensionLite].add(204L)
          this[UnittestLite.repeatedSint32ExtensionLite].add(205)
          this[UnittestLite.repeatedSint64ExtensionLite].add(206L)
          this[UnittestLite.repeatedFixed32ExtensionLite].add(207)
          this[UnittestLite.repeatedFixed64ExtensionLite].add(208L)
          this[UnittestLite.repeatedSfixed32ExtensionLite].add(209)
          this[UnittestLite.repeatedSfixed64ExtensionLite].add(210L)
          this[UnittestLite.repeatedFloatExtensionLite].add(211F)
          this[UnittestLite.repeatedDoubleExtensionLite].add(212.0)
          this[UnittestLite.repeatedBoolExtensionLite].add(true)
          this[UnittestLite.repeatedStringExtensionLite].add("215")
          this[UnittestLite.repeatedBytesExtensionLite].add(toBytes("216"))
          this[UnittestLite.repeatedGroupExtensionLite].add(repeatedGroupExtensionLite { a = 217 })
          this[UnittestLite.repeatedNestedMessageExtensionLite].add(
            TestAllTypesLiteKt.nestedMessage { bb = 218 }
          )
          this[UnittestLite.repeatedForeignMessageExtensionLite].add(foreignMessageLite { c = 219 })
          this[UnittestLite.repeatedImportMessageExtensionLite].add(
            ImportMessageLite.newBuilder().setD(220).build()
          )
          this[UnittestLite.repeatedLazyMessageExtensionLite].add(
            TestAllTypesLiteKt.nestedMessage { bb = 227 }
          )
          this[UnittestLite.repeatedNestedEnumExtensionLite].add(NestedEnum.BAR)
          this[UnittestLite.repeatedForeignEnumExtensionLite].add(ForeignEnumLite.FOREIGN_LITE_BAR)
          this[UnittestLite.repeatedImportEnumExtensionLite].add(ImportEnumLite.IMPORT_LITE_BAR)
          this[UnittestLite.repeatedStringPieceExtensionLite].add("224")
          this[UnittestLite.repeatedCordExtensionLite].add("225")
          this[UnittestLite.repeatedInt32ExtensionLite] += 301
          this[UnittestLite.repeatedInt64ExtensionLite] += 302L
          this[UnittestLite.repeatedUint32ExtensionLite] += 303
          this[UnittestLite.repeatedUint64ExtensionLite] += 304L
          this[UnittestLite.repeatedSint32ExtensionLite] += 305
          this[UnittestLite.repeatedSint64ExtensionLite] += 306L
          this[UnittestLite.repeatedFixed32ExtensionLite] += 307
          this[UnittestLite.repeatedFixed64ExtensionLite] += 308L
          this[UnittestLite.repeatedSfixed32ExtensionLite] += 309
          this[UnittestLite.repeatedSfixed64ExtensionLite] += 310L
          this[UnittestLite.repeatedFloatExtensionLite] += 311F
          this[UnittestLite.repeatedDoubleExtensionLite] += 312.0
          this[UnittestLite.repeatedBoolExtensionLite] += false
          this[UnittestLite.repeatedStringExtensionLite] += "315"
          this[UnittestLite.repeatedBytesExtensionLite] += toBytes("316")
          this[UnittestLite.repeatedGroupExtensionLite] += repeatedGroupExtensionLite { a = 317 }
          this[UnittestLite.repeatedNestedMessageExtensionLite] +=
            TestAllTypesLiteKt.nestedMessage { bb = 318 }
          this[UnittestLite.repeatedForeignMessageExtensionLite] += foreignMessageLite { c = 319 }
          this[UnittestLite.repeatedImportMessageExtensionLite] +=
            ImportMessageLite.newBuilder().setD(320).build()
          this[UnittestLite.repeatedLazyMessageExtensionLite] +=
            TestAllTypesLiteKt.nestedMessage { bb = 327 }
          this[UnittestLite.repeatedNestedEnumExtensionLite] += NestedEnum.BAZ
          this[UnittestLite.repeatedForeignEnumExtensionLite] += ForeignEnumLite.FOREIGN_LITE_BAZ
          this[UnittestLite.repeatedImportEnumExtensionLite] += ImportEnumLite.IMPORT_LITE_BAZ
          this[UnittestLite.repeatedStringPieceExtensionLite] += "324"
          this[UnittestLite.repeatedCordExtensionLite] += "325"
          this[UnittestLite.defaultInt32ExtensionLite] = 401
          this[UnittestLite.defaultInt64ExtensionLite] = 402L
          this[UnittestLite.defaultUint32ExtensionLite] = 403
          this[UnittestLite.defaultUint64ExtensionLite] = 404L
          this[UnittestLite.defaultSint32ExtensionLite] = 405
          this[UnittestLite.defaultSint64ExtensionLite] = 406L
          this[UnittestLite.defaultFixed32ExtensionLite] = 407
          this[UnittestLite.defaultFixed64ExtensionLite] = 408L
          this[UnittestLite.defaultSfixed32ExtensionLite] = 409
          this[UnittestLite.defaultSfixed64ExtensionLite] = 410L
          this[UnittestLite.defaultFloatExtensionLite] = 411F
          this[UnittestLite.defaultDoubleExtensionLite] = 412.0
          this[UnittestLite.defaultBoolExtensionLite] = false
          this[UnittestLite.defaultStringExtensionLite] = "415"
          this[UnittestLite.defaultBytesExtensionLite] = toBytes("416")
          this[UnittestLite.defaultNestedEnumExtensionLite] = NestedEnum.FOO
          this[UnittestLite.defaultForeignEnumExtensionLite] = ForeignEnumLite.FOREIGN_LITE_FOO
          this[UnittestLite.defaultImportEnumExtensionLite] = ImportEnumLite.IMPORT_LITE_FOO
          this[UnittestLite.defaultStringPieceExtensionLite] = "424"
          this[UnittestLite.defaultCordExtensionLite] = "425"
          this[UnittestLite.oneofUint32ExtensionLite] = 601
          this[UnittestLite.oneofNestedMessageExtensionLite] =
            TestAllTypesLiteKt.nestedMessage { bb = 602 }
          this[UnittestLite.oneofStringExtensionLite] = "603"
          this[UnittestLite.oneofBytesExtensionLite] = toBytes("604")
        }
      )
      .isEqualTo(TestUtilLite.getAllLiteExtensionsSet())
  }

  @Test
  fun testExtensionGetters() {
    testAllExtensionsLite {
      this[UnittestLite.optionalInt32ExtensionLite] = 101
      assertThat(this[UnittestLite.optionalInt32ExtensionLite]).isEqualTo(101)
      this[UnittestLite.optionalStringExtensionLite] = "115"
      assertThat(this[UnittestLite.optionalStringExtensionLite]).isEqualTo("115")
      this[UnittestLite.optionalGroupExtensionLite] = optionalGroupExtensionLite { a = 117 }
      assertThat(this[UnittestLite.optionalGroupExtensionLite])
        .isEqualTo(optionalGroupExtensionLite { a = 117 })
      this[UnittestLite.optionalNestedMessageExtensionLite] =
        TestAllTypesLiteKt.nestedMessage { bb = 118 }
      assertThat(this[UnittestLite.optionalNestedMessageExtensionLite])
        .isEqualTo(TestAllTypesLiteKt.nestedMessage { bb = 118 })
      this[UnittestLite.optionalNestedEnumExtensionLite] = NestedEnum.BAZ
      assertThat(this[UnittestLite.optionalNestedEnumExtensionLite]).isEqualTo(NestedEnum.BAZ)
      this[UnittestLite.defaultInt32ExtensionLite] = 401
      assertThat(this[UnittestLite.defaultInt32ExtensionLite]).isEqualTo(401)
      this[UnittestLite.oneofUint32ExtensionLite] = 601
      assertThat(this[UnittestLite.oneofUint32ExtensionLite]).isEqualTo(601)
    }
  }

  @Test
  fun testRepeatedExtensionGettersAndSetters() {
    testAllExtensionsLite {
      this[UnittestLite.repeatedInt32ExtensionLite].addAll(listOf(1, 2))
      assertThat(this[UnittestLite.repeatedInt32ExtensionLite]).isEqualTo(listOf(1, 2))
      this[UnittestLite.repeatedInt32ExtensionLite] += listOf(3, 4)
      assertThat(this[UnittestLite.repeatedInt32ExtensionLite]).isEqualTo(listOf(1, 2, 3, 4))
      this[UnittestLite.repeatedInt32ExtensionLite][0] = 5
      assertThat(this[UnittestLite.repeatedInt32ExtensionLite]).isEqualTo(listOf(5, 2, 3, 4))

      this[UnittestLite.repeatedStringExtensionLite].addAll(listOf("1", "2"))
      assertThat(this[UnittestLite.repeatedStringExtensionLite]).isEqualTo(listOf("1", "2"))
      this[UnittestLite.repeatedStringExtensionLite] += listOf("3", "4")
      assertThat(this[UnittestLite.repeatedStringExtensionLite])
        .isEqualTo(listOf("1", "2", "3", "4"))
      this[UnittestLite.repeatedStringExtensionLite][0] = "5"
      assertThat(this[UnittestLite.repeatedStringExtensionLite])
        .isEqualTo(listOf("5", "2", "3", "4"))

      this[UnittestLite.repeatedGroupExtensionLite].addAll(
        listOf(repeatedGroupExtensionLite { a = 1 }, repeatedGroupExtensionLite { a = 2 })
      )
      assertThat(this[UnittestLite.repeatedGroupExtensionLite])
        .isEqualTo(
          listOf(repeatedGroupExtensionLite { a = 1 }, repeatedGroupExtensionLite { a = 2 })
        )
      this[UnittestLite.repeatedGroupExtensionLite] +=
        listOf(repeatedGroupExtensionLite { a = 3 }, repeatedGroupExtensionLite { a = 4 })
      assertThat(this[UnittestLite.repeatedGroupExtensionLite])
        .isEqualTo(
          listOf(
            repeatedGroupExtensionLite { a = 1 },
            repeatedGroupExtensionLite { a = 2 },
            repeatedGroupExtensionLite { a = 3 },
            repeatedGroupExtensionLite { a = 4 }
          )
        )
      this[UnittestLite.repeatedGroupExtensionLite][0] = repeatedGroupExtensionLite { a = 5 }
      assertThat(this[UnittestLite.repeatedGroupExtensionLite])
        .isEqualTo(
          listOf(
            repeatedGroupExtensionLite { a = 5 },
            repeatedGroupExtensionLite { a = 2 },
            repeatedGroupExtensionLite { a = 3 },
            repeatedGroupExtensionLite { a = 4 }
          )
        )

      this[UnittestLite.repeatedNestedMessageExtensionLite].addAll(
        listOf(nestedMessage { bb = 1 }, nestedMessage { bb = 2 })
      )
      assertThat(this[UnittestLite.repeatedNestedMessageExtensionLite])
        .isEqualTo(listOf(nestedMessage { bb = 1 }, nestedMessage { bb = 2 }))
      this[UnittestLite.repeatedNestedMessageExtensionLite] +=
        listOf(nestedMessage { bb = 3 }, nestedMessage { bb = 4 })
      assertThat(this[UnittestLite.repeatedNestedMessageExtensionLite])
        .isEqualTo(
          listOf(
            nestedMessage { bb = 1 },
            nestedMessage { bb = 2 },
            nestedMessage { bb = 3 },
            nestedMessage { bb = 4 }
          )
        )
      this[UnittestLite.repeatedNestedMessageExtensionLite][0] = nestedMessage { bb = 5 }
      assertThat(this[UnittestLite.repeatedNestedMessageExtensionLite])
        .isEqualTo(
          listOf(
            nestedMessage { bb = 5 },
            nestedMessage { bb = 2 },
            nestedMessage { bb = 3 },
            nestedMessage { bb = 4 }
          )
        )

      this[UnittestLite.repeatedNestedEnumExtensionLite].addAll(
        listOf(NestedEnum.FOO, NestedEnum.BAR)
      )
      assertThat(this[UnittestLite.repeatedNestedEnumExtensionLite])
        .isEqualTo(listOf(NestedEnum.FOO, NestedEnum.BAR))
      this[UnittestLite.repeatedNestedEnumExtensionLite] += listOf(NestedEnum.BAZ, NestedEnum.FOO)
      assertThat(this[UnittestLite.repeatedNestedEnumExtensionLite])
        .isEqualTo(listOf(NestedEnum.FOO, NestedEnum.BAR, NestedEnum.BAZ, NestedEnum.FOO))
      this[UnittestLite.repeatedNestedEnumExtensionLite][0] = NestedEnum.BAR
      assertThat(this[UnittestLite.repeatedNestedEnumExtensionLite])
        .isEqualTo(listOf(NestedEnum.BAR, NestedEnum.BAR, NestedEnum.BAZ, NestedEnum.FOO))
    }
  }

  @Test
  fun testExtensionContains() {
    testAllExtensionsLite {
      this[UnittestLite.optionalInt32ExtensionLite] = 101
      assertThat(contains(UnittestLite.optionalInt32ExtensionLite)).isTrue()
      assertThat(contains(UnittestLite.optionalStringExtensionLite)).isFalse()
      this[UnittestLite.optionalGroupExtensionLite] = optionalGroupExtensionLite { a = 117 }
      assertThat(contains(UnittestLite.optionalGroupExtensionLite)).isTrue()
      assertThat(contains(UnittestLite.optionalNestedMessageExtensionLite)).isFalse()
      this[UnittestLite.optionalNestedEnumExtensionLite] = NestedEnum.BAZ
      assertThat(contains(UnittestLite.optionalNestedEnumExtensionLite)).isTrue()
      assertThat(contains(UnittestLite.defaultInt32ExtensionLite)).isFalse()
      this[UnittestLite.oneofUint32ExtensionLite] = 601
      assertThat(contains(UnittestLite.oneofUint32ExtensionLite)).isTrue()
    }

    testAllExtensionsLite {
      assertThat(contains(UnittestLite.optionalInt32ExtensionLite)).isFalse()
      this[UnittestLite.optionalStringExtensionLite] = "115"
      assertThat(contains(UnittestLite.optionalStringExtensionLite)).isTrue()
      assertThat(contains(UnittestLite.optionalGroupExtensionLite)).isFalse()
      this[UnittestLite.optionalNestedMessageExtensionLite] =
        TestAllTypesLiteKt.nestedMessage { bb = 118 }
      assertThat(contains(UnittestLite.optionalNestedMessageExtensionLite)).isTrue()
      assertThat(contains(UnittestLite.optionalNestedEnumExtensionLite)).isFalse()
      this[UnittestLite.defaultInt32ExtensionLite] = 401
      assertThat(contains(UnittestLite.defaultInt32ExtensionLite)).isTrue()
      assertThat(contains(UnittestLite.oneofUint32ExtensionLite)).isFalse()
    }
  }

  @Test
  fun testExtensionClears() {
    testAllExtensionsLite {
      this[UnittestLite.optionalInt32ExtensionLite] = 101
      clear(UnittestLite.optionalInt32ExtensionLite)
      assertThat(contains(UnittestLite.optionalInt32ExtensionLite)).isFalse()

      this[UnittestLite.optionalStringExtensionLite] = "115"
      clear(UnittestLite.optionalStringExtensionLite)
      assertThat(contains(UnittestLite.optionalStringExtensionLite)).isFalse()

      this[UnittestLite.optionalGroupExtensionLite] = optionalGroupExtensionLite { a = 117 }
      clear(UnittestLite.optionalGroupExtensionLite)
      assertThat(contains(UnittestLite.optionalGroupExtensionLite)).isFalse()

      this[UnittestLite.optionalNestedMessageExtensionLite] =
        TestAllTypesLiteKt.nestedMessage { bb = 118 }
      clear(UnittestLite.optionalNestedMessageExtensionLite)
      assertThat(contains(UnittestLite.optionalNestedMessageExtensionLite)).isFalse()

      this[UnittestLite.optionalNestedEnumExtensionLite] = NestedEnum.BAZ
      clear(UnittestLite.optionalNestedEnumExtensionLite)
      assertThat(contains(UnittestLite.optionalNestedEnumExtensionLite)).isFalse()

      this[UnittestLite.defaultInt32ExtensionLite] = 401
      clear(UnittestLite.defaultInt32ExtensionLite)
      assertThat(contains(UnittestLite.defaultInt32ExtensionLite)).isFalse()

      this[UnittestLite.oneofUint32ExtensionLite] = 601
      clear(UnittestLite.oneofUint32ExtensionLite)
      assertThat(contains(UnittestLite.oneofUint32ExtensionLite)).isFalse()
    }
  }

  @Test
  fun testEmptyMessages() {
    assertThat(testEmptyMessageLite {}).isEqualTo(TestEmptyMessageLite.newBuilder().build())

    assertThat(testEmptyMessageWithExtensionsLite {})
      .isEqualTo(TestEmptyMessageWithExtensionsLite.newBuilder().build())
  }

  @Test
  fun testMapSetters() {
    assertThat(
        testMapLite {
          mapInt32Int32[1] = 2
          mapInt64Int64[1L] = 2L
          mapUint32Uint32[1] = 2
          mapUint64Uint64[1L] = 2L
          mapSint32Sint32[1] = 2
          mapSint64Sint64[1L] = 2L
          mapFixed32Fixed32[1] = 2
          mapFixed64Fixed64[1L] = 2L
          mapSfixed32Sfixed32[1] = 2
          mapSfixed64Sfixed64[1L] = 2L
          mapInt32Float[1] = 2F
          mapInt32Double[1] = 2.0
          mapBoolBool[true] = true
          mapStringString["1"] = "2"
          mapInt32Bytes[1] = toBytes("2")
          mapInt32Enum[1] = MapEnumLite.MAP_ENUM_FOO_LITE
          mapInt32ForeignMessage[1] = foreignMessageLite { c = 1 }
        }
      )
      .isEqualTo(
        TestMapLite.newBuilder()
          .putMapInt32Int32(1, 2)
          .putMapInt64Int64(1L, 2L)
          .putMapUint32Uint32(1, 2)
          .putMapUint64Uint64(1L, 2L)
          .putMapSint32Sint32(1, 2)
          .putMapSint64Sint64(1L, 2L)
          .putMapFixed32Fixed32(1, 2)
          .putMapFixed64Fixed64(1L, 2L)
          .putMapSfixed32Sfixed32(1, 2)
          .putMapSfixed64Sfixed64(1L, 2L)
          .putMapInt32Float(1, 2F)
          .putMapInt32Double(1, 2.0)
          .putMapBoolBool(true, true)
          .putMapStringString("1", "2")
          .putMapInt32Bytes(1, toBytes("2"))
          .putMapInt32Enum(1, MapEnumLite.MAP_ENUM_FOO_LITE)
          .putMapInt32ForeignMessage(1, foreignMessageLite { c = 1 })
          .build()
      )
  }

  @Test
  fun testMapGettersAndSetters() {
    testMapLite {
      mapInt32Int32.put(1, 2)
      assertThat(mapInt32Int32).isEqualTo(mapOf(1 to 2))
      mapInt32Int32[3] = 4
      assertThat(mapInt32Int32).isEqualTo(mapOf(1 to 2, 3 to 4))
      mapInt32Int32.putAll(mapOf(5 to 6, 7 to 8))
      assertThat(mapInt32Int32).isEqualTo(mapOf(1 to 2, 3 to 4, 5 to 6, 7 to 8))

      mapStringString.put("1", "2")
      assertThat(mapStringString).isEqualTo(mapOf("1" to "2"))
      mapStringString["3"] = "4"
      assertThat(mapStringString).isEqualTo(mapOf("1" to "2", "3" to "4"))
      mapStringString.putAll(mapOf("5" to "6", "7" to "8"))
      assertThat(mapStringString).isEqualTo(mapOf("1" to "2", "3" to "4", "5" to "6", "7" to "8"))

      mapInt32Enum.put(1, MapEnumLite.MAP_ENUM_FOO_LITE)
      assertThat(mapInt32Enum).isEqualTo(mapOf(1 to MapEnumLite.MAP_ENUM_FOO_LITE))
      mapInt32Enum[2] = MapEnumLite.MAP_ENUM_BAR_LITE
      assertThat(mapInt32Enum)
        .isEqualTo(mapOf(1 to MapEnumLite.MAP_ENUM_FOO_LITE, 2 to MapEnumLite.MAP_ENUM_BAR_LITE))
      mapInt32Enum.putAll(
        mapOf(3 to MapEnumLite.MAP_ENUM_BAZ_LITE, 4 to MapEnumLite.MAP_ENUM_FOO_LITE)
      )
      assertThat(mapInt32Enum)
        .isEqualTo(
          mapOf(
            1 to MapEnumLite.MAP_ENUM_FOO_LITE,
            2 to MapEnumLite.MAP_ENUM_BAR_LITE,
            3 to MapEnumLite.MAP_ENUM_BAZ_LITE,
            4 to MapEnumLite.MAP_ENUM_FOO_LITE
          )
        )

      mapInt32ForeignMessage.put(1, foreignMessageLite { c = 1 })
      assertThat(mapInt32ForeignMessage).isEqualTo(mapOf(1 to foreignMessageLite { c = 1 }))
      mapInt32ForeignMessage[2] = foreignMessageLite { c = 2 }
      assertThat(mapInt32ForeignMessage)
        .isEqualTo(mapOf(1 to foreignMessageLite { c = 1 }, 2 to foreignMessageLite { c = 2 }))
      mapInt32ForeignMessage.putAll(
        mapOf(3 to foreignMessageLite { c = 3 }, 4 to foreignMessageLite { c = 4 })
      )
      assertThat(mapInt32ForeignMessage)
        .isEqualTo(
          mapOf(
            1 to foreignMessageLite { c = 1 },
            2 to foreignMessageLite { c = 2 },
            3 to foreignMessageLite { c = 3 },
            4 to foreignMessageLite { c = 4 }
          )
        )
    }
  }

  @Test
  fun testMapRemove() {
    testMapLite {
      mapInt32Int32.putAll(mapOf(1 to 2, 3 to 4))
      mapInt32Int32.remove(1)
      assertThat(mapInt32Int32).isEqualTo(mapOf(3 to 4))

      mapStringString.putAll(mapOf("1" to "2", "3" to "4"))
      mapStringString.remove("1")
      assertThat(mapStringString).isEqualTo(mapOf("3" to "4"))

      mapInt32Enum.putAll(
        mapOf(1 to MapEnumLite.MAP_ENUM_FOO_LITE, 2 to MapEnumLite.MAP_ENUM_BAR_LITE)
      )
      mapInt32Enum.remove(1)
      assertThat(mapInt32Enum).isEqualTo(mapOf(2 to MapEnumLite.MAP_ENUM_BAR_LITE))

      mapInt32ForeignMessage.putAll(
        mapOf(1 to foreignMessageLite { c = 1 }, 2 to foreignMessageLite { c = 2 })
      )
      mapInt32ForeignMessage.remove(1)
      assertThat(mapInt32ForeignMessage).isEqualTo(mapOf(2 to foreignMessageLite { c = 2 }))
    }
  }

  @Test
  fun testMapClear() {
    testMapLite {
      mapInt32Int32.putAll(mapOf(1 to 2, 3 to 4))
      mapInt32Int32.clear()
      assertThat(mapInt32Int32.isEmpty()).isTrue()

      mapStringString.putAll(mapOf("1" to "2", "3" to "4"))
      mapStringString.clear()
      assertThat(mapStringString.isEmpty()).isTrue()

      mapInt32Enum.putAll(
        mapOf(1 to MapEnumLite.MAP_ENUM_FOO_LITE, 2 to MapEnumLite.MAP_ENUM_BAR_LITE)
      )
      mapInt32Enum.clear()
      assertThat(mapInt32Enum.isEmpty()).isTrue()

      mapInt32ForeignMessage.putAll(
        mapOf(1 to foreignMessageLite { c = 1 }, 2 to foreignMessageLite { c = 2 })
      )
      mapInt32ForeignMessage.clear()
      assertThat(mapInt32ForeignMessage.isEmpty()).isTrue()
    }
  }

  @Test
  fun testEvilNames() {
    assertThat(
        evilNamesProto2 {
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
          class_ += 1
          int = 1.0
          long = true
          boolean = 1L
          sealed = "foo"
          interface_ = 1F
          in_ = 1
          object_ = "foo"
          cachedSize_ = "foo"
          serializedSize_ = true
          by = "foo"
        }
      )
      .isEqualTo(
        EvilNamesProto2.newBuilder()
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
          .addClass_(1)
          .setInt(1.0)
          .setLong(true)
          .setBoolean(1L)
          .setSealed("foo")
          .setInterface(1F)
          .setIn(1)
          .setObject("foo")
          .setCachedSize_("foo")
          .setSerializedSize_(true)
          .setBy("foo")
          .build()
      )

    assertThat(interface_ {}).isEqualTo(Interface.newBuilder().build())
  }

  @Test
  fun testHardKeywordGettersAndSetters() {
    hardKeywordsAllTypesProto2 {
      as_ = 1
      assertThat(as_).isEqualTo(1)

      in_ = "foo"
      assertThat(in_).isEqualTo("foo")

      break_ = HardKeywordsAllTypesProto2.NestedEnum.FOO
      assertThat(break_).isEqualTo(HardKeywordsAllTypesProto2.NestedEnum.FOO)

      do_ = HardKeywordsAllTypesProto2Kt.nestedMessage { while_ = 1 }
      assertThat(do_).isEqualTo(HardKeywordsAllTypesProto2Kt.nestedMessage { while_ = 1 })

      continue_[1] = 1
      assertThat(continue_[1]).isEqualTo(1)

      else_ += 1
      assertThat(else_).isEqualTo(listOf(1))

      for_ += "foo"
      assertThat(for_).isEqualTo(listOf("foo"))

      fun_ += HardKeywordsAllTypesProto2.NestedEnum.FOO
      assertThat(fun_).isEqualTo(listOf(HardKeywordsAllTypesProto2.NestedEnum.FOO))

      if_ += HardKeywordsAllTypesProto2Kt.nestedMessage { while_ = 1 }
      assertThat(if_).isEqualTo(listOf(HardKeywordsAllTypesProto2Kt.nestedMessage { while_ = 1 }))
    }
  }

  @Test
  fun testHardKeywordHazzers() {
    hardKeywordsAllTypesProto2 {
      as_ = 1
      assertThat(hasAs_()).isTrue()

      in_ = "foo"
      assertThat(hasIn_()).isTrue()

      break_ = HardKeywordsAllTypesProto2.NestedEnum.FOO
      assertThat(hasBreak_()).isTrue()

      do_ = HardKeywordsAllTypesProto2Kt.nestedMessage { while_ = 1 }
      assertThat(hasDo_()).isTrue()
    }
  }

  @Test
  fun testHardKeywordClears() {
    hardKeywordsAllTypesProto2 {
      as_ = 1
      clearAs_()
      assertThat(hasAs_()).isFalse()

      in_ = "foo"
      clearIn_()
      assertThat(hasIn_()).isFalse()

      break_ = HardKeywordsAllTypesProto2.NestedEnum.FOO
      clearBreak_()
      assertThat(hasBreak_()).isFalse()

      do_ = HardKeywordsAllTypesProto2Kt.nestedMessage { while_ = 1 }
      clearDo_()
      assertThat(hasDo_()).isFalse()
    }
  }
}
