// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for {@link LazyField}. */
@RunWith(JUnit4.class)
public class LazyFieldTest {

  @Test
  public void testHashCode() {
    MessageLite message = TestUtil.getAllSet();
    LazyField lazyField = createLazyFieldFromMessage(message);
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    lazyField.getValue();
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    changeValue(lazyField);
    // make sure two messages have different hash code
    assertNotEqual(message.hashCode(), lazyField.hashCode());
  }

  @Test
  public void testHashCodeEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyField lazyField = createLazyFieldFromMessage(message);
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    lazyField.getValue();
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    changeValue(lazyField);
    // make sure two messages have different hash code
    assertNotEqual(message.hashCode(), lazyField.hashCode());
  }

  @Test
  public void testGetValue() {
    MessageLite message = TestUtil.getAllSet();
    LazyField lazyField = createLazyFieldFromMessage(message);
    assertThat(message).isEqualTo(lazyField.getValue());
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue());
  }

  @Test
  public void testGetValueEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyField lazyField = createLazyFieldFromMessage(message);
    assertThat(message).isEqualTo(lazyField.getValue());
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue());
  }

  @Test
  public void testEqualsObject() {
    MessageLite message = TestUtil.getAllSet();
    LazyField lazyField = createLazyFieldFromMessage(message);
    assertThat(lazyField).isEqualTo(message);
    changeValue(lazyField);
    assertThat(lazyField).isNotEqualTo(message);
    assertThat(message).isNotEqualTo(lazyField.getValue());
  }

  @Test
  @SuppressWarnings("TruthIncompatibleType") // LazyField.equals() is not symmetric
  public void testEqualsObjectEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyField lazyField = createLazyFieldFromMessage(message);
    assertThat(lazyField).isEqualTo(message);
    changeValue(lazyField);
    assertThat(lazyField).isNotEqualTo(message);
    assertThat(message).isNotEqualTo(lazyField.getValue());
  }

  // Help methods.

  private LazyField createLazyFieldFromMessage(MessageLite message) {
    ByteString bytes = message.toByteString();
    return new LazyField(
        message.getDefaultInstanceForType(), TestUtil.getExtensionRegistry(), bytes);
  }

  private void changeValue(LazyField lazyField) {
    TestAllTypes.Builder builder = TestUtil.getAllSet().toBuilder();
    builder.addRepeatedBool(true);
    MessageLite newMessage = builder.build();
    lazyField.setValue(newMessage);
  }

  private void assertNotEqual(Object unexpected, Object actual) {
    assertThat(unexpected).isNotSameInstanceAs(actual);
    assertThat((unexpected != null && unexpected.equals(actual))).isFalse();
  }
}
