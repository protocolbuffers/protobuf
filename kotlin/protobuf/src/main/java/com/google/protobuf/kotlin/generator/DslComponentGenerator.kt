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

import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.MessageComponent
import com.google.protobuf.kotlin.protoc.Scope
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec
import kotlin.reflect.KClass
import kotlin.reflect.full.safeCast

/**
 * Generates a set of members in a DSL for a message type, for a specific component of that
 * message's definition, such as a field.
 */
abstract class DslComponentGenerator<C : MessageComponent>(
  /** The type of component this generator can generate code for. */
  private val componentType: KClass<C>
) {

  /**
   * Generate declarations for the specified component, if this generator knows how to handle it.
   *
   * @param component The component to generate declarations for.
   * @param builderCode A [CodeBlock] to get access to the underlying
   *        [com.google.protobuf.Message.Builder] in this DSL.
   * @param dslBuilder A builder for the DSL class, to add declarations to.
   * @param companionBuilder A builder for the companion object of the DSL class, to add
   *        declarations to.
   * @param enclosingScope The scope containing the DSL class.
   * @param extensionScope The scope containing extensions for the message type.
   * @param dslType The fully qualified name of the DSL class.
   */
  fun GeneratorConfig.generateIfApplicable(
    component: MessageComponent,
    builderCode: CodeBlock,
    dslBuilder: TypeSpec.Builder,
    companionBuilder: TypeSpec.Builder,
    enclosingScope: Scope,
    extensionScope: Scope,
    dslType: TypeName
  ) {
    componentType.safeCast(component)?.let {
      generate(
        it, builderCode, dslBuilder, companionBuilder, enclosingScope, extensionScope, dslType
      )
    }
  }

  /**
   * Generate declarations for the specified component, which is of the type handled by this
   * generator.
   */
  abstract fun GeneratorConfig.generate(
    component: C,
    builderCode: CodeBlock,
    dslBuilder: TypeSpec.Builder,
    companionBuilder: TypeSpec.Builder,
    enclosingScope: Scope,
    extensionScope: Scope,
    dslType: TypeName
  )
}
