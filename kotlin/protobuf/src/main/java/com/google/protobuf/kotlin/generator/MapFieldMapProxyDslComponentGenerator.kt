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
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.MapFieldDescriptor
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
 * Generates a DSL property for a map field, of type `DslMap<KeyType, ValueType, ThisMapField>`,
 * which is a thin unmodifiable wrapper around the map from the backing builder.  In addition,
 * provides extension methods for that type that modify the contents of that map field.
 */
class MapFieldMapProxyDslComponentGenerator(
  private val proxyTypeNameSuffix: String = "Proxy",
  private val propertyNameSuffix: String = "",
  private vararg val extensionGenerators: MapRepresentativeExtensionGenerator
) : DslComponentGenerator<MapFieldDescriptor>(MapFieldDescriptor::class) {

  override fun GeneratorConfig.generate(
    component: MapFieldDescriptor,
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
          "A behaviorless, uninstantiable type to represent the field %L in generics.",
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
      DslMap::class
        .asTypeName()
        .parameterizedBy(component.keyType(), component.valueType(), proxyClassName)

    val propertyName = component.propertySimpleName.withSuffix(propertyNameSuffix)
    dslBuilder.addProperty(
      PropertySpec
        .builder(propertyName, propertyType)
        .addKdoc(
          """
          |A [%T] view of the map field %L.
          |
          |While the view object itself is a `Map`, not a `MutableMap`, within the context of the
          |DSL it has extension methods and operator overloads that mutate it appropriately.
          |For example, writing `%L[k] = v` adds a mapping from `k` to `v` in the map field,
          |but that code only compiles within the DSL.
        """.trimMargin(),
          Map::class,
          component.descriptor.fieldName,
          component.propertySimpleName
        )
        .addAnnotations(Deprecation.apiAnnotations(component))
        .getter(
          getterBuilder()
            .addStatement(
              "return %T(%L.%L())",
              DslMap::class,
              builderCode,
              component.mapGetterSimpleName
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
