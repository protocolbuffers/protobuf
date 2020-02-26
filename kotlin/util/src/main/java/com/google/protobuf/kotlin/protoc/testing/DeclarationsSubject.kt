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

package com.google.protobuf.kotlin.protoc.testing

import com.google.common.truth.FailureMetadata
import com.google.common.truth.Subject
import com.google.common.truth.Truth.assertAbout
import com.google.protobuf.kotlin.protoc.Declarations
import com.squareup.kotlinpoet.FileSpec

val declarationsSubjectFactory: Subject.Factory<DeclarationsSubject, Declarations> =
  Subject.Factory(::DeclarationsSubject)

/** Make Truth assertions about [declarations]. */
fun assertThat(declarations: Declarations): DeclarationsSubject =
  assertAbout(declarationsSubjectFactory).that(declarations)

/** A Truth subject for [Declarations]. */
class DeclarationsSubject(
  failureMetadata: FailureMetadata,
  private val actual: Declarations
) : Subject(failureMetadata, actual) {
  fun generatesTopLevel(indentedCode: String) {
    val actualCode =
      FileSpec.builder("", "MyDeclarations.kt")
        .apply { actual.writeOnlyTopLevel(this) }
        .build()
    check("topLevel").about(fileSpecs).that(actualCode).generates(indentedCode)
  }

  fun generatesEnclosed(indentedCode: String) {
    val actualCode =
      FileSpec.builder("", "MyDeclarations.kt")
        .apply { actual.writeToEnclosingFile(this) }
        .build()
    check("enclosed").about(fileSpecs).that(actualCode).generates(indentedCode)
  }

  fun generatesNoTopLevelMembers() {
    val actualCode =
      FileSpec.builder("", "MyDeclarations.kt")
        .apply { actual.writeOnlyTopLevel(this) }
        .build()
    check("topLevel")
      .withMessage("top level declarations: %s", actualCode)
      .that(actual.hasTopLevelDeclarations)
      .isFalse()
  }

  fun generatesNoEnclosedMembers() {
    val actualCode =
      FileSpec.builder("", "MyDeclarations.kt")
        .apply { actual.writeToEnclosingFile(this) }
        .build()
    check("enclosed")
      .withMessage("enclosed declarations: %s", actualCode)
      .that(actual.hasEnclosingScopeDeclarations)
      .isFalse()
  }
}
