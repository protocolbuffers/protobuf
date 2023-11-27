// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.wrapperstest.WrappersTestProto.TopLevelMessage;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class WrappersOfMethodTest {

  @Test
  public void testOf() throws Exception {
    TopLevelMessage.Builder builder = TopLevelMessage.newBuilder();
    builder.setFieldDouble(DoubleValue.of(2.333));
    builder.setFieldFloat(FloatValue.of(2.333f));
    builder.setFieldInt32(Int32Value.of(2333));
    builder.setFieldInt64(Int64Value.of(23333333333333L));
    builder.setFieldUint32(UInt32Value.of(2333));
    builder.setFieldUint64(UInt64Value.of(23333333333333L));
    builder.setFieldBool(BoolValue.of(true));
    builder.setFieldString(StringValue.of("23333"));
    builder.setFieldBytes(BytesValue.of(ByteString.wrap("233".getBytes(Internal.UTF_8))));

    TopLevelMessage message = builder.build();
    assertThat(message.getFieldDouble().getValue()).isEqualTo(2.333);
    assertThat(message.getFieldFloat().getValue()).isEqualTo(2.333F);
    assertThat(message.getFieldInt32().getValue()).isEqualTo(2333);
    assertThat(message.getFieldInt64().getValue()).isEqualTo(23333333333333L);
    assertThat(message.getFieldUint32().getValue()).isEqualTo(2333);
    assertThat(message.getFieldUint64().getValue()).isEqualTo(23333333333333L);
    assertThat(true).isSameInstanceAs(message.getFieldBool().getValue());
    assertThat(message.getFieldString().getValue().equals("23333")).isTrue();
    assertThat(message.getFieldBytes().getValue().toStringUtf8().equals("233")).isTrue();
  }
}
