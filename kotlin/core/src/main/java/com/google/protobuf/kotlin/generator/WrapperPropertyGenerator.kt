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

import com.google.protobuf.BoolValue
import com.google.protobuf.BytesValue
import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.DoubleValue
import com.google.protobuf.FloatValue
import com.google.protobuf.Int32Value
import com.google.protobuf.Int64Value
import com.google.protobuf.StringValue
import com.google.protobuf.UInt32Value
import com.google.protobuf.UInt64Value
import com.google.protobuf.kotlin.protoc.Declarations
import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.Scope
import com.google.protobuf.kotlin.protoc.SingletonFieldDescriptor
import com.google.protobuf.kotlin.protoc.TypeNames.BYTE_STRING
import com.google.protobuf.kotlin.protoc.builder
import com.google.protobuf.kotlin.protoc.declarations
import com.google.protobuf.kotlin.protoc.fieldName
import com.google.protobuf.kotlin.protoc.of
import com.squareup.kotlinpoet.BOOLEAN
import com.squareup.kotlinpoet.CodeBlock
import com.squareup.kotlinpoet.DOUBLE
import com.squareup.kotlinpoet.FLOAT
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.LONG
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.TypeSpec
import com.squareup.kotlinpoet.asTypeName

/**
 * Given a field of a proto wrapper type like `Int32Value`, which is primarily used to distinguish
 * absent and set-to-default values of a proto scalar type, generates an extension property that
 * views it as a nullable property of the appropriate Kotlin type.  For example, an `Int32Value`
 * proto field is represented as a Kotlin property of type `Int?`.
 */
class WrapperPropertyGenerator(private val propertyNameSuffix: String = "Value") {
  companion object {
    private val STRING = String::class.asTypeName()

    private val mapping: Map<String, TypeName> = mapOf(
      DoubleValue.getDescriptor().fullName to DOUBLE,
      FloatValue.getDescriptor().fullName to FLOAT,
      Int64Value.getDescriptor().fullName to LONG,
      UInt64Value.getDescriptor().fullName to LONG,
      Int32Value.getDescriptor().fullName to INT,
      UInt32Value.getDescriptor().fullName to INT,
      BoolValue.getDescriptor().fullName to BOOLEAN,
      StringValue.getDescriptor().fullName to STRING,
      BytesValue.getDescriptor().fullName to BYTE_STRING
    )

    private val names: Map<String, String> = mapOf(
      DoubleValue.getDescriptor().fullName to "double",
      FloatValue.getDescriptor().fullName to "float",
      Int64Value.getDescriptor().fullName to "int64",
      UInt64Value.getDescriptor().fullName to "uint64",
      Int32Value.getDescriptor().fullName to "int32",
      UInt32Value.getDescriptor().fullName to "uint32",
      BoolValue.getDescriptor().fullName to "bool",
      StringValue.getDescriptor().fullName to "string",
      BytesValue.getDescriptor().fullName to "bytes"
    )
  }

  private fun GeneratorConfig.wrapperProperty(
    component: SingletonFieldDescriptor,
    mutable: Boolean,
    receiverType: TypeName?,
    orBuilderCode: CodeBlock
  ): PropertySpec? {
    if (!component.isMessage()) {
      return null
    }
    val componentType: String = component.descriptor.messageType.fullName
    val valueType = mapping[componentType] ?: return null
    val valuePropertyName = component.propertySimpleName.withSuffix(propertyNameSuffix)

    val nullableValueType = valueType.copy(nullable = true)

    val propertyBuilder =
      PropertySpec
        .builder(valuePropertyName, nullableValueType)

    receiverType?.let { propertyBuilder.receiver(it) }

    propertyBuilder.addKdoc(
      "A nullable view of the field %L of the message %L as an optional %L.",
      component.descriptor.fieldName,
      component.containingType.fullName,
      names.getValue(componentType)
    )

    val wrapperProperty = component.property()
    val wrapperHazzer = component.hazzer()!!

    propertyBuilder.getter(
      getterBuilder()
        .addStatement(
          "return if (%L.%N()) %L.%N.value else null",
          orBuilderCode,
          wrapperHazzer,
          orBuilderCode,
          wrapperProperty
        )
        .build()
    )

    if (mutable) {
      val setterParam = ParameterSpec.of("newValue", nullableValueType)
      propertyBuilder.mutable(true)
      propertyBuilder.setter(
        setterBuilder()
          .addParameter(setterParam)
          .beginControlFlow("if (%N == null)", setterParam)
          .addStatement(
            "%L.%L()",
            orBuilderCode,
            component.propertySimpleName.withPrefix("clear")
          )
          .nextControlFlow("else")
          .addStatement(
            "%L.%N = %T.newBuilder().setValue(%N).build()",
            orBuilderCode,
            component.property(),
            component.type(),
            setterParam
          )
          .endControlFlow()
          .build()
      )
    }

    return propertyBuilder.build()
  }

  val forMessageOrBuilder: ComponentExtensionGenerator<SingletonFieldDescriptor> =
    PropertyGenerator({ it.orBuilderClass() }, mutable = false)

  val forBuilder: ComponentExtensionGenerator<SingletonFieldDescriptor> =
    PropertyGenerator({ it.builderClass() }, mutable = true)

  val forDsl: DslComponentGenerator<SingletonFieldDescriptor> = DslPropertyGenerator()

  private inner class PropertyGenerator(
    val receiverType: GeneratorConfig.(Descriptor) -> TypeName,
    val mutable: Boolean
  ) : ComponentExtensionGenerator<SingletonFieldDescriptor>(SingletonFieldDescriptor::class) {
    override fun GeneratorConfig.componentExtensions(
      component: SingletonFieldDescriptor,
      enclosingScope: Scope,
      extensionScope: Scope
    ): Declarations = declarations {
      wrapperProperty(
        component = component,
        mutable = mutable,
        orBuilderCode = CodeBlock.of("this"),
        receiverType = receiverType(component.containingType)
      )?.let { addTopLevelProperty(it) }
    }
  }

  private inner class DslPropertyGenerator :
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
      wrapperProperty(
        component = component,
        receiverType = null, // instance property on the DSL class
        orBuilderCode = builderCode,
        mutable = true
      )?.let { dslBuilder.addProperty(it) }
    }
  }
}
