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

import com.google.protobuf.kotlin.protoc.SpecializedFieldDescriptor
import com.squareup.kotlinpoet.AnnotationSpec

/**
 * Helpers for annotations related to deprecated proto fields.
 */
object Deprecation {
  private val suppressDeprecation =
    AnnotationSpec
      .builder(Suppress::class)
      .addMember("%S", "DEPRECATION")
      .build()

  /**
   * Returns annotations for public APIs related to deprecation of this field, if any.
   */
  fun apiAnnotations(descriptor: SpecializedFieldDescriptor): List<AnnotationSpec> =
    if (descriptor.isDeprecated) {
      listOf(
        AnnotationSpec.builder(Deprecated::class)
          .addMember("message = %S", "Field ${descriptor.fieldName} is deprecated")
          .build()
      )
    } else {
      listOf()
    }
}