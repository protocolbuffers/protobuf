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
import com.google.protobuf.kotlin.ProtoDsl
import com.google.protobuf.kotlin.ProtoDslMarker
import com.google.protobuf.kotlin.protoc.ClassSimpleName
import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.google.protobuf.kotlin.protoc.OneofComponent
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.classBuilder
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.messageClassSimpleName
import com.google.protobuf.kotlin.protoc.of
import com.google.protobuf.kotlin.protoc.specialize
import com.squareup.kotlinpoet.ANY
import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.LambdaTypeName
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.ParameterizedTypeName.Companion.parameterizedBy
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeSpec
import com.squareup.kotlinpoet.UNIT
import com.squareup.kotlinpoet.asTypeName

/** Generates a DSL class and factory method for a message. */
class DslGenerator(
  private val extendableDslSuperclass: ClassName,
  /** Any suffix, e.g. "Dsl", to add to the DSL class for the message type. */
  private val dslClassNameSuffix: String = "Dsl",
  /** Any prefix, e.g. "create", to add to the factory method for the message type. */
  private val dslFactoryNamePrefix: String = "",
  /** If a "copy with modifications" method should be generated, what to name it. */
  private val copyName: MemberSimpleName? = null,
  /** Generators for the members of the DSL. */
  private vararg val componentGenerators: DslComponentGenerator<*>
) : ExtensionGenerator() {

  companion object {
    // so as not to conflict with any proto fields named builder
    private val BUILDER_SIMPLE_NAME = MemberSimpleName("builder_")

    private val DEFAULT_DSL_SUPERCLASS = ProtoDsl::class.asTypeName()

    private const val DEFAULT_DSL_BUILDER_PROPERTY_NAME = "_proto_dsl_builder"
  }

  /** Generate the DSL as a peer of the extension class, not inside it. */
  override val generateWithinExtensionClass: Boolean
    get() = false

  /** A simple name for the DSL class for this message. */
  private val Descriptor.dslClassSimpleName: ClassSimpleName
    get() = messageClassSimpleName.withSuffix(dslClassNameSuffix)

  /** A simple name for the factory method for this message. */
  private val Descriptor.dslFactorySimpleName: MemberSimpleName
    get() = messageClassSimpleName.asMemberWithPrefix(dslFactoryNamePrefix)

  private fun TypeSpec.isEmpty(): Boolean =
    propertySpecs.isEmpty() &&
      funSpecs.isEmpty() &&
      typeSpecs.isEmpty() &&
      superinterfaces.isEmpty() &&
      superclass == ANY

  override fun GeneratorConfig.shallowExtensions(
    descriptor: Descriptor,
    enclosingScope: Scope,
    extensionScope: Scope
  ): Declarations =
    declarations {
      val builderClass = descriptor.builderClass()
      val messageClass = descriptor.messageClass()

      // what is the DSL class' name and what scope is it defined in?
      val dslClassSimpleName: ClassSimpleName = descriptor.dslClassSimpleName
      val dslScope: Scope = scope(enclosingScope, extensionScope)

      val dslClass = dslScope.nestedClass(dslClassSimpleName)

      // the constructor parameter for the underlying builder
      val builderParam = ParameterSpec.of(BUILDER_SIMPLE_NAME, builderClass)

      val dslClassSpecBuilder =
        TypeSpec.classBuilder(dslClassSimpleName)
          .addAnnotation(ProtoDslMarker::class)
          .primaryConstructor(
            FunSpec.constructorBuilder()
              .addModifiers(KModifier.INTERNAL)
              .addAnnotation(PublishedApi::class)
              .addParameter(builderParam)
              .build()
          )
          .addKdoc(
            "A DSL type for creating %L messages.  To use it, call the [%L] method.",
            descriptor.name,
            descriptor.dslFactorySimpleName
          )

      if (descriptor.isExtendable) {
        dslClassSpecBuilder
          .superclass(extendableDslSuperclass.parameterizedBy(messageClass, builderClass))
      } else {
        dslClassSpecBuilder
          .superclass(DEFAULT_DSL_SUPERCLASS.parameterizedBy(builderClass))
      }
      dslClassSpecBuilder.addSuperclassConstructorParameter("%N", builderParam)

      val dslBuilderProperty =
        PropertySpec.builder(BUILDER_SIMPLE_NAME, builderClass)
          .addModifiers(KModifier.INTERNAL)
          .addAnnotation(PublishedApi::class)
          .getter(
            FunSpec.getterBuilder()
              .addStatement("return %L", DEFAULT_DSL_BUILDER_PROPERTY_NAME)
              .build()
          )
          .build()
      dslClassSpecBuilder.addProperty(dslBuilderProperty)
      val dslBuilderCode = CodeBlock.of("%N", dslBuilderProperty)

      val dslBuildFun =
        FunSpec.builder("build")
          .returns(messageClass)
          .addModifiers(KModifier.INTERNAL)
          .addAnnotation(PublishedApi::class)
          // to allow the public inline factory method to call it
          .addStatement("return %N.build()", dslBuilderProperty)
          .addKdoc("Builds the %L.", descriptor.name)
          .build()
      dslClassSpecBuilder.addFunction(dslBuildFun)

      val companionObjectBuilder = TypeSpec.companionObjectBuilder()

      // pieces of the message to generate API for
      val components =
        descriptor.fields.map { it.specialize() } + descriptor.oneofs.map(::OneofComponent)

      for (generator in componentGenerators) {
        generator.run {
          for (component in components) {
            generateIfApplicable(
              component,
              dslBuilderCode,
              dslClassSpecBuilder,
              companionObjectBuilder,
              enclosingScope,
              extensionScope,
              dslClass
            )
          }
        }
      }

      // add in the companion object
      val companionObject = companionObjectBuilder.build()
      if (!companionObject.isEmpty()) {
        // avoiding companion objects makes it easier to optimize (no static initializers), so
        // don't add one unless necessary.
        dslClassSpecBuilder.addType(companionObject)
      }

      val dslClassSpec = dslClassSpecBuilder.build()

      // parameter for the DSL factory method
      val dslFactoryMethodCallback = ParameterSpec.of(
        "block",
        LambdaTypeName.get(
          receiver = dslClass,
          returnType = UNIT
        )
      )

      addFunction(
        FunSpec
          .builder(descriptor.dslFactorySimpleName)
          .addModifiers(KModifier.INLINE)
          .addParameter(dslFactoryMethodCallback)
          .addAnnotation(JvmSynthetic::class) // uncallable from Java
          .addKdoc("Entry point for creating %L messages in Kotlin.", descriptor.name)
          .addStatement(
            "return %N(%T.newBuilder()).apply(%N).%N()",
            dslClassSpec,
            messageClass,
            dslFactoryMethodCallback,
            dslBuildFun
          )
          .build()
      )

      if (copyName != null) {
        addTopLevelFunction(
          FunSpec
            .builder(copyName)
            .addModifiers(KModifier.INLINE)
            .addAnnotation(JvmSynthetic::class) // uncallable from Java
            .receiver(messageClass)
            .returns(messageClass)
            .addParameter(dslFactoryMethodCallback)
            .addKdoc(
              "Creates a %T equivalent to this one, but with the specified modifications.",
              messageClass
            )
            .addStatement(
              "return %T(toBuilder()).apply(%N).%N()",
              dslClass,
              dslFactoryMethodCallback,
              dslBuildFun
            )
            .build()
        )
      }

      addType(dslClassSpec)
    }
}
