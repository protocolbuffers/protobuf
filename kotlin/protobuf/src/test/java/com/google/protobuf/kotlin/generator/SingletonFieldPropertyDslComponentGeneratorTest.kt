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

/** Tests for [SingletonFieldPropertyDslComponentGenerator]. */
@RunWith(JUnit4::class)
class SingletonFieldPropertyDslComponentGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)

  @Test
  fun generate() {
    val fieldDescriptor = ExampleMessage.getDescriptor()
      .findFieldByNumber(ExampleMessage.STRING_FIELD_FIELD_NUMBER)
      .specialize() as SingletonFieldDescriptor
    val dslSimpleName = ClassSimpleName("DslClass")
    val dslBuilder = TypeSpec.classBuilder(dslSimpleName)
    val dslCompanionBuilder = TypeSpec.companionObjectBuilder()

    with(config) {
      SingletonFieldPropertyDslComponentGenerator.run {
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

    assertThat(dslBuilder.build()).generates(
      """
        class DslClass {
          /**
           * The protobuf.kotlin.generator.ExampleMessage.string_field field.
           */
          var stringField: kotlin.String
            @kotlin.jvm.JvmName("getStringField")
            get() = myBuilder.getStringField()
            @kotlin.jvm.JvmName("setStringField")
            set(newValue) {
              myBuilder.setStringField(newValue)
            }
        }
      """
    )
  }
}
