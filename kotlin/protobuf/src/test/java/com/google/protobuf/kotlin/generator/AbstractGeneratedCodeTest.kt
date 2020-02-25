/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.protobuf.kotlin.generator

import com.google.common.truth.Truth.assertThat
import com.google.common.truth.extensions.proto.LiteProtoSubject
import com.google.protobuf.kotlin.extensions.contains
import com.google.protobuf.kotlin.extensions.get
import com.google.protobuf.kotlin.generator.EvilNamesOuterClass.EvilNames
import com.google.protobuf.kotlin.generator.EvilNamesOuterClassKt.evilNames
import com.google.protobuf.kotlin.generator.Example2.int32Extension
import com.google.protobuf.kotlin.generator.Example2.repeatedStringExtension
import com.google.protobuf.kotlin.generator.Example3.ExampleMessage
import org.junit.Test

/** Supertype of tests for generated code for `example3.proto`. */
abstract class AbstractGeneratedCodeTest {
  abstract fun assertThat(message: ExampleMessage): LiteProtoSubject
  abstract fun assertThat(message: EvilNames): LiteProtoSubject
  abstract fun assertThat(message: Example2.ExampleProto2Message): LiteProtoSubject

  @Test
  fun dslSetScalarField() {
    assertThat(
      Example3Kt.exampleMessage {
        int32Field = 5
      }
    ).isEqualTo(ExampleMessage.newBuilder().setInt32Field(5).build())
  }

  @Test
  fun dslSetMessageField() {
    val exampleMessage = Example3Kt.exampleMessage {
      subMessageField = Example3Kt.ExampleMessageKt.subMessage {
      }
    }
    assertThat(exampleMessage)
      .isEqualTo(
        ExampleMessage.newBuilder()
          .setSubMessageField(ExampleMessage.SubMessage.getDefaultInstance())
          .build()
      )
    assertThat(exampleMessage.hasSubMessageField()).isTrue()
  }

  @Test
  fun dslMessageFieldHazzer() {
    val exampleMessage = Example3Kt.exampleMessage {
      assertThat(hasSubMessageField()).isFalse()
      subMessageField = Example3Kt.ExampleMessageKt.subMessage {
      }
      assertThat(hasSubMessageField()).isTrue()
    }
    assertThat(exampleMessage)
      .isEqualTo(
        ExampleMessage.newBuilder()
          .setSubMessageField(ExampleMessage.SubMessage.getDefaultInstance())
          .build()
      )
    assertThat(exampleMessage.hasSubMessageField()).isTrue()
  }

  @Test
  fun dslRepeatedFieldAdd() {
    val exampleMessage = Example3Kt.exampleMessage {
      repeatedStringField.add("foobar")
      repeatedStringField.add("quux")
    }
    assertThat(exampleMessage).isEqualTo(
      ExampleMessage.newBuilder()
        .addRepeatedStringField("foobar")
        .addRepeatedStringField("quux")
        .build()
    )
  }

  @Test
  fun dslRepeatedFieldPlusAssignElement() {
    val exampleMessage = Example3Kt.exampleMessage {
      repeatedStringField += "foobar"
      repeatedStringField += "quux"
    }
    assertThat(exampleMessage).isEqualTo(
      ExampleMessage.newBuilder()
        .addRepeatedStringField("foobar")
        .addRepeatedStringField("quux")
        .build()
    )
  }

  @Test
  fun dslRepeatedFieldQuery() {
    val exampleMessage = Example3Kt.exampleMessage {
      repeatedStringField += "foobar"
      repeatedStringField += "quux"
      assertThat(repeatedStringField).containsExactly("foobar", "quux").inOrder()
      assertThat(repeatedStringField[0]).isEqualTo("foobar")
      assertThat(repeatedStringField.size).isEqualTo(2)
    }
    assertThat(exampleMessage).isEqualTo(
      ExampleMessage.newBuilder()
        .addRepeatedStringField("foobar")
        .addRepeatedStringField("quux")
        .build()
    )
  }

  @Test
  fun dslRepeatedFieldSet() {
    val exampleMessage = Example3Kt.exampleMessage {
      repeatedStringField += "foobar"
      repeatedStringField += "quux"
      repeatedStringField[0] = "banana"
    }
    assertThat(exampleMessage).isEqualTo(
      ExampleMessage.newBuilder()
        .addRepeatedStringField("banana")
        .addRepeatedStringField("quux")
        .build()
    )
  }

  @Test
  fun dslRepeatedFieldClear() {
    val exampleMessage = Example3Kt.exampleMessage {
      repeatedStringField += "foobar"
      repeatedStringField.clear()
    }
    assertThat(exampleMessage).isEqualTo(ExampleMessage.getDefaultInstance())
  }

