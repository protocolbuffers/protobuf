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
import com.google.protobuf.kotlin.protoc.testproto.Example3
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [ProtoFileName]. */
@RunWith(JUnit4::class)
class ProtoFileNameTest {
  @Test
  fun fileName() {
    assertThat(Example3.getDescriptor().fileName)
      .isEqualTo(
        ProtoFileName(
          "testing/example3.proto"
        )
      )
  }

  @Test
  fun name() {
    assertThat(ProtoFileName("foo/bar/baz/quux.proto").name)
      .isEqualTo("quux")
  }
}
