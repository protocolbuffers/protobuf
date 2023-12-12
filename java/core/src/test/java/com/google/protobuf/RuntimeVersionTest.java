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
  public void versionValidation_liteGencodeWithFullVersion() {
    RuntimeVersionLite.ProtobufRuntimeVersionException thrown =
        assertThrows(
            RuntimeVersionLite.ProtobufRuntimeVersionException.class,
            () ->
                RuntimeVersionLite.validateProtobufLiteGencodeVersion(
                    RuntimeVersionLite.RuntimeDomain.GOOGLE_INTERNAL, 0, 0, 0, ""));
    assertThat(thrown).hasMessageThat().contains("Expected Protobuf Java Lite runtime");
  }

  @Test
  public void versionValidation_linkFullVersion() {

      RuntimeVersion.validateProtobufGencodeVersion(
          RuntimeVersionLite.DOMAIN,
          RuntimeVersionLite.MAJOR,
          RuntimeVersionLite.MINOR,
          RuntimeVersionLite.PATCH,
          RuntimeVersionLite.SUFFIX);
  }
}
