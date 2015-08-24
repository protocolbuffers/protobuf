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

import com.google.protobuf.testing.UnittestProto3Utf8.TestData;
import com.google.protobuf.testing.UnittestProto3Utf8.TestUtf8Checked;
import com.google.protobuf.testing.UnittestProto3Utf8.TestUtf8Unchecked;

import junit.framework.TestCase;

/**
 * Test the UTF-8 checking semantics in proto3.
 */
public class Proto3Utf8Test extends TestCase {

  private static final ByteString INVALID_UTF8_BYTES =
      ByteString.copyFrom(new byte[]{(byte) 0xff});
  private static final ByteString VALID_UTF8_BYTES =
      ByteString.copyFrom(Internal.toByteArray("Hello world!"));

  private void assertAccepted(Message defaultInstance, ByteString data)
      throws Exception {
    defaultInstance.newBuilderForType().mergeFrom(data).build();
  }

  private void assertRejected(Message defaultInstance, ByteString data)
      throws Exception {
    try {
      defaultInstance.newBuilderForType().mergeFrom(data).build();
      fail("Expects exceptions.");
    } catch (Exception e) {
      // Expected.
    }
  }
  
  private void assertAcceptedByUncheckedMessage(ByteString data)
      throws Exception {
  }
  
  private void assertRejectedByUncheckedMessage(ByteString data)
      throws Exception {
  }
  
  private void assertRejectedByCheckedMessage(ByteString data)
      throws Exception {
    assertRejected(TestUtf8Checked.getDefaultInstance(), data);
    assertRejected(DynamicMessage.getDefaultInstance(
        TestUtf8Checked.getDescriptor()), data);
  }
  
  public void testOptionalFields() throws Exception {
    TestData.Builder builder = TestData.newBuilder();
    builder.setOptionalValue(INVALID_UTF8_BYTES);
    TestData data = builder.build();

    assertAcceptedByUncheckedMessage(data.toByteString());
    assertRejectedByCheckedMessage(data.toByteString());
  }
  
  public void testRepeatedFields() throws Exception {
    TestData.Builder builder = TestData.newBuilder();
    builder.addRepeatedValue(VALID_UTF8_BYTES);
    builder.setOptionalValue(INVALID_UTF8_BYTES);
    TestData data = builder.build();

    assertAcceptedByUncheckedMessage(data.toByteString());
    assertRejectedByCheckedMessage(data.toByteString());
  }
  
  public void testOneofFields() throws Exception {
    TestData.Builder builder = TestData.newBuilder();
    builder.setOneofValue(INVALID_UTF8_BYTES);
    TestData data = builder.build();

    assertAcceptedByUncheckedMessage(data.toByteString());
    assertRejectedByCheckedMessage(data.toByteString());
  }
  
  public void testMapKey() throws Exception {
    TestData.Builder builder = TestData.newBuilder();
    builder.addMapValue(
        TestData.MapValueEntry.newBuilder()
            .setKey(INVALID_UTF8_BYTES)
            .setValue(VALID_UTF8_BYTES).build());
    TestData data = builder.build();

    // Map fields in Java are enforcing UTF-8 checking already.
    assertRejectedByUncheckedMessage(data.toByteString());
    assertRejectedByCheckedMessage(data.toByteString());
  }
  
  public void testMapValue() throws Exception {
    TestData.Builder builder = TestData.newBuilder();
    builder.addMapValue(
        TestData.MapValueEntry.newBuilder()
            .setKey(VALID_UTF8_BYTES)
            .setValue(INVALID_UTF8_BYTES).build());
    TestData data = builder.build();

    // Map fields in Java are enforcing UTF-8 checking already.
    assertRejectedByUncheckedMessage(data.toByteString());
    assertRejectedByCheckedMessage(data.toByteString());
  }
}
