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

/** Most version validations are being tested under RuntimeVersionLiteTest. */
@RunWith(JUnit4.class)
public final class RuntimeVersionTest {

  @Test
  public void versionValidation_sameVersion_success() {
    RuntimeVersion.validateProtobufLiteGencodeVersion(
        RuntimeVersionLite.DOMAIN,
        RuntimeVersionLite.MAJOR,
        RuntimeVersionLite.MINOR,
        RuntimeVersionLite.PATCH,
        RuntimeVersionLite.SUFFIX,
        "dummy");
  }

  @Test
  public void versionValidation_differentVersion_failure() {
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    RuntimeVersionLite.DOMAIN,
                    RuntimeVersionLite.MAJOR + 1,
                    RuntimeVersionLite.MINOR,
                    RuntimeVersionLite.PATCH,
                    RuntimeVersionLite.SUFFIX,
                    "testing.Foo"));
    assertThat(thrown)
        .hasMessageThat()
        .contains(
            "Detected mismatched Protobuf Gencode/Runtime major versions when loading testing.Foo");
  }
}
