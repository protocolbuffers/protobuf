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

import com.google.common.base.Ascii
import com.google.common.base.CaseFormat.LOWER_UNDERSCORE
import com.google.common.base.CaseFormat.UPPER_CAMEL
import com.google.protobuf.DescriptorProtos.DescriptorProto
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto
import com.google.protobuf.DescriptorProtos.FileDescriptorProto
import com.google.protobuf.Descriptors.Descriptor
import com.google.protobuf.Descriptors.EnumDescriptor
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.Descriptors.FieldDescriptor.JavaType
import com.google.protobuf.Descriptors.FileDescriptor
import com.google.protobuf.Descriptors.OneofDescriptor
import com.google.protobuf.kotlin.protoc.TypeNames.BYTE_STRING
import com.google.protobuf.kotlin.protoc.TypeNames.STRING
import com.squareup.kotlinpoet.BOOLEAN
import com.squareup.kotlinpoet.DOUBLE
import com.squareup.kotlinpoet.FLOAT
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.LONG
import com.squareup.kotlinpoet.TypeName

private val JavaType.scalarType: TypeName
  get() = when (this) {
    JavaType.BOOLEAN -> BOOLEAN
    JavaType.INT -> INT
    JavaType.LONG -> LONG
    JavaType.FLOAT -> FLOAT
    JavaType.DOUBLE -> DOUBLE
    JavaType.STRING -> STRING
    JavaType.BYTE_STRING -> BYTE_STRING
    else -> throw IllegalArgumentException("Not a scalar type")
  }

/**
 * Returns the fully qualified Kotlin type representing values of this field, assuming that it
 * is a scalar field (defined at https://developers.google.com/protocol-buffers/docs/proto3#scalar).
 */
val FieldDescriptor.scalarType: TypeName
  get() = javaType.scalarType

private val JavaType.isJavaPrimitive: Boolean
  get() = when (this) {
    JavaType.BOOLEAN, JavaType.INT, JavaType.LONG, JavaType.FLOAT, JavaType.DOUBLE -> true
    else -> false
  }

/** True if the Java type representing the contents of this field is a primitive. */
val FieldDescriptor.isJavaPrimitive: Boolean
  get() = javaType.isJavaPrimitive

/** A property name for this oneof, without prefixes or suffixes. */
val OneofDescriptor.propertySimpleName: MemberSimpleName
  get() = oneofName.propertySimpleName

/** The simple name of the class representing the case of the oneof. */
val OneofDescriptor.caseEnumSimpleName: ClassSimpleName
  get() = oneofName.toClassSimpleNameWithSuffix("Case")

/** The simple name of the property to get the case of the oneof. */
val OneofDescriptor.casePropertySimpleName: MemberSimpleName
  get() = propertySimpleName.withSuffix("Case")

/** The name of the enum constant representing that this oneof is unset. */
val OneofDescriptor.notSetCaseSimpleName: ConstantName
  get() = ConstantName(Ascii.toUpperCase(name.filterNot { it == '_' }) + "_NOT_SET")

/**
 * Returns the simple name of the Java class that represents a message type, given its descriptor.
 */
val Descriptor.messageClassSimpleName: ClassSimpleName
  get() = toProto().messageClassSimpleName

/**
 * Returns the simple name of the Java class that represents a message type, given its descriptor
 * in proto form.
 */
val DescriptorProto.messageClassSimpleName: ClassSimpleName
  get() = simpleName.toClassSimpleName()

/** Returns the name of the Java class representing the proto enum type, given its descriptor. */
val EnumDescriptor.enumClassSimpleName: ClassSimpleName
  get() = toProto().enumClassSimpleName

/**
 * Returns the name of the Java class representing the proto enum type, given its descriptor
 * in proto form.
 */
val EnumDescriptorProto.enumClassSimpleName: ClassSimpleName
  get() = simpleName.toClassSimpleName()

val FileDescriptor.outerClassSimpleName: ClassSimpleName
  get() = toProto().outerClassSimpleName

private val FileDescriptorProto.explicitOuterClassSimpleName: ClassSimpleName?
  get() = when (val name = options.javaOuterClassname) {
    "" -> null
    else -> ClassSimpleName(name)
  }

/** The simple name of the outer class of a proto file. */
val FileDescriptorProto.outerClassSimpleName: ClassSimpleName
  get() {
    explicitOuterClassSimpleName?.let { return it }

    val defaultOuterClassName = ClassSimpleName(
      LOWER_UNDERSCORE.to(UPPER_CAMEL, fileName.name.replace("-", "_"))
    )

    val foundDuplicate =
      enumTypeList.any { it.enumClassSimpleName == defaultOuterClassName } ||
        messageTypeList.any { it.anyHaveNameRecursive(defaultOuterClassName) }

    return if (foundDuplicate) {
      defaultOuterClassName.withSuffix("OuterClass")
    } else {
      defaultOuterClassName
    }
  }

private fun DescriptorProto.anyHaveNameRecursive(name: ClassSimpleName): Boolean =
  messageClassSimpleName == name ||
    enumTypeList.any { it.enumClassSimpleName == name } ||
    nestedTypeList.any { it.anyHaveNameRecursive(name) }
