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

import com.google.protobuf.Descriptors
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.JavaPackagePolicy
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3
import com.google.protobuf.kotlin.generator.JavaMultipleFiles
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.PropertySpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [ProtoFileKotlinApiGenerator]. */
@RunWith(JUnit4::class)
class ProtoFileKotlinApiGeneratorTest {
  private val extensionGenerator: ExtensionGenerator = object : ExtensionGenerator() {
    override fun GeneratorConfig.shallowExtensions(
      descriptor: Descriptors.Descriptor,
      enclosingScope: Scope,
      extensionScope: Scope
    ) = declarations {
      addProperty(PropertySpec.builder("extensionProperty", INT).build())
    }

    override val generateWithinExtensionClass: Boolean
      get() = true
  }

  @Test
  fun generate() {
    val recursiveGenerator = RecursiveExtensionGenerator("Kt", extensionGenerator)
    val fileGenerator = ProtoFileKotlinApiGenerator(
      generator = recursiveGenerator,
      config = GeneratorConfig(
        javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE,
        aggressiveInlining = false
      )
    )

    val fileSpec = fileGenerator.generate(Example3.getDescriptor())
    assertThat(fileSpec).hasName("Example3Kt")
    assertThat(fileSpec).generates(
      """
      @file:Suppress("DEPRECATION")

      package ${Example3::class.java.`package`.name}

      import kotlin.Int
      import kotlin.Suppress

      /**
       * Kotlin extensions for protos defined in the file ${Example3.getDescriptor().file.name}.
       */
      object Example3Kt {
        /**
         * Kotlin extensions around the proto message type protobuf.kotlin.generator.ExampleMessage.
         */
        object ExampleMessageKt {
          val extensionProperty: Int

          /**
           * Kotlin extensions around the proto message type
           * protobuf.kotlin.generator.ExampleMessage.SubMessage.
           */
          object SubMessageKt {
            val extensionProperty: Int
          }
        }
      }
      """
    )
  }

  @Test
  fun generateMultipleFiles() {
    val recursiveGenerator = RecursiveExtensionGenerator("Kt", extensionGenerator)
    val fileGenerator = ProtoFileKotlinApiGenerator(
      generator = recursiveGenerator,
      config = GeneratorConfig(
        javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE,
        aggressiveInlining = false
      )
    )

    val fileSpec = fileGenerator.generate(JavaMultipleFiles.getDescriptor())
    assertThat(fileSpec).hasName("JavaMultipleFilesKt")
    assertThat(fileSpec).generates(
      """
    @file:Suppress("DEPRECATION")

    package com.google.protobuf.kotlin.generator

    import kotlin.Int
    import kotlin.Suppress

    /**
     * Kotlin extensions around the proto message type protobuf.kotlin.generator.MultipleFilesMessageA.
     */
    object MultipleFilesMessageAKt {
      val extensionProperty: Int
    }

    /**
     * Kotlin extensions around the proto message type protobuf.kotlin.generator.MultipleFilesMessageB.
     */
    object MultipleFilesMessageBKt {
      val extensionProperty: Int
    }
      """
    )
  }

  @Test
  fun generateInlining() {
    val recursiveGenerator = RecursiveExtensionGenerator("Kt", extensionGenerator)
    val fileGenerator = ProtoFileKotlinApiGenerator(
      generator = recursiveGenerator,
      config = GeneratorConfig(
        javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE,
        aggressiveInlining = true
      )
    )

    val fileSpec = fileGenerator.generate(Example3.getDescriptor())
    assertThat(fileSpec).hasName("Example3Kt")
    assertThat(fileSpec).generates(
      """
      @file:Suppress("NOTHING_TO_INLINE")
      @file:Suppress("DEPRECATION")

      package com.google.protobuf.kotlin.generator

      import kotlin.Int
      import kotlin.Suppress

      /**
       * Kotlin extensions for protos defined in the file testing/example3.proto.
       */
      object Example3Kt {
        /**
         * Kotlin extensions around the proto message type protobuf.kotlin.generator.ExampleMessage.
         */
        object ExampleMessageKt {
          val extensionProperty: Int

          /**
           * Kotlin extensions around the proto message type
           * protobuf.kotlin.generator.ExampleMessage.SubMessage.
           */
          object SubMessageKt {
            val extensionProperty: Int
          }
        }
      }
      """
    )
  }
}
