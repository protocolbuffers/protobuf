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
import com.google.protobuf.kotlin.protoc.SingletonFieldDescriptor
import com.google.protobuf.kotlin.protoc.UnqualifiedScope
import com.google.protobuf.kotlin.protoc.classBuilder
import com.google.protobuf.kotlin.protoc.specialize
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3
import com.google.protobuf.kotlin.generator.Example3.ExampleMessage
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.TypeSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [OneofOrNullPropertyGenerator]. */
@RunWith(JUnit4::class)
class OneofOrNullPropertyGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)
  private val example3 = Example3::class.qualifiedName!!

  @Test
  fun forMessageOrBuilder() {
    val fieldDescriptor = ExampleMessage.getDescriptor().findFieldByNumber(
      ExampleMessage.STRING_ONEOF_OPTION_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor

    val declarations = OneofOrNullPropertyGenerator.forMessageOrBuilder.run {
      config.componentExtensions(
        fieldDescriptor,
        UnqualifiedScope,
        UnqualifiedScope
      )
    }
    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
import $example3
import $example3.ExampleMessage.MyOneofCase.STRING_ONEOF_OPTION
import kotlin.String

/**
 * The string_oneof_option field of the proto ExampleMessage, or null if it isn't set.
 */
val Example3.ExampleMessageOrBuilder.stringOneofOptionOrNull: String?
  get() = if (this.myOneofCase == STRING_ONEOF_OPTION) this.stringOneofOption else null
    """
    )
  }
  @Test
  fun forBuilder() {
    val fieldDescriptor = ExampleMessage.getDescriptor().findFieldByNumber(
      ExampleMessage.STRING_ONEOF_OPTION_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor

    val declarations = OneofOrNullPropertyGenerator.forBuilder.run {
      config.componentExtensions(
        fieldDescriptor,
        UnqualifiedScope,
        UnqualifiedScope
      )
    }
    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
import $example3
import $example3.ExampleMessage.MyOneofCase.STRING_ONEOF_OPTION
import kotlin.String

/**
 * The string_oneof_option field of the proto ExampleMessage, or null if it isn't set.
 */
var Example3.ExampleMessage.Builder.stringOneofOptionOrNull: String?
  get() = if (this.myOneofCase == STRING_ONEOF_OPTION) this.stringOneofOption else null
  set(_newValue) {
    if (_newValue == null) {
      this.clearStringOneofOption()
    } else {
      this.stringOneofOption = _newValue
    }
  }
    """
    )
  }

  @Test
  fun forDsl() {
    val fieldDescriptor = ExampleMessage.getDescriptor().findFieldByNumber(
      ExampleMessage.STRING_ONEOF_OPTION_FIELD_NUMBER
    ).specialize() as SingletonFieldDescriptor

    val dslSimpleName = ClassSimpleName("DslClass")
    val dslBuilder = TypeSpec.classBuilder(dslSimpleName)
    val dslCompanionBuilder = TypeSpec.companionObjectBuilder()

    with(config) {
      OneofOrNullPropertyGenerator.forDsl.run {
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

    val myOneofCase = ExampleMessage.MyOneofCase::class.qualifiedName!!
    assertThat(dslBuilder.build()).generates(
      """
class DslClass {
  /**
   * The string_oneof_option field of the proto ExampleMessage, or null if it isn't set.
   */
  var stringOneofOptionOrNull: kotlin.String?
    get() = if (myBuilder.myOneofCase == $myOneofCase.STRING_ONEOF_OPTION) myBuilder.stringOneofOption else null
    set(_newValue) {
      if (_newValue == null) {
        myBuilder.clearStringOneofOption()
      } else {
        myBuilder.stringOneofOption = _newValue
      }
    }
}
      """
    )
  }
}
