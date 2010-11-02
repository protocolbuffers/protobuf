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


import protobuf_unittest.UnittestProto;

import junit.framework.TestCase;

import java.io.IOException;

/**
 * Tests to make sure the lazy conversion of UTF8-encoded byte arrays to
 * strings works correctly.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class LazyStringEndToEndTest extends TestCase {

  private static ByteString TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8 =
      ByteString.copyFrom(new byte[] {
          114, 4, -1, 0, -1, 0, -30, 2, 4, -1,
          0, -1, 0, -30, 2, 4, -1, 0, -1, 0, });

  /**
   * Tests that an invalid UTF8 string will roundtrip through a parse
   * and serialization.
   */
  public void testParseAndSerialize() throws InvalidProtocolBufferException {
    UnittestProto.TestAllTypes tV2 = UnittestProto.TestAllTypes.parseFrom(
        TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8);
    ByteString bytes = tV2.toByteString();
    assertEquals(TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8, bytes);

    tV2.getOptionalString();
    bytes = tV2.toByteString();
    assertEquals(TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8, bytes);
  }

  public void testParseAndWrite() throws IOException {
    UnittestProto.TestAllTypes tV2 = UnittestProto.TestAllTypes.parseFrom(
        TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8);
    byte[] sink = new byte[TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8.size()];
    CodedOutputStream outputStream = CodedOutputStream.newInstance(sink);
    tV2.writeTo(outputStream);
    outputStream.flush();
    assertEquals(
        TEST_ALL_TYPES_SERIALIZED_WITH_ILLEGAL_UTF8,
        ByteString.copyFrom(sink));
  }

  public void testCaching() {
    String a = "a";
    String b = "b";
    String c = "c";
    UnittestProto.TestAllTypes proto = UnittestProto.TestAllTypes.newBuilder()
        .setOptionalString(a)
        .addRepeatedString(b)
        .addRepeatedString(c)
        .build();

    // String should be the one we passed it.
    assertSame(a, proto.getOptionalString());
    assertSame(b, proto.getRepeatedString(0));
    assertSame(c, proto.getRepeatedString(1));


    // There's no way to directly observe that the ByteString is cached
    // correctly on serialization, but we can observe that it had to recompute
    // the string after serialization.
    proto.toByteString();
    String aPrime = proto.getOptionalString();
    assertNotSame(a, aPrime);
    assertEquals(a, aPrime);
    String bPrime = proto.getRepeatedString(0);
    assertNotSame(b, bPrime);
    assertEquals(b, bPrime);
    String cPrime = proto.getRepeatedString(1);
    assertNotSame(c, cPrime);
    assertEquals(c, cPrime);

    // And now the string should stay cached.
    assertSame(aPrime, proto.getOptionalString());
    assertSame(bPrime, proto.getRepeatedString(0));
    assertSame(cPrime, proto.getRepeatedString(1));
  }
}
