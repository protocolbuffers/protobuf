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

import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;

import java.io.IOException;
import junit.framework.TestCase;

/**
 * Unit test for {@link LazyFieldLite}.
 *
 * @author xiangl@google.com (Xiang Li)
 */
public class LazyFieldLiteTest extends TestCase {

  public void testGetValue() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertEquals(message, lazyField.getValue(TestAllTypes.getDefaultInstance()));
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllTypes.getDefaultInstance()));
  }

  public void testGetValueEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertEquals(message, lazyField.getValue(TestAllExtensions.getDefaultInstance()));
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllExtensions.getDefaultInstance()));
  }

  public void testSetValue() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllTypes.getDefaultInstance()));
    message = lazyField.getValue(TestAllTypes.getDefaultInstance());
    changeValue(lazyField);
    assertEquals(message, lazyField.getValue(TestAllTypes.getDefaultInstance()));
  }

  public void testSetValueEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    changeValue(lazyField);
    assertNotEqual(message, lazyField.getValue(TestAllExtensions.getDefaultInstance()));
    MessageLite value = lazyField.getValue(TestAllExtensions.getDefaultInstance());
    changeValue(lazyField);
    assertEquals(value, lazyField.getValue(TestAllExtensions.getDefaultInstance()));
  }

  public void testGetSerializedSize() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertEquals(message.getSerializedSize(), lazyField.getSerializedSize());
    changeValue(lazyField);
    assertNotEqual(message.getSerializedSize(), lazyField.getSerializedSize());
  }

  public void testGetSerializedSizeEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertEquals(message.getSerializedSize(), lazyField.getSerializedSize());
    changeValue(lazyField);
    assertNotEqual(message.getSerializedSize(), lazyField.getSerializedSize());
  }

  public void testGetByteString() {
    MessageLite message = TestUtil.getAllSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertEquals(message.toByteString(), lazyField.toByteString());
    changeValue(lazyField);
    assertNotEqual(message.toByteString(), lazyField.toByteString());
  }

  public void testGetByteStringEx() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    LazyFieldLite lazyField = createLazyFieldLiteFromMessage(message);
    assertEquals(message.toByteString(), lazyField.toByteString());
    changeValue(lazyField);
    assertNotEqual(message.toByteString(), lazyField.toByteString());
  }


  // Help methods.

  private LazyFieldLite createLazyFieldLiteFromMessage(MessageLite message) {
    ByteString bytes = message.toByteString();
    return new LazyFieldLite(TestUtil.getExtensionRegistry(), bytes);
  }

  private void changeValue(LazyFieldLite lazyField) {
    TestAllTypes.Builder builder = TestUtil.getAllSet().toBuilder();
    builder.addRepeatedBool(true);
    MessageLite newMessage = builder.build();
    lazyField.setValue(newMessage);
  }

  private void assertNotEqual(Object unexpected, Object actual) {
    assertFalse(unexpected == actual
        || (unexpected != null && unexpected.equals(actual)));
  }

}
