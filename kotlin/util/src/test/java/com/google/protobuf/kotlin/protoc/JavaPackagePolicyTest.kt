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
import com.google.protos.testing.proto2api.ProtoApi1ExplicitPackage
import testing.ImplicitJavaPackage
import testing.ProtoApi1ImplicitPackage
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

/** Tests for [JavaPackagePolicy]. */
@RunWith(JUnit4::class)
class JavaPackagePolicyTest {
  @Test
  fun explicitJavaPackageGoogle() {
    with(JavaPackagePolicy.OPEN_SOURCE) {
      assertThat(javaPackage(Example3.getDescriptor().toProto()))
        .isEqualTo(PackageScope(Example3::class.java.`package`.name))
    }
  }

  @Test
  fun explicitJavaPackageExternal() {
    with(JavaPackagePolicy.OPEN_SOURCE) {
      assertThat(javaPackage(Example3.getDescriptor().toProto()))
        .isEqualTo(PackageScope(Example3::class.java.`package`.name))
    }
  }

  @Test
  fun implicitJavaPackageGoogle() {
    with(JavaPackagePolicy.OPEN_SOURCE) {
      assertThat(javaPackage(ImplicitJavaPackage.getDescriptor().toProto()))
        .isEqualTo(
          PackageScope("testing")
        )
    }
  }

  @Test
  fun implicitJavaPackageExternal() {
    with(JavaPackagePolicy.OPEN_SOURCE) {
      assertThat(javaPackage(ImplicitJavaPackage.getDescriptor().toProto()))
        .isEqualTo(PackageScope("testing"))
    }
  }

  @Test
  fun protoApi1ExplicitPackage() {
    with(JavaPackagePolicy.OPEN_SOURCE) {
      assertThat(javaPackage(ProtoApi1ExplicitPackage.getDescriptor().toProto()))
        .isEqualTo(PackageScope(ProtoApi1ExplicitPackage::class.java.`package`.name))
    }
  }

  @Test
  fun protoApi1ImplicitPackage() {
    with(JavaPackagePolicy.OPEN_SOURCE) {
      assertThat(javaPackage(ProtoApi1ImplicitPackage.getDescriptor().toProto()))
        .isEqualTo(PackageScope(ProtoApi1ImplicitPackage::class.java.`package`.name))
    }
  }
}
