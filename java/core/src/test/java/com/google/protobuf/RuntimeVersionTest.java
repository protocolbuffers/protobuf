// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class RuntimeVersionTest {

  @Test
  public void versionValidation_invalidVersionNumbers() {
    RuntimeVersion.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersion.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersion.validateProtobufGencodeVersion(
                    RuntimeVersion.DOMAIN,
                    -1,
                    RuntimeVersion.MINOR,
                    RuntimeVersion.PATCH,
                    RuntimeVersion.SUFFIX));
    assertThat(thrown).hasMessageThat().contains("Invalid gencode version: -1");
  }

  @Test
  public void versionValidation_mismatchingMajorDisallowed() {
    int gencodeMajor = 1;
    RuntimeVersion.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersion.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersion.validateProtobufGencodeVersion(
                    RuntimeVersion.DOMAIN,
                    gencodeMajor,
                    RuntimeVersion.MINOR,
                    RuntimeVersion.PATCH,
                    RuntimeVersion.SUFFIX));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Mismatched Protobuf Gencode/Runtime major versions");
  }

  @Test
  public void versionValidation_versionNumbersAllTheSameAllowed() {
    RuntimeVersion.validateProtobufGencodeVersion(
        RuntimeVersion.DOMAIN,
        RuntimeVersion.MAJOR,
        RuntimeVersion.MINOR,
        RuntimeVersion.PATCH,
        RuntimeVersion.SUFFIX);
  }

  @Test
  public void versionValidation_NewerRuntimeVersionAllowed() {
    int gencodeMinor = RuntimeVersion.MINOR - 1;
    RuntimeVersion.validateProtobufGencodeVersion(
        RuntimeVersion.DOMAIN,
        RuntimeVersion.MAJOR,
        gencodeMinor,
        RuntimeVersion.PATCH,
        RuntimeVersion.SUFFIX);
  }

  @Test
  public void versionValidation_OlderRuntimeVersionDisallowed() {
    int gencodeMinor = RuntimeVersion.MINOR + 1;
    RuntimeVersion.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersion.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersion.validateProtobufGencodeVersion(
                    RuntimeVersion.DOMAIN,
                    RuntimeVersion.MAJOR,
                    gencodeMinor,
                    RuntimeVersion.PATCH,
                    RuntimeVersion.SUFFIX));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Protobuf Java runtime version cannot be older than the gencode version");

    int gencodePatch = RuntimeVersion.PATCH + 1;
    thrown =
        assertThrows(
            RuntimeVersion.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersion.validateProtobufGencodeVersion(
                    RuntimeVersion.DOMAIN,
                    RuntimeVersion.MAJOR,
                    RuntimeVersion.MINOR,
                    gencodePatch,
                    RuntimeVersion.SUFFIX));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Protobuf Java runtime version cannot be older than the gencode version");
  }

  @Test
  public void versionValidation_differentVesionSuffixDisallowed() {
    String gencodeSuffix = "-test";
    RuntimeVersion.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersion.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersion.validateProtobufGencodeVersion(
                    RuntimeVersion.DOMAIN,
                    RuntimeVersion.MAJOR,
                    RuntimeVersion.MINOR,
                    RuntimeVersion.PATCH,
                    gencodeSuffix));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Mismatched Protobuf Gencode/Runtime version suffixes");
  }
}
