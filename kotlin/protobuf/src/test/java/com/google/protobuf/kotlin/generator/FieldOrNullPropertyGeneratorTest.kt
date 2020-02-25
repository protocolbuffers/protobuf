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

import com.google.protobuf.kotlin.protoc.ClassSimpleName
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.JavaPackagePolicy
import com.google.protobuf.kotlin.protoc.OneofOptionFieldDescriptor
import com.google.protobuf.kotlin.protoc.SingletonFieldDescriptor
import com.google.protobuf.kotlin.protoc.UnqualifiedScope
import com.google.protobuf.kotlin.protoc.classBuilder
import com.google.protobuf.kotlin.protoc.specialize
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example2
import com.google.protobuf.kotlin.generator.Example3
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.TypeSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [FieldOrNullPropertyGenerator]. */
@RunWith(JUnit4::class)
class FieldOrNullPropertyGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)

  private val fieldDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
    Example3.ExampleMessage.SUB_MESSAGE_FIELD_FIELD_NUMBER
  ).specialize() as SingletonFieldDescriptor

  private val oneofOptionDescriptor = Example3.ExampleMessage.getDescriptor().findFieldByNumber(
    Example3.ExampleMessage.SUB_MESSAGE_ONEOF_OPTION_FIELD_NUMBER
  ).specialize() as OneofOptionFieldDescriptor

  private val requiredFieldDescriptor =
    Example2.ExampleProto2Message.getDescriptor().findFieldByNumber(
      Example2.ExampleProto2Message.REQUIRED_STRING_FIELD_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor

  @Test
  fun forMessageOrBuilder() {
    val declarations = FieldOrNullPropertyGenerator().forMessageOrBuilder.run {
      config.componentExtensions(fieldDescriptor, UnqualifiedScope, UnqualifiedScope)
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
    import com.google.protobuf.kotlin.generator.Example3

    /**
     * The sub_message_field field of the proto ExampleMessage, or null if it isn't set.
     */
    val Example3.ExampleMessageOrBuilder.subMessageFieldOrNull: Example3.ExampleMessage.SubMessage?
      get() = if (this.hasSubMessageField()) this.subMessageField else null
    """
    )
  }

  @Test
  fun forMessageOrBuilderSkipsRequiredFields() {
    val declarations = FieldOrNullPropertyGenerator().forMessageOrBuilder.run {
      config.componentExtensions(requiredFieldDescriptor, UnqualifiedScope, UnqualifiedScope)
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesNoTopLevelMembers()
  }

  @Test
  fun skipOneOf() {
    val declarations =
      FieldOrNullPropertyGenerator(skipOneofFields = true).forMessageOrBuilder.run {
        config.componentExtensions(oneofOptionDescriptor, UnqualifiedScope, UnqualifiedScope)
      }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesNoTopLevelMembers()
  }

  @Test
  fun skipOneOfSetToFalse() {
    val declarations =
      FieldOrNullPropertyGenerator(skipOneofFields = false).forMessageOrBuilder.run {
        config.componentExtensions(oneofOptionDescriptor, UnqualifiedScope, UnqualifiedScope)
      }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
    import com.google.protobuf.kotlin.generator.Example3

    /**
     * The sub_message_oneof_option field of the proto ExampleMessage, or null if it isn't set.
     */
    val Example3.ExampleMessageOrBuilder.subMessageOneofOptionOrNull:
        Example3.ExampleMessage.SubMessage?
      get() = if (this.hasSubMessageOneofOption()) this.subMessageOneofOption else null
    """
    )
  }

  @Test
  fun forBuilder() {
    val declarations = FieldOrNullPropertyGenerator().forBuilder.run {
      config.componentExtensions(fieldDescriptor, UnqualifiedScope, UnqualifiedScope)
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
    import com.google.protobuf.kotlin.generator.Example3

    /**
     * The sub_message_field field of the proto ExampleMessage, or null if it isn't set.
     */
    var Example3.ExampleMessage.Builder.subMessageFieldOrNull: Example3.ExampleMessage.SubMessage?
      get() = if (this.hasSubMessageField()) this.subMessageField else null
      set(_newValue) {
        if (_newValue == null) {
          this.clearSubMessageField()
        } else {
          this.subMessageField = _newValue
        }
      }
    """
    )
  }

  @Test
  fun forBuilderIncludesRequiredFields() {
    val declarations = FieldOrNullPropertyGenerator().forBuilder.run {
      config.componentExtensions(requiredFieldDescriptor, UnqualifiedScope, UnqualifiedScope)
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
    import com.google.protobuf.kotlin.generator.Example2
    import kotlin.String

    /**
     * The required_string_field field of the proto ExampleProto2Message, or null if it isn't set.
     */
    var Example2.ExampleProto2Message.Builder.requiredStringFieldOrNull: String?
      get() = if (this.hasRequiredStringField()) this.requiredStringField else null
      set(_newValue) {
        if (_newValue == null) {
          this.clearRequiredStringField()
        } else {
          this.requiredStringField = _newValue
        }
      }
    """
    )
  }

  @Test
  fun forDsl() {
    val dslSimpleName = ClassSimpleName("DslClass")
    val dslBuilder = TypeSpec.classBuilder(dslSimpleName)
    val dslCompanionBuilder = TypeSpec.companionObjectBuilder()

    with(config) {
      FieldOrNullPropertyGenerator().forDsl.run {
        generate(
          component = fieldDescriptor,
          dslType = UnqualifiedScope.nestedClass(dslSimpleName),
          extensionScope = UnqualifiedScope,
          enclosingScope = UnqualifiedScope,
          dslBuilder = dslBuilder,
          companionBuilder = dslCompanionBuilder,
          builderCode = CodeBlock.of("myBuilder")
        )
      }
    }

    val subMessage = Example3.ExampleMessage.SubMessage::class.qualifiedName!!
    assertThat(dslBuilder.build()).generates(
      """
class DslClass {
  /**
   * The sub_message_field field of the proto ExampleMessage, or null if it isn't set.
   */
  var subMessageFieldOrNull: $subMessage?
    get() = if (myBuilder.hasSubMessageField()) myBuilder.subMessageField else null
    set(_newValue) {
      if (_newValue == null) {
        myBuilder.clearSubMessageField()
      } else {
        myBuilder.subMessageField = _newValue
      }
    }
}
      """
    )
  }
}
