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
import com.squareup.kotlinpoet.ClassName
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [Scope]. */
@RunWith(JUnit4::class)
class ScopeTest {
  @Test
  fun unqualifiedScope() {
    val name = UnqualifiedScope.nestedClass(ClassSimpleName("FooBar"))
    assertThat(name).isEqualTo(ClassName(packageName = "", simpleNames = listOf("FooBar")))
  }

  @Test
  fun packageScope() {
    val name = PackageScope("com.foo.bar").nestedClass(ClassSimpleName("FooBar"))
    assertThat(name).isEqualTo(ClassName(packageName = "com.foo.bar", simpleNames = listOf("FooBar")))
  }

  @Test
  fun classScope() {
    val name =
      ClassScope(ClassName(packageName = "com.foo.bar", simpleNames = listOf("Baz")))
        .nestedClass(ClassSimpleName("Quux"))
    assertThat(name).isEqualTo(
      ClassName(packageName = "com.foo.bar", simpleNames = listOf("Baz", "Quux"))
    )
  }
}
