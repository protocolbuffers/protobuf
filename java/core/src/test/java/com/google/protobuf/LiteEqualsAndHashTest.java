// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import junit.framework.TestCase;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Bar;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.BarPrime;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Foo;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.TestOneofEquals;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.TestRecursiveOneof;

/**
 * Test generate equal and hash methods for the lite runtime.
 *
 * @author pbogle@google.com Phil Bogle
 */
public class LiteEqualsAndHashTest extends TestCase {

  public void testEquals() throws Exception {
    // Since the generated equals and hashCode methods for lite messages are a
    // mostly complete subset of those for regular messages, we can mostly assume
    // that the generated methods are already thoroughly tested by the regular tests.

    // This test mostly just verifies is that a proto with
    // optimize_for = LITE_RUNTIME and java_generates_equals_and_hash_compiles
    // correctly when linked only against the lite library.

    // We do however do some basic testing to make sure that equals is actually
    // overridden to test for value equality rather than simple object equality.

    // Check that two identical objs are equal.
    Foo foo1a = Foo.newBuilder()
        .setValue(1)
        .addBar(Bar.newBuilder().setName("foo1"))
        .build();
    Foo foo1b = Foo.newBuilder()
        .setValue(1)
        .addBar(Bar.newBuilder().setName("foo1"))
        .build();
    Foo foo2 = Foo.newBuilder()
        .setValue(1)
        .addBar(Bar.newBuilder().setName("foo2"))
        .build();

    // Check that equals is doing value rather than object equality.
    assertEquals(foo1a, foo1b);
    assertEquals(foo1a.hashCode(), foo1b.hashCode());

    // Check that a different object is not equal.
    assertFalse(foo1a.equals(foo2));

    // Check that two objects which have different types but the same field values are not
    // considered to be equal.
    Bar bar = Bar.newBuilder().setName("bar").build();
    BarPrime barPrime = BarPrime.newBuilder().setName("bar").build();
    assertFalse(bar.equals(barPrime));
  }

  public void testOneofEquals() throws Exception {
    TestOneofEquals.Builder builder = TestOneofEquals.newBuilder();
    TestOneofEquals message1 = builder.build();
    // Set message2's name field to default value. The two messages should be different when we
    // check with the oneof case.
    builder.setName("");
    TestOneofEquals message2 = builder.build();
    assertFalse(message1.equals(message2));
  }

  public void testEqualsAndHashCodeWithUnknownFields() throws InvalidProtocolBufferException {
    Foo fooWithOnlyValue = Foo.newBuilder()
        .setValue(1)
        .build();

    Foo fooWithValueAndExtension = fooWithOnlyValue.toBuilder()
        .setValue(1)
        .setExtension(Bar.fooExt, Bar.newBuilder()
            .setName("name")
            .build())
        .build();

    Foo fooWithValueAndUnknownFields = Foo.parseFrom(fooWithValueAndExtension.toByteArray());

    assertEqualsAndHashCodeAreFalse(fooWithOnlyValue, fooWithValueAndUnknownFields);
    assertEqualsAndHashCodeAreFalse(fooWithValueAndExtension, fooWithValueAndUnknownFields);
  }

  private void assertEqualsAndHashCodeAreFalse(Object o1, Object o2) {
    assertFalse(o1.equals(o2));
    assertFalse(o1.hashCode() == o2.hashCode());
  }

  public void testRecursiveHashcode() {
    // This tests that we don't infinite loop.
    int unused = TestRecursiveOneof.getDefaultInstance().hashCode();
  }
}
