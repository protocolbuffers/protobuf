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
import com.squareup.kotlinpoet.FileSpec
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.PropertySpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [FileSpecSubject]. */
@RunWith(JUnit4::class)
class FileSpecSubjectTest {
  private val fileSpec =
    FileSpec.builder(packageName = "", fileName = "FileSpecContents.kt")
      .addProperty(PropertySpec.builder("bar", INT).build())
      .build()

  @Test
  fun generates() {
    com.google.protobuf.kotlin.protoc.testing.assertThat(fileSpec).generates(
      """
      import kotlin.Int

      val bar: Int
    """
    )
  }

  @Test
  fun generatesFailure() {
    expectFailureAbout(
      com.google.protobuf.kotlin.protoc.testing.fileSpecs,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(fileSpec).generates("")
      }
    )
    expectFailureAbout(
      com.google.protobuf.kotlin.protoc.testing.fileSpecs,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(fileSpec).generates("object Foo")
      }
    )
  }
}
