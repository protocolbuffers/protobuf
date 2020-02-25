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

package com.google.protobuf.kotlin.protoc

import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.Descriptors.EnumDescriptor
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.Descriptors.FieldDescriptor.JavaType
import com.google.protobuf.Descriptors.FileDescriptor
import com.google.protobuf.Descriptors.OneofDescriptor
import com.squareup.kotlinpoet.BOOLEAN
import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.MemberName
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeName

/**
 * Configuration for proto code generation, including settings on inlining and on the mapping
 * between protos and Java packages.
 */
data class GeneratorConfig(
  val javaPackagePolicy: JavaPackagePolicy,
  val aggressiveInlining: Boolean
) {

  private val inlineModifiers: Array<KModifier> =
    if (aggressiveInlining) arrayOf(KModifier.INLINE) else arrayOf()

  /** Generates a [FunSpec.Builder] with appropriate modifiers. */
  fun funSpecBuilder(name: MemberSimpleName): FunSpec.Builder =
    FunSpec.builder(name).addModifiers(*inlineModifiers)

  /** Generates a [FunSpec.Builder] for a getter with appropriate modifiers. */
  fun getterBuilder(): FunSpec.Builder =
    FunSpec.getterBuilder().addModifiers(*inlineModifiers)

  /** Generates a [FunSpec.Builder] for a setter with appropriate modifiers. */
  fun setterBuilder(): FunSpec.Builder =
    FunSpec.setterBuilder().addModifiers(*inlineModifiers)

  /** Returns the package associated with Java APIs for protos in the specified file. */
  fun javaPackage(fileDescriptor: FileDescriptor): PackageScope =
    javaPackagePolicy.javaPackage(fileDescriptor.toProto())

  // Helpers on FileDescriptor.

  /** Returns the fully qualified name of the outer class generated for this proto file. */
  fun FileDescriptor.outerClass(): ClassName = javaPackage(this).nestedClass(outerClassSimpleName)

  // Helpers on EnumDescriptor.

  /** Returns the fully qualified name of the JVM enum type generated for this proto enum. */
  fun EnumDescriptor.enumClass(): ClassName {
    val contType: Descriptor? = containingType
    return when {
      contType != null -> contType.messageClass().nestedClass(enumClassSimpleName)
      file.options.javaMultipleFiles -> javaPackage(file).nestedClass(enumClassSimpleName)
      else -> file.outerClass().nestedClass(enumClassSimpleName)
    }
  }

  // Helpers on Descriptor.

  /** Returns the fully qualified name of the JVM class generated for this message type. */
  fun Descriptor.messageClass(): ClassName {
    val contType: Descriptor? = containingType
    return when {
      contType != null -> contType.messageClass().nestedClass(messageClassSimpleName)
      file.options.javaMultipleFiles -> javaPackage(file).nestedClass(messageClassSimpleName)
      else -> file.outerClass().nestedClass(messageClassSimpleName)
    }
  }

  /**
   * Returns the fully qualified name of the JVM class for builders of messages of this type.
   */
  fun Descriptor.builderClass(): ClassName = messageClass().nestedClass("Builder")

  /**
   * Returns the fully qualified name of the JVM interface referring to messages or builders for
   * this type.
   */
  fun Descriptor.orBuilderClass(): ClassName =
    messageClassSimpleName.withSuffix("OrBuilder").asPeer(messageClass())

  // Helpers on field descriptors.

  private fun fieldType(fieldDescriptor: FieldDescriptor): TypeName =
    when (fieldDescriptor.javaType!!) {
      JavaType.MESSAGE -> fieldDescriptor.messageType.messageClass()
      JavaType.ENUM -> fieldDescriptor.enumType.enumClass()
      else -> fieldDescriptor.scalarType
    }

  // Helpers on singleton fields.

  /** Returns the fully qualified Kotlin type of values of this field. */
  fun SingletonFieldDescriptor.type(): TypeName = fieldType(descriptor)

  /**
   * Returns the Kotlin property for this field, inferred from the getter on the
   * associated OrBuilder interface.
   *
   * Currently a [PropertySpec] to work around https://github.com/square/kotlinpoet/issues/667.
   */
  fun SingletonFieldDescriptor.property(): PropertySpec =
    PropertySpec.builder(simpleName = propertySimpleName, type = type())
      .receiver(containingType.orBuilderClass())
      .build()

  /** Returns the fully qualified hazzer method of this field, if it has one. */
  fun SingletonFieldDescriptor.hazzer(): FunSpec? =
    if (hasHazzer()) {
      FunSpec
        .builder(simpleName = hazzerSimpleName)
        .returns(BOOLEAN)
        .receiver(containingType.orBuilderClass())
        .build()
    } else {
      null
    }

  // Helpers on repeated fields.

  /** Returns the fully qualified Kotlin type of elements of this repeated field. */
  fun RepeatedFieldDescriptor.elementType(): TypeName = fieldType(descriptor)

  // Helpers on map fields.

  /** Returns the fully qualified Kotlin type of keys of this map field. */
  fun MapFieldDescriptor.keyType(): TypeName = keyFieldDescriptor.scalarType

  /** Returns the fully qualified Kotlin type of values of this map field. */
  fun MapFieldDescriptor.valueType(): TypeName = fieldType(valueFieldDescriptor)

  // Helpers on oneof fields.

  /** Returns the fully qualified type of the enum for the case of this oneof. */
  fun OneofDescriptor.caseEnum(): ClassName =
    containingType.messageClass().nestedClass(caseEnumSimpleName)

  /**
   * Returns the Kotlin property for the case of this oneof, inferred from the getter on the
   * associated OrBuilder interface.
   *
   * (This is a [PropertySpec] to work around https://github.com/square/kotlinpoet/issues/667 .)
   */
  fun OneofDescriptor.caseProperty(): PropertySpec =
    PropertySpec.builder(simpleName = casePropertySimpleName, type = caseEnum())
      .receiver(containingType.orBuilderClass())
      .build()

  /** Returns the fully qualified enum constant for the "not set" case for this oneof. */
  fun OneofDescriptor.notSet(): MemberName = caseEnum().member(notSetCaseSimpleName)

  /** Returns the fully qualified enum constant for this case of the associated oneof. */
  fun OneofOptionFieldDescriptor.caseEnumMember(): MemberName =
    oneof.caseEnum().member(enumConstantName)

  /** Returns the Kotlin function that clears this oneof in the builder. */
  fun OneofDescriptor.builderClear(): FunSpec =
    FunSpec.builder(simpleName = propertySimpleName.withPrefix("clear"))
      .receiver(containingType.builderClass())
      .build()
}
