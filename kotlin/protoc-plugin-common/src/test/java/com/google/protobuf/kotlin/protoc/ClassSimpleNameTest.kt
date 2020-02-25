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
import com.google.protobuf.kotlin.protoc.testing.assertThat
import com.squareup.kotlinpoet.ClassName
import com.squareup.kotlinpoet.TypeSpec
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [ClassSimpleName]. */
@RunWith(JUnit4::class)
class ClassSimpleNameTest {
  @Test
  fun withSuffix() {
    assertThat(ClassSimpleName("FooBar").withSuffix("Baz"))
      .isEqualTo(ClassSimpleName("FooBarBaz"))
  }

  @Test
  fun asPeer() {
    val className = ClassName(packageName = "com.google.protobuf", simpleNames = listOf("ByteString"))
    val peerName = ClassSimpleName("Peer").asPeer(className)
    assertThat(peerName.packageName).isEqualTo("com.google.protobuf")
    assertThat(peerName.simpleName).isEqualTo("Peer")
  }

  @Test
  fun asMemberWithPrefix() {
    val simpleName = ClassSimpleName("SimpleName")

    assertThat(simpleName.asMemberWithPrefix("get"))
      .isEqualTo(MemberSimpleName("getSimpleName"))

    assertThat(simpleName.asMemberWithPrefix(""))
      .isEqualTo(MemberSimpleName("simpleName"))
  }

  @Test
  fun classBuilder() {
    val simpleName = ClassSimpleName("SimpleName")
    assertThat(TypeSpec.classBuilder(simpleName).build()).generates("class SimpleName")
  }

  @Test
  fun objectBuilder() {
    val simpleName = ClassSimpleName("SimpleName")
    assertThat(TypeSpec.objectBuilder(simpleName).build()).generates("object SimpleName")
  }
}
