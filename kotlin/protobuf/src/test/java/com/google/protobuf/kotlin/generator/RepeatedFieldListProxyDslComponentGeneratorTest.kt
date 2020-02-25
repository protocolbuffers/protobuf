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

import com.google.protobuf.kotlin.DslList
import com.google.protobuf.kotlin.protoc.ClassSimpleName
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.JavaPackagePolicy
import com.google.protobuf.kotlin.protoc.RepeatedFieldDescriptor
import com.google.protobuf.kotlin.protoc.UnqualifiedScope
import com.google.protobuf.kotlin.protoc.classBuilder
import com.google.protobuf.kotlin.protoc.specialize
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.TypeSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [RepeatedFieldListProxyDslComponentGeneratorTest]. */
@RunWith(JUnit4::class)
class RepeatedFieldListProxyDslComponentGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)

  private val repeatedFieldDescriptor: RepeatedFieldDescriptor =
    Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.REPEATED_STRING_FIELD_FIELD_NUMBER
    ).specialize() as RepeatedFieldDescriptor

  @Test
  fun generate() {
    val dslSimpleName = ClassSimpleName("DslClass")
    val dslBuilder = TypeSpec.classBuilder(dslSimpleName)
    val dslCompanionBuilder = TypeSpec.companionObjectBuilder()
    val generator = RepeatedFieldListProxyDslComponentGenerator(
      proxyTypeNameSuffix = "ProxySuffix"
    )

    with(config) {
      generator.run {
        generate(
          component = repeatedFieldDescriptor,
          builderCode = CodeBlock.of("myBuilder"),
          enclosingScope = UnqualifiedScope,
          extensionScope = UnqualifiedScope,
          dslType = UnqualifiedScope.nestedClass(dslSimpleName),
          companionBuilder = dslCompanionBuilder,
          dslBuilder = dslBuilder
        )
      }
    }

    assertThat(dslCompanionBuilder.build()).generates("companion object")
    val dslList = DslList::class.qualifiedName!!
    assertThat(dslBuilder.build()).generates(
      """
      class DslClass {
        /**
         * A [kotlin.collections.List] view of the repeated field repeated_string_field.
         *
         * While the view object itself is a `List`, not a `MutableList`, within the context of the
         * DSL it has extension methods and operator overloads that mutate it appropriately.
         * For example, writing `repeatedStringField += element` adds that element to the repeated field, but that
         * code only compiles within the DSL.
         */
        val repeatedStringField: $dslList<kotlin.String, RepeatedStringFieldProxySuffix>
          inline get() = $dslList(myBuilder.getRepeatedStringFieldList())

        /**
         * An uninstantiable, behaviorless type to represent the field repeated_string_field in generics.
         */
        class RepeatedStringFieldProxySuffix private constructor()
      }
    """
    )
  }
}
