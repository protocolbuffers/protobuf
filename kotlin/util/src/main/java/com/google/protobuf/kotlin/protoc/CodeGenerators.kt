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

import com.google.common.base.Throwables
import com.google.common.graph.GraphBuilder
import com.google.common.graph.TopologicalSortGraph
import com.google.protobuf.DescriptorProtos.FileDescriptorProto
import com.google.protobuf.Descriptors.FileDescriptor
import com.google.protobuf.compiler.PluginProtos
import com.squareup.kotlinpoet.FileSpec

internal object CodeGenerators {
  fun descriptorMap(
    topologicalSortedProtoFileList: List<FileDescriptorProto>
  ): Map<ProtoFileName, FileDescriptor> {
    val descriptorsByName = mutableMapOf<ProtoFileName, FileDescriptor>()
    for (protoFile in topologicalSortedProtoFileList) {
      // we should have visited all the dependencies, so they should be present in the map
      val dependencies = protoFile.dependencyNames.map(descriptorsByName::getValue)

      // build and link the descriptor for this file to its dependencies
      val fileDescriptor = FileDescriptor.buildFrom(protoFile, dependencies.toTypedArray())

      descriptorsByName[protoFile.fileName] = fileDescriptor
    }
    return descriptorsByName
  }

  fun descriptorMapFromUnsorted(
    protoFileList: List<FileDescriptorProto>
  ): Map<ProtoFileName, FileDescriptor> {
    val byFileName = protoFileList.associateBy { it.fileName }

    val depGraph =
      GraphBuilder
        .directed()
        .expectedNodeCount(protoFileList.size)
        .build<ProtoFileName>()

    byFileName.keys.forEach { depGraph.addNode(it) }

    for ((fileName, fileDescriptorProto) in byFileName) {
      for (dep in fileDescriptorProto.dependencyNames) {
        depGraph.putEdge(dep, fileName)
      }
    }

    return descriptorMap(
      TopologicalSortGraph.topologicalOrdering(depGraph)
        .map { byFileName.getValue(it) }
    )
  }

  fun toCodeGeneratorResponseFile(fileSpec: FileSpec): PluginProtos.CodeGeneratorResponse.File =
    PluginProtos.CodeGeneratorResponse.File.newBuilder().also {
      it.name = fileSpec.path.toString()
      it.content = fileSpec.toString()
    }.build()

  inline fun codeGeneratorResponse(build: () -> List<FileSpec>): PluginProtos.CodeGeneratorResponse {
    val builder = PluginProtos.CodeGeneratorResponse.newBuilder()
    try {
      builder.addAllFile(build().map { toCodeGeneratorResponseFile(it) })
    } catch (failure: Exception) {
      builder.error = Throwables.getStackTraceAsString(failure)
    }
    return builder.build()
  }
}
