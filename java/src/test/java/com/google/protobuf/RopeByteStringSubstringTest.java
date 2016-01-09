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
import java.util.Iterator;

/**
 * This class tests {@link RopeByteString#substring(int, int)} by inheriting the tests from
 * {@link LiteralByteStringTest}.  Only a couple of methods are overridden.
 *
 * @author carlanton@google.com (Carl Haverl)
 */
public class RopeByteStringSubstringTest extends LiteralByteStringTest {

  @Override
  protected void setUp() throws Exception {
    classUnderTest = "RopeByteString";
    byte[] sourceBytes = ByteStringTest.getTestBytes(22341, 22337766L);
    Iterator<ByteString> iter = ByteStringTest.makeConcretePieces(sourceBytes).iterator();
    ByteString sourceString = iter.next();
    while (iter.hasNext()) {
      sourceString = sourceString.concat(iter.next());
    }

    int from = 1130;
    int to = sourceBytes.length - 5555;
    stringUnderTest = sourceString.substring(from, to);
    referenceBytes = new byte[to - from];
    System.arraycopy(sourceBytes, from, referenceBytes, 0, to - from);
    expectedHashCode = -1259260680;
  }

  @Override
  public void testGetTreeDepth() {
    assertEquals(classUnderTest + " must have the expected tree depth",
        3, stringUnderTest.getTreeDepth());
  }

  @Override
  public void testToString() throws UnsupportedEncodingException {
    String sourceString = "I love unicode \u1234\u5678 characters";
    ByteString sourceByteString = ByteString.copyFromUtf8(sourceString);
    int copies = 250;

    // By building the RopeByteString by concatenating, this is actually a fairly strenuous test.
    StringBuilder builder = new StringBuilder(copies * sourceString.length());
    ByteString unicode = ByteString.EMPTY;
    for (int i = 0; i < copies; ++i) {
      builder.append(sourceString);
      unicode = RopeByteString.concatenate(unicode, sourceByteString);
    }
    String testString = builder.toString();

    // Do the substring part
    testString = testString.substring(2, testString.length() - 6);
    unicode = unicode.substring(2, unicode.size() - 6);

    assertEquals(classUnderTest + " from string must have the expected type",
        classUnderTest, getActualClassName(unicode));
    String roundTripString = unicode.toString(UTF_8);
    assertEquals(classUnderTest + " unicode bytes must match",
        testString, roundTripString);
    ByteString flatString = ByteString.copyFromUtf8(testString);
    assertEquals(classUnderTest + " string must equal the flat string", flatString, unicode);
    assertEquals(classUnderTest + " string must must have same hashCode as the flat string",
        flatString.hashCode(), unicode.hashCode());
  }

  @Override
  public void testCharsetToString() {
    String sourceString = "I love unicode \u1234\u5678 characters";
    ByteString sourceByteString = ByteString.copyFromUtf8(sourceString);
    int copies = 250;

    // By building the RopeByteString by concatenating, this is actually a fairly strenuous test.
    StringBuilder builder = new StringBuilder(copies * sourceString.length());
    ByteString unicode = ByteString.EMPTY;
    for (int i = 0; i < copies; ++i) {
      builder.append(sourceString);
      unicode = RopeByteString.concatenate(unicode, sourceByteString);
    }
    String testString = builder.toString();

    // Do the substring part
    testString = testString.substring(2, testString.length() - 6);
    unicode = unicode.substring(2, unicode.size() - 6);

    assertEquals(classUnderTest + " from string must have the expected type",
        classUnderTest, getActualClassName(unicode));
    String roundTripString = unicode.toString(Internal.UTF_8);
    assertEquals(classUnderTest + " unicode bytes must match",
        testString, roundTripString);
    ByteString flatString = ByteString.copyFromUtf8(testString);
    assertEquals(classUnderTest + " string must equal the flat string", flatString, unicode);
    assertEquals(classUnderTest + " string must must have same hashCode as the flat string",
        flatString.hashCode(), unicode.hashCode());
  }
}
