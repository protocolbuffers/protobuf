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

import com.google.common.base.CaseFormat
import com.google.protobuf.DescriptorProtos.MethodDescriptorProto
import com.google.protobuf.Descriptors.MethodDescriptor

/** Represents the unqualified name of an RPC method in a proto file, in UpperCamelCase. */
data class ProtoMethodName(val name: String) : CharSequence by name {
  fun toMemberSimpleName(): MemberSimpleName =
    MemberSimpleName(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_CAMEL, name))

  override fun toString() = name
}

val MethodDescriptor.methodName: ProtoMethodName
  get() = toProto().methodName

val MethodDescriptorProto.methodName: ProtoMethodName
  get() = ProtoMethodName(name)
