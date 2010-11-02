// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

import com.google.protobuf.UnittestLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import com.google.protobuf.UnittestLite.TestNestedExtensionLite;

import junit.framework.TestCase;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

/**
 * Test lite runtime.
 *
 * @author kenton@google.com Kenton Varda
 */
public class LiteTest extends TestCase {
  public void setUp() throws Exception {
    // Test that nested extensions are initialized correctly even if the outer
    // class has not been accessed directly.  This was once a bug with lite
    // messages.
    //
    // We put this in setUp() rather than in its own test method because we
    // need to make sure it runs before any actual tests.
    assertTrue(TestNestedExtensionLite.nestedExtension != null);
  }

  public void testLite() throws Exception {
    // Since lite messages are a subset of regular messages, we can mostly
    // assume that the functionality of lite messages is already thoroughly
    // tested by the regular tests.  All this test really verifies is that
    // a proto with optimize_for = LITE_RUNTIME compiles correctly when
    // linked only against the lite library.  That is all tested at compile
    // time, leaving not much to do in this method.  Let's just do some random
    // stuff to make sure the lite message is actually here and usable.

    TestAllTypesLite message =
      TestAllTypesLite.newBuilder()
                      .setOptionalInt32(123)
                      .addRepeatedString("hello")
                      .setOptionalNestedMessage(
                          TestAllTypesLite.NestedMessage.newBuilder().setBb(7))
                      .build();

    ByteString data = message.toByteString();

    TestAllTypesLite message2 = TestAllTypesLite.parseFrom(data);

    assertEquals(123, message2.getOptionalInt32());
    assertEquals(1, message2.getRepeatedStringCount());
    assertEquals("hello", message2.getRepeatedString(0));
    assertEquals(7, message2.getOptionalNestedMessage().getBb());
  }

  public void testLiteExtensions() throws Exception {
    // TODO(kenton):  Unlike other features of the lite library, extensions are
    //   implemented completely differently from the regular library.  We
    //   should probably test them more thoroughly.

    TestAllExtensionsLite message =
      TestAllExtensionsLite.newBuilder()
        .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
        .addExtension(UnittestLite.repeatedStringExtensionLite, "hello")
        .setExtension(UnittestLite.optionalNestedEnumExtensionLite,
            TestAllTypesLite.NestedEnum.BAZ)
        .setExtension(UnittestLite.optionalNestedMessageExtensionLite,
            TestAllTypesLite.NestedMessage.newBuilder().setBb(7).build())
        .build();

    // Test copying a message, since coping extensions actually does use a
    // different code path between lite and regular libraries, and as of this
    // writing, parsing hasn't been implemented yet.
    TestAllExtensionsLite message2 = message.toBuilder().build();

    assertEquals(123, (int) message2.getExtension(
        UnittestLite.optionalInt32ExtensionLite));
    assertEquals(1, message2.getExtensionCount(
        UnittestLite.repeatedStringExtensionLite));
    assertEquals(1, message2.getExtension(
        UnittestLite.repeatedStringExtensionLite).size());
    assertEquals("hello", message2.getExtension(
        UnittestLite.repeatedStringExtensionLite, 0));
    assertEquals(TestAllTypesLite.NestedEnum.BAZ, message2.getExtension(
        UnittestLite.optionalNestedEnumExtensionLite));
    assertEquals(7, message2.getExtension(
        UnittestLite.optionalNestedMessageExtensionLite).getBb());
  }

  public void testSerialize() throws Exception {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    TestAllTypesLite expected =
      TestAllTypesLite.newBuilder()
                      .setOptionalInt32(123)
                      .addRepeatedString("hello")
                      .setOptionalNestedMessage(
                          TestAllTypesLite.NestedMessage.newBuilder().setBb(7))
                      .build();
    ObjectOutputStream out = new ObjectOutputStream(baos);
    out.writeObject(expected);
    out.close();
    ByteArrayInputStream bais = new ByteArrayInputStream(baos.toByteArray());
    ObjectInputStream in = new ObjectInputStream(bais);
    TestAllTypesLite actual = (TestAllTypesLite) in.readObject();
    assertEquals(expected.getOptionalInt32(), actual.getOptionalInt32());
    assertEquals(expected.getRepeatedStringCount(),
        actual.getRepeatedStringCount());
    assertEquals(expected.getRepeatedString(0),
        actual.getRepeatedString(0));
    assertEquals(expected.getOptionalNestedMessage().getBb(),
        actual.getOptionalNestedMessage().getBb());
  }
}
