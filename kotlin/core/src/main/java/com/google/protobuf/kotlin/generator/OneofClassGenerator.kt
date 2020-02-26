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

import com.google.common.annotations.VisibleForTesting
import com.google.protobuf.Descriptors.OneofDescriptor
import com.google.protobuf.kotlin.OneofRepresentation
import com.google.protobuf.kotlin.protoc.ClassSimpleName
import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.google.protobuf.kotlin.protoc.OneofComponent
import com.google.protobuf.kotlin.protoc.OneofOptionFieldDescriptor
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.classBuilder
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.nestedClass
import com.google.protobuf.kotlin.protoc.objectBuilder
import com.google.protobuf.kotlin.protoc.of
import com.google.protobuf.kotlin.protoc.oneofName
import com.google.protobuf.kotlin.protoc.options
import com.google.protobuf.kotlin.protoc.propertySimpleName
import com.squareup.kotlinpoet.ANY
import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.MemberName
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.ParameterizedTypeName.Companion.parameterizedBy
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.STAR
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec
import com.squareup.kotlinpoet.TypeVariableName
import com.squareup.kotlinpoet.UNIT
import com.squareup.kotlinpoet.asTypeName

/**
 * Generates a class to represent a oneof for a message class, such as:
 *
 *     class MyOneof<T> {
 *       class OptionA(val value: Int): MyOneof<Int>
 *       class OptionB(val value: String): MyOneof<String>
 *       object NotSet: MyOneof<Unit>
 *     }
 *
 * ...and associated utilities to get and set that oneof with this representation.
 */
