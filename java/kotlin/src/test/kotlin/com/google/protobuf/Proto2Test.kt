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
import com.google.protobuf.TestUtil
import com.google.protobuf.TestUtil.toBytes
import com.google.protobuf.kotlin.generator.EvilNamesProto2OuterClass.EvilNamesProto2
import com.google.protobuf.kotlin.generator.EvilNamesProto2OuterClass.HardKeywordsAllTypesProto2
import com.google.protobuf.kotlin.generator.EvilNamesProto2OuterClass.Interface
import com.google.protobuf.kotlin.generator.HardKeywordsAllTypesProto2Kt
import com.google.protobuf.kotlin.generator.evilNamesProto2
import com.google.protobuf.kotlin.generator.hardKeywordsAllTypesProto2
import com.google.protobuf.kotlin.generator.interface_
import com.google.protobuf.test.UnittestImport.ImportEnum
import com.google.protobuf.test.UnittestImport.ImportMessage
import com.google.protobuf.test.UnittestImportPublic.PublicImportMessage
import protobuf_unittest.MapProto2Unittest.Proto2MapEnum
import protobuf_unittest.MapProto2Unittest.TestEnumMap
import protobuf_unittest.MapProto2Unittest.TestIntIntMap
import protobuf_unittest.MapProto2Unittest.TestMaps
import protobuf_unittest.TestAllTypesKt
import protobuf_unittest.TestAllTypesKt.nestedMessage
import protobuf_unittest.UnittestProto
import protobuf_unittest.UnittestProto.ForeignEnum
import protobuf_unittest.UnittestProto.TestAllTypes
import protobuf_unittest.UnittestProto.TestAllTypes.NestedEnum
import protobuf_unittest.UnittestProto.TestEmptyMessage
import protobuf_unittest.UnittestProto.TestEmptyMessageWithExtensions
import protobuf_unittest.copy
import protobuf_unittest.foreignMessage
import protobuf_unittest.optionalGroupExtension
import protobuf_unittest.repeatedGroupExtension
import protobuf_unittest.testAllExtensions
import protobuf_unittest.testAllTypes
import protobuf_unittest.testEmptyMessage
import protobuf_unittest.testEmptyMessageWithExtensions
import protobuf_unittest.testEnumMap
import protobuf_unittest.testIntIntMap
import protobuf_unittest.testMaps
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

@RunWith(JUnit4::class)
class Proto2Test {
  @Test
  fun testSetters() {
    assertThat(
      testAllTypes {
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
        optionalGroup =
          TestAllTypesKt.optionalGroup { a = 117 }
        optionalNestedMessage = nestedMessage { bb = 118 }
        optionalForeignMessage = foreignMessage { c = 119 }
        optionalImportMessage =
          ImportMessage.newBuilder().setD(120).build()
        optionalPublicImportMessage =
          PublicImportMessage.newBuilder().setE(126).build()
        optionalLazyMessage = nestedMessage { bb = 127 }
        optionalNestedEnum = NestedEnum.BAZ
        optionalForeignEnum = ForeignEnum.FOREIGN_BAZ
        optionalImportEnum = ImportEnum.IMPORT_BAZ
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
        repeatedGroup.add(TestAllTypesKt.repeatedGroup { a = 217 })
        repeatedNestedMessage.add(nestedMessage { bb = 218 })
        repeatedForeignMessage.add(foreignMessage { c = 219 })
        repeatedImportMessage.add(
          ImportMessage.newBuilder().setD(220).build()
        )
        repeatedLazyMessage.add(nestedMessage { bb = 227 })
        repeatedNestedEnum.add(NestedEnum.BAR)
        repeatedForeignEnum.add(ForeignEnum.FOREIGN_BAR)
        repeatedImportEnum.add(ImportEnum.IMPORT_BAR)
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
        repeatedGroup += TestAllTypesKt.repeatedGroup { a = 317 }
        repeatedNestedMessage += nestedMessage { bb = 318 }
        repeatedForeignMessage += foreignMessage { c = 319 }
        repeatedImportMessage +=
          ImportMessage.newBuilder().setD(320).build()
        repeatedLazyMessage +=
          TestAllTypesKt.nestedMessage { bb = 327 }
        repeatedNestedEnum += NestedEnum.BAZ
        repeatedForeignEnum += ForeignEnum.FOREIGN_BAZ
        repeatedImportEnum += ImportEnum.IMPORT_BAZ
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
        defaultForeignEnum = ForeignEnum.FOREIGN_FOO
        defaultImportEnum = ImportEnum.IMPORT_FOO
        defaultStringPiece = "424"
        defaultCord = "425"
        oneofUint32 = 601
        oneofNestedMessage =
          TestAllTypesKt.nestedMessage { bb = 602 }
        oneofString = "603"
        oneofBytes = toBytes("604")
      }
    ).isEqualTo(
      TestUtil.getAllSetBuilder().build()
    )
  }

