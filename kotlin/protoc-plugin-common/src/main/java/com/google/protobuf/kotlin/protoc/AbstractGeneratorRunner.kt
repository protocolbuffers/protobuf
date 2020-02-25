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

import com.google.common.annotations.VisibleForTesting
import com.google.protobuf.DescriptorProtos
import com.google.protobuf.DescriptorProtos.*
import com.google.protobuf.Descriptors.FileDescriptor
import com.google.protobuf.compiler.PluginProtos
import com.squareup.kotlinpoet.FileSpec
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.nio.file.FileSystem
import java.nio.file.FileSystems
import java.nio.file.Files
import java.nio.file.Path

/** Superclass for generators. */
abstract class AbstractGeneratorRunner {
  abstract fun generateCodeForFile(file: FileDescriptor): List<FileSpec>

  @VisibleForTesting
  fun mainAsProtocPlugin(input: InputStream, output: OutputStream) {
    val generatorRequest = try {
      input.buffered().use {
        PluginProtos.CodeGeneratorRequest.parseFrom(it)
      }
    } catch (failure: Exception) {
      throw IOException(
        """
        Attempted to run proto extension generator as protoc plugin, but could not read
        CodeGeneratorRequest.
        """.trimIndent(),
        failure
      )
    }
    output.buffered().use {
      CodeGenerators.codeGeneratorResponse {
        val descriptorMap = CodeGenerators.descriptorMap(generatorRequest.protoFileList)
        generatorRequest.filesToGenerate
          .map(descriptorMap::getValue) // compiled descriptors to generate code for
          .flatMap(::generateCodeForFile) // generated extensions
      }.writeTo(it)
    }
  }

  @VisibleForTesting
  fun mainAsCommandLine(args: Array<String>, fs: FileSystem) {
    val dashIndex = args.indexOf("--")
    val outputDir = fs.getPath(args[0])
    val toGenerateExtensionsFor = args.slice(1 until dashIndex)
    val inTransitiveClosure = args.drop(dashIndex + 1)

    val fileNameToDescriptorSet =
      inTransitiveClosure.associateWith { readFileDescriptorSet(fs.getPath(it)) }

    val descriptorMap = CodeGenerators.descriptorMapFromUnsorted(
      fileNameToDescriptorSet.values.flatMap { it.fileList }
    )

    toGenerateExtensionsFor
      .asSequence()
      .map { fileNameToDescriptorSet.getValue(it) } // descriptor sets to output
      .flatMap { it.fileList.asSequence() } // descriptors to output
      .map { it.fileName } // file names of descriptors to output
      .map { descriptorMap.getValue(it) } // compiled descriptors to generate code for
      .flatMap { generateCodeForFile(it).asSequence() } // generated extensions
      .forEach { it.writeTo(outputDir) } // write to output directory
  }

  fun doMain(args: Array<String>) {
    if (args.isEmpty()) {
      mainAsProtocPlugin(System.`in`, System.out)
    } else {
      mainAsCommandLine(args, FileSystems.getDefault())
    }
  }

  private fun readFileDescriptorSet(path: Path): FileDescriptorSet =
    Files.newInputStream(path).buffered().use { FileDescriptorSet.parseFrom(it) }
}