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

import com.squareup.kotlinpoet.FileSpec
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeSpec

/** Create a set of [Declarations] in a DSL style. */
inline fun declarations(callback: Declarations.Builder.() -> Unit): Declarations =
  Declarations.Builder().apply(callback).build()

/**
 * An immutable set of declarations, some of which may be always at the top level, and some of which
 * may be in some containing class or namespace (which may be a type or a file).
 */
class Declarations private constructor(
  private val atTopLevel: List<FileSpec.Builder.() -> Unit>,
  private val atEnclosing: List<ForEnclosing>
) {

  private data class ForEnclosing(
    val onFileSpec: FileSpec.Builder.() -> Unit,
    val onTypeSpec: TypeSpec.Builder.() -> Unit
  )

  /** Whether or not there are any declarations designated for the top level. */
  val hasTopLevelDeclarations: Boolean
    get() = atTopLevel.isNotEmpty()

  /** Whether or not there are any declarations designated for an enclosing scope. */
  val hasEnclosingScopeDeclarations: Boolean
    get() = atEnclosing.isNotEmpty()

  /**
   * Writes all declarations to the top level of the specified [FileSpec.Builder], using the top
   * level file as an enclosing scope for those declarations.
   */
  fun writeAllAtTopLevel(builder: FileSpec.Builder) {
    writeOnlyTopLevel(builder)
    atEnclosing.forEach {
      val callback = it.onFileSpec
      builder.callback()
    }
  }

  /**
   * Write only the top-level declarations in these [Declarations] to the specified
   * [FileSpec.Builder].
   */
  fun writeOnlyTopLevel(builder: FileSpec.Builder) {
    atTopLevel.forEach { builder.it() }
  }

  /**
   * Write only the declarations for an enclosing scope in these [Declarations] to the specified
   * [TypeSpec.Builder].
   */
  fun writeToEnclosingType(builder: TypeSpec.Builder) {
    atEnclosing.forEach {
      val callback = it.onTypeSpec
      builder.callback()
    }
  }

  /**
   * Write only the declarations for an enclosing scope in these [Declarations] to the specified
   * [FileSpec.Builder].
   */
  fun writeToEnclosingFile(builder: FileSpec.Builder) {
    atEnclosing.forEach {
      val callback = it.onFileSpec
      builder.callback()
    }
  }

  /** A builder for a set of [Declarations]. */
  class Builder {
    private val atTopLevel: MutableList<FileSpec.Builder.() -> Unit> = mutableListOf()
    private val atEnclosing: MutableList<ForEnclosing> = mutableListOf()

    /** Declare a property at the top level. */
    fun addTopLevelProperty(property: PropertySpec) {
      atTopLevel.add { addProperty(property) }
    }

    /** Declare a function at the top level. */
    fun addTopLevelFunction(function: FunSpec) {
      atTopLevel.add { addFunction(function) }
    }

    /** Declare a type at the top level. */
    fun addTopLevelType(type: TypeSpec) {
      atTopLevel.add { addType(type) }
    }

    /** Declare a type in the enclosing scope. */
    fun addType(type: TypeSpec) {
      atEnclosing.add(ForEnclosing({ addType(type) }, { addType(type) }))
    }

    /** Declare a function in the enclosing scope. */
    fun addFunction(function: FunSpec) {
      atEnclosing.add(ForEnclosing({ addFunction(function) }, { addFunction(function) }))
    }

    /** Declare a property in the enclosing scope. */
    fun addProperty(property: PropertySpec) {
      atEnclosing.add(ForEnclosing({ addProperty(property) }, { addProperty(property) }))
    }

    /** Merge with another set of [Declarations]. */
    fun merge(other: Declarations) {
      atTopLevel += other.atTopLevel
      atEnclosing += other.atEnclosing
    }

    /** Merge only the top-level declarations in the specified [Declarations]. */
    fun mergeTopLevelOnly(other: Declarations) {
      atTopLevel += other.atTopLevel
    }

    /** Build [Declarations]. */
    fun build() = Declarations(atTopLevel.toList(), atEnclosing.toList())
  }
}
