// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertWithMessage;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * This class tests {@link BoundedByteString}, which extends {@link LiteralByteString}, by
 * inheriting the tests from {@link LiteralByteStringTest}. The only method which is strange enough
 * that it needs to be overridden here is {@link #testToString()}.
 */
@RunWith(JUnit4.class)
public class BoundedByteStringTest extends LiteralByteStringTest {

  @Override
  @Before
  public void setUp() throws Exception {
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
  @Test
  public void testToString() throws UnsupportedEncodingException {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = ByteString.wrap(testString.getBytes(Internal.UTF_8));
    ByteString chopped = unicode.substring(2, unicode.size() - 6);
    assertWithMessage("%s.substring() must have the expected type", classUnderTest)
        .that(classUnderTest)
        .isEqualTo(getActualClassName(chopped));

    String roundTripString = chopped.toString(UTF_8);
    assertWithMessage("%s unicode bytes must match", classUnderTest)
        .that(testString.substring(2, testString.length() - 6))
        .isEqualTo(roundTripString);
  }

  @Override
  @Test
  public void testCharsetToString() {
    String testString = "I love unicode \u1234\u5678 characters";
    ByteString unicode = ByteString.wrap(testString.getBytes(Internal.UTF_8));
    ByteString chopped = unicode.substring(2, unicode.size() - 6);
    assertWithMessage("%s.substring() must have the expected type", classUnderTest)
        .that(classUnderTest)
        .isEqualTo(getActualClassName(chopped));

    String roundTripString = chopped.toString(Internal.UTF_8);
    assertWithMessage("%s unicode bytes must match", classUnderTest)
        .that(testString.substring(2, testString.length() - 6))
        .isEqualTo(roundTripString);
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
    assertWithMessage("Should get an equal ByteString back").that(stringUnderTest).isEqualTo(o);
  }
}
