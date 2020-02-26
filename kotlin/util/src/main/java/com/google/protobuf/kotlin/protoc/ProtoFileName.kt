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

import com.google.protobuf.DescriptorProtos.FileDescriptorProto
import com.google.protobuf.Descriptors.FileDescriptor
import com.google.protobuf.compiler.PluginProtos

/**
 * Represents the name of a proto file, relative to the root of the source tree, with
 * lower_underscore naming.
 */
data class ProtoFileName(private val path: String) : Comparable<ProtoFileName> {
  val name: String
    get() = path.substringAfterLast('/').removeSuffix(".proto")

  override operator fun compareTo(other: ProtoFileName): Int = path.compareTo(other.path)
}

/** Returns the filename of the specified file descriptor in proto form. */
val FileDescriptorProto.fileName: ProtoFileName
  get() = ProtoFileName(name)

/** Returns the filename of the specified file descriptor. */
val FileDescriptor.fileName: ProtoFileName
  get() = toProto().fileName

val FileDescriptorProto.dependencyNames: List<ProtoFileName>
  get() = dependencyList.map(::ProtoFileName)

val PluginProtos.CodeGeneratorRequest.filesToGenerate: List<ProtoFileName>
  get() = fileToGenerateList.map(::ProtoFileName)
