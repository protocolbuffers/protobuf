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
public final class RuntimeVersionLiteTest {

  @Test
  public void versionValidation_invalidVersionNumbers() {
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    RuntimeVersionLite.DOMAIN, 1, -2, -3, ""));
    assertThat(thrown).hasMessageThat().contains("Invalid gencode version: 1.-2.-3");
  }

  @Test
  public void versionValidation_crossDomainDisallowed() {

    RuntimeVersionLite.RuntimeDomain gencodeDomain =
        RuntimeVersionLite.RuntimeDomain.GOOGLE_INTERNAL;
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () -> RuntimeVersionLite.validateProtobufGencodeDomain(gencodeDomain));
    assertThat(thrown).hasMessageThat().contains("Mismatched Protobuf Gencode/Runtime domains");
  }

  @Test
  public void versionValidation_sameDomainAllowed() {

    RuntimeVersionLite.RuntimeDomain gencodeDomain = RuntimeVersionLite.RuntimeDomain.PUBLIC;
    RuntimeVersionLite.validateProtobufGencodeDomain(gencodeDomain);
  }

  @Test
  public void versionValidation_mismatchingMajorDisallowed() {
    int gencodeMajor = 1;
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    RuntimeVersionLite.DOMAIN,
                    gencodeMajor,
                    RuntimeVersionLite.MINOR,
                    RuntimeVersionLite.PATCH,
                    RuntimeVersionLite.SUFFIX));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Mismatched Protobuf Gencode/Runtime major versions");
  }

  @Test
  public void versionValidation_versionNumbersAllTheSameAllowed() {
    RuntimeVersionLite.validateProtobufLiteGencodeVersion(
        RuntimeVersionLite.DOMAIN,
        RuntimeVersionLite.MAJOR,
        RuntimeVersionLite.MINOR,
        RuntimeVersionLite.PATCH,
        RuntimeVersionLite.SUFFIX);
  }

  @Test
  public void versionValidation_NewerRuntimeVersionAllowed() {
    int gencodeMinor = RuntimeVersionLite.MINOR - 1;
    RuntimeVersionLite.validateProtobufLiteGencodeVersion(
        RuntimeVersionLite.DOMAIN,
        RuntimeVersionLite.MAJOR,
        gencodeMinor,
        RuntimeVersionLite.PATCH,
        RuntimeVersionLite.SUFFIX);
  }

  @Test
  public void versionValidation_OlderRuntimeVersionDisallowed() {
    int gencodeMinor = RuntimeVersionLite.MINOR + 1;
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    RuntimeVersionLite.DOMAIN,
                    RuntimeVersionLite.MAJOR,
                    gencodeMinor,
                    RuntimeVersionLite.PATCH,
                    RuntimeVersionLite.SUFFIX));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Protobuf Java runtime version cannot be older than the gencode version");

    int gencodePatch = RuntimeVersionLite.PATCH + 1;
    thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    RuntimeVersionLite.DOMAIN,
                    RuntimeVersionLite.MAJOR,
                    RuntimeVersionLite.MINOR,
                    gencodePatch,
                    RuntimeVersionLite.SUFFIX));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Protobuf Java runtime version cannot be older than the gencode version");
  }

  @Test
  public void versionValidation_differentVesionSuffixDisallowed() {
    String gencodeSuffix = "-test";
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    RuntimeVersionLite.DOMAIN,
                    RuntimeVersionLite.MAJOR,
                    RuntimeVersionLite.MINOR,
                    RuntimeVersionLite.PATCH,
                    gencodeSuffix));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Mismatched Protobuf Gencode/Runtime version suffixes");
  }
}
