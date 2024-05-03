// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypesOrBuilder;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests for {@link SingleFieldBuilderV3}. This tests basic functionality. More extensive testing is
 * provided via other tests that exercise the builder.
 */
@RunWith(JUnit4.class)
public class SingleFieldBuilderV3Test {

  @Test
  public void testBasicUseAndInvalidations() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    SingleFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        new SingleFieldBuilderV3<>(TestAllTypes.getDefaultInstance(), mockParent, false);
    assertThat(builder.getMessage()).isSameInstanceAs(TestAllTypes.getDefaultInstance());
    assertThat(builder.getBuilder().buildPartial()).isEqualTo(TestAllTypes.getDefaultInstance());
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);

    builder.getBuilder().setOptionalInt32(10);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);
    TestAllTypes message = builder.build();
    assertThat(message.getOptionalInt32()).isEqualTo(10);

    // Test that we receive invalidations now that build has been called.
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);
    builder.getBuilder().setOptionalInt32(20);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);

    // Test that we don't keep getting invalidations on every change
    builder.getBuilder().setOptionalInt32(30);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);
  }

  @Test
  public void testSetMessage() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    SingleFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        new SingleFieldBuilderV3<>(TestAllTypes.getDefaultInstance(), mockParent, false);
    builder.setMessage(TestAllTypes.newBuilder().setOptionalInt32(0).build());
    assertThat(builder.getMessage().getOptionalInt32()).isEqualTo(0);

    // Update message using the builder
    builder.getBuilder().setOptionalInt32(1);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);
    assertThat(builder.getBuilder().getOptionalInt32()).isEqualTo(1);
    assertThat(builder.getMessage().getOptionalInt32()).isEqualTo(1);
    TestAllTypes unused = builder.build();
    builder.getBuilder().setOptionalInt32(2);
    assertThat(builder.getBuilder().getOptionalInt32()).isEqualTo(2);
    assertThat(builder.getMessage().getOptionalInt32()).isEqualTo(2);

    // Make sure message stays cached
    assertThat(builder.getMessage()).isSameInstanceAs(builder.getMessage());
  }

  @Test
  public void testClear() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    SingleFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        new SingleFieldBuilderV3<>(TestAllTypes.getDefaultInstance(), mockParent, false);
    builder.setMessage(TestAllTypes.newBuilder().setOptionalInt32(0).build());
    assertThat(TestAllTypes.getDefaultInstance()).isNotSameInstanceAs(builder.getMessage());
    builder.clear();
    assertThat(builder.getMessage()).isSameInstanceAs(TestAllTypes.getDefaultInstance());

    builder.getBuilder().setOptionalInt32(1);
    assertThat(TestAllTypes.getDefaultInstance()).isNotSameInstanceAs(builder.getMessage());
    builder.clear();
    assertThat(builder.getMessage()).isSameInstanceAs(TestAllTypes.getDefaultInstance());
  }

  @Test
  public void testMerge() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    SingleFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        new SingleFieldBuilderV3<>(TestAllTypes.getDefaultInstance(), mockParent, false);

    // Merge into default field.
    builder.mergeFrom(TestAllTypes.getDefaultInstance());
    assertThat(builder.getMessage()).isSameInstanceAs(TestAllTypes.getDefaultInstance());

    // Merge into non-default field on existing builder.
    builder.getBuilder().setOptionalInt32(2);
    builder.mergeFrom(TestAllTypes.newBuilder().setOptionalDouble(4.0).buildPartial());
    assertThat(builder.getMessage().getOptionalInt32()).isEqualTo(2);
    assertThat(builder.getMessage().getOptionalDouble()).isEqualTo(4.0);

    // Merge into non-default field on existing message
    builder.setMessage(TestAllTypes.newBuilder().setOptionalInt32(10).buildPartial());
    builder.mergeFrom(TestAllTypes.newBuilder().setOptionalDouble(5.0).buildPartial());
    assertThat(builder.getMessage().getOptionalInt32()).isEqualTo(10);
    assertThat(builder.getMessage().getOptionalDouble()).isEqualTo(5.0);
  }
}
