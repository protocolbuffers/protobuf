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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import protobuf_unittest.UnittestProto.TestAllTypes;
import junit.framework.TestCase;

/** Test @{link TextFormatParseInfoTree}. */
public class TextFormatParseInfoTreeTest extends TestCase {

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

  @Override
  public void setUp() {
    rootBuilder = TextFormatParseInfoTree.builder();
  }

  public void testBuildEmptyParseTree() {
    TextFormatParseInfoTree tree = rootBuilder.build();
    assertTrue(tree.getLocations(null).isEmpty());
  }

  public void testGetLocationReturnsSingleLocation() {
    rootBuilder.setLocation(OPTIONAL_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertEquals(LOC0, root.getLocation(OPTIONAL_INT32, 0));
    assertEquals(1, root.getLocations(OPTIONAL_INT32).size());
  }

  public void testGetLocationsReturnsNoParseLocationsForUnknownField() {
    assertTrue(rootBuilder.build().getLocations(OPTIONAL_INT32).isEmpty());
    rootBuilder.setLocation(OPTIONAL_BOOLEAN, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertTrue(root.getLocations(OPTIONAL_INT32).isEmpty());
    assertEquals(LOC0, root.getLocations(OPTIONAL_BOOLEAN).get(0));
  }

  public void testGetLocationThrowsIllegalArgumentExceptionForUnknownField() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    try {
      root.getNestedTree(OPTIONAL_INT32, 0);
      fail("Did not detect unknown field");
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  public void testGetLocationThrowsIllegalArgumentExceptionForInvalidIndex() {
    TextFormatParseInfoTree root = rootBuilder.setLocation(OPTIONAL_INT32, LOC0).build();
    try {
      root.getLocation(OPTIONAL_INT32, 1);
      fail("Invalid index not detected");
    } catch (IllegalArgumentException expected) {
      // pass
    }
    try {
      root.getLocation(OPTIONAL_INT32, -1);
      fail("Negative index not detected");
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  public void testGetLocationsReturnsMultipleLocations() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    rootBuilder.setLocation(REPEATED_INT32, LOC1);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertEquals(LOC0, root.getLocation(REPEATED_INT32, 0));
    assertEquals(LOC1, root.getLocation(REPEATED_INT32, 1));
    assertEquals(2, root.getLocations(REPEATED_INT32).size());
  }

  public void testGetNestedTreeThrowsIllegalArgumentExceptionForUnknownField() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    try {
      root.getNestedTree(OPTIONAL_NESTED_MESSAGE, 0);
      fail("Did not detect unknown field");
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  public void testGetNestedTreesReturnsNoParseInfoTreesForUnknownField() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertTrue(root.getNestedTrees(OPTIONAL_NESTED_MESSAGE).isEmpty());
  }

  public void testGetNestedTreeThrowsIllegalArgumentExceptionForInvalidIndex() {
    rootBuilder.setLocation(REPEATED_INT32, LOC0);
    rootBuilder.getBuilderForSubMessageField(OPTIONAL_NESTED_MESSAGE);
    TextFormatParseInfoTree root = rootBuilder.build();
    try {
      root.getNestedTree(OPTIONAL_NESTED_MESSAGE, 1);
      fail("Submessage index that is too large not detected");
    } catch (IllegalArgumentException expected) {
      // pass
    }
    try {
      rootBuilder.build().getNestedTree(OPTIONAL_NESTED_MESSAGE, -1);
      fail("Invalid submessage index (-1) not detected");
    } catch (IllegalArgumentException expected) {
      // pass
    }
  }

  public void testGetNestedTreesReturnsSingleTree() {
    rootBuilder.getBuilderForSubMessageField(OPTIONAL_NESTED_MESSAGE);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertEquals(1, root.getNestedTrees(OPTIONAL_NESTED_MESSAGE).size());
    TextFormatParseInfoTree subtree = root.getNestedTrees(OPTIONAL_NESTED_MESSAGE).get(0);
    assertNotNull(subtree);
  }

  public void testGetNestedTreesReturnsMultipleTrees() {
    TextFormatParseInfoTree.Builder subtree1Builder =
        rootBuilder.getBuilderForSubMessageField(REPEATED_NESTED_MESSAGE);
    subtree1Builder.getBuilderForSubMessageField(FIELD_BB);
    subtree1Builder.getBuilderForSubMessageField(FIELD_BB);
    TextFormatParseInfoTree.Builder subtree2Builder =
        rootBuilder.getBuilderForSubMessageField(REPEATED_NESTED_MESSAGE);
    subtree2Builder.getBuilderForSubMessageField(FIELD_BB);
    TextFormatParseInfoTree root = rootBuilder.build();
    assertEquals(2, root.getNestedTrees(REPEATED_NESTED_MESSAGE).size());
    assertEquals(
        2, root.getNestedTrees(REPEATED_NESTED_MESSAGE).get(0).getNestedTrees(FIELD_BB).size());
    assertEquals(
        1, root.getNestedTrees(REPEATED_NESTED_MESSAGE).get(1).getNestedTrees(FIELD_BB).size());
  }
}
