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

import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypesOrBuilder;
import java.util.Collections;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests for {@link RepeatedFieldBuilderV3}. This tests basic functionality. More extensive testing is
 * provided via other tests that exercise the builder.
 */
@RunWith(JUnit4.class)
public class RepeatedFieldBuilderV3Test {

  @Test
  public void testBasicUse() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    RepeatedFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        newRepeatedFieldBuilderV3(mockParent);
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(0).build());
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(1).build());
    assertThat(builder.getMessage(0).getOptionalInt32()).isEqualTo(0);
    assertThat(builder.getMessage(1).getOptionalInt32()).isEqualTo(1);

    List<TestAllTypes> list = builder.build();
    assertThat(list).hasSize(2);
    assertThat(list.get(0).getOptionalInt32()).isEqualTo(0);
    assertThat(list.get(1).getOptionalInt32()).isEqualTo(1);
    assertIsUnmodifiable(list);

    // Make sure it doesn't change.
    List<TestAllTypes> list2 = builder.build();
    assertThat(list).isSameInstanceAs(list2);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);
  }

  @Test
  public void testGoingBackAndForth() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    RepeatedFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        newRepeatedFieldBuilderV3(mockParent);
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(0).build());
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(1).build());
    assertThat(builder.getMessage(0).getOptionalInt32()).isEqualTo(0);
    assertThat(builder.getMessage(1).getOptionalInt32()).isEqualTo(1);

    // Convert to list
    List<TestAllTypes> list = builder.build();
    assertThat(list).hasSize(2);
    assertThat(list.get(0).getOptionalInt32()).isEqualTo(0);
    assertThat(list.get(1).getOptionalInt32()).isEqualTo(1);
    assertIsUnmodifiable(list);

    // Update 0th item
    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);
    builder.getBuilder(0).setOptionalString("foo");
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);
    list = builder.build();
    assertThat(list).hasSize(2);
    assertThat(list.get(0).getOptionalInt32()).isEqualTo(0);
    assertThat(list.get(0).getOptionalString()).isEqualTo("foo");
    assertThat(list.get(1).getOptionalInt32()).isEqualTo(1);
    assertIsUnmodifiable(list);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);
  }

  @Test
  public void testVariousMethods() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    RepeatedFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        newRepeatedFieldBuilderV3(mockParent);
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(1).build());
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(2).build());
    builder.addBuilder(0, TestAllTypes.getDefaultInstance()).setOptionalInt32(0);
    builder.addBuilder(TestAllTypes.getDefaultInstance()).setOptionalInt32(3);

    assertThat(builder.getMessage(0).getOptionalInt32()).isEqualTo(0);
    assertThat(builder.getMessage(1).getOptionalInt32()).isEqualTo(1);
    assertThat(builder.getMessage(2).getOptionalInt32()).isEqualTo(2);
    assertThat(builder.getMessage(3).getOptionalInt32()).isEqualTo(3);

    assertThat(mockParent.getInvalidationCount()).isEqualTo(0);
    List<TestAllTypes> messages = builder.build();
    assertThat(messages).hasSize(4);
    assertThat(messages).isSameInstanceAs(builder.build()); // expect same list

    // Remove a message.
    builder.remove(2);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);
    assertThat(builder.getCount()).isEqualTo(3);
    assertThat(builder.getMessage(0).getOptionalInt32()).isEqualTo(0);
    assertThat(builder.getMessage(1).getOptionalInt32()).isEqualTo(1);
    assertThat(builder.getMessage(2).getOptionalInt32()).isEqualTo(3);

    // Remove a builder.
    builder.remove(0);
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);
    assertThat(builder.getCount()).isEqualTo(2);
    assertThat(builder.getMessage(0).getOptionalInt32()).isEqualTo(1);
    assertThat(builder.getMessage(1).getOptionalInt32()).isEqualTo(3);

    // Test clear.
    builder.clear();
    assertThat(mockParent.getInvalidationCount()).isEqualTo(1);
    assertThat(builder.getCount()).isEqualTo(0);
    assertThat(builder.isEmpty()).isTrue();
  }

  @Test
  public void testLists() {
    TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
    RepeatedFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder> builder =
        newRepeatedFieldBuilderV3(mockParent);
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(1).build());
    builder.addMessage(0, TestAllTypes.newBuilder().setOptionalInt32(0).build());
    assertThat(builder.getMessage(0).getOptionalInt32()).isEqualTo(0);
    assertThat(builder.getMessage(1).getOptionalInt32()).isEqualTo(1);

    // Use list of builders.
    List<TestAllTypes.Builder> builders = builder.getBuilderList();
    assertThat(builders.get(0).getOptionalInt32()).isEqualTo(0);
    assertThat(builders.get(1).getOptionalInt32()).isEqualTo(1);
    builders.get(0).setOptionalInt32(10);
    builders.get(1).setOptionalInt32(11);

    // Use list of protos
    List<TestAllTypes> protos = builder.getMessageList();
    assertThat(protos.get(0).getOptionalInt32()).isEqualTo(10);
    assertThat(protos.get(1).getOptionalInt32()).isEqualTo(11);

    // Add an item to the builders and verify it's updated in both
    builder.addMessage(TestAllTypes.newBuilder().setOptionalInt32(12).build());
    assertThat(builders).hasSize(3);
    assertThat(protos).hasSize(3);
  }

  private void assertIsUnmodifiable(List<?> list) {
    if (list == Collections.emptyList()) {
      // OKAY -- Need to check this b/c EmptyList allows you to call clear.
    } else {
      try {
        list.clear();
        assertWithMessage("List wasn't immutable").fail();
      } catch (UnsupportedOperationException e) {
        // good
      }
    }
  }

  private RepeatedFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder>
      newRepeatedFieldBuilderV3(AbstractMessage.BuilderParent parent) {
    return new RepeatedFieldBuilderV3<TestAllTypes, TestAllTypes.Builder, TestAllTypesOrBuilder>(
        Collections.<TestAllTypes>emptyList(), false, parent, false);
  }
}
