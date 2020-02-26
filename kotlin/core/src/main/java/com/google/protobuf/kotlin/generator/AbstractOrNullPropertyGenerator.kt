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

import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.SingletonFieldDescriptor
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.of
import com.google.protobuf.kotlin.protoc.simpleName
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec

/** Superclass of generators for OrNull properties. */
abstract class AbstractOrNullPropertyGenerator {

  abstract fun shouldGenerate(
    field: SingletonFieldDescriptor
  ): Boolean

  /** Generate logic for "does <receiver> have this field set." */
  abstract fun GeneratorConfig.has(
    receiver: CodeBlock,
    field: SingletonFieldDescriptor
  ): CodeBlock

  private fun GeneratorConfig.orNullProperty(
    field: SingletonFieldDescriptor,
    receiverTy: TypeName?,
    receiverCode: CodeBlock,
    mutable: Boolean
  ): PropertySpec {
    val orNullName = field.propertySimpleName.withSuffix("OrNull")
    val orNullType = field.type().copy(nullable = true)
    val builder =
      PropertySpec
        .builder(orNullName, orNullType)
        .addKdoc(
          "The %L field of the proto %L, or null if it isn't set.",
          field.fieldName, field.containingType.simpleName
        )

    receiverTy?.let { builder.receiver(it) }

    builder.getter(
      getterBuilder()
        .addStatement(
          "return if (%L) %L.%N else null",
          has(receiverCode, field),
          receiverCode,
          field.property()
        )
        .build()
    )

    if (mutable) {
      val param = ParameterSpec.of("_newValue", orNullType)
      builder.mutable(true)
      builder.setter(
        setterBuilder()
          .addParameter(param)
          .beginControlFlow("if (%N == null)", param)
          .addStatement("%L.%L()", receiverCode, field.propertySimpleName.withPrefix("clear"))
          .nextControlFlow("else")
          .addStatement("%L.%N = %N", receiverCode, field.property(), param)
          .endControlFlow()
          .build()
      )
    }

    return builder.build()
  }

  val forMessageOrBuilder: ComponentExtensionGenerator<SingletonFieldDescriptor> =
    ComponentGenerator(
      receiverType = { it.orBuilderClass() },
      generateIfRequired = false,
      mutable = false
    )

  val forBuilder: ComponentExtensionGenerator<SingletonFieldDescriptor> =
    ComponentGenerator(
      receiverType = { it.builderClass() },
      generateIfRequired = true,
      mutable = true
    )

  private inner class ComponentGenerator(
    val receiverType: GeneratorConfig.(Descriptor) -> TypeName,
    val mutable: Boolean,
    val generateIfRequired: Boolean
  ) : ComponentExtensionGenerator<SingletonFieldDescriptor>(SingletonFieldDescriptor::class) {
    override fun GeneratorConfig.componentExtensions(
      component: SingletonFieldDescriptor,
      enclosingScope: Scope,
      extensionScope: Scope
    ): Declarations = declarations {
      if (shouldGenerate(component) && (generateIfRequired || !component.isRequired())) {
        addTopLevelProperty(
          orNullProperty(
            component,
            receiverType(component.containingType),
            CodeBlock.of("this"),
            mutable
          )
        )
      }
    }
  }

  val forDsl: DslComponentGenerator<SingletonFieldDescriptor> =
    object : DslComponentGenerator<SingletonFieldDescriptor>(SingletonFieldDescriptor::class) {
      override fun GeneratorConfig.generate(
        component: SingletonFieldDescriptor,
        builderCode: CodeBlock,
        dslBuilder: TypeSpec.Builder,
        companionBuilder: TypeSpec.Builder,
        enclosingScope: Scope,
        extensionScope: Scope,
        dslType: TypeName
      ) {
        if (shouldGenerate(component)) {
          dslBuilder.addProperty(
            orNullProperty(
              field = component,
              mutable = true,
              receiverCode = builderCode,
              receiverTy = null // instance property
            )
          )
        }
      }
    }
}