class OneofClassGenerator(
  val oneofClassSuffix: String = "",
  val oneofSubclassSuffix: String = "",
  val notSetSimpleName: ClassSimpleName = ClassSimpleName("NotSet"),
  val propertySuffix: String = "",
  vararg extensionFactories: OneofClassGenerator.() -> OneofClassExtensionGenerator
) : ComponentExtensionGenerator<OneofComponent>(OneofComponent::class) {
  companion object {
    private val VALUE_PROPERTY_NAME = MemberSimpleName("value")
    private val CASE_PROPERTY_NAME = MemberSimpleName("case")
    private val WRITE_TO_METHOD_NAME = MemberSimpleName("writeTo")
    private val READ_FROM_METHOD_NAME = MemberSimpleName("readFrom")
  }

  private val extensions by lazy {
    extensionFactories.map { extension -> this@OneofClassGenerator.extension() }
  }

  /** Simple name for the class for the oneof. */
  private val OneofDescriptor.classSimpleName
    get() = oneofName.toClassSimpleNameWithSuffix(oneofClassSuffix)

  /** Simple name for the subclass for this particular option of the oneof. */
  private val OneofOptionFieldDescriptor.classSimpleName: ClassSimpleName
    get() = fieldName.toClassSimpleNameWithSuffix(oneofSubclassSuffix)

  /**
   * Fully qualified name for the subclass for this particular option of the oneof.
   *
   * @param oneofClass The fully qualified name for the class for the oneof.
   */
  private fun OneofOptionFieldDescriptor.className(oneofClass: ClassName): ClassName =
    oneofClass.nestedClass(classSimpleName)

  /** Builds an extension function on the oneof class to copy its contents into a builder. */
  @VisibleForTesting
  fun GeneratorConfig.writeToFunction(oneof: OneofDescriptor): FunSpec {
    val builderParam = ParameterSpec.of("builder", oneof.containingType.builderClass())
    val typeParam = TypeVariableName("T", ANY)
    val valueProperty = PropertySpec
      .builder(VALUE_PROPERTY_NAME, typeParam, KModifier.OVERRIDE)
      .build()
    val caseProperty = PropertySpec
      .builder(CASE_PROPERTY_NAME, oneof.caseEnum(), KModifier.OVERRIDE)
      .build()

    val builder = funSpecBuilder(WRITE_TO_METHOD_NAME)
      .addParameter(builderParam)
      .returns(UNIT)
      .addModifiers(KModifier.INTERNAL)

    builder.beginControlFlow("when (%N)", caseProperty)

    for (option in oneof.options()) {
      // · represents a space which must not be broken
      builder.addStatement(
        "%M -> %N.%N·= %N as %T",
        option.caseEnumMember(),
        builderParam,
        option.property(),
        valueProperty,
        option.type()
      )
    }
    builder
      .addStatement("else -> %N.%N()", builderParam, oneof.builderClear())
      .endControlFlow()

    return builder.build()
  }

  /**
   * Builds a function to live on the oneof class' companion object to copy a message or builder's
   * oneof value into a oneof representation object.
   */
  @VisibleForTesting
  fun GeneratorConfig.readFromFunction(oneof: OneofDescriptor, oneofClassName: ClassName): FunSpec {
    val orBuilder = oneof.containingType.orBuilderClass()
    val orBuilderParam = ParameterSpec.of("input", orBuilder)

    val readFromBuilder =
      funSpecBuilder(READ_FROM_METHOD_NAME)
        .addModifiers(KModifier.INTERNAL)
        .addAnnotation(PublishedApi::class)
        .addKdoc(
          "Extract an object representation of the set field of the oneof %L.",
          oneof.oneofName
        )
        .returns(oneofClassName.parameterizedBy(STAR))
        .addParameter(orBuilderParam)

    // Copy the case to a typed local variable, which lets us convert it from either a
    // platform-nullable type or a nonnull type to a nonnull type without warnings.

    readFromBuilder.addStatement(
      "val _theCase: %T = %N.%N",
      oneof.caseEnum(),
      orBuilderParam,
      oneof.caseProperty()
    )

    readFromBuilder.beginControlFlow("return when (_theCase)")

    for (option in oneof.options()) {
      val optionProperty =
        PropertySpec
          .builder(simpleName = option.propertySimpleName, type = option.type())
          .receiver(orBuilder)
          .build()
      readFromBuilder.addStatement(
        "%M -> %T(%N.%N)",
        option.caseEnumMember(),
        option.className(oneofClassName),
        orBuilderParam,
        optionProperty
      )
    }
    readFromBuilder.addStatement(
      "else -> %T",
      oneofClassName.nestedClass(notSetSimpleName)
    ).endControlFlow()

    return readFromBuilder.build()
  }

  /** Generates a class for this option of the oneof. */
  @VisibleForTesting
  fun GeneratorConfig.oneofOptionClass(
    option: OneofOptionFieldDescriptor,
    oneofClassName: ClassName
  ): TypeSpec {
    val oneof = option.oneof
    val optionType = option.type()
    val valueParam = ParameterSpec.of(VALUE_PROPERTY_NAME, optionType)

    return TypeSpec
      .classBuilder(option.classSimpleName)
      .addAnnotations(Deprecation.apiAnnotations(option))
      .addKdoc(
        "A representation of the %L option of the %L oneof.",
        option.fieldName,
        oneof.oneofName
      )
      .superclass(oneofClassName.parameterizedBy(optionType))
      .primaryConstructor(
        FunSpec
          .constructorBuilder()
          .addParameter(valueParam)
          .build()
      )
      .addSuperclassConstructorParameter(valueParam.name)
      .addSuperclassConstructorParameter("%M", option.caseEnumMember())
      .build()
  }

  /** Generates the class (or object) to represent "this oneof is not set." */
  @VisibleForTesting
  fun GeneratorConfig.notSetClass(
    oneof: OneofDescriptor,
    oneofClassName: ClassName
  ): TypeSpec {
    return TypeSpec.objectBuilder(notSetSimpleName)
      .addSuperclassConstructorParameter("Unit")
      .addSuperclassConstructorParameter("%M", oneof.notSet())
      .superclass(oneofClassName.parameterizedBy(UNIT))
      .addKdoc("Represents the absence of a value for %L.", oneof.oneofName)
      .build()
  }

  override fun GeneratorConfig.componentExtensions(
    component: OneofComponent,
    enclosingScope: Scope,
    extensionScope: Scope
  ): Declarations = declarations {
    val oneof = component.descriptor
    val oneofClassSimpleName = oneof.classSimpleName
    val oneofClassName =
      scope(enclosingScope, extensionScope).nestedClass(oneofClassSimpleName)
    val typeParam = TypeVariableName("T", ANY)
    val caseEnum = oneof.caseEnum()

    val valueParam = ParameterSpec
      .builder(VALUE_PROPERTY_NAME, typeParam, KModifier.OVERRIDE)
      .build()
    val valueProperty = PropertySpec.builder(VALUE_PROPERTY_NAME, typeParam, KModifier.OVERRIDE)
      .initializer("%N", valueParam)
      .build()
    val caseParam = ParameterSpec
      .builder(CASE_PROPERTY_NAME, caseEnum, KModifier.OVERRIDE)
      .build()
    val caseProperty = PropertySpec.builder(CASE_PROPERTY_NAME, caseEnum, KModifier.OVERRIDE)
      .initializer("%N", caseParam)
      .build()

    val oneofClassBuilder =
      TypeSpec
        .classBuilder(oneofClassSimpleName)
        .addModifiers(KModifier.ABSTRACT)
        .addTypeVariable(typeParam)
        .addKdoc("Representation of a value for the oneof %L.", oneof.oneofName)
        .primaryConstructor(
          FunSpec
            .constructorBuilder()
            .addParameter(valueParam)
            .addParameter(caseParam)
            .build()
        )
        .addProperty(valueProperty)
        .addProperty(caseProperty)
        .superclass(OneofRepresentation::class.asTypeName().parameterizedBy(typeParam, caseEnum))
        .addFunction(writeToFunction(oneof))

    for (option in oneof.options()) {
      oneofClassBuilder.addType(oneofOptionClass(option, oneofClassName))
    }

    oneofClassBuilder.addType(notSetClass(oneof, oneofClassName))

    oneofClassBuilder
      .addType(
        TypeSpec
          .companionObjectBuilder()
          .addFunction(readFromFunction(oneof, oneofClassName))
          .build()
      )

    addType(oneofClassBuilder.build())

    for (extension in extensions) {
      extension.run {
        merge(generate(oneof, oneofClassName))
      }
    }
  }

  /** A generator for extension properties that make use of the generated oneof class. */
  abstract inner class OneofClassExtensionGenerator {
    abstract fun GeneratorConfig.generate(
      oneof: OneofDescriptor,
      oneofClass: ClassName
    ): Declarations
  }

  /** Generates a val property for this oneof on the message's OrBuilder interface. */
  val orBuilderVal = object : OneofClassExtensionGenerator() {
    override fun GeneratorConfig.generate(
      oneof: OneofDescriptor,
      oneofClass: ClassName
    ) = declarations {
      addTopLevelProperty(
        PropertySpec
          .builder(
            oneof.propertySimpleName.withSuffix(propertySuffix),
            oneofClass.parameterizedBy(STAR)
          )
          .receiver(oneof.containingType.orBuilderClass())
          .getter(
            getterBuilder()
              .addStatement("return %T.%L(this)", oneofClass, READ_FROM_METHOD_NAME)
              .build()
          )
          .addKdoc("The value of the oneof %L.", oneof.oneofName)
          .build()
      )
    }
  }

  /** Generates a mutable var property for this oneof on the message builder class. */
  val builderVar = object : OneofClassExtensionGenerator() {
    override fun GeneratorConfig.generate(
      oneof: OneofDescriptor,
      oneofClass: ClassName
    ) = declarations {
      val newOneofParam = ParameterSpec.of("newOneofParam", oneofClass.parameterizedBy(STAR))
      addTopLevelProperty(
        PropertySpec
          .builder(
            oneof.propertySimpleName.withSuffix(propertySuffix),
            oneofClass.parameterizedBy(STAR)
          )
          .getter(
            getterBuilder()
              .addStatement("return %T.%L(this)", oneofClass, READ_FROM_METHOD_NAME)
              .build()
          )
          .addKdoc("The value of the oneof %L.", oneof.oneofName)
          .receiver(oneof.containingType.builderClass())
          .mutable(true)
          .setter(
            setterBuilder()
              .addParameter(newOneofParam)
              .addStatement("%N.%L(this)", newOneofParam, WRITE_TO_METHOD_NAME)
              .build()
          )
          .build()
      )
    }
  }

  /** Generates a DSL property for this oneof using the generated oneof class. */
  val dslPropertyGenerator: DslComponentGenerator<OneofComponent> =
    object : DslComponentGenerator<OneofComponent>(OneofComponent::class) {
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
        val oneofClass = oneofClass(oneof, enclosingScope, extensionScope)
        val propertyName = oneof.propertySimpleName.withSuffix(propertySuffix)
        val setterParam = ParameterSpec.of("newValue", oneofClass)

        dslBuilder.addProperty(
          PropertySpec
            .builder(
              propertyName,
              oneofClass.parameterizedBy(STAR)
            )
            .mutable(true)
            .addKdoc("An object representing a value assigned to the oneof %L.", oneof.oneofName)
            .getter(
              getterBuilder()
                .addStatement("return %T.%L(%L)", oneofClass, READ_FROM_METHOD_NAME, builderCode)
                .build()
            )
            .setter(
              setterBuilder()
                .addParameter(setterParam)
                .addStatement("%N.%L(%L)", setterParam, WRITE_TO_METHOD_NAME, builderCode)
                .build()
            )
            .build()
        )
      }
    }

  /**
   * Returns the fully qualified name of the oneof class generated by this generator for
   * the specified oneof.
   */
  private fun oneofClass(
    oneof: OneofDescriptor,
    enclosingScope: Scope,
    extensionScope: Scope
  ): ClassName = scope(enclosingScope, extensionScope).nestedClass(oneof.classSimpleName)
}
