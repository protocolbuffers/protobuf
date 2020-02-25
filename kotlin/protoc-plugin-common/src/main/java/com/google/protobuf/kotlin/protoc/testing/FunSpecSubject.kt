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
import com.google.common.truth.Truth
import com.squareup.kotlinpoet.FunSpec

val funSpecs: Subject.Factory<FunSpecSubject, FunSpec> = Subject.Factory(::FunSpecSubject)

/** Make Truth assertions about [funSpec]. */
fun assertThat(funSpec: FunSpec): FunSpecSubject = Truth.assertAbout(funSpecs).that(funSpec)

/** A Truth subject for [FunSpec]. */
class FunSpecSubject(
  failureMetadata: FailureMetadata,
  private val actual: FunSpec
) : Subject(failureMetadata, actual) {
  fun generates(indentedCode: String) {
    val expectedCode = indentedCode.trimIndent()
    val actualCode = actual.toString().trim()
    check("code").that(actualCode).isEqualTo(expectedCode)
  }
}
