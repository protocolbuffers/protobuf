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
import com.squareup.kotlinpoet.TypeSpec

/**
 * Represents a simple (unqualified, unnested) name of a Kotlin/Java class, interface, or enum,
 * in UpperCamelCase.
 */
data class ClassSimpleName(val name: String) : CharSequence by name {
  /** Returns this class name with a suffix. */
  fun withSuffix(suffix: String): ClassSimpleName = ClassSimpleName(name + suffix)

  /**
   * Returns a fully qualified class name as a peer of the specified class, with this simple name.
   */
  fun asPeer(className: ClassName): ClassName = className.peerClass(name)

  /** Returns a name of a member based on this class name with a prefix. */
  fun asMemberWithPrefix(prefix: String): MemberSimpleName {
    return if (prefix.isEmpty()) {
      MemberSimpleName(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_CAMEL, name))
    } else {
      MemberSimpleName(prefix + name)
    }
  }

  override fun toString() = name
}

/** Create a builder for a class with the specified simple name. */
fun TypeSpec.Companion.classBuilder(
  simpleName: ClassSimpleName
): TypeSpec.Builder = classBuilder(simpleName.name)

/** Create a builder for an object with the specified simple name. */
fun TypeSpec.Companion.objectBuilder(
  simpleName: ClassSimpleName
): TypeSpec.Builder = objectBuilder(simpleName.name)

/** Given a fully qualified class name, get the fully qualified name of a nested class inside it. */
fun ClassName.nestedClass(classSimpleName: ClassSimpleName): ClassName =
  nestedClass(classSimpleName.name)
