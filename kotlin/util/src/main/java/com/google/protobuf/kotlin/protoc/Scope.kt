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

import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.FileSpec

/**
 * Describes a location classes can be nested in, such as a package or another class.  This can
 * convert a [ClassSimpleName] to a fully qualified [ClassName].
 */
sealed class Scope {
  abstract fun nestedClass(simpleName: ClassSimpleName): ClassName

  fun nestedScope(simpleName: ClassSimpleName): Scope = ClassScope(nestedClass(simpleName))
}

/**
 * The unqualified, top-level scope.
 */
object UnqualifiedScope : Scope() {
  override fun nestedClass(simpleName: ClassSimpleName): ClassName =
    ClassName("", simpleName.name)
}

/**
 * The scope of a package.
 */
data class PackageScope(val pkg: String) : Scope() {
  override fun nestedClass(simpleName: ClassSimpleName): ClassName =
    ClassName(pkg, simpleName.name)
}

/**
 * The scope of a fully qualified class.
 */
class ClassScope(private val className: ClassName) : Scope() {
  override fun nestedClass(simpleName: ClassSimpleName): ClassName =
    className.nestedClass(simpleName)
}

/**
 * Creates a [FileSpec.Builder] for a class with the specified simple name in the specified package.
 */
fun FileSpec.Companion.builder(
  packageName: PackageScope,
  simpleName: ClassSimpleName
): FileSpec.Builder = builder(packageName.pkg, simpleName.name)
