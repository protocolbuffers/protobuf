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

import com.google.common.base.CaseFormat
import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.MemberName
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeName
import kotlin.reflect.KClass

/**
 * Represents a simple (unqualified, unnested) name of a property or function, in lowerCamelCase.
 * This helps us keep track of what names are in what format.
 */
data class MemberSimpleName(val name: String) : CharSequence by name {
  companion object {
    val OPERATOR_GET = MemberSimpleName("get")
    val OPERATOR_SET = MemberSimpleName("set")
    val OPERATOR_CONTAINS = MemberSimpleName("contains")
    val OPERATOR_PLUS_ASSIGN = MemberSimpleName("plusAssign")
    val OPERATOR_MINUS_ASSIGN = MemberSimpleName("minusAssign")
  }

  fun withSuffix(suffix: String): MemberSimpleName = MemberSimpleName(name + suffix)
  fun withPrefix(prefix: String): MemberSimpleName =
    MemberSimpleName(prefix + CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, name))

  operator fun plus(other: MemberSimpleName): MemberSimpleName =
    MemberSimpleName(name + CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, other.name))

  override fun toString() = name
}

/** Create a builder for a parameter with the specified simple name and type. */
fun ParameterSpec.Companion.builder(
  simpleName: MemberSimpleName,
  type: TypeName,
  vararg modifiers: KModifier
): ParameterSpec.Builder = builder(simpleName.name, type, *modifiers)

/** Create a builder for a parameter with the specified simple name and type. */
fun ParameterSpec.Companion.builder(
  simpleName: MemberSimpleName,
  type: KClass<*>,
  vararg modifiers: KModifier
): ParameterSpec.Builder = builder(simpleName.name, type, *modifiers)

/** Create a parameter with the specified simple name and type. */
fun ParameterSpec.Companion.of(
  simpleName: MemberSimpleName,
  type: TypeName,
  vararg modifiers: KModifier
): ParameterSpec = builder(simpleName, type, *modifiers).build()

/** Create a parameter with the specified simple name and type. */
fun ParameterSpec.Companion.of(
  simpleName: MemberSimpleName,
  type: KClass<*>,
  vararg modifiers: KModifier
): ParameterSpec = builder(simpleName, type, *modifiers).build()

/** Create a property with the specified simple name and type. */
fun PropertySpec.Companion.of(
  simpleName: MemberSimpleName,
  type: KClass<*>,
  vararg modifiers: KModifier
): PropertySpec = builder(simpleName.name, type, *modifiers).build()

/** Create a property with the specified simple name and type. */
fun PropertySpec.Companion.of(
  simpleName: MemberSimpleName,
  type: TypeName,
  vararg modifiers: KModifier
): PropertySpec = builder(simpleName, type, *modifiers).build()

/** Create a builder for a property with the specified simple name and type. */
fun PropertySpec.Companion.builder(
  simpleName: MemberSimpleName,
  type: TypeName,
  vararg modifiers: KModifier
): PropertySpec.Builder = builder(simpleName.name, type, *modifiers)

/** Create a builder for a function with the specified simple name. */
fun FunSpec.Companion.builder(
  simpleName: MemberSimpleName
): FunSpec.Builder = builder(simpleName.name)

/** Create a fully qualified [MemberName] in this class with the specified name. */
fun ClassName.member(memberSimpleName: MemberSimpleName): MemberName = member(memberSimpleName.name)

/** Create a fully qualified [MemberName] in this class with the specified name. */
fun ClassName.member(memberName: String): MemberName = MemberName(this, memberName)
