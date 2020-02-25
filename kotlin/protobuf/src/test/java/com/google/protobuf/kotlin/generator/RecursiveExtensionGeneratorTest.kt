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
import com.google.protobuf.kotlin.protoc.PackageScope
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3
import com.squareup.kotlinpoet.FileSpec
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.PropertySpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [RecursiveExtensionGenerator]. */
@RunWith(JUnit4::class)
class RecursiveExtensionGeneratorTest {
  @Test
  fun generate() {
    val peerGenerator = object : ExtensionGenerator() {
      override fun GeneratorConfig.shallowExtensions(
        descriptor: Descriptors.Descriptor,
        enclosingScope: Scope,
        extensionScope: Scope
      ) = declarations {
        addProperty(PropertySpec.builder("peerProperty", INT).build())
      }

      override val generateWithinExtensionClass: Boolean
        get() = false
    }
    val extensionGenerator = object : ExtensionGenerator() {
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

    val recursiveGenerator = RecursiveExtensionGenerator(
      "KtExtensions", peerGenerator, extensionGenerator
    )
    val genFile =
      FileSpec.builder("com.google", "FooBar.kt")
        .apply {
          recursiveGenerator.generate(
            GeneratorConfig(JavaPackagePolicy.OPEN_SOURCE, aggressiveInlining = false),
            Example3.ExampleMessage.getDescriptor(),
            PackageScope("com.google")
          ).writeAllAtTopLevel(this)
        }
        .build()
    assertThat(genFile)
      .generates(
        """
          package com.google

          import kotlin.Int

          val peerProperty: Int

          /**
           * Kotlin extensions around the proto message type protobuf.kotlin.generator.ExampleMessage.
           */
          object ExampleMessageKtExtensions {
            val extensionProperty: Int

            val peerProperty: Int

            /**
             * Kotlin extensions around the proto message type
             * protobuf.kotlin.generator.ExampleMessage.SubMessage.
             */
            object SubMessageKtExtensions {
              val extensionProperty: Int
            }
          }
        """
      )
  }
}
