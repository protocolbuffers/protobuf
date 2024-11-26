// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import java.util.logging.Logger;
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
                    RuntimeVersion.DOMAIN, 1, -2, -3, "", "dummy"));
    assertThat(thrown).hasMessageThat().contains("Invalid gencode version: 1.-2.-3");
  }

  @Test
  public void versionValidation_crossDomainDisallowed() {

    RuntimeVersion.RuntimeDomain gencodeDomain =
        RuntimeVersion.RuntimeDomain.GOOGLE_INTERNAL;
    RuntimeVersion.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersion.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersion.validateProtobufGencodeVersion(
                    gencodeDomain, 1, 2, 3, "", "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains("Detected mismatched Protobuf Gencode/Runtime domains when loading testing.Foo");
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
                    RuntimeVersion.SUFFIX,
                    "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected mismatched Protobuf Gencode/Runtime major versions when loading testing.Foo");
  }

  @Test
  public void versionValidation_versionNumbersAllTheSameAllowed() {
    RuntimeVersion.validateProtobufGencodeVersion(
        RuntimeVersion.DOMAIN,
        RuntimeVersion.MAJOR,
        RuntimeVersion.MINOR,
        RuntimeVersion.PATCH,
        RuntimeVersion.SUFFIX,
        "dummy");
  }

  @Test
  public void versionValidation_newerRuntimeVersionAllowed() {
    int gencodeMinor = RuntimeVersion.MINOR - 1;
    RuntimeVersion.validateProtobufGencodeVersion(
        RuntimeVersion.DOMAIN,
        RuntimeVersion.MAJOR,
        gencodeMinor,
        RuntimeVersion.PATCH,
        RuntimeVersion.SUFFIX,
        "dummy");
  }

  @Test
  public void versionValidation_olderRuntimeVersionDisallowed() {
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
                    RuntimeVersion.SUFFIX,
                    "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected incompatible Protobuf Gencode/Runtime versions when loading testing.Foo");

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
                    RuntimeVersion.SUFFIX,
                    "testing.Bar"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected incompatible Protobuf Gencode/Runtime versions when loading testing.Bar");
  }

  @Test
  public void versionValidation_differentVersionSuffixDisallowed() {
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
                    gencodeSuffix,
                    "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected mismatched Protobuf Gencode/Runtime version suffixes when loading"
                + " testing.Foo");
  }

  @Test
  public void versionValidation_gencodeOneMajorVersionOlderWarning() {
    TestUtil.TestLogHandler logHandler = new TestUtil.TestLogHandler();
    Logger logger = Logger.getLogger(RuntimeVersion.class.getName());
    logger.addHandler(logHandler);
    RuntimeVersion.validateProtobufGencodeVersion(
        RuntimeVersion.DOMAIN,
        RuntimeVersion.MAJOR - 1,
        RuntimeVersion.MINOR,
        RuntimeVersion.PATCH,
        RuntimeVersion.SUFFIX,
        "dummy");
    assertThat(logHandler.getStoredLogRecords()).hasSize(1);
    assertThat(logHandler.getStoredLogRecords().get(0).getMessage())
        .contains("is exactly one major version older than the runtime version");
  }
}
