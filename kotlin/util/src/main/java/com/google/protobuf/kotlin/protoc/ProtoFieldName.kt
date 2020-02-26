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
import com.google.common.base.CaseFormat
import com.google.common.base.CharMatcher
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.Descriptors.OneofDescriptor

/** Represents the name of a proto field or oneof, which is in lower_underscore. */
data class ProtoFieldName(private val name: String) : CharSequence by name {
  companion object {
    // based on compiler/java/internal/helpers.cc
    private val SPECIAL_CASES = setOf(
      ProtoFieldName("class"),
      ProtoFieldName("cached_size"),
      ProtoFieldName("serialized_size")
    )

    private val LETTER = CharMatcher.inRange('A', 'Z').or(CharMatcher.inRange('a', 'z'))
    private val DIGIT = CharMatcher.inRange('0', '9')
    private operator fun CharMatcher.contains(c: Char) = matches(c)
  }

  private fun nameToCase(caseFormat: CaseFormat) = CaseFormat.LOWER_UNDERSCORE.to(caseFormat, name)

  val propertySimpleName: MemberSimpleName
    get() {
      val nameComponents = name.split('_')
      val finalCamelCaseName = StringBuilder(name.length)
      for (word in nameComponents) {
        if (finalCamelCaseName.isEmpty()) {
          finalCamelCaseName.append(
            Ascii.toLowerCase(word[0])
          ).append(upperCaseAfterNumeric(word), 1, word.length)
        } else {
          finalCamelCaseName.append(
            Ascii.toUpperCase(word[0])
          ).append(upperCaseAfterNumeric(word), 1, word.length)
        }
      }
      return MemberSimpleName(finalCamelCaseName.toString())
    }

  private fun upperCaseAfterNumeric(input: String): String {
    val result = StringBuilder(input.length)
    var lastCharNum = false
    for (char in input) {
      if (lastCharNum && char in LETTER) {
        result.append(Ascii.toUpperCase(char))
      } else {
        result.append(char)
      }
      lastCharNum = char in DIGIT
    }

    return result.toString()
  }

  val javaSimpleName: MemberSimpleName
    get() = if (this in SPECIAL_CASES) propertySimpleName.withSuffix("_") else propertySimpleName

  fun toClassSimpleNameWithSuffix(suffix: String): ClassSimpleName {
    val finalCamelCaseName = StringBuilder(name.length + suffix.length)
    for (word in name.split('_')) {
      // uppercase the first letter of each word, and leave the rest of the case the same
      finalCamelCaseName.append(Ascii.toUpperCase(word[0])).append(word, 1, word.length)
    }
    finalCamelCaseName.append(suffix)
    return ClassSimpleName(finalCamelCaseName.toString())
  }

  fun toEnumConstantName(): ConstantName = ConstantName(Ascii.toUpperCase(name))

  override fun toString() = name
}

val FieldDescriptor.fieldName: ProtoFieldName
  get() = ProtoFieldName(name)

val OneofDescriptor.oneofName: ProtoFieldName
  get() = ProtoFieldName(name)
