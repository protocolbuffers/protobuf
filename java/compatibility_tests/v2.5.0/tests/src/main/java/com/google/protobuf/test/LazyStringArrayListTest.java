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

package com.google.protobuf.test;
import com.google.protobuf.*;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for {@link LazyStringArrayList}.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class LazyStringArrayListTest extends TestCase {

  private static String STRING_A = "A";
  private static String STRING_B = "B";
  private static String STRING_C = "C";

  private static ByteString BYTE_STRING_A = ByteString.copyFromUtf8("A");
  private static ByteString BYTE_STRING_B = ByteString.copyFromUtf8("B");
  private static ByteString BYTE_STRING_C = ByteString.copyFromUtf8("C");

  public void testJustStrings() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(STRING_A);
    list.add(STRING_B);
    list.add(STRING_C);

    assertEquals(3, list.size());
    assertSame(STRING_A, list.get(0));
    assertSame(STRING_B, list.get(1));
    assertSame(STRING_C, list.get(2));

    list.set(1, STRING_C);
    assertSame(STRING_C, list.get(1));

    list.remove(1);
    assertSame(STRING_A, list.get(0));
    assertSame(STRING_C, list.get(1));
  }

  public void testJustByteString() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(BYTE_STRING_A);
    list.add(BYTE_STRING_B);
    list.add(BYTE_STRING_C);

    assertEquals(3, list.size());
    assertSame(BYTE_STRING_A, list.getByteString(0));
    assertSame(BYTE_STRING_B, list.getByteString(1));
    assertSame(BYTE_STRING_C, list.getByteString(2));

    list.remove(1);
    assertSame(BYTE_STRING_A, list.getByteString(0));
    assertSame(BYTE_STRING_C, list.getByteString(1));
  }

  public void testConversionBackAndForth() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(STRING_A);
    list.add(BYTE_STRING_B);
    list.add(BYTE_STRING_C);

    // String a should be the same because it was originally a string
    assertSame(STRING_A, list.get(0));

    // String b and c should be different because the string has to be computed
    // from the ByteString
    String bPrime = list.get(1);
    assertNotSame(STRING_B, bPrime);
    assertEquals(STRING_B, bPrime);
    String cPrime = list.get(2);
    assertNotSame(STRING_C, cPrime);
    assertEquals(STRING_C, cPrime);

    // String c and c should stay the same once cached.
    assertSame(bPrime, list.get(1));
    assertSame(cPrime, list.get(2));

    // ByteString needs to be computed from string for both a and b
    ByteString aPrimeByteString = list.getByteString(0);
    assertEquals(BYTE_STRING_A, aPrimeByteString);
    ByteString bPrimeByteString = list.getByteString(1);
    assertNotSame(BYTE_STRING_B, bPrimeByteString);
    assertEquals(BYTE_STRING_B, list.getByteString(1));

    // Once cached, ByteString should stay cached.
    assertSame(aPrimeByteString, list.getByteString(0));
    assertSame(bPrimeByteString, list.getByteString(1));
  }

  public void testCopyConstructorCopiesByReference() {
    LazyStringArrayList list1 = new LazyStringArrayList();
    list1.add(STRING_A);
    list1.add(BYTE_STRING_B);
    list1.add(BYTE_STRING_C);

    LazyStringArrayList list2 = new LazyStringArrayList(list1);
    assertEquals(3, list2.size());
    assertSame(STRING_A, list2.get(0));
    assertSame(BYTE_STRING_B, list2.getByteString(1));
    assertSame(BYTE_STRING_C, list2.getByteString(2));
  }

  public void testListCopyConstructor() {
    List<String> list1 = new ArrayList<String>();
    list1.add(STRING_A);
    list1.add(STRING_B);
    list1.add(STRING_C);

    LazyStringArrayList list2 = new LazyStringArrayList(list1);
    assertEquals(3, list2.size());
    assertSame(STRING_A, list2.get(0));
    assertSame(STRING_B, list2.get(1));
    assertSame(STRING_C, list2.get(2));
  }

  public void testAddAllCopiesByReferenceIfPossible() {
    LazyStringArrayList list1 = new LazyStringArrayList();
    list1.add(STRING_A);
    list1.add(BYTE_STRING_B);
    list1.add(BYTE_STRING_C);

    LazyStringArrayList list2 = new LazyStringArrayList();
    list2.addAll(list1);

    assertEquals(3, list2.size());
    assertSame(STRING_A, list2.get(0));
    assertSame(BYTE_STRING_B, list2.getByteString(1));
    assertSame(BYTE_STRING_C, list2.getByteString(2));
  }
}
