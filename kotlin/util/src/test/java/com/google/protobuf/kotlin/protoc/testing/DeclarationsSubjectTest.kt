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
import com.google.protobuf.kotlin.protoc.declarations
import com.squareup.kotlinpoet.INT
import com.squareup.kotlinpoet.PropertySpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [DeclarationsSubject]. */
@RunWith(JUnit4::class)
class DeclarationsSubjectTest {
  private val topLevelProperty = PropertySpec.builder("topLevel", INT).build()
  private val enclosedProperty = PropertySpec.builder("enclosed", INT).build()
  private val decls = declarations {
    addTopLevelProperty(topLevelProperty)
    addProperty(enclosedProperty)
  }

  @Test
  fun generatesTopLevel() {
    assertThat(decls).generatesTopLevel(
      """
      import kotlin.Int

      val topLevel: Int
    """
    )
  }

  @Test
  fun generatesEnclosed() {
    assertThat(decls).generatesEnclosed(
      """
      import kotlin.Int

      val enclosed: Int
      """
    )
  }

  @Test
  fun generatesTopLevelFailure() {
    expectFailureAbout(
      declarationsSubjectFactory,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(decls).generatesTopLevel("")
      }
    )
  }

  @Test
  fun generatesEnclosedFailure() {
    expectFailureAbout(
      declarationsSubjectFactory,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(decls).generatesEnclosed("")
      }
    )
  }

  @Test
  fun generatesNoTopLevel() {
    assertThat(
      declarations {
        addProperty(enclosedProperty)
      }
    ).generatesNoTopLevelMembers()
  }

  @Test
  fun generatesNoEnclosed() {
    assertThat(
      declarations {
        addTopLevelProperty(topLevelProperty)
      }
    ).generatesNoEnclosedMembers()
  }

  @Test
  fun generatesNoTopLevelFailure() {
    expectFailureAbout(
      declarationsSubjectFactory,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(decls).generatesNoTopLevelMembers()
      }
    )
  }

  @Test
  fun generatesNoEnclosedFailure() {
    expectFailureAbout(
      declarationsSubjectFactory,
      SimpleSubjectBuilderCallback { whenTesting ->
        whenTesting.that(decls).generatesNoEnclosedMembers()
      }
    )
  }
}
