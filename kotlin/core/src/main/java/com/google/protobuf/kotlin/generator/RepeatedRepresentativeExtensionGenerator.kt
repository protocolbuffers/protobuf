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

import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.MemberSimpleName
import com.google.protobuf.kotlin.protoc.RepeatedFieldDescriptor
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.of
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.ParameterizedTypeName.Companion.parameterizedBy
import com.squareup.kotlinpoet.UNIT
import com.squareup.kotlinpoet.asTypeName

/** A generator for extension methods on a type representing a repeated field of a proto message. */
abstract class RepeatedRepresentativeExtensionGenerator private constructor() {
  /**
   * Generates an extension function that delegates its implementation to modifying the appropriate
   * repeated field in a backing builder.
   *
   * @param funSpec Creates a [FunSpec.Builder] for a method with the specified name, with an
   *        appropriate receiver type.
   * @param repeatedField Indicates which repeated field to modify in the backing builder.
   * @param builderCode Code to access the backing builder.
   */
  abstract fun GeneratorConfig.generate(
    funSpec: (MemberSimpleName) -> FunSpec.Builder,
    repeatedField: RepeatedFieldDescriptor,
    builderCode: CodeBlock,
    proxyPropertyName: MemberSimpleName
  ): Declarations

  companion object {
    private val ITERABLE = Iterable::class.asTypeName()

    protected val indexParam = ParameterSpec.of("index", INT)

    protected fun GeneratorConfig.newValueParameter(
      repeatedField: RepeatedFieldDescriptor
    ): ParameterSpec = ParameterSpec.of("newValue", repeatedField.elementType())
  }

  /** Generates an extension function to provide the set operator. */
  object SetOperator : RepeatedRepresentativeExtensionGenerator() {
    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      repeatedField: RepeatedFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val newValueParam = newValueParameter(repeatedField)
      addFunction(
        funSpec(MemberSimpleName.OPERATOR_SET)
          .addModifiers(KModifier.OPERATOR)
          .returns(UNIT)
          .addParameter(indexParam)
          .addParameter(newValueParam)
          .addKdoc(
            """
            Sets the element of the repeated field %L at a given index.

            For example, `%L[i] = value` sets the `i`th element of %L to `value`.
            """.trimIndent(),
            repeatedField.fieldName,
            proxyPropertyName,
            repeatedField.fieldName
          )
          .addStatement(
            "%L.%L(%N, %N)",
            builderCode,
            repeatedField.setterSimpleName,
            indexParam,
            newValueParam
          )
          .build()
      )
    }
  }

  /** Generates an extension function to provide a function to add a single element. */
  class AddElement private constructor(
    private val name: MemberSimpleName,
    /** CodeBlock template that, given a %L parameter, adds an element named `value` to it. */
    private val addValue: String,
    private vararg val modifiers: KModifier
  ) : RepeatedRepresentativeExtensionGenerator() {
    companion object {
      /** Generates a normal function with the name `add`. */
      val add = AddElement(MemberSimpleName("add"), "%L.add(value)")

      val plusAssign = AddElement(
        MemberSimpleName.OPERATOR_PLUS_ASSIGN, "%L += value", KModifier.OPERATOR
      )
    }

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      repeatedField: RepeatedFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val newValueParam = newValueParameter(repeatedField)
      addFunction(
        funSpec(this@AddElement.name)
          .addModifiers(*modifiers)
          .returns(UNIT)
          .addKdoc(
            """
            Adds an element to the repeated field %L.

            For example, `$addValue` adds `value` to %L.
            """.trimIndent(),
            repeatedField.fieldName,
            proxyPropertyName,
            repeatedField.fieldName
          )
          .addParameter(newValueParam)
          .addStatement(
            "%L.%L(%N)",
            builderCode,
            repeatedField.adderSimpleName,
            newValueParam
          )
          .build()
      )
    }
  }

  /** Generates an extension function to add an iterable of elements. */
  class AddAllIterable private constructor(
    private val name: MemberSimpleName,
    /** CodeBlock template that, given a %L parameter, adds an iterable named `values` to it. */
    private val addAllValues: String,
    private vararg val modifiers: KModifier
  ) : RepeatedRepresentativeExtensionGenerator() {
    companion object {
      /** Generates a normal function with the name `addAll`. */
      val addAll = AddAllIterable(MemberSimpleName("addAll"), "%L.addAll(values)")

      /** Generates an operator function for the `+=` operator, adding an iterable of elements. */
      val plusAssign = AddAllIterable(
        MemberSimpleName.OPERATOR_PLUS_ASSIGN, "%L += values", KModifier.OPERATOR
      )
    }

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      repeatedField: RepeatedFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val newValuesParam =
        ParameterSpec
          .of("newValues", ITERABLE.parameterizedBy(repeatedField.elementType()))
      addFunction(
        funSpec(this@AddAllIterable.name)
          .addModifiers(*modifiers)
          .returns(UNIT)
          .addKdoc(
            """
            Adds elements to the repeated field %L.

            For example, `$addAllValues` adds each of the elements of `values`, in order, to %L.
            """.trimIndent(),
            repeatedField.fieldName,
            proxyPropertyName,
            repeatedField.fieldName
          )
          .addParameter(newValuesParam)
          .addStatement(
            "%L.%L(%N)",
            builderCode,
            repeatedField.addAllSimpleName,
            newValuesParam
          )
          .build()
      )
    }
  }

  /** Generates an extension function, named `addAll`, to add a vararg set of elements. */
  object AddAllVararg : RepeatedRepresentativeExtensionGenerator() {
    private val addAllSimpleName = MemberSimpleName("addAll")

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      repeatedField: RepeatedFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      val newValuesParam =
        ParameterSpec.of("newValues", repeatedField.elementType(), KModifier.VARARG)

      addFunction(
        funSpec(addAllSimpleName)
          .returns(UNIT)
          .addKdoc(
            """
            Add some elements to the repeated field %L.

            For example, `%L.addAll(v1, v2, v3)` adds `v1`, `v2`, and `v3`, in order, to %L.
            """.trimIndent(),
            repeatedField.fieldName,
            proxyPropertyName,
            repeatedField.fieldName
          )
          .addParameter(newValuesParam)
          .addStatement(
            "%L.%L(%N.asList())",
            builderCode,
            repeatedField.addAllSimpleName,
            newValuesParam
          )
          .build()
      )
    }
  }

  /**
   * Generates an extension function, named `clear`, to clear the contents of the repeated field.
   */
  object Clear : RepeatedRepresentativeExtensionGenerator() {
    private val clearSimpleName = MemberSimpleName("clear")

    override fun GeneratorConfig.generate(
      funSpec: (MemberSimpleName) -> FunSpec.Builder,
      repeatedField: RepeatedFieldDescriptor,
      builderCode: CodeBlock,
      proxyPropertyName: MemberSimpleName
    ): Declarations = declarations {
      addFunction(
        funSpec(clearSimpleName)
          .addKdoc(
            """
            Removes all elements from the repeated field %L.

            For example, `%L.clear()` deletes all elements from %L.
            """.trimIndent(),
            repeatedField.fieldName,
            proxyPropertyName,
            repeatedField.fieldName
          )
          .addStatement(
            "%L.%L()", builderCode, repeatedField.clearerSimpleName
          )
          .build()
      )
    }
  }
}
