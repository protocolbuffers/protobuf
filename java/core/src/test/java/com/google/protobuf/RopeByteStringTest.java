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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import java.util.Iterator;

/**
 * This class tests {@link RopeByteString} by inheriting the tests from {@link
 * LiteralByteStringTest}. Only a couple of methods are overridden.
 *
 * <p>A full test of the result of {@link RopeByteString#substring(int, int)} is found in the
 * separate class {@link RopeByteStringSubstringTest}.
 *
 * @author carlanton@google.com (Carl Haverl)
 */
public class RopeByteStringTest extends LiteralByteStringTest {

  @Override
  protected void setUp() throws Exception {
    classUnderTest = "RopeByteString";
    referenceBytes = ByteStringTest.getTestBytes(22341, 22337766L);
    Iterator<ByteString> iter = ByteStringTest.makeConcretePieces(referenceBytes).iterator();
    stringUnderTest = iter.next();
    while (iter.hasNext()) {
      stringUnderTest = stringUnderTest.concat(iter.next());
    }
    expectedHashCode = -1214197238;
  }

  public void testMinLength() {
    // minLength should match the Fibonacci sequence
    int a = 1;
    int b = 1;
    int i;
    for (i = 0; a > 0; i++) {
      assertEquals(a, RopeByteString.minLength(i));
      int c = a + b;
      a = b;
      b = c;
    }
    assertEquals(Integer.MAX_VALUE, RopeByteString.minLength(i));
    assertEquals(Integer.MAX_VALUE, RopeByteString.minLength(i + 1));
    assertEquals(i + 1, RopeByteString.minLengthByDepth.length);
  }

  @Override
  public void testGetTreeDepth() {
    assertEquals(
        classUnderTest + " must have the expected tree depth", 4, stringUnderTest.getTreeDepth());
  }

  public void testBalance() {
    int numberOfPieces = 10000;
    int pieceSize = 64;
    byte[] testBytes = ByteStringTest.getTestBytes(numberOfPieces * pieceSize, 113377L);

    // Build up a big ByteString from smaller pieces to force a rebalance
    ByteString concatenated = ByteString.EMPTY;
    for (int i = 0; i < numberOfPieces; ++i) {
      concatenated = concatenated.concat(ByteString.copyFrom(testBytes, i * pieceSize, pieceSize));
    }

    assertEquals(
        classUnderTest + " from string must have the expected type",
        classUnderTest,
        getActualClassName(concatenated));
    assertTrue(
        classUnderTest + " underlying bytes must match after balancing",
        Arrays.equals(testBytes, concatenated.toByteArray()));
    ByteString testString = ByteString.copyFrom(testBytes);
    assertEquals(
        classUnderTest + " balanced string must equal flat string", testString, concatenated);
    assertEquals(
        classUnderTest + " flat string must equal balanced string", concatenated, testString);
    assertEquals(
        classUnderTest + " balanced string must have same hash code as flat string",
        testString.hashCode(),
        concatenated.hashCode());
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

    assertEquals(
        classUnderTest + " from string must have the expected type",
        classUnderTest,
        getActualClassName(unicode));
    String roundTripString = unicode.toString(UTF_8);
    assertEquals(classUnderTest + " unicode bytes must match", testString, roundTripString);
    ByteString flatString = ByteString.copyFromUtf8(testString);
    assertEquals(classUnderTest + " string must equal the flat string", flatString, unicode);
    assertEquals(
        classUnderTest + " string must must have same hashCode as the flat string",
        flatString.hashCode(),
        unicode.hashCode());
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

    assertEquals(
        classUnderTest + " from string must have the expected type",
        classUnderTest,
        getActualClassName(unicode));
    String roundTripString = unicode.toString(Internal.UTF_8);
    assertEquals(classUnderTest + " unicode bytes must match", testString, roundTripString);
    ByteString flatString = ByteString.copyFromUtf8(testString);
    assertEquals(classUnderTest + " string must equal the flat string", flatString, unicode);
    assertEquals(
        classUnderTest + " string must must have same hashCode as the flat string",
        flatString.hashCode(),
        unicode.hashCode());
  }

  @Override
  public void testToString_returnsCanonicalEmptyString() {
    RopeByteString ropeByteString =
        RopeByteString.newInstanceForTest(ByteString.EMPTY, ByteString.EMPTY);
    assertSame(
        classUnderTest + " must be the same string references",
        ByteString.EMPTY.toString(Internal.UTF_8),
        ropeByteString.toString(Internal.UTF_8));
  }

  @Override
  public void testToString_raisesException() {
    try {
      ByteString byteString = RopeByteString.newInstanceForTest(ByteString.EMPTY, ByteString.EMPTY);
      byteString.toString("invalid");
      fail("Should have thrown an exception.");
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }

    try {
      ByteString byteString =
          RopeByteString.concatenate(
              ByteString.copyFromUtf8("foo"), ByteString.copyFromUtf8("bar"));
      byteString.toString("invalid");
      fail("Should have thrown an exception.");
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }
  }

  @Override
  public void testJavaSerialization() throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    ObjectOutputStream oos = new ObjectOutputStream(out);
    oos.writeObject(stringUnderTest);
    oos.close();
    byte[] pickled = out.toByteArray();
    InputStream in = new ByteArrayInputStream(pickled);
    ObjectInputStream ois = new ObjectInputStream(in);
    Object o = ois.readObject();
    assertTrue("Didn't get a ByteString back", o instanceof ByteString);
    assertEquals("Should get an equal ByteString back", stringUnderTest, o);
  }
}
