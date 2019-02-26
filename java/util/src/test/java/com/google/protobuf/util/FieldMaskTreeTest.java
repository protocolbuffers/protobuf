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

import com.google.protobuf.DynamicMessage;
import com.google.protobuf.Message;
import com.google.protobuf.UninitializedMessageException;
import protobuf_unittest.UnittestProto.NestedTestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes.NestedMessage;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestRequiredMessage;
import junit.framework.TestCase;

public class FieldMaskTreeTest extends TestCase {
  public void testAddFieldPath() throws Exception {
    FieldMaskTree tree = new FieldMaskTree();
    assertEquals("", tree.toString());
    tree.addFieldPath("");
    assertEquals("", tree.toString());
    // New branch.
    tree.addFieldPath("foo");
    assertEquals("foo", tree.toString());
    // Redundant path.
    tree.addFieldPath("foo");
    assertEquals("foo", tree.toString());
    // New branch.
    tree.addFieldPath("bar.baz");
    assertEquals("bar.baz,foo", tree.toString());
    // Redundant sub-path.
    tree.addFieldPath("foo.bar");
    assertEquals("bar.baz,foo", tree.toString());
    // New branch from a non-root node.
    tree.addFieldPath("bar.quz");
    assertEquals("bar.baz,bar.quz,foo", tree.toString());
    // A path that matches several existing sub-paths.
    tree.addFieldPath("bar");
    assertEquals("bar,foo", tree.toString());
  }

  public void testMergeFromFieldMask() throws Exception {
    FieldMaskTree tree = new FieldMaskTree(FieldMaskUtil.fromString("foo,bar.baz,bar.quz"));
    assertEquals("bar.baz,bar.quz,foo", tree.toString());
    tree.mergeFromFieldMask(FieldMaskUtil.fromString("foo.bar,bar"));
    assertEquals("bar,foo", tree.toString());
  }

  public void testIntersectFieldPath() throws Exception {
    FieldMaskTree tree = new FieldMaskTree(FieldMaskUtil.fromString("foo,bar.baz,bar.quz"));
    FieldMaskTree result = new FieldMaskTree();
    // Empty path.
    tree.intersectFieldPath("", result);
    assertEquals("", result.toString());
    // Non-exist path.
    tree.intersectFieldPath("quz", result);
    assertEquals("", result.toString());
    // Sub-path of an existing leaf.
    tree.intersectFieldPath("foo.bar", result);
    assertEquals("foo.bar", result.toString());
    // Match an existing leaf node.
    tree.intersectFieldPath("foo", result);
    assertEquals("foo", result.toString());
    // Non-exist path.
    tree.intersectFieldPath("bar.foo", result);
    assertEquals("foo", result.toString());
    // Match a non-leaf node.
    tree.intersectFieldPath("bar", result);
    assertEquals("bar.baz,bar.quz,foo", result.toString());
  }

  public void testMerge() throws Exception {
    testMergeImpl(true);
    testMergeImpl(false);
    testMergeRequire(false);
    testMergeRequire(true);
  }

  private void merge(
      FieldMaskTree tree,
      Message source,
      Message.Builder builder,
      FieldMaskUtil.MergeOptions options,
      boolean useDynamicMessage)
      throws Exception {
    if (useDynamicMessage) {
      Message.Builder newBuilder =
          DynamicMessage.newBuilder(source.getDescriptorForType())
              .mergeFrom(builder.buildPartial().toByteArray());
      tree.merge(
          DynamicMessage.newBuilder(source.getDescriptorForType())
              .mergeFrom(source.toByteArray())
              .build(),
          newBuilder,
          options);
      builder.clear();
      builder.mergeFrom(newBuilder.buildPartial());
    } else {
      tree.merge(source, builder, options);
    }
  }

  private void testMergeRequire(boolean useDynamicMessage) throws Exception {
    TestRequired value = TestRequired.newBuilder().setA(4321).setB(8765).setC(233333).build();
    TestRequiredMessage source = TestRequiredMessage.newBuilder().setRequiredMessage(value).build();

    FieldMaskUtil.MergeOptions options = new FieldMaskUtil.MergeOptions();
    TestRequiredMessage.Builder builder = TestRequiredMessage.newBuilder();
    merge(
        new FieldMaskTree().addFieldPath("required_message.a"),
        source,
        builder,
        options,
        useDynamicMessage);
    assertTrue(builder.hasRequiredMessage());
    assertTrue(builder.getRequiredMessage().hasA());
    assertFalse(builder.getRequiredMessage().hasB());
    assertFalse(builder.getRequiredMessage().hasC());
    merge(
        new FieldMaskTree().addFieldPath("required_message.b").addFieldPath("required_message.c"),
        source,
        builder,
        options,
        useDynamicMessage);
    try {
      assertEquals(builder.build(), source);
    } catch (UninitializedMessageException e) {
      throw new AssertionError("required field isn't set", e);
    }
  }

