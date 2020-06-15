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

import com.google.common.collect.ImmutableList;
import com.google.protobuf.FieldMask;
import protobuf_unittest.UnittestProto.NestedTestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes;
import junit.framework.TestCase;

/** Unit tests for {@link FieldMaskUtil}. */
public class FieldMaskUtilTest extends TestCase {
  public void testIsValid() throws Exception {
    assertTrue(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload"));
    assertFalse(FieldMaskUtil.isValid(NestedTestAllTypes.class, "nonexist"));
    assertTrue(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_int32"));
    assertTrue(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.repeated_int32"));
    assertTrue(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_nested_message"));
    assertTrue(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.repeated_nested_message"));
    assertFalse(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.nonexist"));

    assertTrue(
        FieldMaskUtil.isValid(NestedTestAllTypes.class, FieldMaskUtil.fromString("payload")));
    assertFalse(
        FieldMaskUtil.isValid(NestedTestAllTypes.class, FieldMaskUtil.fromString("nonexist")));
    assertFalse(
        FieldMaskUtil.isValid(
            NestedTestAllTypes.class, FieldMaskUtil.fromString("payload,nonexist")));

    assertTrue(FieldMaskUtil.isValid(NestedTestAllTypes.getDescriptor(), "payload"));
    assertFalse(FieldMaskUtil.isValid(NestedTestAllTypes.getDescriptor(), "nonexist"));

    assertTrue(
        FieldMaskUtil.isValid(
            NestedTestAllTypes.getDescriptor(), FieldMaskUtil.fromString("payload")));
    assertFalse(
        FieldMaskUtil.isValid(
            NestedTestAllTypes.getDescriptor(), FieldMaskUtil.fromString("nonexist")));

    assertTrue(
        FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_nested_message.bb"));
    // Repeated fields cannot have sub-paths.
    assertFalse(
        FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.repeated_nested_message.bb"));
    // Non-message fields cannot have sub-paths.
    assertFalse(FieldMaskUtil.isValid(NestedTestAllTypes.class, "payload.optional_int32.bb"));
  }

  public void testToString() throws Exception {
    assertEquals("", FieldMaskUtil.toString(FieldMask.getDefaultInstance()));
    FieldMask mask = FieldMask.newBuilder().addPaths("foo").build();
    assertEquals("foo", FieldMaskUtil.toString(mask));
    mask = FieldMask.newBuilder().addPaths("foo").addPaths("bar").build();
    assertEquals("foo,bar", FieldMaskUtil.toString(mask));

    // Empty field paths are ignored.
    mask =
        FieldMask.newBuilder()
            .addPaths("")
            .addPaths("foo")
            .addPaths("")
            .addPaths("bar")
            .addPaths("")
            .build();
    assertEquals("foo,bar", FieldMaskUtil.toString(mask));
  }

  public void testFromString() throws Exception {
    FieldMask mask = FieldMaskUtil.fromString("");
    assertEquals(0, mask.getPathsCount());
    mask = FieldMaskUtil.fromString("foo");
    assertEquals(1, mask.getPathsCount());
    assertEquals("foo", mask.getPaths(0));
    mask = FieldMaskUtil.fromString("foo,bar.baz");
    assertEquals(2, mask.getPathsCount());
    assertEquals("foo", mask.getPaths(0));
    assertEquals("bar.baz", mask.getPaths(1));

    // Empty field paths are ignore.
    mask = FieldMaskUtil.fromString(",foo,,bar,");
    assertEquals(2, mask.getPathsCount());
    assertEquals("foo", mask.getPaths(0));
    assertEquals("bar", mask.getPaths(1));

    // Check whether the field paths are valid if a class parameter is provided.
    mask = FieldMaskUtil.fromString(NestedTestAllTypes.class, ",payload");

    try {
      mask = FieldMaskUtil.fromString(NestedTestAllTypes.class, "payload,nonexist");
      fail("Exception is expected.");
    } catch (IllegalArgumentException e) {
      // Expected.
    }
  }

  public void testFromFieldNumbers() throws Exception {
    FieldMask mask = FieldMaskUtil.fromFieldNumbers(TestAllTypes.class);
    assertEquals(0, mask.getPathsCount());
    mask =
        FieldMaskUtil.fromFieldNumbers(
            TestAllTypes.class, TestAllTypes.OPTIONAL_INT32_FIELD_NUMBER);
    assertEquals(1, mask.getPathsCount());
    assertEquals("optional_int32", mask.getPaths(0));
    mask =
        FieldMaskUtil.fromFieldNumbers(
            TestAllTypes.class,
            TestAllTypes.OPTIONAL_INT32_FIELD_NUMBER,
            TestAllTypes.OPTIONAL_INT64_FIELD_NUMBER);
    assertEquals(2, mask.getPathsCount());
    assertEquals("optional_int32", mask.getPaths(0));
    assertEquals("optional_int64", mask.getPaths(1));

    try {
      int invalidFieldNumber = 1000;
      mask = FieldMaskUtil.fromFieldNumbers(TestAllTypes.class, invalidFieldNumber);
      fail("Exception is expected.");
    } catch (IllegalArgumentException expected) {
    }
  }

  public void testToJsonString() throws Exception {
    FieldMask mask = FieldMask.getDefaultInstance();
    assertEquals("", FieldMaskUtil.toJsonString(mask));
    mask = FieldMask.newBuilder().addPaths("foo").build();
    assertEquals("foo", FieldMaskUtil.toJsonString(mask));
    mask = FieldMask.newBuilder().addPaths("foo.bar_baz").addPaths("").build();
    assertEquals("foo.barBaz", FieldMaskUtil.toJsonString(mask));
    mask = FieldMask.newBuilder().addPaths("foo").addPaths("bar_baz").build();
    assertEquals("foo,barBaz", FieldMaskUtil.toJsonString(mask));
  }

  public void testFromJsonString() throws Exception {
    FieldMask mask = FieldMaskUtil.fromJsonString("");
    assertEquals(0, mask.getPathsCount());
    mask = FieldMaskUtil.fromJsonString("foo");
    assertEquals(1, mask.getPathsCount());
    assertEquals("foo", mask.getPaths(0));
    mask = FieldMaskUtil.fromJsonString("foo.barBaz");
    assertEquals(1, mask.getPathsCount());
    assertEquals("foo.bar_baz", mask.getPaths(0));
    mask = FieldMaskUtil.fromJsonString("foo,barBaz");
    assertEquals(2, mask.getPathsCount());
    assertEquals("foo", mask.getPaths(0));
    assertEquals("bar_baz", mask.getPaths(1));
  }

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

  public void testUnion() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testAddFieldPath} to cover all scenarios.
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar");
    FieldMask result = FieldMaskUtil.union(mask1, mask2);
    assertEquals("bar,foo", FieldMaskUtil.toString(result));
  }

  public void testUnion_usingVarArgs() throws Exception {
    FieldMask mask1 = FieldMaskUtil.fromString("foo");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar.quz");
    FieldMask mask3 = FieldMaskUtil.fromString("bar.quz");
    FieldMask mask4 = FieldMaskUtil.fromString("bar");
    FieldMask result = FieldMaskUtil.union(mask1, mask2, mask3, mask4);
    assertEquals("bar,foo", FieldMaskUtil.toString(result));
  }

  public void testSubstract() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testRemoveFieldPath} to cover all scenarios.
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar");
    FieldMask result = FieldMaskUtil.subtract(mask1, mask2);
    assertEquals("foo", FieldMaskUtil.toString(result));
  }

  public void testSubstract_usingVarArgs() throws Exception {
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz.bar");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar.baz.quz");
    FieldMask mask3 = FieldMaskUtil.fromString("bar.quz");
    FieldMask mask4 = FieldMaskUtil.fromString("foo,bar.baz");
    FieldMask result = FieldMaskUtil.subtract(mask1, mask2, mask3, mask4);
    assertEquals("bar", FieldMaskUtil.toString(result));
  }

  public void testIntersection() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testIntersectFieldPath} to cover all scenarios.
    FieldMask mask1 = FieldMaskUtil.fromString("foo,bar.baz,bar.quz");
    FieldMask mask2 = FieldMaskUtil.fromString("foo.bar,bar");
    FieldMask result = FieldMaskUtil.intersection(mask1, mask2);
    assertEquals("bar.baz,bar.quz,foo.bar", FieldMaskUtil.toString(result));
  }

  public void testMerge() throws Exception {
    // Only test a simple case here and expect
    // {@link FieldMaskTreeTest#testMerge} to cover all scenarios.
    NestedTestAllTypes source =
        NestedTestAllTypes.newBuilder()
            .setPayload(TestAllTypes.newBuilder().setOptionalInt32(1234))
            .build();
    NestedTestAllTypes.Builder builder = NestedTestAllTypes.newBuilder();
    FieldMaskUtil.merge(FieldMaskUtil.fromString("payload"), source, builder);
    assertEquals(1234, builder.getPayload().getOptionalInt32());
  }
}
