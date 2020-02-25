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
import com.google.protobuf.kotlin.generator.Example3.ExampleMessage
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.TypeSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [WrapperPropertyGenerator]. */
@RunWith(JUnit4::class)
class WrapperPropertyGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(
    javaPackagePolicy = javaPackagePolicy,
    aggressiveInlining = false
  )
  private val generator = WrapperPropertyGenerator(
    propertyNameSuffix = "Value"
  )
  private val fieldDescriptor = ExampleMessage.getDescriptor().findFieldByNumber(
    ExampleMessage.OPTIONAL_INT32_FIELD_NUMBER
  ).specialize() as SingletonFieldDescriptor

  @Test
  fun orBuilder() {
    val declarations = generator.forMessageOrBuilder.run {
      config.componentExtensions(
        fieldDescriptor,
        UnqualifiedScope,
        UnqualifiedScope
      )
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
import com.google.protobuf.kotlin.generator.Example3
import kotlin.Int

/**
 * A nullable view of the field optional_int32 of the message
 * protobuf.kotlin.generator.ExampleMessage as an optional int32.
 */
val Example3.ExampleMessageOrBuilder.optionalInt32Value: Int?
  get() = if (this.hasOptionalInt32()) this.optionalInt32.value else null
      """
    )
  }

  @Test
  fun builder() {
    val declarations = generator.forBuilder.run {
      config.componentExtensions(
        fieldDescriptor,
        UnqualifiedScope,
        UnqualifiedScope
      )
    }

    assertThat(declarations).generatesNoEnclosedMembers()
    assertThat(declarations).generatesTopLevel(
      """
import com.google.protobuf.Int32Value
import com.google.protobuf.kotlin.generator.Example3
import kotlin.Int

/**
 * A nullable view of the field optional_int32 of the message
 * protobuf.kotlin.generator.ExampleMessage as an optional int32.
 */
var Example3.ExampleMessage.Builder.optionalInt32Value: Int?
  get() = if (this.hasOptionalInt32()) this.optionalInt32.value else null
  set(newValue) {
    if (newValue == null) {
      this.clearOptionalInt32()
    } else {
      this.optionalInt32 = Int32Value.newBuilder().setValue(newValue).build()
    }
  }
      """
    )
  }

  @Test
  fun dsl() {
    val dslSimpleName = ClassSimpleName("DslClass")
    val dslBuilder = TypeSpec.classBuilder(dslSimpleName)
    val dslCompanionBuilder = TypeSpec.companionObjectBuilder()

    with(config) {
      generator.forDsl.run {
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

    assertThat(dslCompanionBuilder.build()).generates("companion object")
    assertThat(dslBuilder.build()).generates(
      """
class DslClass {
  /**
   * A nullable view of the field optional_int32 of the message protobuf.kotlin.generator.ExampleMessage as an optional int32.
   */
  var optionalInt32Value: kotlin.Int?
    get() = if (myBuilder.hasOptionalInt32()) myBuilder.optionalInt32.value else null
    set(newValue) {
      if (newValue == null) {
        myBuilder.clearOptionalInt32()
      } else {
        myBuilder.optionalInt32 = com.google.protobuf.Int32Value.newBuilder().setValue(newValue).build()
      }
    }
}
    """
    )
  }
}
