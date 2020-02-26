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
import com.squareup.kotlinpoet.MemberName
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Test for [ConstantName]. */
@RunWith(JUnit4::class)
class ConstantNameTest {
  @Test
  fun memberConstantName() {
    val className = ClassName(packageName = "com.google.protobuf", simpleNames = listOf("ByteString"))
    val memberName: MemberName = className.member(ConstantName("EMPTY"))
    assertThat(memberName.canonicalName).isEqualTo("com.google.protobuf.ByteString.EMPTY")
    assertThat(memberName.packageName).isEqualTo("com.google.protobuf")
    assertThat(memberName.enclosingClassName!!.canonicalName)
      .isEqualTo("com.google.protobuf.ByteString")
  }
}
