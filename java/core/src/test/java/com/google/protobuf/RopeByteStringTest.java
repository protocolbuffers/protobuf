// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import java.util.Iterator;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * This class tests {@link RopeByteString} by inheriting the tests from {@link
 * LiteralByteStringTest}. Only a couple of methods are overridden.
 *
 * <p>A full test of the result of {@link RopeByteString#substring(int, int)} is found in the
 * separate class {@link RopeByteStringSubstringTest}.
 */
@RunWith(JUnit4.class)
public class RopeByteStringTest extends LiteralByteStringTest {

  @Override
  @Before
  public void setUp() throws Exception {
    classUnderTest = "RopeByteString";
    referenceBytes = ByteStringTest.getTestBytes(22341, 22337766L);
    Iterator<ByteString> iter = ByteStringTest.makeConcretePieces(referenceBytes).iterator();
    stringUnderTest = iter.next();
    while (iter.hasNext()) {
      stringUnderTest = stringUnderTest.concat(iter.next());
    }
    expectedHashCode = -1214197238;
  }

  @Test
  public void testMinLength() {
    // minLength should match the Fibonacci sequence
    int a = 1;
    int b = 1;
    int i;
    for (i = 0; a > 0; i++) {
      assertThat(a).isEqualTo(RopeByteString.minLength(i));
      int c = a + b;
      a = b;
      b = c;
    }
    assertThat(RopeByteString.minLength(i)).isEqualTo(Integer.MAX_VALUE);
    assertThat(RopeByteString.minLength(i + 1)).isEqualTo(Integer.MAX_VALUE);
    assertThat(RopeByteString.minLengthByDepth).hasLength(i + 1);
  }

  @Override
  @Test
  public void testGetTreeDepth() {
    assertWithMessage("%s must have the expected tree depth", classUnderTest)
        .that(stringUnderTest.getTreeDepth())
        .isEqualTo(4);
  }

  @Test
  public void testBalance() {
    int numberOfPieces = 10000;
    int pieceSize = 64;
    byte[] testBytes = ByteStringTest.getTestBytes(numberOfPieces * pieceSize, 113377L);

    // Build up a big ByteString from smaller pieces to force a rebalance
    ByteString concatenated = ByteString.EMPTY;
    for (int i = 0; i < numberOfPieces; ++i) {
      concatenated = concatenated.concat(ByteString.copyFrom(testBytes, i * pieceSize, pieceSize));
    }

    assertWithMessage("%s from string must have the expected type", classUnderTest)
        .that(classUnderTest)
        .isEqualTo(getActualClassName(concatenated));
    assertWithMessage("%s underlying bytes must match after balancing", classUnderTest)
        .that(Arrays.equals(testBytes, concatenated.toByteArray()))
        .isTrue();
    ByteString testString = ByteString.copyFrom(testBytes);
    assertWithMessage("%s balanced string must equal flat string", classUnderTest)
        .that(testString)
        .isEqualTo(concatenated);
    assertWithMessage("%s flat string must equal balanced string", classUnderTest)
        .that(concatenated)
        .isEqualTo(testString);
    assertWithMessage("%s balanced string must have same hash code as flat string", classUnderTest)
        .that(testString.hashCode())
        .isEqualTo(concatenated.hashCode());
  }

  @Override
  @Test
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

    assertWithMessage("%s from string must have the expected type", classUnderTest)
        .that(classUnderTest)
        .isEqualTo(getActualClassName(unicode));
    String roundTripString = unicode.toString(UTF_8);
    assertWithMessage("%s unicode bytes must match", classUnderTest)
        .that(testString)
        .isEqualTo(roundTripString);
    ByteString flatString = ByteString.copyFromUtf8(testString);
    assertWithMessage("%s string must equal the flat string", classUnderTest)
        .that(flatString)
        .isEqualTo(unicode);
    assertWithMessage("%s string must must have same hashCode as the flat string", classUnderTest)
        .that(flatString.hashCode())
        .isEqualTo(unicode.hashCode());
  }

  @Override
  @Test
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

    assertWithMessage("%s from string must have the expected type", classUnderTest)
        .that(classUnderTest)
        .isEqualTo(getActualClassName(unicode));
    String roundTripString = unicode.toString(Internal.UTF_8);
    assertWithMessage("%s unicode bytes must match", classUnderTest)
        .that(testString)
        .isEqualTo(roundTripString);
    ByteString flatString = ByteString.copyFromUtf8(testString);
    assertWithMessage("%s string must equal the flat string", classUnderTest)
        .that(flatString)
        .isEqualTo(unicode);
    assertWithMessage("%s string must must have same hashCode as the flat string", classUnderTest)
        .that(flatString.hashCode())
        .isEqualTo(unicode.hashCode());
  }

  @Override
  @Test
  public void testToString_returnsCanonicalEmptyString() {
    RopeByteString ropeByteString =
        RopeByteString.newInstanceForTest(ByteString.EMPTY, ByteString.EMPTY);
    assertWithMessage("%s must be the same string references", classUnderTest)
        .that(ByteString.EMPTY.toString(Internal.UTF_8))
        .isSameInstanceAs(ropeByteString.toString(Internal.UTF_8));
  }

  @Override
  @Test
  public void testToString_raisesException() {
    try {
      ByteString byteString = RopeByteString.newInstanceForTest(ByteString.EMPTY, ByteString.EMPTY);
      byteString.toString("invalid");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }

    try {
      ByteString byteString =
          RopeByteString.concatenate(
              ByteString.copyFromUtf8("foo"), ByteString.copyFromUtf8("bar"));
      byteString.toString("invalid");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (UnsupportedEncodingException expected) {
      // This is success
    }
  }

  @Override
  @Test
  public void testJavaSerialization() throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    ObjectOutputStream oos = new ObjectOutputStream(out);
    oos.writeObject(stringUnderTest);
    oos.close();
    byte[] pickled = out.toByteArray();
    InputStream in = new ByteArrayInputStream(pickled);
    ObjectInputStream ois = new ObjectInputStream(in);
    Object o = ois.readObject();
    assertWithMessage("Didn't get a ByteString back").that(o).isInstanceOf(ByteString.class);
    assertWithMessage("Should get an equal ByteString back").that(o).isEqualTo(stringUnderTest);
  }
}
