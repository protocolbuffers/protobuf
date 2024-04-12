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

package com.google.protobuf.util;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.common.collect.ImmutableList;
import com.google.protobuf.FieldMask;
import protobuf_unittest.UnittestProto.NestedTestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link FieldMaskUtil}. */
@RunWith(JUnit4.class)
public class FieldMaskUtilTest {
  @Test
  public void testIsValid() throws Exception {
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload")).isTrue();
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "nonexist")).isFalse();
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_int32")).isTrue();
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.repeated_int32")).isTrue();
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_nested_message"))
        .isTrue();
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.repeated_nested_message"))
        .isTrue();
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.nonexist")).isFalse();

    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, FieldMaskUtil.fromString("payload")))
        .isTrue();
    assertThat(
            FieldMaskUtil.isValid(NestedTestAllTypes.class, FieldMaskUtil.fromString("nonexist")))
        .isFalse();
    assertThat(
            FieldMaskUtil.isValid(
                NestedTestAllTypes.class, FieldMaskUtil.fromString("payload,nonexist")))
        .isFalse();

    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.getDescriptor(), "payload")).isTrue();
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.getDescriptor(), "nonexist")).isFalse();

    assertThat(
            FieldMaskUtil.isValid(
                NestedTestAllTypes.getDescriptor(), FieldMaskUtil.fromString("payload")))
        .isTrue();
    assertThat(
            FieldMaskUtil.isValid(
                NestedTestAllTypes.getDescriptor(), FieldMaskUtil.fromString("nonexist")))
        .isFalse();

    assertThat(
            FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_nested_message.bb"))
        .isTrue();
    // Repeated fields cannot have sub-paths.
    assertThat(
            FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.repeated_nested_message.bb"))
        .isFalse();
    // Non-message fields cannot have sub-paths.
    assertThat(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_int32.bb"))
        .isFalse();
  }

  @Test
  public void testToString() throws Exception {
    assertThat(FieldMaskUtil.toString(FieldMask.getDefaultInstance())).isEmpty();
    FieldMask mask = FieldMask.newBuilder().addPaths("foo").build();
    assertThat(FieldMaskUtil.toString(mask)).isEqualTo("foo");
    mask = FieldMask.newBuilder().addPaths("foo").addPaths("bar").build();
    assertThat(FieldMaskUtil.toString(mask)).isEqualTo("foo,bar");

    // Empty field paths are ignored.
    mask =
        FieldMask.newBuilder()
            .addPaths("")
            .addPaths("foo")
            .addPaths("")
            .addPaths("bar")
            .addPaths("")
            .build();
    assertThat(FieldMaskUtil.toString(mask)).isEqualTo("foo,bar");
  }

  @Test
  public void testFromString() throws Exception {
    FieldMask mask = FieldMaskUtil.fromString("");
    assertThat(mask.getPathsCount()).isEqualTo(0);
    mask = FieldMaskUtil.fromString("foo");
    assertThat(mask.getPathsCount()).isEqualTo(1);
    assertThat(mask.getPaths(0)).isEqualTo("foo");
    mask = FieldMaskUtil.fromString("foo,bar.baz");
    assertThat(mask.getPathsCount()).isEqualTo(2);
    assertThat(mask.getPaths(0)).isEqualTo("foo");
    assertThat(mask.getPaths(1)).isEqualTo("bar.baz");

    // Empty field paths are ignore.
    mask = FieldMaskUtil.fromString(",foo,,bar,");
    assertThat(mask.getPathsCount()).isEqualTo(2);
    assertThat(mask.getPaths(0)).isEqualTo("foo");
    assertThat(mask.getPaths(1)).isEqualTo("bar");

    // Check whether the field paths are valid if a class parameter is provided.
    mask = FieldMaskUtil.fromString(NestedTestAllTypes.class, ",payload");

    try {
      mask = FieldMaskUtil.fromString(NestedTestAllTypes.class, "payload,nonexist");
      assertWithMessage("Exception is expected.").fail();
    } catch (IllegalArgumentException e) {
      // Expected.
    }
  }

  @Test
  public void testFromFieldNumbers() throws Exception {
    FieldMask mask = FieldMaskUtil.fromFieldNumbers(TestAllTypes.class);
    assertThat(mask.getPathsCount()).isEqualTo(0);
    mask =
        FieldMaskUtil.fromFieldNumbers(
            TestAllTypes.class, TestAllTypes.OPTIONAL_INT32_FIELD_NUMBER);
    assertThat(mask.getPathsCount()).isEqualTo(1);
    assertThat(mask.getPaths(0)).isEqualTo("optional_int32");
    mask =
        FieldMaskUtil.fromFieldNumbers(
            TestAllTypes.class,
            TestAllTypes.OPTIONAL_INT32_FIELD_NUMBER,
            TestAllTypes.OPTIONAL_INT64_FIELD_NUMBER);
    assertThat(mask.getPathsCount()).isEqualTo(2);
    assertThat(mask.getPaths(0)).isEqualTo("optional_int32");
    assertThat(mask.getPaths(1)).isEqualTo("optional_int64");

    try {
      int invalidFieldNumber = 1000;
      mask = FieldMaskUtil.fromFieldNumbers(TestAllTypes.class, invalidFieldNumber);
      assertWithMessage("Exception is expected.").fail();
    } catch (IllegalArgumentException expected) {
    }
  }

  @Test
  public void testToJsonString() throws Exception {
    FieldMask mask = FieldMask.getDefaultInstance();
    assertThat(FieldMaskUtil.toJsonString(mask)).isEmpty();
    mask = FieldMask.newBuilder().addPaths("foo").build();
    assertThat(FieldMaskUtil.toJsonString(mask)).isEqualTo("foo");
    mask = FieldMask.newBuilder().addPaths("foo.bar_baz").addPaths("").build();
    assertThat(FieldMaskUtil.toJsonString(mask)).isEqualTo("foo.barBaz");
    mask = FieldMask.newBuilder().addPaths("foo").addPaths("bar_baz").build();
    assertThat(FieldMaskUtil.toJsonString(mask)).isEqualTo("foo,barBaz");
  }

  @Test
  public void testFromJsonString() throws Exception {
    FieldMask mask = FieldMaskUtil.fromJsonString("");
    assertThat(mask.getPathsCount()).isEqualTo(0);
    mask = FieldMaskUtil.fromJsonString("foo");
    assertThat(mask.getPathsCount()).isEqualTo(1);
    assertThat(mask.getPaths(0)).isEqualTo("foo");
    mask = FieldMaskUtil.fromJsonString("foo.barBaz");
    assertThat(mask.getPathsCount()).isEqualTo(1);
    assertThat(mask.getPaths(0)).isEqualTo("foo.bar_baz");
    mask = FieldMaskUtil.fromJsonString("foo,barBaz");
    assertThat(mask.getPathsCount()).isEqualTo(2);
    assertThat(mask.getPaths(0)).isEqualTo("foo");
    assertThat(mask.getPaths(1)).isEqualTo("bar_baz");
  }

  @Test
  public void testFromStringList() throws Exception {
    FieldMask mask =
        FieldMaskUtil.fromStringList(
            NestedTestAllTypes.class, ImmutableList.of("payload.repeated_nested_message", "child"));
    assertThat(mask)
        .isEqualTo(
            FieldMask.newBuilder()
                .addPaths("payload.repeated_nested_message")
                .addPaths("child")
                .build());

    mask =
        FieldMaskUtil.fromStringList(
            NestedTestAllTypes.getDescriptor(),
            ImmutableList.of("payload.repeated_nested_message", "child"));
    assertThat(mask)
        .isEqualTo(
            FieldMask.newBuilder()
                .addPaths("payload.repeated_nested_message")
                .addPaths("child")
                .build());

    mask =
        FieldMaskUtil.fromStringList(ImmutableList.of("payload.repeated_nested_message", "child"));
    assertThat(mask)
        .isEqualTo(
            FieldMask.newBuilder()
                .addPaths("payload.repeated_nested_message")
                .addPaths("child")
                .build());
  }

  @Test
  public void testUnion() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testAddFieldPath} to cover all scenarios.
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar");
    FieldMask result = FieldMaskUtil.union(mask1, mask2);
    assertThat(FieldMaskUtil.toString(result)).isEqualTo("bar,foo");
  }

  @Test
  public void testUnion_usingVarArgs() throws Exception {
    FieldMask mask1 = FieldMaskUtil.fromString("foo");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar.quz");
    FieldMask mask3 = FieldMaskUtil.fromString("bar.quz");
    FieldMask mask4 = FieldMaskUtil.fromString("bar");
    FieldMask result = FieldMaskUtil.union(mask1, mask2, mask3, mask4);
    assertThat(FieldMaskUtil.toString(result)).isEqualTo("bar,foo");
  }

  @Test
  public void testSubstract() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testRemoveFieldPath} to cover all scenarios.
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar");
    FieldMask result = FieldMaskUtil.subtract(mask1, mask2);
    assertThat(FieldMaskUtil.toString(result)).isEqualTo("foo");
  }

  @Test
  public void testSubstract_usingVarArgs() throws Exception {
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz.bar");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar.baz.quz");
    FieldMask mask3 = FieldMaskUtil.fromString("bar.quz");
    FieldMask mask4 = FieldMaskUtil.fromString("foo,bar.baz");
    FieldMask result = FieldMaskUtil.subtract(mask1, mask2, mask3, mask4);
    assertThat(FieldMaskUtil.toString(result)).isEmpty();
  }

  @Test
  public void testIntersection() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testIntersectFieldPath} to cover all scenarios.
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar");
    FieldMask result = FieldMaskUtil.intersection(mask1, mask2);
    assertThat(FieldMaskUtil.toString(result)).isEqualTo("bar.baz,bar.quz,foo.bar");
  }

  @Test
  public void testMerge() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testMerge} to cover all scenarios.
    NestedTestAllTypes source =
        NestedTestAllTypes.newBuilder()
            .setPayload(TestAllTypes.newBuilder().setOptionalInt32(1234))
            .build();
    NestedTestAllTypes.Builder builder = NestedTestAllTypes.newBuilder();
    FieldMaskUtil.merge(FieldMaskUtil.fromString("payload"), source, builder);
    assertThat(builder.getPayload().getOptionalInt32()).isEqualTo(1234);
  }

  @Test
  public void testTrim() throws Exception {
    NestedTestAllTypes source =
        NestedTestAllTypes.newBuilder()
            .setPayload(
                TestAllTypes.newBuilder()
                    .setOptionalInt32(1234)
                    .setOptionalString("1234")
                    .setOptionalBool(true))
            .build();
    FieldMask mask =
        FieldMaskUtil.fromStringList(
            ImmutableList.of("payload.optional_int32", "payload.optional_string"));

    NestedTestAllTypes actual = FieldMaskUtil.trim(mask, source);

    assertThat(actual)
        .isEqualTo(
            NestedTestAllTypes.newBuilder()
                .setPayload(
                    TestAllTypes.newBuilder().setOptionalInt32(1234).setOptionalString("1234"))
                .build());
  }
}
