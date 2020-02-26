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
import com.squareup.kotlinpoet.KModifier
import com.squareup.kotlinpoet.MemberName
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.TypeName
import com.squareup.kotlinpoet.asClassName
import java.nio.file.Path
import java.nio.file.Paths
import kotlin.reflect.KClass

fun ParameterSpec.Companion.of(
  name: String,
  type: TypeName,
  vararg modifiers: KModifier
): ParameterSpec = ParameterSpec.builder(name, type, *modifiers).build()

/** Create a fully qualified [MemberName] in this class with the specified name. */
fun KClass<*>.member(memberName: String): MemberName = MemberName(asClassName(), memberName)

private fun path(vararg component: String): Path =
  Paths.get(component[0], *component.sliceArray(1 until component.size))

val FileSpec.path: Path
  get() {
    return if (packageName.isEmpty()) {
      Paths.get("$name.kt")
    } else {
      path(*packageName.split('.').toTypedArray(), "$name.kt")
    }
  }
