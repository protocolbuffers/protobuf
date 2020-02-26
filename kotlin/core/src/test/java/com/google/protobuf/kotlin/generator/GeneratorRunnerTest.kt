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

import com.google.common.jimfs.Configuration
import com.google.common.jimfs.Jimfs
import com.google.common.truth.Truth.assertThat
import com.google.common.truth.extensions.proto.ProtoTruth.assertThat
import com.google.protobuf.ByteString
import com.google.protobuf.DescriptorProtos.FileDescriptorProto
import com.google.protobuf.DescriptorProtos.FileDescriptorSet
import com.google.protobuf.WrappersProto
import com.google.protobuf.kotlin.generator.GeneratorRunner.RuntimeVariant
import com.google.protobuf.kotlin.generator.Example2
import com.google.protobuf.kotlin.generator.Example3
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorRequest
import com.google.protobuf.compiler.PluginProtos.CodeGeneratorResponse
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.JUnit4
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Paths

/** Tests for [GeneratorRunner]. */
@RunWith(JUnit4::class)
class GeneratorRunnerTest {
  companion object {
    private const val EXAMPLE3_DESCRIPTOR_SET_FILE_NAME = "example3-descriptor-set.proto.bin"
    private const val WRAPPERS_DESCRIPTOR_SET_FILE_NAME = "wrappers-descriptor-set.proto.bin"
    private const val OUTPUT_DIR_NAME = "output"

    private val example3DescriptorProto: FileDescriptorProto = Example3.getDescriptor().toProto()
    private val example2DescriptorProto: FileDescriptorProto = Example2.getDescriptor().toProto()
    private val wrappersDescriptorProto: FileDescriptorProto =
      WrappersProto.getDescriptor().toProto()

    private val example3DescriptorSet =
      FileDescriptorSet
        .newBuilder()
        .addFile(example3DescriptorProto)
        .build()
    private val wellKnownTypesDescriptorSet =
      FileDescriptorSet
        .newBuilder()
        .addFile(wrappersDescriptorProto)
        .build()
  }

  @Test
  fun runnerCommandLine() {
    val fileSystem = Jimfs.newFileSystem(Configuration.unix())

    Files.write(
      fileSystem.getPath(EXAMPLE3_DESCRIPTOR_SET_FILE_NAME),
      example3DescriptorSet.toByteArray()
    )
    Files.write(
      fileSystem.getPath(WRAPPERS_DESCRIPTOR_SET_FILE_NAME),
      wellKnownTypesDescriptorSet.toByteArray()
    )

    GeneratorRunner(GeneratorRunner.RuntimeVariant.FULL).mainAsCommandLine(
      arrayOf(
        OUTPUT_DIR_NAME,
        EXAMPLE3_DESCRIPTOR_SET_FILE_NAME,
        "--",
        EXAMPLE3_DESCRIPTOR_SET_FILE_NAME,
        WRAPPERS_DESCRIPTOR_SET_FILE_NAME
      ),
      fileSystem
    )

    val outputDir = fileSystem.getPath(
      OUTPUT_DIR_NAME,
      "com/google/protobuf/kotlin/generator"
    )
    val outputFile = outputDir.resolve("Example3Kt.kt")
    val outputFileContents = Files.readAllLines(outputFile, StandardCharsets.UTF_8)

    val expectedFileContents = Files.readAllLines(
      Paths.get(
        "src/test/java/com/google/protobuf/kotlin/generator",
        "GeneratorRunnerExample3.kt.expected"
      ),
      StandardCharsets.UTF_8
    )

    assertThat(outputFileContents).containsExactlyElementsIn(expectedFileContents).inOrder()
  }

  @Test
  fun runnerProtocPlugin() {
    val output = ByteString.newOutput()
    GeneratorRunner(GeneratorRunner.RuntimeVariant.FULL).mainAsProtocPlugin(
      CodeGeneratorRequest.newBuilder()
        .addProtoFile(wrappersDescriptorProto)
        .addProtoFile(example3DescriptorProto)
        .addFileToGenerate(example3DescriptorProto.name)
        .build()
        .toByteString()
        .newInput(),
      output
    )

    val expectedFileContents = Files.readAllLines(
      Paths.get(
        "src/test/java/com/google/protobuf/kotlin/generator",
        "GeneratorRunnerExample3.kt.expected"
      ),
      StandardCharsets.UTF_8
    )

    val result = CodeGeneratorResponse.parseFrom(output.toByteString())

    assertThat(result.fileList.single().content)
      .isEqualTo(expectedFileContents.joinToString("\n") + "\n")
    assertThat(result).isEqualTo(
      CodeGeneratorResponse.newBuilder()
        .addFile(
          CodeGeneratorResponse.File.newBuilder().apply {
            name = "com/google/protobuf/kotlin/generator/Example3Kt.kt"
            content = expectedFileContents.joinToString("\n") + "\n"
            // we end up having an extra newline
          }.build()
        )
        .build()
    )
  }

  private fun runnerWithExtensions(variant: RuntimeVariant, expectedFileName: String) {
    val output = ByteString.newOutput()
    GeneratorRunner(variant).mainAsProtocPlugin(
      CodeGeneratorRequest.newBuilder()
        .addProtoFile(example2DescriptorProto)
        .addFileToGenerate(example2DescriptorProto.name)
        .build()
        .toByteString()
        .newInput(),
      output
    )

    val expectedFileContents = Files.readAllLines(
      Paths.get(
        "src/test/java/com/google/protobuf/kotlin/generator",
        expectedFileName
      ),
      StandardCharsets.UTF_8
    )

    val result = CodeGeneratorResponse.parseFrom(output.toByteString())

    assertThat(result.fileList.single().content)
      .isEqualTo(expectedFileContents.joinToString("\n") + "\n")
    assertThat(result).isEqualTo(
      CodeGeneratorResponse.newBuilder()
        .addFile(
          CodeGeneratorResponse.File.newBuilder().apply {
            name = "com/google/protobuf/kotlin/generator/Example2Kt.kt"
            content = expectedFileContents.joinToString("\n") + "\n"
            // we end up having an extra newline
          }.build()
        )
        .build()
    )
  }

  @Test
  fun runnerWithExtensionsFull() {
    runnerWithExtensions(RuntimeVariant.FULL, "GeneratorRunnerExample2.kt.expected")
  }

  @Test
  fun runnerWithExtensionsLite() {
    runnerWithExtensions(RuntimeVariant.LITE, "GeneratorRunnerExample2Lite.kt.expected")
  }
}
