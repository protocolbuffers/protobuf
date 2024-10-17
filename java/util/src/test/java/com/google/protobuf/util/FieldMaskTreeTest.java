// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.DynamicMessage;
import com.google.protobuf.Message;
import com.google.protobuf.UninitializedMessageException;
import protobuf_unittest.UnittestProto.NestedTestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes.NestedMessage;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestRequiredMessage;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FieldMaskTreeTest {
  @Test
  public void testAddFieldPath() throws Exception {
    FieldMaskTree tree = new FieldMaskTree();
    assertThat(tree.toString()).isEmpty();
    tree.addFieldPath("");
    assertThat(tree.toString()).isEmpty();
    // New branch.
    tree.addFieldPath("foo");
    assertThat(tree.toString()).isEqualTo("foo");
    // Redundant path.
    tree.addFieldPath("foo");
    assertThat(tree.toString()).isEqualTo("foo");
    // New branch.
    tree.addFieldPath("bar.baz");
    assertThat(tree.toString()).isEqualTo("bar.baz,foo");
    // Redundant sub-path.
    tree.addFieldPath("foo.bar");
    assertThat(tree.toString()).isEqualTo("bar.baz,foo");
    // New branch from a non-root node.
    tree.addFieldPath("bar.quz");
    assertThat(tree.toString()).isEqualTo("bar.baz,bar.quz,foo");
    // A path that matches several existing sub-paths.
    tree.addFieldPath("bar");
    assertThat(tree.toString()).isEqualTo("bar,foo");
  }

  @Test
  public void testMergeFromFieldMask() throws Exception {
    FieldMaskTree tree = new FieldMaskTree(FieldMaskUtil.fromString("foo,bar.baz,bar.quz"));
    assertThat(tree.toString()).isEqualTo("bar.baz,bar.quz,foo");
    tree.mergeFromFieldMask(FieldMaskUtil.fromString("foo.bar,bar"));
    assertThat(tree.toString()).isEqualTo("bar,foo");
  }

  @Test
  public void testRemoveFieldPath() throws Exception {
    String initialTreeString = "bar.baz,bar.quz.bar,foo";
    FieldMaskTree tree;

    // Empty path.
    tree = new FieldMaskTree(FieldMaskUtil.fromString(initialTreeString));
    tree.removeFieldPath("");
    assertThat(tree.toString()).isEqualTo(initialTreeString);

    // Non-exist sub-path of an existing leaf.
    tree = new FieldMaskTree(FieldMaskUtil.fromString(initialTreeString));
    tree.removeFieldPath("foo.bar");
    assertThat(tree.toString()).isEqualTo(initialTreeString);

    // Non-exist path.
    tree = new FieldMaskTree(FieldMaskUtil.fromString(initialTreeString));
    tree.removeFieldPath("bar.foo");
    assertThat(tree.toString()).isEqualTo(initialTreeString);

    // Match an existing leaf node -> remove leaf node.
    tree = new FieldMaskTree(FieldMaskUtil.fromString(initialTreeString));
    tree.removeFieldPath("foo");
    assertThat(tree.toString()).isEqualTo("bar.baz,bar.quz.bar");

    // Match sub-path of an existing leaf node -> recursive removal.
    tree = new FieldMaskTree(FieldMaskUtil.fromString(initialTreeString));
    tree.removeFieldPath("bar.quz.bar");
    assertThat(tree.toString()).isEqualTo("bar.baz,foo");

    // Match a non-leaf node -> remove all children.
    tree = new FieldMaskTree(FieldMaskUtil.fromString(initialTreeString));
    tree.removeFieldPath("bar");
    assertThat(tree.toString()).isEqualTo("foo");
  }

  @Test
  public void testRemoveFromFieldMask() throws Exception {
    FieldMaskTree tree = new FieldMaskTree(FieldMaskUtil.fromString("foo,bar.baz,bar.quz"));
    assertThat(tree.toString()).isEqualTo("bar.baz,bar.quz,foo");
    tree.removeFromFieldMask(FieldMaskUtil.fromString("foo.bar,bar"));
    assertThat(tree.toString()).isEqualTo("foo");
  }

  @Test
  public void testIntersectFieldPath() throws Exception {
    FieldMaskTree tree = new FieldMaskTree(FieldMaskUtil.fromString("foo,bar.baz,bar.quz"));
    FieldMaskTree result = new FieldMaskTree();
    // Empty path.
    tree.intersectFieldPath("", result);
    assertThat(result.toString()).isEmpty();
    // Non-exist path.
    tree.intersectFieldPath("quz", result);
    assertThat(result.toString()).isEmpty();
    // Sub-path of an existing leaf.
    tree.intersectFieldPath("foo.bar", result);
    assertThat(result.toString()).isEqualTo("foo.bar");
    // Match an existing leaf node.
    tree.intersectFieldPath("foo", result);
    assertThat(result.toString()).isEqualTo("foo");
    // Non-exist path.
    tree.intersectFieldPath("bar.foo", result);
    assertThat(result.toString()).isEqualTo("foo");
    // Match a non-leaf node.
    tree.intersectFieldPath("bar", result);
    assertThat(result.toString()).isEqualTo("bar.baz,bar.quz,foo");
  }

  @Test
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
    assertThat(builder.hasRequiredMessage()).isTrue();
    assertThat(builder.getRequiredMessage().hasA()).isTrue();
    assertThat(builder.getRequiredMessage().hasB()).isFalse();
    assertThat(builder.getRequiredMessage().hasC()).isFalse();
    merge(
        new FieldMaskTree().addFieldPath("required_message.b").addFieldPath("required_message.c"),
        source,
        builder,
        options,
        useDynamicMessage);
    try {
      assertThat(source).isEqualTo(builder.build());
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

    // Test merging with an empty FieldMask.
    NestedTestAllTypes.Builder builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().addRepeatedInt32(1000);
    merge(new FieldMaskTree(), source, builder, options, useDynamicMessage);
    NestedTestAllTypes.Builder expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().addRepeatedInt32(1000);
    assertThat(builder.build()).isEqualTo(expected.build());

    // Test merging each individual field.
    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().setOptionalInt32(1234);
    assertThat(builder.build()).isEqualTo(expected.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_nested_message"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().setOptionalNestedMessage(NestedMessage.newBuilder().setBb(5678));
    assertThat(builder.build()).isEqualTo(expected.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().addRepeatedInt32(4321);
    assertThat(builder.build()).isEqualTo(expected.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.repeated_nested_message"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getPayloadBuilder().addRepeatedNestedMessage(NestedMessage.newBuilder().setBb(8765));
    assertThat(builder.build()).isEqualTo(expected.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(
        new FieldMaskTree().addFieldPath("child.payload.optional_int32"),
        source,
        builder,
        options,
        useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getChildBuilder().getPayloadBuilder().setOptionalInt32(1234);
    assertThat(builder.build()).isEqualTo(expected.build());

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
    assertThat(builder.build()).isEqualTo(expected.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("child.payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected.getChildBuilder().getPayloadBuilder().addRepeatedInt32(4321);
    assertThat(builder.build()).isEqualTo(expected.build());

    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("child.payload.repeated_nested_message"),
        source, builder, options, useDynamicMessage);
    expected = NestedTestAllTypes.newBuilder();
    expected
        .getChildBuilder()
        .getPayloadBuilder()
        .addRepeatedNestedMessage(NestedMessage.newBuilder().setBb(8765));
    assertThat(builder.build()).isEqualTo(expected.build());

    // Test merging all fields.
    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("child").addFieldPath("payload"),
        source, builder, options, useDynamicMessage);
    assertThat(builder.build()).isEqualTo(source);

    // Test repeated options.
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().addRepeatedInt32(1000);
    merge(new FieldMaskTree().addFieldPath("payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    // Default behavior is to append repeated fields.
    assertThat(builder.getPayload().getRepeatedInt32Count()).isEqualTo(2);
    assertThat(builder.getPayload().getRepeatedInt32(0)).isEqualTo(1000);
    assertThat(builder.getPayload().getRepeatedInt32(1)).isEqualTo(4321);
    // Change to replace repeated fields.
    options.setReplaceRepeatedFields(true);
    merge(new FieldMaskTree().addFieldPath("payload.repeated_int32"),
        source, builder, options, useDynamicMessage);
    assertThat(builder.getPayload().getRepeatedInt32Count()).isEqualTo(1);
    assertThat(builder.getPayload().getRepeatedInt32(0)).isEqualTo(4321);

    // Test message options.
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().setOptionalInt32(1000);
    builder.getPayloadBuilder().setOptionalUint32(2000);
    merge(new FieldMaskTree().addFieldPath("payload"),
        source, builder, options, useDynamicMessage);
    // Default behavior is to merge message fields.
    assertThat(builder.getPayload().getOptionalInt32()).isEqualTo(1234);
    assertThat(builder.getPayload().getOptionalUint32()).isEqualTo(2000);

    // Test merging unset message fields.
    NestedTestAllTypes clearedSource = source.toBuilder().clearPayload().build();
    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload"),
        clearedSource, builder, options, useDynamicMessage);
    assertThat(builder.hasPayload()).isFalse();

    // Skip a message field if they are unset in both source and target.
    builder = NestedTestAllTypes.newBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        clearedSource, builder, options, useDynamicMessage);
    assertThat(builder.hasPayload()).isFalse();

    // Change to replace message fields.
    options.setReplaceMessageFields(true);
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().setOptionalInt32(1000);
    builder.getPayloadBuilder().setOptionalUint32(2000);
    merge(new FieldMaskTree().addFieldPath("payload"),
        source, builder, options, useDynamicMessage);
    assertThat(builder.getPayload().getOptionalInt32()).isEqualTo(1234);
    assertThat(builder.getPayload().getOptionalUint32()).isEqualTo(0);

    // Test merging unset message fields.
    builder = NestedTestAllTypes.newBuilder();
    builder.getPayloadBuilder().setOptionalInt32(1000);
    builder.getPayloadBuilder().setOptionalUint32(2000);
    merge(new FieldMaskTree().addFieldPath("payload"),
        clearedSource, builder, options, useDynamicMessage);
    assertThat(builder.hasPayload()).isFalse();

    // Test merging unset primitive fields.
    builder = source.toBuilder();
    builder.getPayloadBuilder().clearOptionalInt32();
    NestedTestAllTypes sourceWithPayloadInt32Unset = builder.build();
    builder = source.toBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        sourceWithPayloadInt32Unset, builder, options, useDynamicMessage);
    assertThat(builder.getPayload().hasOptionalInt32()).isTrue();
    assertThat(builder.getPayload().getOptionalInt32()).isEqualTo(0);

    // Change to clear unset primitive fields.
    options.setReplacePrimitiveFields(true);
    builder = source.toBuilder();
    merge(new FieldMaskTree().addFieldPath("payload.optional_int32"),
        sourceWithPayloadInt32Unset, builder, options, useDynamicMessage);
    assertThat(builder.hasPayload()).isTrue();
    assertThat(builder.getPayload().hasOptionalInt32()).isFalse();
  }
}
