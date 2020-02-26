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

import com.google.common.truth.ExpectFailure
import com.google.common.truth.ExpectFailure.SimpleSubjectBuilderCallback
import com.squareup.kotlinpoet.FunSpec
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.ParameterSpec
import com.squareup.kotlinpoet.asTypeName
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [TypeSpecSubject]. */
@RunWith(JUnit4::class)
class FunSpecSubjectTest {
  private val funSpec =
    FunSpec
      .builder("foo")
      .addParameter(ParameterSpec.builder("bar", INT).build())
      .returns(String::class.asTypeName())
      .build()

  @Test
  fun generates() {
    com.google.protobuf.kotlin.protoc.testing.assertThat(funSpec).generates(
      """
      fun foo(bar: kotlin.Int): kotlin.String {
      }
    """
    )
  }

  @Test
  fun generatesFailure() {
    ExpectFailure.expectFailureAbout(
      com.google.protobuf.kotlin.protoc.testing.funSpecs,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(funSpec).generates("")
      }
    )
    ExpectFailure.expectFailureAbout(
      com.google.protobuf.kotlin.protoc.testing.funSpecs,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(funSpec).generates("fun bar")
      }
    )
  }
}