  private void testMergeImpl(boolean useDynamicMessage) throws Exception {
    TestAllTypes value =
        TestAllTypes.newBuilder()
            .setOptionalInt32(1234)
            .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(5678))
            .addRepeatedInt32(4321)
            .addRepeatedNestedMessage(NestedMessage.newBuilder().setBb(8765))
            .build();
    NestedTestAllTypes source =
        NestedTestAllTypes.newBuilder()
            .setPayload(value)
            .setChild(NestedTestAllTypes.newBuilder().setPayload(value))
            .build();
    // Now we have a message source with the following structure:
    //   [root] -+- payload -+- optional_int32
    //           |           +- optional_nested_message
    //           |           +- repeated_int32
    //           |           +- repeated_nested_message
    //           |
    //           +- child --- payload -+- optional_int32
    //                                 +- optional_nested_message
    //                                 +- repeated_int32
    //                                 +- repeated_nested_message

    FieldMaskUtil.MergeOptions options = new FieldMaskUtil.MergeOptions();

    // Test merging each individual field.
    NestedTestAllTypes.Builder builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        source, builder, options, useDynamicMessage);
    NestedTestAllTypes.Builder expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().setOptionalInt32(1234);
    assertEquals(expected.build(), builder.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_nested_message"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().setOptionalNestedMessage(NestedMessage.newBuilder().setBb(5678));
    assertEquals(expected.build(), builder.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().addRepeatedInt32(4321);
    assertEquals(expected.build(), builder.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.repeated_nested_message"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().addRepeatedNestedMessage(NestedMessage.newBuilder().setBb(8765));
    assertEquals(expected.build(), builder.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(
        new FieldMaskTree().addFieldPath("child.payload.optional_int32"),
        source,
        builder,
        options,
        useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getChildBuilder().getPayloadBuilder().setOptionalInt32(1234);
    assertEquals(expected.build(), builder.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(
        new FieldMaskTree().addFieldPath("child.payload.optional_nested_message"),
        source,
        builder,
        options,
        useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected
        .getChildBuilder()
        .getPayloadBuilder()
        .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(5678));
    assertEquals(expected.build(), builder.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("child.payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getChildBuilder().getPayloadBuilder().addRepeatedInt32(4321);
    assertEquals(expected.build(), builder.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("child.payload.repeated_nested_message"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected
        .getChildBuilder()
        .getPayloadBuilder()
        .addRepeatedNestedMessage(NestedMessage.newBuilder().setBb(8765));
    assertEquals(expected.build(), builder.build());

    // Test merging all fields.
    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("child").addFieldPath("payload"),
        source, builder, options, useDynamicMessage);
    assertEquals(source, builder.build());

    // Test repeated options.
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().addRepeatedInt32(1000);
    merge(new FieldMaskTree().addFieldPath("payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    // Default behavior is to append repeated fields.
    assertEquals(2, builder.getPayload().getRepeatedInt32Count());
    assertEquals(1000, builder.getPayload().getRepeatedInt32(0));
    assertEquals(4321, builder.getPayload().getRepeatedInt32(1));
    // Change to replace repeated fields.
    options.setReplaceRepeatedFields(true);
    merge(new FieldMaskTree().addFieldPath("payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    assertEquals(1, builder.getPayload().getRepeatedInt32Count());
    assertEquals(4321, builder.getPayload().getRepeatedInt32(0));

    // Test message options.
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().setOptionalInt32(1000);
    builder.getPayloadBuilder().setOptionalUint32(2000);
    merge(new FieldMaskTree().addFieldPath("payload"),
        source, builder, options, useDynamicMessage);
    // Default behavior is to merge message fields.
    assertEquals(1234, builder.getPayload().getOptionalInt32());
    assertEquals(2000, builder.getPayload().getOptionalUint32());

    // Test merging unset message fields.
    NestedTestAllTypes clearedSource = source.toBuilder().clearPayload().build();
    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload"),
        clearedSource, builder, options, useDynamicMessage);
    assertEquals(false, builder.hasPayload());

    // Skip a message field if they are unset in both source and target.
    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        clearedSource, builder, options, useDynamicMessage);
    assertEquals(false, builder.hasPayload());

    // Change to replace message fields.
    options.setReplaceMessageFields(true);
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().setOptionalInt32(1000);
    builder.getPayloadBuilder().setOptionalUint32(2000);
    merge(new FieldMaskTree().addFieldPath("payload"),
        source, builder, options, useDynamicMessage);
    assertEquals(1234, builder.getPayload().getOptionalInt32());
    assertEquals(0, builder.getPayload().getOptionalUint32());

    // Test merging unset message fields.
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().setOptionalInt32(1000);
    builder.getPayloadBuilder().setOptionalUint32(2000);
    merge(new FieldMaskTree().addFieldPath("payload"),
        clearedSource, builder, options, useDynamicMessage);
    assertEquals(false, builder.hasPayload());

    // Test merging unset primitive fields.
    builder = source.toBuilder();
    builder.getPayloadBuilder().clearOptionalInt32();
    NestedTestAllTypes sourceWithPayloadInt32Unset = builder.build();
    builder = source.toBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        sourceWithPayloadInt32Unset, builder, options, useDynamicMessage);
    assertEquals(true, builder.getPayload().hasOptionalInt32());
    assertEquals(0, builder.getPayload().getOptionalInt32());

    // Change to clear unset primitive fields.
    options.setReplacePrimitiveFields(true);
    builder = source.toBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        sourceWithPayloadInt32Unset, builder, options, useDynamicMessage);
    assertEquals(true, builder.hasPayload());
    assertEquals(false, builder.getPayload().hasOptionalInt32());
  }
}
