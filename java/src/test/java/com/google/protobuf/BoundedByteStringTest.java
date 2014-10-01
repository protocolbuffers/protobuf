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

import java.io.UnsupportedEncodingException;

/**
 * This class tests {@link BoundedByteString}, which extends {@link LiteralByteString},
 * by inheriting the tests from {@link LiteralByteStringTest}.  The only method which
 * is strange enough that it needs to be overridden here is {@link #testToString()}.
 *
 * @author carlanton@google.com (Carl Haverl)
 */
public class BoundedByteStringTest extends LiteralByteStringTest {

  @Override
  protected void setUp() throws Exception {
    classUnderTest = "BoundedByteString";
    byte[] sourceBytes = ByteStringTest.getTestBytes(2341, 11337766L);
    int from = 100;
    int to = sourceBytes.length - 100;
    stringUnderTest = ByteString.copyFrom(sourceBytes).substring(from, to);
    referenceBytes = new byte[to - from];
    System.arraycopy(sourceBytes, from, referenceBytes, 0, to - from);
    expectedHashCode = 727575887;
  }

  @Override
  public void testToString() throws UnsupportedEncodingException {
    String testString = "I love unicode \u1234\u5678 characters";
    LiteralByteString unicode = new LiteralByteString(testString.getBytes(UTF_8));
    ByteString chopped = unicode.substring(2, unicode.size() - 6);
    assertEquals(classUnderTest + ".substring() must have the expected type",
        classUnderTest, getActualClassName(chopped));

    String roundTripString = chopped.toString(UTF_8);
    assertEquals(classUnderTest + " unicode bytes must match",
        testString.substring(2, testString.length() - 6), roundTripString);
  }
}
