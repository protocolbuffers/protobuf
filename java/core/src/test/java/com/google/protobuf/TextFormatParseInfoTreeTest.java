// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import protobuf_unittest.UnittestProto.TestAllTypes;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test {@link TextFormatParseInfoTree}. */
@RunWith(JUnit4.class)
public class TextFormatParseInfoTreeTest {

  private static final Descriptor DESCRIPTOR = TestAllTypes.getDescriptor();
  private static final FieldDescriptor OPTIONAL_INT32 =
      DESCRIPTOR.findFieldByName("optional_int32");
  private static final FieldDescriptor OPTIONAL_BOOLEAN =
      DESCRIPTOR.findFieldByName("optional_boolean");
  private static final FieldDescriptor REPEATED_INT32 =
      DESCRIPTOR.findFieldByName("repeated_int32");
  private static final FieldDescriptor OPTIONAL_NESTED_MESSAGE =
      DESCRIPTOR.findFieldByName("optional_nested_message");
  private static final FieldDescriptor REPEATED_NESTED_MESSAGE =
      DESCRIPTOR.findFieldByName("repeated_nested_message");
  private static final FieldDescriptor FIELD_BB =
      TestAllTypes.NestedMessage.getDescriptor().findFieldByName("bb");

  private static final TextFormatParseLocation LOC0 = TextFormatParseLocation.create(1, 2);
  private static final TextFormatParseLocation LOC1 = TextFormatParseLocation.create(2, 3);

  private TextFormatParseInfoTree.Builder rootBuilder;

  @Before
  public void setUp() {
    rootBuilder = TextFormatParseInfoTree.builder();
  }

  @Test
  public void testBuildEmptyParseTree() {
    TextFormatParseInfoTree tree = rootBuilder.build();
    assertThat(tree.getLocations(null)).isEmpty();
  }

  @Test
  public void testGetLocationReturnsSingleLocation() {
    rootBuilder.setLocation(OPTIONAL_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertThat(root.getLocation(OPTIONAL_INT32, 0)).isEqualTo(LOC0);
    assertThat(root.getLocations(OPTIONAL_INT32)).hasSize(1);
  }

  @Test
  public void testGetLocationsReturnsNoParseLocationsForUnknownField() {
    assertThat(rootBuilder.build().getLocations(OPTIONAL_INT32)).isEmpty();
    rootBuilder.setLocation(OPTIONAL_BOOLEAN, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertThat(root.getLocations(OPTIONAL_INT32)).isEmpty();
    assertThat(root.getLocations(OPTIONAL_BOOLEAN).get(0)).isEqualTo(LOC0);
  }

  @Test
  public void testGetLocationThrowsIllegalArgumentExceptionForUnknownField() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    try {
      root.getNestedTree(OPTIONAL_INT32, 0);
      assertWithMessage("Did not detect unknown field").fail();
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  @Test
  public void testGetLocationThrowsIllegalArgumentExceptionForInvalidIndex() {
    TextFormatParseInfoTree root = rootBuilder.setLocation(OPTIONAL_INT32, LOC0).build();
    try {
      root.getLocation(OPTIONAL_INT32, 1);
      assertWithMessage("Invalid index not detected").fail();
    } catch (IllegalArgumentException expected) {
      // pass
    }
    try {
      root.getLocation(OPTIONAL_INT32, -1);
      assertWithMessage("Negative index not detected").fail();
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  @Test
  public void testGetLocationsReturnsMultipleLocations() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    rootBuilder.setLocation(REPEATED_INT32, LOC1);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertThat(root.getLocation(REPEATED_INT32, 0)).isEqualTo(LOC0);
    assertThat(root.getLocation(REPEATED_INT32, 1)).isEqualTo(LOC1);
    assertThat(root.getLocations(REPEATED_INT32)).hasSize(2);
  }

  @Test
  public void testGetNestedTreeThrowsIllegalArgumentExceptionForUnknownField() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    try {
      root.getNestedTree(OPTIONAL_NESTED_MESSAGE, 0);
      assertWithMessage("Did not detect unknown field").fail();
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  @Test
  public void testGetNestedTreesReturnsNoParseInfoTreesForUnknownField() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertThat(root.getNestedTrees(OPTIONAL_NESTED_MESSAGE)).isEmpty();
  }

  @Test
  public void testGetNestedTreeThrowsIllegalArgumentExceptionForInvalidIndex() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    rootBuilder.getBuilderForSubMessageField(OPTIONAL_NESTED_MESSAGE);
    TextFormatParseInfoTree root = rootBuilder.build();
    try {
      root.getNestedTree(OPTIONAL_NESTED_MESSAGE, 1);
      assertWithMessage("Submessage index that is too large not detected").fail();
    } catch (IllegalArgumentException expected) {
      // pass
    }
    try {
      rootBuilder.build().getNestedTree(OPTIONAL_NESTED_MESSAGE, -1);
      assertWithMessage("Invalid submessage index (-1) not detected").fail();
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  @Test
  public void testGetNestedTreesReturnsSingleTree() {
    rootBuilder.getBuilderForSubMessageField(OPTIONAL_NESTED_MESSAGE);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertThat(root.getNestedTrees(OPTIONAL_NESTED_MESSAGE)).hasSize(1);
    TextFormatParseInfoTree subtree = root.getNestedTrees(OPTIONAL_NESTED_MESSAGE).get(0);
    assertThat(subtree).isNotNull();
  }

  @Test
  public void testGetNestedTreesReturnsMultipleTrees() {
    TextFormatParseInfoTree.Builder subtree1Builder =
        rootBuilder.getBuilderForSubMessageField(REPEATED_NESTED_MESSAGE);
    subtree1Builder.getBuilderForSubMessageField(FIELD_BB);
    subtree1Builder.getBuilderForSubMessageField(FIELD_BB);
    TextFormatParseInfoTree.Builder subtree2Builder =
        rootBuilder.getBuilderForSubMessageField(REPEATED_NESTED_MESSAGE);
    subtree2Builder.getBuilderForSubMessageField(FIELD_BB);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertThat(root.getNestedTrees(REPEATED_NESTED_MESSAGE)).hasSize(2);
    assertThat(root.getNestedTrees(REPEATED_NESTED_MESSAGE).get(0).getNestedTrees(FIELD_BB))
        .hasSize(2);
    assertThat(root.getNestedTrees(REPEATED_NESTED_MESSAGE).get(1).getNestedTrees(FIELD_BB))
        .hasSize(1);
  }
}
