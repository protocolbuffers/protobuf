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
import com.google.protobuf.kotlin.protoc.OneofComponent
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.casePropertySimpleName
import com.google.protobuf.kotlin.protoc.oneofName
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec

/**
 * Generates the property `MessageDsl.oneofCase` to match the one built into the message type.
 */
object OneofCaseDslComponentGenerator :
  DslComponentGenerator<OneofComponent>(OneofComponent::class) {
  override fun GeneratorConfig.generate(
    component: OneofComponent,
    builderCode: CodeBlock,
    dslBuilder: TypeSpec.Builder,
    companionBuilder: TypeSpec.Builder,
    enclosingScope: Scope,
    extensionScope: Scope,
    dslType: TypeName
  ) {
    val oneof = component.descriptor
    val caseEnum = oneof.caseEnum()
    dslBuilder.addProperty(
      PropertySpec
        .builder(oneof.casePropertySimpleName, caseEnum)
        .addKdoc("An enum reflecting which field of the oneof %L is set.", oneof.oneofName)
        .getter(
          getterBuilder()
            .addStatement("return %L.%N", builderCode, oneof.caseProperty())
            .build()
        )
        .build()
    )
  }
}
