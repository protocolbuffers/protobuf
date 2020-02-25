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
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.SingletonFieldDescriptor
import com.google.protobuf.kotlin.protoc.fieldName
import com.squareup.kotlinpoet.BOOLEAN
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec

/** Generates hazzers in the DSL for fields that have them in the builder. */
object SingletonFieldHazzerDslComponentGenerator :
  DslComponentGenerator<SingletonFieldDescriptor>(SingletonFieldDescriptor::class) {

  override fun GeneratorConfig.generate(
    component: SingletonFieldDescriptor,
    builderCode: CodeBlock,
    dslBuilder: TypeSpec.Builder,
    companionBuilder: TypeSpec.Builder,
    enclosingScope: Scope,
    extensionScope: Scope,
    dslType: TypeName
  ) {
    component.hazzer()?.let { hazzer ->
      dslBuilder.addFunction(
        funSpecBuilder(component.hazzerSimpleName)
          .returns(BOOLEAN)
          .addStatement("return %L.%N()", builderCode, hazzer)
          .addKdoc("True if the %L field has been set.", component.descriptor.fieldName)
          .build()
      )
    }
  }
}
