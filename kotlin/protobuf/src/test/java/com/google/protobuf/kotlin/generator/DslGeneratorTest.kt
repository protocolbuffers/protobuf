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
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.google.protobuf.kotlin.protoc.PackageScope
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.google.protobuf.kotlin.generator.Example3
import com.squareup.kotlinpoet.ClassName
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [DslGenerator]. */
@RunWith(JUnit4::class)
class DslGeneratorTest {
  private val javaPackagePolicy = JavaPackagePolicy.OPEN_SOURCE
  private val config = GeneratorConfig(javaPackagePolicy, aggressiveInlining = false)

  @Test
  fun simpleDslNoComponents() {
    val dslGenerator = DslGenerator(
      extendableDslSuperclass = ClassName("com.google.protobuf.kotlin", "ProtoDsl"),
      dslClassNameSuffix = "DslClassNameSuffix",
      dslFactoryNamePrefix = "factoryNamePrefix",
      copyName = null
    )
    val enclosingScope = PackageScope(Example3.ExampleMessage::class.java.`package`.name)
    val extensionScope = enclosingScope.nestedScope(ClassSimpleName("Extensions"))
    with(config) {
      dslGenerator.run {
        val decls = shallowExtensions(
          descriptor = Example3.ExampleMessage.getDescriptor(),
          enclosingScope = enclosingScope,
          extensionScope = extensionScope
        )
        assertThat(decls)
          .generatesEnclosed(
            """
import com.google.protobuf.kotlin.ProtoDsl
import com.google.protobuf.kotlin.ProtoDslMarker
import com.google.protobuf.kotlin.generator.Example3
import com.google.protobuf.kotlin.generator.ExampleMessageDslClassNameSuffix
import kotlin.PublishedApi
import kotlin.Unit
import kotlin.jvm.JvmSynthetic

/**
 * Entry point for creating ExampleMessage messages in Kotlin.
 */
@JvmSynthetic
inline fun factoryNamePrefixExampleMessage(block: ExampleMessageDslClassNameSuffix.() -> Unit) =
    ExampleMessageDslClassNameSuffix(Example3.ExampleMessage.newBuilder()).apply(block).build()

/**
 * A DSL type for creating ExampleMessage messages.  To use it, call the
 * [factoryNamePrefixExampleMessage] method.
 */
@ProtoDslMarker
class ExampleMessageDslClassNameSuffix @PublishedApi internal constructor(
  builder_: Example3.ExampleMessage.Builder
) : ProtoDsl<Example3.ExampleMessage.Builder>(builder_) {
  @PublishedApi
  internal val builder_: Example3.ExampleMessage.Builder
    get() = _proto_dsl_builder

  /**
   * Builds the ExampleMessage.
   */
  @PublishedApi
  internal fun build(): Example3.ExampleMessage = builder_.build()
}
            """
          )
        // note we explicitly expect there to be no companion object by default
        assertThat(decls).generatesNoTopLevelMembers()
      }
    }
  }

  @Test
  fun copyDslNoComponents() {
    val dslGenerator = DslGenerator(
      extendableDslSuperclass = ClassName("com.google.protobuf.kotlin", "ProtoDsl"),
      dslClassNameSuffix = "DslClassNameSuffix",
      dslFactoryNamePrefix = "factoryNamePrefix",
      copyName = MemberSimpleName("copyMethod")
    )
    val enclosingScope = PackageScope(Example3.ExampleMessage::class.java.`package`.name)
    val extensionScope = enclosingScope.nestedScope(ClassSimpleName("Extensions"))
    with(config) {
      dslGenerator.run {
        val decls = shallowExtensions(
          descriptor = Example3.ExampleMessage.getDescriptor(),
          enclosingScope = enclosingScope,
          extensionScope = extensionScope
        )
        assertThat(decls)
          .generatesEnclosed(
            """
import com.google.protobuf.kotlin.ProtoDsl
import com.google.protobuf.kotlin.ProtoDslMarker
import com.google.protobuf.kotlin.generator.Example3
import com.google.protobuf.kotlin.generator.ExampleMessageDslClassNameSuffix
import kotlin.PublishedApi
import kotlin.Unit
import kotlin.jvm.JvmSynthetic

/**
 * Entry point for creating ExampleMessage messages in Kotlin.
 */
@JvmSynthetic
inline fun factoryNamePrefixExampleMessage(block: ExampleMessageDslClassNameSuffix.() -> Unit) =
    ExampleMessageDslClassNameSuffix(Example3.ExampleMessage.newBuilder()).apply(block).build()

/**
 * A DSL type for creating ExampleMessage messages.  To use it, call the
 * [factoryNamePrefixExampleMessage] method.
 */
@ProtoDslMarker
class ExampleMessageDslClassNameSuffix @PublishedApi internal constructor(
  builder_: Example3.ExampleMessage.Builder
) : ProtoDsl<Example3.ExampleMessage.Builder>(builder_) {
  @PublishedApi
  internal val builder_: Example3.ExampleMessage.Builder
    get() = _proto_dsl_builder

  /**
   * Builds the ExampleMessage.
   */
  @PublishedApi
  internal fun build(): Example3.ExampleMessage = builder_.build()
}
            """
          )
        assertThat(decls).generatesTopLevel(
          """
import com.google.protobuf.kotlin.generator.Example3
import com.google.protobuf.kotlin.generator.ExampleMessageDslClassNameSuffix
import kotlin.Unit
import kotlin.jvm.JvmSynthetic

/**
 * Creates a Example3.ExampleMessage equivalent to this one, but with the specified modifications.
 */
@JvmSynthetic
inline fun Example3.ExampleMessage.copyMethod(block: ExampleMessageDslClassNameSuffix.() -> Unit):
    Example3.ExampleMessage = ExampleMessageDslClassNameSuffix(toBuilder()).apply(block).build()
        """
        )
      }
    }
  }
}
