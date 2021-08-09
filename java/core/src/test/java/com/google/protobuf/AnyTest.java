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
import static com.google.common.truth.Truth.assertWithMessage;

import any_test.AnyTestProto.TestAny;
import protobuf_unittest.UnittestProto.TestAllTypes;
import java.util.Objects;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for Any message. */
@RunWith(JUnit4.class)
public class AnyTest {

  @Test
  public void testAnyGeneratedApi() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    TestAny container = TestAny.newBuilder().setValue(Any.pack(message)).build();

    assertThat(container.getValue().is(TestAllTypes.class)).isTrue();
    assertThat(container.getValue().is(TestAny.class)).isFalse();

    TestAllTypes result = container.getValue().unpack(TestAllTypes.class);
    TestUtil.assertAllFieldsSet(result);


    // Unpacking to a wrong type will throw an exception.
    try {
      container.getValue().unpack(TestAny.class);
      assertWithMessage("Exception is expected.").fail();
    } catch (InvalidProtocolBufferException e) {
      // expected.
    }

    // Test that unpacking throws an exception if parsing fails.
    TestAny.Builder containerBuilder = container.toBuilder();
    containerBuilder.getValueBuilder().setValue(ByteString.copyFrom(new byte[] {0x11}));
    container = containerBuilder.build();
    try {
      container.getValue().unpack(TestAllTypes.class);
      assertWithMessage("Exception is expected.").fail();
    } catch (InvalidProtocolBufferException e) {
      // expected.
    }
  }

  @Test
  public void testCustomTypeUrls() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    TestAny container = TestAny.newBuilder().setValue(Any.pack(message, "xxx.com")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("xxx.com/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().is(TestAllTypes.class)).isTrue();
    assertThat(container.getValue().is(TestAny.class)).isFalse();

    TestAllTypes result = container.getValue().unpack(TestAllTypes.class);
    TestUtil.assertAllFieldsSet(result);

    container = TestAny.newBuilder().setValue(Any.pack(message, "yyy.com/")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("yyy.com/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().is(TestAllTypes.class)).isTrue();
    assertThat(container.getValue().is(TestAny.class)).isFalse();

    result = container.getValue().unpack(TestAllTypes.class);
    TestUtil.assertAllFieldsSet(result);

    container = TestAny.newBuilder().setValue(Any.pack(message, "")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().is(TestAllTypes.class)).isTrue();
    assertThat(container.getValue().is(TestAny.class)).isFalse();

    result = container.getValue().unpack(TestAllTypes.class);
    TestUtil.assertAllFieldsSet(result);
  }

  @Test
  public void testCachedUnpackResult() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    TestAny container = TestAny.newBuilder().setValue(Any.pack(message)).build();

    assertThat(container.getValue().is(TestAllTypes.class)).isTrue();

    TestAllTypes result1 = container.getValue().unpack(TestAllTypes.class);
    TestAllTypes result2 = container.getValue().unpack(TestAllTypes.class);
    assertThat(Objects.equals(result1, result2)).isTrue();
  }
}