  @Test
  fun dslMapFieldPut() {
    val exampleMessage = Example3Kt.exampleMessage {
      mapField.put(5L, ExampleMessage.SubMessage.getDefaultInstance())
    }
    assertThat(exampleMessage)
      .isEqualTo(
        ExampleMessage.newBuilder()
          .putMapField(5L, ExampleMessage.SubMessage.getDefaultInstance())
          .build()
      )
  }

  @Test
  fun dslMapFieldSet() {
    val exampleMessage = Example3Kt.exampleMessage {
      mapField[5L] = ExampleMessage.SubMessage.getDefaultInstance()
    }
    assertThat(exampleMessage)
      .isEqualTo(
        ExampleMessage.newBuilder()
          .putMapField(5L, ExampleMessage.SubMessage.getDefaultInstance())
          .build()
      )
  }

  @Test
  fun dslMapFieldPutAll() {
    val exampleMessage = Example3Kt.exampleMessage {
      mapField.putAll(mapOf(5L to ExampleMessage.SubMessage.getDefaultInstance()))
    }
    assertThat(exampleMessage)
      .isEqualTo(
        ExampleMessage.newBuilder()
          .putMapField(5L, ExampleMessage.SubMessage.getDefaultInstance())
          .build()
      )
  }

  @Test
  fun dslMapFieldQuery() {
    val exampleMessage = Example3Kt.exampleMessage {
      mapField[5L] = ExampleMessage.SubMessage.getDefaultInstance()
      assertThat(mapField[5L]).isNotNull()
      assertThat(mapField[3L]).isNull()
      assertThat(mapField.size).isEqualTo(1)
      assertThat(mapField).containsExactly(5L, ExampleMessage.SubMessage.getDefaultInstance())
    }
    assertThat(exampleMessage)
      .isEqualTo(
        ExampleMessage.newBuilder()
          .putMapField(5L, ExampleMessage.SubMessage.getDefaultInstance())
          .build()
      )
  }

  @Test
  fun dslMapFieldClear() {
    val exampleMessage = Example3Kt.exampleMessage {
      mapField[5L] = ExampleMessage.SubMessage.getDefaultInstance()
      mapField.clear()
    }
    assertThat(exampleMessage).isEqualTo(ExampleMessage.getDefaultInstance())
  }

  @Test
  fun dslCopy() {
    assertThat(
      Example3Kt.exampleMessage {
        int32Field = 5
      }.copy {
        int32Field = 4
        stringField = "foo"
      }
    ).isEqualTo(ExampleMessage.newBuilder().setInt32Field(4).setStringField("foo").build())
  }

  @Test
  fun oneofNotSet() {
    assertThat(ExampleMessage.getDefaultInstance().myOneof)
      .isEqualTo(Example3Kt.ExampleMessageKt.MyOneofOneof.NotSet)
  }

  @Test
  fun oneofString() {
    assertThat(
      Example3Kt.exampleMessage {
        stringOneofOption = "foo"
      }.myOneof
    ).isEqualTo(Example3Kt.ExampleMessageKt.MyOneofOneof.StringOneofOption("foo"))
  }

  @Test
  fun oneofSetDsl() {
    val exampleMessage = Example3Kt.exampleMessage {
      myOneof = Example3Kt.ExampleMessageKt.MyOneofOneof.StringOneofOption("bar")
    }
    assertThat(exampleMessage.myOneofCase).isEqualTo(ExampleMessage.MyOneofCase.STRING_ONEOF_OPTION)
    assertThat(exampleMessage.stringOneofOption).isEqualTo("bar")
  }

  @Test
  fun oneofSetBuilder() {
    val builder = ExampleMessage.newBuilder()
    builder.myOneof =
      Example3Kt.ExampleMessageKt.MyOneofOneof.SubMessageOneofOption(
        ExampleMessage.SubMessage.getDefaultInstance()
      )
    val exampleMessage = builder.build()
    assertThat(exampleMessage.myOneofCase)
      .isEqualTo(ExampleMessage.MyOneofCase.SUB_MESSAGE_ONEOF_OPTION)
  }

  @Test
  fun evilNames() {
    val evil = evilNames {
      isInitialized = true
      initialized = false
      bar = "bar"
      hasFoo = true
      `class` = "classy"
      `in` = -5
      `object` = "objectified"
      long = true
      serializedSize = true
      cachedSize = "foo"
    }
    assertThat(evil).isEqualTo(
      EvilNames
        .newBuilder()
        .setIsInitialized(true)
        .setInitialized(false)
        .setBar("bar")
        .setHasFoo(true)
        .setClass_("classy")
        .setIn(-5)
        .setObject("objectified")
        .setLong(true)
        .setSerializedSize_(true)
        .setCachedSize_("foo")
        .build()
    )

  }

