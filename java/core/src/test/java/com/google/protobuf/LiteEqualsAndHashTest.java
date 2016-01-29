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

import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Bar;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.BarPrime;
import protobuf_unittest.lite_equals_and_hash.LiteEqualsAndHash.Foo;

import junit.framework.TestCase;

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
    // overriden to test for value equality rather than simple object equality.

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

    // Check that a diffeent object is not equal.
    assertFalse(foo1a.equals(foo2));

    // Check that two objects which have different types but the same field values are not
    // considered to be equal.
    Bar bar = Bar.newBuilder().setName("bar").build();
    BarPrime barPrime = BarPrime.newBuilder().setName("bar").build();
    assertFalse(bar.equals(barPrime));
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
}
