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

import protobuf_unittest.UnittestProto.TestPackedTypes;
import proto3_unittest.UnittestProto3;
import protobuf_unittest.TestCachedFieldSizeMessage;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class CachedFieldSizeTest {
  // Regression test for b/74087933
  @Test
  public void testCachedFieldSize() throws Exception {
    TestCachedFieldSizeMessage.Builder builder = TestCachedFieldSizeMessage.newBuilder();
    builder.setProto2Child(TestUtil.getPackedSet());
    builder.setProto3Child(
        UnittestProto3.TestPackedTypes.parseFrom(TestUtil.getPackedSet().toByteArray()));
    TestCachedFieldSizeMessage message = builder.build();

    // Serialize once to cache all field sizes. This will use the experimental runtime because
    // the proto has optimize_for = CODE_SIZE.
    message.toByteArray();
    // Serialize individual submessages. This will use the generated implementation. If the
    // experimental runtime hasn't set the correct cached size, this will throw an exception.
    byte[] data2 = message.getProto2Child().toByteArray();
    byte[] data3 = message.getProto3Child().toByteArray();

    // Make sure the serialized data is correct.
    assertThat(TestPackedTypes.parseFrom(data2)).isEqualTo(message.getProto2Child());
    assertThat(UnittestProto3.TestPackedTypes.parseFrom(data3)).isEqualTo(message.getProto3Child());
  }
}
