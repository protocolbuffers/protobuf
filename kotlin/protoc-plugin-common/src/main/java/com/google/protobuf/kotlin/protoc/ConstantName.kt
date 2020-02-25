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
import com.squareup.kotlinpoet.MemberName

/** Represents a name of an constant, in UPPER_UNDERSCORE. */
data class ConstantName(val name: String) : CharSequence by name {
  override fun toString() = name
}

/** Returns the fully qualified name of this constant, as a member of the specified class. */
fun ClassName.member(constantName: ConstantName): MemberName =
  MemberName(this, constantName.name)
