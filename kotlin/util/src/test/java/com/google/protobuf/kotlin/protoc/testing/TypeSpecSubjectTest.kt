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

import com.google.common.truth.ExpectFailure.SimpleSubjectBuilderCallback
import com.google.common.truth.ExpectFailure.expectFailureAbout
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.PropertySpec
import com.squareup.kotlinpoet.TypeSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [TypeSpecSubject]. */
@RunWith(JUnit4::class)
class TypeSpecSubjectTest {
  private val typeSpec =
    TypeSpec
      .objectBuilder("Foo")
      .addProperty(PropertySpec.builder("bar", INT).build())
      .build()

  @Test
  fun generates() {
    com.google.protobuf.kotlin.protoc.testing.assertThat(typeSpec).generates(
      """
      object Foo {
        val bar: kotlin.Int
      }
    """
    )
  }

  @Test
  fun generatesFailure() {
    expectFailureAbout(
      com.google.protobuf.kotlin.protoc.testing.typeSpecs,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(typeSpec).generates("")
      }
    )
    expectFailureAbout(
      com.google.protobuf.kotlin.protoc.testing.typeSpecs,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(typeSpec).generates("object Foo")
      }
    )
  }
}
