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
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.of
import com.squareup.kotlinpoet.AnnotationSpec
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec

/**
 * Generates a property for a singleton field in the DSL that simply delegates to the builder.
 * `var myField: FieldType get() = builder.myField set(myValue) { builder.myField = myValue }`
 */
object SingletonFieldPropertyDslComponentGenerator :
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
    val type = component.type()
    val newValueParam = ParameterSpec.of("newValue", type)
    val propertyGetterName = component.getterSimpleName
    val propertySetterName = component.setterSimpleName

    // We explicitly assign JvmNames to the getter and setter to avoid naming conflicts,
    // and explicitly call the getter and setter instead of using the inferred property for the same
    // reason.

    dslBuilder.addProperty(
      PropertySpec
        .builder(component.propertySimpleName, type)
        .mutable(true)
        .addAnnotations(Deprecation.apiAnnotations(component))
        .addKdoc("The %L field.", component)
        .getter(
          getterBuilder()
            .addAnnotation(
              AnnotationSpec.builder(JvmName::class)
                .addMember("%S", propertyGetterName.toString())
                .build()
            )
            .addStatement("return %L.%L()", builderCode, propertyGetterName)
            .build()
        )
        .setter(
          setterBuilder()
            .addAnnotation(
              AnnotationSpec.builder(JvmName::class)
                .addMember("%S", propertySetterName.toString())
                .build()
            )
            .addParameter(newValueParam)
            .addStatement("%L.%L(%N)", builderCode, propertySetterName, newValueParam)
            .build()
        )
        .build()
    )
  }
}
