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
                    RuntimeVersionLite.DOMAIN, 1, -2, -3, "", "dummy"));
    assertThat(thrown).hasMessageThat().contains("Invalid gencode version: 1.-2.-3");
  }

  @Test
  public void versionValidation_crossDomainDisallowed() {

    RuntimeVersionLite.RuntimeDomain gencodeDomain =
        RuntimeVersionLite.RuntimeDomain.GOOGLE_INTERNAL;
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    gencodeDomain, 1, 2, 3, "", "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Detected mismatched Protobuf Gencode/Runtime domains when loading testing.Foo");
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
                    RuntimeVersionLite.SUFFIX,
                    "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected mismatched Protobuf Gencode/Runtime major versions when loading testing.Foo");
  }

  @Test
  public void versionValidation_versionNumbersAllTheSameAllowed() {
    RuntimeVersionLite.validateProtobufLiteGencodeVersion(
        RuntimeVersionLite.DOMAIN,
        RuntimeVersionLite.MAJOR,
        RuntimeVersionLite.MINOR,
        RuntimeVersionLite.PATCH,
        RuntimeVersionLite.SUFFIX,
        "dummy");
  }

  @Test
  public void versionValidation_newerRuntimeVersionAllowed() {
    int gencodeMinor = RuntimeVersionLite.MINOR - 1;
    RuntimeVersionLite.validateProtobufLiteGencodeVersion(
        RuntimeVersionLite.DOMAIN,
        RuntimeVersionLite.MAJOR,
        gencodeMinor,
        RuntimeVersionLite.PATCH,
        RuntimeVersionLite.SUFFIX,
        "dummy");
  }

  @Test
  public void versionValidation_olderRuntimeVersionDisallowed() {
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
                    RuntimeVersionLite.SUFFIX,
                    "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected incompatible Protobuf Gencode/Runtime versions when loading testing.Foo");

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
                    RuntimeVersionLite.SUFFIX,
                    "testing.Bar"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected incompatible Protobuf Gencode/Runtime versions when loading testing.Bar");
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
                    gencodeSuffix,
                    "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected mismatched Protobuf Gencode/Runtime version suffixes when loading"
                + " testing.Foo");
  }
}
