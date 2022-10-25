// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.testing.textformat.performance.proto2.Proto2TextFormatPerformanceProto;
import com.google.protobuf.testing.textformat.performance.proto3.Proto3TextFormatPerformanceProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class TextFormatPerformanceTest {
  // 10 Seconds is longer than we'd really like, but necesssary to keep this test from being flaky.
  // This test is mostly to make sure it doesn't explode to many 10s of seconds for some reason.
  private static final long MAX_PARSE_TIME_MS = 10_000L;

  private static final int REPEAT_COUNT = 400000;
  private static final String CONTAINS_SUB_MESSAGE_WITH_REPEATED_INT32 =
      repeat("sub_msg { value: 123 }", REPEAT_COUNT);
  private static final String CONTAINS_EXTENSION_SUB_MESSAGE_WITH_REPEATED_INT32 =
      repeat(
          "[protobuf.testing.textformat.performance.proto2.sub_msg_ext] { value: 123 }",
          REPEAT_COUNT);

  // OSS Tests are still using JDK 8, which doesn't have JDK 11 String.repeat()
  private static String repeat(String text, int count) {
    StringBuilder builder = new StringBuilder(text.length() * count);
    for (int i = 0; i < count; ++i) {
      builder.append(text);
    }
    return builder.toString();
  }

  @Test(timeout = MAX_PARSE_TIME_MS)
  public void testProto2ImmutableTextFormatParsing() throws Exception {
    Proto2TextFormatPerformanceProto.ContainsSubMessageWithRepeatedInt32.Builder builder =
        Proto2TextFormatPerformanceProto.ContainsSubMessageWithRepeatedInt32.newBuilder();

    TextFormat.merge(CONTAINS_SUB_MESSAGE_WITH_REPEATED_INT32, builder);

    Proto2TextFormatPerformanceProto.ContainsSubMessageWithRepeatedInt32 msg = builder.build();
    assertThat(msg.getSubMsg().getValueCount()).isEqualTo(REPEAT_COUNT);
    for (int i = 0; i < msg.getSubMsg().getValueCount(); ++i) {
      assertThat(msg.getSubMsg().getValue(i)).isEqualTo(123);
    }
  }

  @Test(timeout = MAX_PARSE_TIME_MS)
  public void testProto2ImmutableExtensionTextFormatParsing() throws Exception {
    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    Proto2TextFormatPerformanceProto.registerAllExtensions(registry);

    Proto2TextFormatPerformanceProto.ContainsExtensionSubMessage.Builder builder =
        Proto2TextFormatPerformanceProto.ContainsExtensionSubMessage.newBuilder();

    TextFormat.merge(CONTAINS_EXTENSION_SUB_MESSAGE_WITH_REPEATED_INT32, registry, builder);

    Proto2TextFormatPerformanceProto.ContainsExtensionSubMessage msg = builder.build();
    assertThat(msg.getExtension(Proto2TextFormatPerformanceProto.subMsgExt).getValueCount())
        .isEqualTo(REPEAT_COUNT);
    for (int i = 0;
        i < msg.getExtension(Proto2TextFormatPerformanceProto.subMsgExt).getValueCount();
        ++i) {
      assertThat(msg.getExtension(Proto2TextFormatPerformanceProto.subMsgExt).getValue(i))
          .isEqualTo(123);
    }
  }

  @Test(timeout = MAX_PARSE_TIME_MS)
  public void testProto3ImmutableTextFormatParsing() throws Exception {
    Proto3TextFormatPerformanceProto.ContainsSubMessageWithRepeatedInt32.Builder builder =
        Proto3TextFormatPerformanceProto.ContainsSubMessageWithRepeatedInt32.newBuilder();

    TextFormat.merge(CONTAINS_SUB_MESSAGE_WITH_REPEATED_INT32, builder);

    Proto3TextFormatPerformanceProto.ContainsSubMessageWithRepeatedInt32 msg = builder.build();
    assertThat(msg.getSubMsg().getValueCount()).isEqualTo(REPEAT_COUNT);
    for (int i = 0; i < msg.getSubMsg().getValueCount(); ++i) {
      assertThat(msg.getSubMsg().getValue(i)).isEqualTo(123);
    }
  }
}