  @Test
  fun testGetters() {
    testAllTypes {
      optionalInt32 = 101
      assertThat(optionalInt32).isEqualTo(101)
      optionalString = "115"
      assertThat(optionalString).isEqualTo("115")
      optionalGroup = TestAllTypesKt.optionalGroup { a = 117 }
      assertThat(optionalGroup).isEqualTo(TestAllTypesKt.optionalGroup { a = 117 })
      optionalNestedMessage = TestAllTypesKt.nestedMessage { bb = 118 }
      assertThat(optionalNestedMessage).isEqualTo(TestAllTypesKt.nestedMessage { bb = 118 })
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
    testAllTypes {
      assertThat(defaultInt32).isEqualTo(41)
      assertThat(defaultString).isEqualTo("hello")
      assertThat(defaultNestedEnum).isEqualTo(NestedEnum.BAR)
      assertThat(defaultStringPiece).isEqualTo("abc")
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

      repeatedGroup.addAll(
        listOf(
          TestAllTypesKt.repeatedGroup { a = 1 },
          TestAllTypesKt.repeatedGroup { a = 2 }
        )
      )
      assertThat(repeatedGroup).isEqualTo(
        listOf(
          TestAllTypesKt.repeatedGroup { a = 1 },
          TestAllTypesKt.repeatedGroup { a = 2 }
        )
      )
      repeatedGroup +=
        listOf(
          TestAllTypesKt.repeatedGroup { a = 3 },
          TestAllTypesKt.repeatedGroup { a = 4 }
        )
      assertThat(repeatedGroup).isEqualTo(
        listOf(
          TestAllTypesKt.repeatedGroup { a = 1 },
          TestAllTypesKt.repeatedGroup { a = 2 },
          TestAllTypesKt.repeatedGroup { a = 3 },
          TestAllTypesKt.repeatedGroup { a = 4 }
        )
      )
      repeatedGroup[0] = TestAllTypesKt.repeatedGroup { a = 5 }
      assertThat(repeatedGroup).isEqualTo(
        listOf(
          TestAllTypesKt.repeatedGroup { a = 5 },
          TestAllTypesKt.repeatedGroup { a = 2 },
          TestAllTypesKt.repeatedGroup { a = 3 },
          TestAllTypesKt.repeatedGroup { a = 4 }
        )
      )

      repeatedNestedMessage.addAll(listOf(nestedMessage { bb = 1 }, nestedMessage { bb = 2 }))
      assertThat(repeatedNestedMessage).isEqualTo(
        listOf(
          nestedMessage { bb = 1 },
          nestedMessage { bb = 2 }
        )
      )
      repeatedNestedMessage += listOf(nestedMessage { bb = 3 }, nestedMessage { bb = 4 })
      assertThat(repeatedNestedMessage).isEqualTo(
        listOf(
          nestedMessage { bb = 1 },
          nestedMessage { bb = 2 },
          nestedMessage { bb = 3 },
          nestedMessage { bb = 4 }
        )
      )
      repeatedNestedMessage[0] = nestedMessage { bb = 5 }
      assertThat(repeatedNestedMessage).isEqualTo(
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
      assertThat(repeatedNestedEnum).isEqualTo(
        listOf(NestedEnum.FOO, NestedEnum.BAR, NestedEnum.BAZ, NestedEnum.FOO)
      )
      repeatedNestedEnum[0] = NestedEnum.BAR
      assertThat(repeatedNestedEnum).isEqualTo(
        listOf(NestedEnum.BAR, NestedEnum.BAR, NestedEnum.BAZ, NestedEnum.FOO)
      )
    }
  }

  @Test
  fun testHazzers() {
    testAllTypes {
      optionalInt32 = 101
      assertThat(hasOptionalInt32()).isTrue()
      assertThat(hasOptionalString()).isFalse()
      optionalGroup = TestAllTypesKt.optionalGroup { a = 117 }
      assertThat(hasOptionalGroup()).isTrue()
      assertThat(hasOptionalNestedMessage()).isFalse()
      optionalNestedEnum = NestedEnum.BAZ
      assertThat(hasOptionalNestedEnum()).isTrue()
      assertThat(hasDefaultInt32()).isFalse()
      oneofUint32 = 601
      assertThat(hasOneofUint32()).isTrue()
    }

    testAllTypes {
      assertThat(hasOptionalInt32()).isFalse()
      optionalString = "115"
      assertThat(hasOptionalString()).isTrue()
      assertThat(hasOptionalGroup()).isFalse()
      optionalNestedMessage = TestAllTypesKt.nestedMessage { bb = 118 }
      assertThat(hasOptionalNestedMessage()).isTrue()
      assertThat(hasOptionalNestedEnum()).isFalse()
      defaultInt32 = 401
      assertThat(hasDefaultInt32()).isTrue()
      assertThat(hasOneofUint32()).isFalse()
    }
  }

  @Test
  fun testClears() {
    testAllTypes {
      optionalInt32 = 101
      clearOptionalInt32()
      assertThat(hasOptionalInt32()).isFalse()

      optionalString = "115"
      clearOptionalString()
      assertThat(hasOptionalString()).isFalse()

      optionalGroup = TestAllTypesKt.optionalGroup { a = 117 }
      clearOptionalGroup()
      assertThat(hasOptionalGroup()).isFalse()

      optionalNestedMessage = TestAllTypesKt.nestedMessage { bb = 118 }
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
    val message = testAllTypes {
      optionalInt32 = 101
      optionalString = "115"
    }
    val modifiedMessage = message.copy {
      optionalInt32 = 201
    }

    assertThat(message).isEqualTo(
      TestAllTypes.newBuilder()
        .setOptionalInt32(101)
        .setOptionalString("115")
        .build()
    )
    assertThat(modifiedMessage).isEqualTo(
      TestAllTypes.newBuilder()
        .setOptionalInt32(201)
        .setOptionalString("115")
        .build()
    )
  }

  @Test
  fun testOneof() {
    val message = testAllTypes {
      oneofString = "foo"
      assertThat(oneofFieldCase)
        .isEqualTo(TestAllTypes.OneofFieldCase.ONEOF_STRING)
      assertThat(oneofString).isEqualTo("foo")
      clearOneofField()
      assertThat(hasOneofUint32()).isFalse()
      assertThat(oneofFieldCase)
        .isEqualTo(TestAllTypes.OneofFieldCase.ONEOFFIELD_NOT_SET)
      oneofUint32 = 5
    }

    assertThat(message.getOneofFieldCase())
      .isEqualTo(TestAllTypes.OneofFieldCase.ONEOF_UINT32)
    assertThat(message.getOneofUint32()).isEqualTo(5)
  }

  @Test
  fun testExtensionsSet() {
    assertThat(
      testAllExtensions {
        this[UnittestProto.optionalInt32Extension] = 101
        this[UnittestProto.optionalInt64Extension] = 102L
        this[UnittestProto.optionalUint32Extension] = 103
        this[UnittestProto.optionalUint64Extension] = 104L
        this[UnittestProto.optionalSint32Extension] = 105
        this[UnittestProto.optionalSint64Extension] = 106L
        this[UnittestProto.optionalFixed32Extension] = 107
        this[UnittestProto.optionalFixed64Extension] = 108L
        this[UnittestProto.optionalSfixed32Extension] = 109
        this[UnittestProto.optionalSfixed64Extension] = 110L
        this[UnittestProto.optionalFloatExtension] = 111F
        this[UnittestProto.optionalDoubleExtension] = 112.0
        this[UnittestProto.optionalBoolExtension] = true
        this[UnittestProto.optionalStringExtension] = "115"
        this[UnittestProto.optionalBytesExtension] = toBytes("116")
        this[UnittestProto.optionalGroupExtension] = optionalGroupExtension { a = 117 }
        this[UnittestProto.optionalNestedMessageExtension] =
          TestAllTypesKt.nestedMessage { bb = 118 }
        this[UnittestProto.optionalForeignMessageExtension] = foreignMessage { c = 119 }
        this[UnittestProto.optionalImportMessageExtension] =
          ImportMessage.newBuilder().setD(120).build()
        this[UnittestProto.optionalPublicImportMessageExtension] =
          PublicImportMessage.newBuilder().setE(126).build()
        this[UnittestProto.optionalLazyMessageExtension] = TestAllTypesKt.nestedMessage { bb = 127 }
        this[UnittestProto.optionalNestedEnumExtension] = NestedEnum.BAZ
        this[UnittestProto.optionalForeignEnumExtension] = ForeignEnum.FOREIGN_BAZ
        this[UnittestProto.optionalImportEnumExtension] = ImportEnum.IMPORT_BAZ
        this[UnittestProto.optionalStringPieceExtension] = "124"
        this[UnittestProto.optionalCordExtension] = "125"
        this[UnittestProto.repeatedInt32Extension].add(201)
        this[UnittestProto.repeatedInt64Extension].add(202L)
        this[UnittestProto.repeatedUint32Extension].add(203)
        this[UnittestProto.repeatedUint64Extension].add(204L)
        this[UnittestProto.repeatedSint32Extension].add(205)
        this[UnittestProto.repeatedSint64Extension].add(206L)
        this[UnittestProto.repeatedFixed32Extension].add(207)
        this[UnittestProto.repeatedFixed64Extension].add(208L)
        this[UnittestProto.repeatedSfixed32Extension].add(209)
        this[UnittestProto.repeatedSfixed64Extension].add(210L)
        this[UnittestProto.repeatedFloatExtension].add(211F)
        this[UnittestProto.repeatedDoubleExtension].add(212.0)
        this[UnittestProto.repeatedBoolExtension].add(true)
        this[UnittestProto.repeatedStringExtension].add("215")
        this[UnittestProto.repeatedBytesExtension].add(toBytes("216"))
        this[UnittestProto.repeatedGroupExtension].add(repeatedGroupExtension { a = 217 })
        this[UnittestProto.repeatedNestedMessageExtension]
          .add(TestAllTypesKt.nestedMessage { bb = 218 })
        this[UnittestProto.repeatedForeignMessageExtension].add(foreignMessage { c = 219 })
        this[UnittestProto.repeatedImportMessageExtension]
          .add(ImportMessage.newBuilder().setD(220).build())
        this[UnittestProto.repeatedLazyMessageExtension]
          .add(TestAllTypesKt.nestedMessage { bb = 227 })
        this[UnittestProto.repeatedNestedEnumExtension].add(NestedEnum.BAR)
        this[UnittestProto.repeatedForeignEnumExtension].add(ForeignEnum.FOREIGN_BAR)
        this[UnittestProto.repeatedImportEnumExtension].add(ImportEnum.IMPORT_BAR)
        this[UnittestProto.repeatedStringPieceExtension].add("224")
        this[UnittestProto.repeatedCordExtension].add("225")
        this[UnittestProto.repeatedInt32Extension] += 301
        this[UnittestProto.repeatedInt64Extension] += 302L
        this[UnittestProto.repeatedUint32Extension] += 303
        this[UnittestProto.repeatedUint64Extension] += 304L
        this[UnittestProto.repeatedSint32Extension] += 305
        this[UnittestProto.repeatedSint64Extension] += 306L
        this[UnittestProto.repeatedFixed32Extension] += 307
        this[UnittestProto.repeatedFixed64Extension] += 308L
        this[UnittestProto.repeatedSfixed32Extension] += 309
        this[UnittestProto.repeatedSfixed64Extension] += 310L
        this[UnittestProto.repeatedFloatExtension] += 311F
        this[UnittestProto.repeatedDoubleExtension] += 312.0
        this[UnittestProto.repeatedBoolExtension] += false
        this[UnittestProto.repeatedStringExtension] += "315"
        this[UnittestProto.repeatedBytesExtension] += toBytes("316")
        this[UnittestProto.repeatedGroupExtension] += repeatedGroupExtension { a = 317 }
        this[UnittestProto.repeatedNestedMessageExtension] +=
	  TestAllTypesKt.nestedMessage { bb = 318 }
        this[UnittestProto.repeatedForeignMessageExtension] += foreignMessage { c = 319 }
        this[UnittestProto.repeatedImportMessageExtension] +=
          ImportMessage.newBuilder().setD(320).build()
        this[UnittestProto.repeatedLazyMessageExtension] +=
	  TestAllTypesKt.nestedMessage { bb = 327 }
        this[UnittestProto.repeatedNestedEnumExtension] += NestedEnum.BAZ
        this[UnittestProto.repeatedForeignEnumExtension] += ForeignEnum.FOREIGN_BAZ
        this[UnittestProto.repeatedImportEnumExtension] += ImportEnum.IMPORT_BAZ
        this[UnittestProto.repeatedStringPieceExtension] += "324"
        this[UnittestProto.repeatedCordExtension] += "325"
        this[UnittestProto.defaultInt32Extension] = 401
        this[UnittestProto.defaultInt64Extension] = 402L
        this[UnittestProto.defaultUint32Extension] = 403
        this[UnittestProto.defaultUint64Extension] = 404L
        this[UnittestProto.defaultSint32Extension] = 405
        this[UnittestProto.defaultSint64Extension] = 406L
        this[UnittestProto.defaultFixed32Extension] = 407
        this[UnittestProto.defaultFixed64Extension] = 408L
        this[UnittestProto.defaultSfixed32Extension] = 409
        this[UnittestProto.defaultSfixed64Extension] = 410L
        this[UnittestProto.defaultFloatExtension] = 411F
        this[UnittestProto.defaultDoubleExtension] = 412.0
        this[UnittestProto.defaultBoolExtension] = false
        this[UnittestProto.defaultStringExtension] = "415"
        this[UnittestProto.defaultBytesExtension] = toBytes("416")
        this[UnittestProto.defaultNestedEnumExtension] = NestedEnum.FOO
        this[UnittestProto.defaultForeignEnumExtension] = ForeignEnum.FOREIGN_FOO
        this[UnittestProto.defaultImportEnumExtension] = ImportEnum.IMPORT_FOO
        this[UnittestProto.defaultStringPieceExtension] = "424"
        this[UnittestProto.defaultCordExtension] = "425"
        this[UnittestProto.oneofUint32Extension] = 601
        this[UnittestProto.oneofNestedMessageExtension] = TestAllTypesKt.nestedMessage { bb = 602 }
        this[UnittestProto.oneofStringExtension] = "603"
        this[UnittestProto.oneofBytesExtension] = toBytes("604")
      }
    ).isEqualTo(
      TestUtil.getAllExtensionsSet()
    )
  }

  @Test
  fun testExtensionGetters() {
    testAllExtensions {
      this[UnittestProto.optionalInt32Extension] = 101
      assertThat(this[UnittestProto.optionalInt32Extension]).isEqualTo(101)
      this[UnittestProto.optionalStringExtension] = "115"
      assertThat(this[UnittestProto.optionalStringExtension]).isEqualTo("115")
      this[UnittestProto.optionalGroupExtension] = optionalGroupExtension { a = 117 }
      assertThat(this[UnittestProto.optionalGroupExtension])
        .isEqualTo(optionalGroupExtension { a = 117 })
      this[UnittestProto.optionalNestedMessageExtension] =
        TestAllTypesKt.nestedMessage { bb = 118 }
      assertThat(this[UnittestProto.optionalNestedMessageExtension])
        .isEqualTo(TestAllTypesKt.nestedMessage { bb = 118 })
      this[UnittestProto.optionalNestedEnumExtension] = NestedEnum.BAZ
      assertThat(this[UnittestProto.optionalNestedEnumExtension]).isEqualTo(NestedEnum.BAZ)
      this[UnittestProto.defaultInt32Extension] = 401
      assertThat(this[UnittestProto.defaultInt32Extension]).isEqualTo(401)
      this[UnittestProto.oneofUint32Extension] = 601
      assertThat(this[UnittestProto.oneofUint32Extension]).isEqualTo(601)
    }
  }

  @Test
  fun testRepeatedExtensionGettersAndSetters() {
    testAllExtensions {
      this[UnittestProto.repeatedInt32Extension].addAll(listOf(1, 2))
      assertThat(this[UnittestProto.repeatedInt32Extension]).isEqualTo(listOf(1, 2))
      this[UnittestProto.repeatedInt32Extension] += listOf(3, 4)
      assertThat(this[UnittestProto.repeatedInt32Extension]).isEqualTo(listOf(1, 2, 3, 4))
      this[UnittestProto.repeatedInt32Extension][0] = 5
      assertThat(this[UnittestProto.repeatedInt32Extension]).isEqualTo(listOf(5, 2, 3, 4))

      this[UnittestProto.repeatedStringExtension].addAll(listOf("1", "2"))
      assertThat(this[UnittestProto.repeatedStringExtension]).isEqualTo(listOf("1", "2"))
      this[UnittestProto.repeatedStringExtension] += listOf("3", "4")
      assertThat(this[UnittestProto.repeatedStringExtension]).isEqualTo(listOf("1", "2", "3", "4"))
      this[UnittestProto.repeatedStringExtension][0] = "5"
      assertThat(this[UnittestProto.repeatedStringExtension]).isEqualTo(listOf("5", "2", "3", "4"))

      this[UnittestProto.repeatedGroupExtension].addAll(
        listOf(
          repeatedGroupExtension { a = 1 },
          repeatedGroupExtension { a = 2 }
        )
      )
      assertThat(this[UnittestProto.repeatedGroupExtension]).isEqualTo(
        listOf(
          repeatedGroupExtension { a = 1 },
          repeatedGroupExtension { a = 2 }
        )
      )
      this[UnittestProto.repeatedGroupExtension] +=
        listOf(
          repeatedGroupExtension { a = 3 },
          repeatedGroupExtension { a = 4 }
        )
      assertThat(this[UnittestProto.repeatedGroupExtension]).isEqualTo(
        listOf(
          repeatedGroupExtension { a = 1 },
          repeatedGroupExtension { a = 2 },
          repeatedGroupExtension { a = 3 },
          repeatedGroupExtension { a = 4 }
        )
      )
      this[UnittestProto.repeatedGroupExtension][0] = repeatedGroupExtension { a = 5 }
      assertThat(this[UnittestProto.repeatedGroupExtension]).isEqualTo(
        listOf(
          repeatedGroupExtension { a = 5 },
          repeatedGroupExtension { a = 2 },
          repeatedGroupExtension { a = 3 },
          repeatedGroupExtension { a = 4 }
        )
      )

      this[UnittestProto.repeatedNestedMessageExtension].addAll(
        listOf(nestedMessage { bb = 1 }, nestedMessage { bb = 2 })
      )
      assertThat(this[UnittestProto.repeatedNestedMessageExtension]).isEqualTo(
        listOf(nestedMessage { bb = 1 }, nestedMessage { bb = 2 })
      )
      this[UnittestProto.repeatedNestedMessageExtension] +=
        listOf(nestedMessage { bb = 3 }, nestedMessage { bb = 4 })
      assertThat(this[UnittestProto.repeatedNestedMessageExtension]).isEqualTo(
        listOf(
          nestedMessage { bb = 1 },
          nestedMessage { bb = 2 },
          nestedMessage { bb = 3 },
          nestedMessage { bb = 4 }
        )
      )
      this[UnittestProto.repeatedNestedMessageExtension][0] = nestedMessage { bb = 5 }
      assertThat(this[UnittestProto.repeatedNestedMessageExtension]).isEqualTo(
        listOf(
          nestedMessage { bb = 5 },
          nestedMessage { bb = 2 },
          nestedMessage { bb = 3 },
          nestedMessage { bb = 4 }
        )
      )

      this[UnittestProto.repeatedNestedEnumExtension]
        .addAll(listOf(NestedEnum.FOO, NestedEnum.BAR))
      assertThat(this[UnittestProto.repeatedNestedEnumExtension])
        .isEqualTo(listOf(NestedEnum.FOO, NestedEnum.BAR))
      this[UnittestProto.repeatedNestedEnumExtension] += listOf(NestedEnum.BAZ, NestedEnum.FOO)
      assertThat(this[UnittestProto.repeatedNestedEnumExtension]).isEqualTo(
        listOf(NestedEnum.FOO, NestedEnum.BAR, NestedEnum.BAZ, NestedEnum.FOO)
      )
    }
  }

  @Test
  fun testExtensionContains() {
    testAllExtensions {
      this[UnittestProto.optionalInt32Extension] = 101
      assertThat(contains(UnittestProto.optionalInt32Extension)).isTrue()
      assertThat(contains(UnittestProto.optionalStringExtension)).isFalse()
      this[UnittestProto.optionalGroupExtension] = optionalGroupExtension { a = 117 }
      assertThat(contains(UnittestProto.optionalGroupExtension)).isTrue()
      assertThat(contains(UnittestProto.optionalNestedMessageExtension)).isFalse()
      this[UnittestProto.optionalNestedEnumExtension] = NestedEnum.BAZ
      assertThat(contains(UnittestProto.optionalNestedEnumExtension)).isTrue()
      assertThat(contains(UnittestProto.defaultInt32Extension)).isFalse()
      this[UnittestProto.oneofUint32Extension] = 601
      assertThat(contains(UnittestProto.optionalInt32Extension)).isTrue()
    }

    testAllExtensions {
      assertThat(contains(UnittestProto.optionalInt32Extension)).isFalse()
      this[UnittestProto.optionalStringExtension] = "115"
      assertThat(contains(UnittestProto.optionalStringExtension)).isTrue()
      assertThat(contains(UnittestProto.optionalGroupExtension)).isFalse()
      this[UnittestProto.optionalNestedMessageExtension] =
        TestAllTypesKt.nestedMessage { bb = 118 }
      assertThat(contains(UnittestProto.optionalNestedMessageExtension)).isTrue()
      assertThat(contains(UnittestProto.optionalNestedEnumExtension)).isFalse()
      this[UnittestProto.defaultInt32Extension] = 401
      assertThat(contains(UnittestProto.defaultInt32Extension)).isTrue()
      assertThat(contains(UnittestProto.oneofUint32Extension)).isFalse()
    }
  }

  @Test
  fun testExtensionClears() {
    testAllExtensions {
      this[UnittestProto.optionalInt32Extension] = 101
      clear(UnittestProto.optionalInt32Extension)
      assertThat(contains(UnittestProto.optionalInt32Extension)).isFalse()

      this[UnittestProto.optionalStringExtension] = "115"
      clear(UnittestProto.optionalStringExtension)
      assertThat(contains(UnittestProto.optionalStringExtension)).isFalse()

      this[UnittestProto.optionalGroupExtension] = optionalGroupExtension { a = 117 }
      clear(UnittestProto.optionalGroupExtension)
      assertThat(contains(UnittestProto.optionalGroupExtension)).isFalse()

      this[UnittestProto.optionalNestedMessageExtension] =
        TestAllTypesKt.nestedMessage { bb = 118 }
      clear(UnittestProto.optionalNestedMessageExtension)
      assertThat(contains(UnittestProto.optionalNestedMessageExtension)).isFalse()

      this[UnittestProto.optionalNestedEnumExtension] = NestedEnum.BAZ
      clear(UnittestProto.optionalNestedEnumExtension)
      assertThat(contains(UnittestProto.optionalNestedEnumExtension)).isFalse()

      this[UnittestProto.defaultInt32Extension] = 401
      clear(UnittestProto.defaultInt32Extension)
      assertThat(contains(UnittestProto.oneofUint32Extension)).isFalse()
    }
  }

  @Test
  fun testEmptyMessages() {
    assertThat(
      testEmptyMessage {}
    ).isEqualTo(
      TestEmptyMessage.newBuilder().build()
    )

    assertThat(
      testEmptyMessageWithExtensions {}
    ).isEqualTo(
      TestEmptyMessageWithExtensions.newBuilder().build()
    )
  }

  @Test
  fun testMapSetters() {
    val intMap = testIntIntMap { m[1] = 2 }
    assertThat(intMap).isEqualTo(
      TestIntIntMap.newBuilder().putM(1, 2).build()
    )

    assertThat(
      testMaps {
        mInt32[1] = intMap
        mInt64[1L] = intMap
        mUint32[1] = intMap
        mUint64[1L] = intMap
        mSint32[1] = intMap
        mSint64[1L] = intMap
        mFixed32[1] = intMap
        mFixed64[1L] = intMap
        mSfixed32[1] = intMap
        mSfixed64[1] = intMap
        mBool[true] = intMap
        mString["1"] = intMap
      }
    ).isEqualTo(
      TestMaps.newBuilder()
        .putMInt32(1, intMap)
        .putMInt64(1L, intMap)
        .putMUint32(1, intMap)
        .putMUint64(1L, intMap)
        .putMSint32(1, intMap)
        .putMSint64(1L, intMap)
        .putMFixed32(1, intMap)
        .putMFixed64(1L, intMap)
        .putMSfixed32(1, intMap)
        .putMSfixed64(1L, intMap)
        .putMBool(true, intMap)
        .putMString("1", intMap)
        .build()
    )

    assertThat(
      testEnumMap {
        knownMapField[1] = Proto2MapEnum.PROTO2_MAP_ENUM_FOO
      }
    ).isEqualTo(
      TestEnumMap.newBuilder()
        .putKnownMapField(1, Proto2MapEnum.PROTO2_MAP_ENUM_FOO)
        .build()
    )
  }

  @Test
  fun testMapGettersAndSetters() {
    val intMap =
      testIntIntMap {
        m.put(1, 2)
        assertThat(m).isEqualTo(mapOf(1 to 2))
        m[3] = 4
        assertThat(m).isEqualTo(mapOf(1 to 2, 3 to 4))
        m.putAll(mapOf(5 to 6, 7 to 8))
        assertThat(m).isEqualTo(mapOf(1 to 2, 3 to 4, 5 to 6, 7 to 8))
      }

    testMaps {
      mInt32.put(1, intMap)
      assertThat(mInt32).isEqualTo(mapOf(1 to intMap))
      mInt32[2] = intMap
      assertThat(mInt32).isEqualTo(mapOf(1 to intMap, 2 to intMap))
      mInt32.putAll(mapOf(3 to intMap, 4 to intMap))
      assertThat(mInt32).isEqualTo(mapOf(1 to intMap, 2 to intMap, 3 to intMap, 4 to intMap))

      mString.put("1", intMap)
      assertThat(mString).isEqualTo(mapOf("1" to intMap))
      mString["2"] = intMap
      assertThat(mString).isEqualTo(mapOf("1" to intMap, "2" to intMap))
      mString.putAll(mapOf("3" to intMap, "4" to intMap))
      assertThat(mString).isEqualTo(
        mapOf("1" to intMap, "2" to intMap, "3" to intMap, "4" to intMap)
      )
    }

    testEnumMap {
      knownMapField.put(1, Proto2MapEnum.PROTO2_MAP_ENUM_FOO)
      assertThat(knownMapField).isEqualTo(mapOf(1 to Proto2MapEnum.PROTO2_MAP_ENUM_FOO))
      knownMapField[2] = Proto2MapEnum.PROTO2_MAP_ENUM_BAR
      assertThat(knownMapField).isEqualTo(
        mapOf(1 to Proto2MapEnum.PROTO2_MAP_ENUM_FOO, 2 to Proto2MapEnum.PROTO2_MAP_ENUM_BAR)
      )
      knownMapField.putAll(
        mapOf(3 to Proto2MapEnum.PROTO2_MAP_ENUM_BAZ, 4 to Proto2MapEnum.PROTO2_MAP_ENUM_FOO)
      )
      assertThat(knownMapField).isEqualTo(
        mapOf(
          1 to Proto2MapEnum.PROTO2_MAP_ENUM_FOO,
          2 to Proto2MapEnum.PROTO2_MAP_ENUM_BAR,
          3 to Proto2MapEnum.PROTO2_MAP_ENUM_BAZ,
          4 to Proto2MapEnum.PROTO2_MAP_ENUM_FOO
        )
      )
    }
  }

  @Test
  fun testMapRemove() {
    val intMap =
      testIntIntMap {
        m.putAll(mapOf(1 to 2, 3 to 4))
        m.remove(1)
        assertThat(m).isEqualTo(mapOf(3 to 4))
      }

    testMaps {
      mInt32.putAll(mapOf(1 to intMap, 2 to intMap))
      mInt32.remove(1)
      assertThat(mInt32).isEqualTo(mapOf(2 to intMap))

      mString.putAll(mapOf("1" to intMap, "2" to intMap))
      mString.remove("1")
      assertThat(mString).isEqualTo(mapOf("2" to intMap))
    }

    testEnumMap {
      knownMapField.putAll(
        mapOf(1 to Proto2MapEnum.PROTO2_MAP_ENUM_FOO, 2 to Proto2MapEnum.PROTO2_MAP_ENUM_BAR)
      )
      knownMapField.remove(1)
      assertThat(knownMapField).isEqualTo(mapOf(2 to Proto2MapEnum.PROTO2_MAP_ENUM_BAR))
    }
  }

  @Test
  fun testMapClear() {
    val intMap =
      testIntIntMap {
        m.putAll(mapOf(1 to 2, 3 to 4))
        m.clear()
        assertThat(m.isEmpty()).isTrue()
      }

    testMaps {
      mInt32.putAll(mapOf(1 to intMap, 2 to intMap))
      mInt32.clear()
      assertThat(mInt32.isEmpty()).isTrue()

      mString.putAll(mapOf("1" to intMap, "2" to intMap))
      mString.clear()
      assertThat(mString.isEmpty()).isTrue()
    }

    testEnumMap {
      knownMapField.putAll(
        mapOf(1 to Proto2MapEnum.PROTO2_MAP_ENUM_FOO, 2 to Proto2MapEnum.PROTO2_MAP_ENUM_BAR)
      )
      knownMapField.clear()
      assertThat(knownMapField.isEmpty()).isTrue()
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
    ).isEqualTo(
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
