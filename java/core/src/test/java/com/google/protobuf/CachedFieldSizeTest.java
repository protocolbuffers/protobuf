// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
    byte[] unused = message.toByteArray();
    // Serialize individual submessages. This will use the generated implementation. If the
    // experimental runtime hasn't set the correct cached size, this will throw an exception.
    byte[] data2 = message.getProto2Child().toByteArray();
    byte[] data3 = message.getProto3Child().toByteArray();

    // Make sure the serialized data is correct.
    assertThat(TestPackedTypes.parseFrom(data2)).isEqualTo(message.getProto2Child());
    assertThat(UnittestProto3.TestPackedTypes.parseFrom(data3)).isEqualTo(message.getProto3Child());
  }
}
