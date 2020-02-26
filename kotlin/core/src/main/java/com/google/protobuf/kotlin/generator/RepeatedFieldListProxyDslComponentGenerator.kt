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
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.RepeatedFieldDescriptor
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.UnqualifiedScope
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.classBuilder
import com.google.protobuf.kotlin.protoc.fieldName
import com.squareup.kotlinpoet.AnnotationSpec
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.ParameterizedTypeName.Companion.parameterizedBy
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec
import com.squareup.kotlinpoet.asTypeName

/**
 * Generates a DSL property for a repeated field, of type `DslList<ElementType, ThisRepeatedField>`,
 * which is a thin unmodifiable wrapper around the list from the backing builder.  In addition,
 * provides extension methods for that type that modify the contents of that repeated field.
 */
class RepeatedFieldListProxyDslComponentGenerator(
  private val proxyTypeNameSuffix: String,
  private val propertyNameSuffix: String = "",
  private vararg val extensionGenerators: RepeatedRepresentativeExtensionGenerator
) : DslComponentGenerator<RepeatedFieldDescriptor>(RepeatedFieldDescriptor::class) {

  override fun GeneratorConfig.generate(
    component: RepeatedFieldDescriptor,
    builderCode: CodeBlock,
    dslBuilder: TypeSpec.Builder,
    companionBuilder: TypeSpec.Builder,
    enclosingScope: Scope,
    extensionScope: Scope,
    dslType: TypeName
  ) {
    val proxyClassSimpleName = component.fieldName.toClassSimpleNameWithSuffix(proxyTypeNameSuffix)

    dslBuilder.addType(
      TypeSpec
        .classBuilder(proxyClassSimpleName)
        .addKdoc(
          "An uninstantiable, behaviorless type to represent the field %L in generics.",
          component.fieldName
        )
        .primaryConstructor(
          FunSpec
            .constructorBuilder()
            .addModifiers(KModifier.PRIVATE)
            .build()
        )
        .build()
    )

    val proxyClassName = UnqualifiedScope.nestedClass(proxyClassSimpleName)

    val propertyType =
      DslList::class.asTypeName().parameterizedBy(component.elementType(), proxyClassName)
    val propertyName = component.propertySimpleName.withSuffix(propertyNameSuffix)
    dslBuilder.addProperty(
      PropertySpec
        .builder(propertyName, propertyType)
        .addKdoc(
          """
          |A [%T] view of the repeated field %L.
          |
          |While the view object itself is a `List`, not a `MutableList`, within the context of the
          |DSL it has extension methods and operator overloads that mutate it appropriately.
          |For example, writing `%L += element` adds that element to the repeated field, but that
          |code only compiles within the DSL.
        """.trimMargin(),
          List::class,
          component.descriptor.fieldName,
          propertyName
        )
        .addAnnotations(Deprecation.apiAnnotations(component))
        .getter(
          FunSpec
            .getterBuilder()
            .addModifiers(KModifier.INLINE)
            .addStatement(
              "return %T(%L.%L())",
              DslList::class,
              builderCode,
              component.listGetterSimpleName()
            )
            .build()
        )
        .build()
    )

    for (extensionGenerator in extensionGenerators) {
      extensionGenerator.run {
        generate(
          { name ->
            funSpecBuilder(name)
              .receiver(propertyType)
              .addAnnotation(
                AnnotationSpec
                  .builder(JvmName::class)
                  .addMember("%S", name + component.propertySimpleName)
                  .build()
              )
          },
          component,
          builderCode,
          propertyName
        ).writeToEnclosingType(dslBuilder)
      }
    }
  }
}
