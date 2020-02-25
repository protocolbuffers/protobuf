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

package com.google.protobuf.kotlin.generator

import com.google.common.truth.extensions.proto.LiteProtoSubject
import com.google.common.truth.extensions.proto.LiteProtoTruth
import com.google.protobuf.kotlin.generator.EvilNamesOuterClass.EvilNames
import com.google.protobuf.kotlin.generator.Example2
import com.google.protobuf.kotlin.generator.Example3.ExampleMessage
import org.junit.runner.RunWith
import org.junit.runners.JUnit4

@RunWith(JUnit4::class)
class LiteGeneratedCodeTest : AbstractGeneratedCodeTest() {
  override fun assertThat(message: ExampleMessage): LiteProtoSubject =
    LiteProtoTruth.assertThat(message)

  override fun assertThat(message: EvilNames): LiteProtoSubject =
    LiteProtoTruth.assertThat(message)

  override fun assertThat(message: Example2.ExampleProto2Message): LiteProtoSubject =
    LiteProtoTruth.assertThat(message)
}
