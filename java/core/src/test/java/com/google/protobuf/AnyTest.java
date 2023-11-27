// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
  public void testAnyGeneratedExemplarApi() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    TestAny container = TestAny.newBuilder().setValue(Any.pack(message)).build();

    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();
    assertThat(container.getValue().isSameTypeAs(TestAny.getDefaultInstance())).isFalse();

    TestAllTypes result = container.getValue().unpackSameTypeAs(TestAllTypes.getDefaultInstance());
    TestUtil.assertAllFieldsSet(result);

    // Unpacking to a wrong exemplar will throw an exception.
    try {
      container.getValue().unpackSameTypeAs(TestAny.getDefaultInstance());
      assertWithMessage("Exception is expected.").fail();
    } catch (InvalidProtocolBufferException e) {
      // expected.
    }

    // Test that unpacking throws an exception if parsing fails.
    TestAny.Builder containerBuilder = container.toBuilder();
    containerBuilder.getValueBuilder().setValue(ByteString.copyFrom(new byte[] {0x11}));
    container = containerBuilder.build();
    try {
      container.getValue().unpackSameTypeAs(TestAllTypes.getDefaultInstance());
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
    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();
    assertThat(container.getValue().is(TestAny.class)).isFalse();
    assertThat(container.getValue().isSameTypeAs(TestAny.getDefaultInstance())).isFalse();

    TestAllTypes result = container.getValue().unpack(TestAllTypes.class);
    TestUtil.assertAllFieldsSet(result);

    container = TestAny.newBuilder().setValue(Any.pack(message, "yyy.com/")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("yyy.com/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().is(TestAllTypes.class)).isTrue();
    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();
    assertThat(container.getValue().is(TestAny.class)).isFalse();
    assertThat(container.getValue().isSameTypeAs(TestAny.getDefaultInstance())).isFalse();

    result = container.getValue().unpack(TestAllTypes.class);
    TestUtil.assertAllFieldsSet(result);

    container = TestAny.newBuilder().setValue(Any.pack(message, "")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().is(TestAllTypes.class)).isTrue();
    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();
    assertThat(container.getValue().is(TestAny.class)).isFalse();
    assertThat(container.getValue().isSameTypeAs(TestAny.getDefaultInstance())).isFalse();

    result = container.getValue().unpack(TestAllTypes.class);
    TestUtil.assertAllFieldsSet(result);
  }

  @Test
  public void testCustomTypeUrlsWithExemplars() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    TestAny container = TestAny.newBuilder().setValue(Any.pack(message, "xxx.com")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("xxx.com/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();
    assertThat(container.getValue().isSameTypeAs(TestAny.getDefaultInstance())).isFalse();

    TestAllTypes result = container.getValue().unpackSameTypeAs(TestAllTypes.getDefaultInstance());
    TestUtil.assertAllFieldsSet(result);

    container = TestAny.newBuilder().setValue(Any.pack(message, "yyy.com/")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("yyy.com/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();
    assertThat(container.getValue().isSameTypeAs(TestAny.getDefaultInstance())).isFalse();

    result = container.getValue().unpackSameTypeAs(TestAllTypes.getDefaultInstance());
    TestUtil.assertAllFieldsSet(result);

    container = TestAny.newBuilder().setValue(Any.pack(message, "")).build();

    assertThat(container.getValue().getTypeUrl())
        .isEqualTo("/" + TestAllTypes.getDescriptor().getFullName());

    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();
    assertThat(container.getValue().isSameTypeAs(TestAny.getDefaultInstance())).isFalse();

    result = container.getValue().unpackSameTypeAs(TestAllTypes.getDefaultInstance());
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

  @Test
  public void testCachedUnpackExemplarResult() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TestUtil.setAllFields(builder);
    TestAllTypes message = builder.build();

    TestAny container = TestAny.newBuilder().setValue(Any.pack(message)).build();

    assertThat(container.getValue().isSameTypeAs(TestAllTypes.getDefaultInstance())).isTrue();

    TestAllTypes result1 = container.getValue().unpackSameTypeAs(TestAllTypes.getDefaultInstance());
    TestAllTypes result2 = container.getValue().unpackSameTypeAs(TestAllTypes.getDefaultInstance());
    assertThat(Objects.equals(result1, result2)).isTrue();
  }
}
