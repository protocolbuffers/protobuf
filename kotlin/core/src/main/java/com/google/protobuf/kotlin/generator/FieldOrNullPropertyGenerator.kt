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

import com.google.protobuf.kotlin.protoc.GeneratorConfig
import com.google.protobuf.kotlin.protoc.OneofOptionFieldDescriptor
import com.google.protobuf.kotlin.protoc.SingletonFieldDescriptor
import com.squareup.kotlinpoet.CodeBlock

/** Generates OrNull nullable extension properties for fields with hazzers. */
class FieldOrNullPropertyGenerator(
  private val skipOneofFields: Boolean = true
) : AbstractOrNullPropertyGenerator() {
  override fun shouldGenerate(field: SingletonFieldDescriptor): Boolean =
    field.hasHazzer() && !(skipOneofFields && field is OneofOptionFieldDescriptor)

  override fun GeneratorConfig.has(
    receiver: CodeBlock,
    field: SingletonFieldDescriptor
  ): CodeBlock = CodeBlock.of("%L.%N()", receiver, field.hazzer()!!)
}
