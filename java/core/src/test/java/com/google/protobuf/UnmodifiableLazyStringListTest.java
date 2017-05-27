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

import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import junit.framework.TestCase;

/**
 * Tests for {@link UnmodifiableLazyStringList}.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class UnmodifiableLazyStringListTest extends TestCase {

  private static String STRING_A = "A";
  private static String STRING_B = "B";
  private static String STRING_C = "C";

  private static ByteString BYTE_STRING_A = ByteString.copyFromUtf8("A");
  private static ByteString BYTE_STRING_B = ByteString.copyFromUtf8("B");
  private static ByteString BYTE_STRING_C = ByteString.copyFromUtf8("C");

  public void testReadOnlyMethods() {
    LazyStringArrayList rawList = createSampleList();
    UnmodifiableLazyStringList list = new UnmodifiableLazyStringList(rawList);
    assertEquals(3, list.size());
    assertSame(STRING_A, list.get(0));
    assertSame(STRING_B, list.get(1));
    assertSame(STRING_C, list.get(2));
    assertEquals(BYTE_STRING_A, list.getByteString(0));
    assertEquals(BYTE_STRING_B, list.getByteString(1));
    assertEquals(BYTE_STRING_C, list.getByteString(2));

    List<ByteString> byteStringList = list.asByteStringList();
    assertSame(list.getByteString(0), byteStringList.get(0));
    assertSame(list.getByteString(1), byteStringList.get(1));
    assertSame(list.getByteString(2), byteStringList.get(2));
  }

  public void testModifyMethods() {
    LazyStringArrayList rawList = createSampleList();
    UnmodifiableLazyStringList list = new UnmodifiableLazyStringList(rawList);

    try {
      list.remove(0);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(3, list.size());

    try {
      list.add(STRING_B);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(3, list.size());

    try {
      list.set(1, STRING_B);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(3, list.size());

    List<ByteString> byteStringList = list.asByteStringList();
    try {
      byteStringList.remove(0);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(3, list.size());
    assertEquals(3, byteStringList.size());

    try {
      byteStringList.add(BYTE_STRING_B);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(3, list.size());
    assertEquals(3, byteStringList.size());

    try {
      byteStringList.set(1, BYTE_STRING_B);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    assertEquals(3, list.size());
    assertEquals(3, byteStringList.size());
  }

  public void testIterator() {
    LazyStringArrayList rawList = createSampleList();
    UnmodifiableLazyStringList list = new UnmodifiableLazyStringList(rawList);

    Iterator<String> iter = list.iterator();
    int count = 0;
    while (iter.hasNext()) {
      iter.next();
      count++;
      try {
        iter.remove();
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
    }
    assertEquals(3, count);

    List<ByteString> byteStringList = list.asByteStringList();
    Iterator<ByteString> byteIter = byteStringList.iterator();
    count = 0;
    while (byteIter.hasNext()) {
      byteIter.next();
      count++;
      try {
        byteIter.remove();
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
    }
    assertEquals(3, count);
  }

  public void testListIterator() {
    LazyStringArrayList rawList = createSampleList();
    UnmodifiableLazyStringList list = new UnmodifiableLazyStringList(rawList);

    ListIterator<String> iter = list.listIterator();
    int count = 0;
    while (iter.hasNext()) {
      iter.next();
      count++;
      try {
        iter.remove();
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
      try {
        iter.set("bar");
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
      try {
        iter.add("bar");
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
    }
    assertEquals(3, count);

    List<ByteString> byteStringList = list.asByteStringList();
    ListIterator<ByteString> byteIter = byteStringList.listIterator();
    count = 0;
    while (byteIter.hasNext()) {
      byteIter.next();
      count++;
      try {
        byteIter.remove();
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
      try {
        byteIter.set(BYTE_STRING_A);
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
      try {
        byteIter.add(BYTE_STRING_A);
        fail();
      } catch (UnsupportedOperationException e) {
        // expected
      }
    }
    assertEquals(3, count);
  }

  private LazyStringArrayList createSampleList() {
    LazyStringArrayList rawList = new LazyStringArrayList();
    rawList.add(STRING_A);
    rawList.add(STRING_B);
    rawList.add(STRING_C);
    return rawList;
  }
}
