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
    builder.build();
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