  @Test
  fun scalarExtensionDslSet() {
    assertThat(
      Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[int32Extension] = 5
      }
    ).isEqualTo(
      Example2.ExampleProto2Message.newBuilder()
        .setRequiredStringField("foo")
        .setExtension(int32Extension, 5)
        .build()
    )
  }

  @Test
  fun scalarExtensionDslContains() {
    Example2Kt.exampleProto2Message {
      requiredStringField = "foo"
      assertThat(int32Extension in this).isFalse()
      this[int32Extension] = 5
      assertThat(int32Extension in this).isTrue()
    }
  }

  @Test
  fun scalarExtensionDslGet() {
    assertThat(
      Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[int32Extension] = 5
        assertThat(this[int32Extension]).isEqualTo(5)
      }
    ).isEqualTo(
      Example2.ExampleProto2Message.newBuilder()
        .setRequiredStringField("foo")
        .setExtension(int32Extension, 5)
        .build()
    )
  }

  @Test
  fun scalarExtensionGetFromProto() {
    val proto = Example2Kt.exampleProto2Message {
      requiredStringField = "foo"
      this[int32Extension] = 5
    }
    assertThat(proto[int32Extension]).isEqualTo(5)
  }

  @Test
  fun scalarExtensionContainsFromProto() {
    assertThat(
      int32Extension in Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[int32Extension] = 5
      }
    ).isTrue()
    assertThat(
      int32Extension in Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
      }
    ).isFalse()
  }

  @Test
  fun scalarExtensionClear() {
    assertThat(
      Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[int32Extension] = 5
        clearExtension(int32Extension)
      }
    ).isEqualTo(
      Example2.ExampleProto2Message.newBuilder()
        .setRequiredStringField("foo")
        .build()
    )
  }

  @Test
  fun repeatedExtensionPlusAssign() {
    assertThat(
      Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[repeatedStringExtension] += "bar"
      }
    ).isEqualTo(
      Example2.ExampleProto2Message.newBuilder()
        .setRequiredStringField("foo")
        .addExtension(repeatedStringExtension, "bar")
        .build()
    )
  }

  @Test
  fun repeatedExtensionPlusAssignIterable() {
    assertThat(
      Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[repeatedStringExtension] += listOf("bar")
      }
    ).isEqualTo(
      Example2.ExampleProto2Message.newBuilder()
        .setRequiredStringField("foo")
        .addExtension(repeatedStringExtension, "bar")
        .build()
    )
  }

  @Test
  fun repeatedExtensionSet() {
    assertThat(
      Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[repeatedStringExtension] += "bar"
        this[repeatedStringExtension][0] = "quux"
      }
    ).isEqualTo(
      Example2.ExampleProto2Message.newBuilder()
        .setRequiredStringField("foo")
        .addExtension(repeatedStringExtension, "quux")
        .build()
    )
  }

  @Test
  fun repeatedExtensionGetFromDsl() {
    Example2Kt.exampleProto2Message {
      requiredStringField = "foo"
      assertThat(this[repeatedStringExtension]).isEmpty()
      this[repeatedStringExtension] += "bar"
      assertThat(this[repeatedStringExtension]).containsExactly("bar")
      this[repeatedStringExtension] += "quux"
      assertThat(this[repeatedStringExtension]).containsExactly("bar", "quux").inOrder()
    }
  }

  @Test
  fun repeatedExtensionClear() {
    assertThat(
      Example2Kt.exampleProto2Message {
        requiredStringField = "foo"
        this[repeatedStringExtension] += "bar"
        this[repeatedStringExtension].clear()
        this[repeatedStringExtension] += "quux"
      }
    ).isEqualTo(
      Example2.ExampleProto2Message.newBuilder()
        .setRequiredStringField("foo")
        .addExtension(repeatedStringExtension, "quux")
        .build()
    )
  }

  @Test
  fun repeatedExtensionGetFromProto() {
    val proto = Example2Kt.exampleProto2Message {
      requiredStringField = "foo"
      this[repeatedStringExtension] += "bar"
      this[repeatedStringExtension] += "quux"
    }
    assertThat(proto[repeatedStringExtension]).containsExactly("bar", "quux").inOrder()
  }

  @Test
  fun javaMultipleFilesFactory() {
    @Suppress("RemoveRedundantQualifierName")
    assertThat(com.google.protobuf.kotlin.generator.multipleFilesMessageA {  }).isEqualTo(
      MultipleFilesMessageA.getDefaultInstance()
    )
  }
}
