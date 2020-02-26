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

import com.google.common.truth.Truth.assertThat
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [MemberSimpleName]. */
@RunWith(JUnit4::class)
class MemberSimpleNameTest {
  @Test
  fun withPrefix() {
    assertThat(MemberSimpleName("myField").withPrefix("get"))
      .isEqualTo(MemberSimpleName("getMyField"))
    assertThat(MemberSimpleName("field").withPrefix("get"))
      .isEqualTo(MemberSimpleName("getField"))
  }

  @Test
  fun withSuffix() {
    assertThat(MemberSimpleName("myField").withSuffix("List"))
      .isEqualTo(MemberSimpleName("myFieldList"))
    assertThat(MemberSimpleName("field").withSuffix("List"))
      .isEqualTo(MemberSimpleName("fieldList"))
  }

  @Test
  fun plus() {
    assertThat(MemberSimpleName("putAll") + MemberSimpleName("myField"))
      .isEqualTo(MemberSimpleName("putAllMyField"))
  }
}
