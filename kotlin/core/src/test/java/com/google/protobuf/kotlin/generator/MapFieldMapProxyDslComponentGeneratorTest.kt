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

import com.google.protobuf.kotlin.DslMap
import com.google.protobuf.kotlin.protoc.ClassSimpleName
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.JavaPackagePolicy
import com.google.protobuf.kotlin.protoc.MapFieldDescriptor
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

/** Tests for [MapFieldMapProxyDslComponentGeneratorTest]. */
@RunWith(JUnit4::class)
class MapFieldMapProxyDslComponentGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)

  private val mapFieldDescriptor: MapFieldDescriptor =
    Example3.ExampleMessage.getDescriptor().findFieldByNumber(
      Example3.ExampleMessage.MAP_FIELD_FIELD_NUMBER
    ).specialize() as MapFieldDescriptor

  @Test
  fun generate() {
    val dslSimpleName = ClassSimpleName("DslClass")
    val dslBuilder = TypeSpec.classBuilder(dslSimpleName)
    val dslCompanionBuilder = TypeSpec.companionObjectBuilder()
    val generator = MapFieldMapProxyDslComponentGenerator(
      proxyTypeNameSuffix = "ProxySuffix"
    )

    with(config) {
      generator.run {
        generate(
          component = mapFieldDescriptor,
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
    val dslMap = DslMap::class.qualifiedName!!
    val exampleMessage = Example3.ExampleMessage::class.qualifiedName!!
    assertThat(dslBuilder.build()).generates(
      """
      class DslClass {
        /**
         * A [kotlin.collections.Map] view of the map field map_field.
         *
         * While the view object itself is a `Map`, not a `MutableMap`, within the context of the
         * DSL it has extension methods and operator overloads that mutate it appropriately.
         * For example, writing `mapField[k] = v` adds a mapping from `k` to `v` in the map field,
         * but that code only compiles within the DSL.
         */
        val mapField: $dslMap<kotlin.Long, $exampleMessage.SubMessage, MapFieldProxySuffix>
          get() = $dslMap(myBuilder.getMapFieldMap())

        /**
         * A behaviorless, uninstantiable type to represent the field map_field in generics.
         */
        class MapFieldProxySuffix private constructor()
      }
    """
    )
  }
}
