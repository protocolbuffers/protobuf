// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import proto2_unittest.NestedExtensionLite.MyNestedExtensionLite;
import proto2_unittest.NonNestedExtensionLite;
import proto2_unittest.NonNestedExtensionLite.MessageLiteToBeExtended;
import proto2_unittest.NonNestedExtensionLite.MyNonNestedExtensionLite;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class GeneratedMessageLiteTest {
  // =================================================================
  // Lite Extensions.

  @Test
  public void testLiteExtensionMessageOrBuilder() throws Exception {
    TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.newBuilder();
    TestUtilLite.setAllExtensions(builder);
    TestUtil.assertAllExtensionsSet(builder);

    TestAllExtensionsLite message = builder.build();
    TestUtil.assertAllExtensionsSet(message);
  }

  @Test
  public void testLiteExtensionRepeatedSetters() throws Exception {
    TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.newBuilder();
    TestUtilLite.setAllExtensions(builder);
    TestUtilLite.modifyRepeatedExtensions(builder);
    TestUtil.assertRepeatedExtensionsModified(builder);

    TestAllExtensionsLite message = builder.build();
    TestUtil.assertRepeatedExtensionsModified(message);
  }

  @Test
  public void testLiteExtensionDefaults() throws Exception {
    TestUtil.assertExtensionsClear(TestAllExtensionsLite.getDefaultInstance());
    TestUtil.assertExtensionsClear(TestAllExtensionsLite.newBuilder().build());
  }

  @Test
  public void testClearLiteExtension() throws Exception {
    // clearExtension() is not actually used in TestUtil, so try it manually.
    assertThat(
            TestAllExtensionsLite.newBuilder()
                .setExtension(UnittestLite.optionalInt32ExtensionLite, 1)
                .clearExtension(UnittestLite.optionalInt32ExtensionLite)
                .hasExtension(UnittestLite.optionalInt32ExtensionLite))
        .isFalse();
    assertThat(
            TestAllExtensionsLite.newBuilder()
                .addExtension(UnittestLite.repeatedInt32ExtensionLite, 1)
                .clearExtension(UnittestLite.repeatedInt32ExtensionLite)
                .getExtensionCount(UnittestLite.repeatedInt32ExtensionLite))
        .isEqualTo(0);
  }

  @Test
  public void testLiteExtensionCopy() throws Exception {
    TestAllExtensionsLite original = TestUtilLite.getAllLiteExtensionsSet();
    TestAllExtensionsLite copy = TestAllExtensionsLite.newBuilder(original).build();
    TestUtil.assertAllExtensionsSet(copy);
  }

  @Test
  public void testLiteExtensionMergeFrom() throws Exception {
    TestAllExtensionsLite original =
        TestAllExtensionsLite.newBuilder()
            .setExtension(UnittestLite.optionalInt32ExtensionLite, 1)
            .build();
    TestAllExtensionsLite merged = TestAllExtensionsLite.newBuilder().mergeFrom(original).build();
    assertThat(merged.hasExtension(UnittestLite.optionalInt32ExtensionLite)).isTrue();
    assertThat((int) merged.getExtension(UnittestLite.optionalInt32ExtensionLite)).isEqualTo(1);
  }

  @Test
  public void testNonNestedExtensionLiteInitialization() {
    assertThat(NonNestedExtensionLite.nonNestedExtensionLite.getMessageDefaultInstance())
        .isInstanceOf(MyNonNestedExtensionLite.class);
  }

  @Test
  public void testNestedExtensionLiteInitialization() {
    assertThat(MyNestedExtensionLite.recursiveExtensionLite.getMessageDefaultInstance())
        .isInstanceOf(MessageLiteToBeExtended.class);
  }
}
