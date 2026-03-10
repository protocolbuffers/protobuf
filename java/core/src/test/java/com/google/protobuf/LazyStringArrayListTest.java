// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static java.util.Arrays.asList;

import java.util.ArrayList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link LazyStringArrayList}. */
@RunWith(JUnit4.class)
public class LazyStringArrayListTest {

  private static final String STRING_A = "A";
  private static final String STRING_B = "B";
  private static final String STRING_C = "C";

  private static final ByteString BYTE_STRING_A = ByteString.copyFromUtf8("A");
  private static final ByteString BYTE_STRING_B = ByteString.copyFromUtf8("B");
  private static final ByteString BYTE_STRING_C = ByteString.copyFromUtf8("C");

  @Test
  public void testJustStrings() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(STRING_A);
    list.add(STRING_B);
    list.add(STRING_C);

    assertThat(list).hasSize(3);
    assertThat(list.get(0)).isSameInstanceAs(STRING_A);
    assertThat(list.get(1)).isSameInstanceAs(STRING_B);
    assertThat(list.get(2)).isSameInstanceAs(STRING_C);

    list.set(1, STRING_C);
    assertThat(list.get(1)).isSameInstanceAs(STRING_C);

    list.remove(1);
    assertThat(list.get(0)).isSameInstanceAs(STRING_A);
    assertThat(list.get(1)).isSameInstanceAs(STRING_C);

    List<ByteString> byteStringList = list.asByteStringList();
    assertThat(byteStringList.get(0)).isEqualTo(BYTE_STRING_A);
    assertThat(byteStringList.get(1)).isEqualTo(BYTE_STRING_C);

    // Underlying list should be transformed.
    assertThat(byteStringList.get(0)).isSameInstanceAs(list.getByteString(0));
    assertThat(byteStringList.get(1)).isSameInstanceAs(list.getByteString(1));
  }

  @Test
  public void testJustByteString() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(BYTE_STRING_A);
    list.add(BYTE_STRING_B);
    list.add(BYTE_STRING_C);

    assertThat(list).hasSize(3);
    assertThat(list.getByteString(0)).isSameInstanceAs(BYTE_STRING_A);
    assertThat(list.getByteString(1)).isSameInstanceAs(BYTE_STRING_B);
    assertThat(list.getByteString(2)).isSameInstanceAs(BYTE_STRING_C);

    list.remove(1);
    assertThat(list.getByteString(0)).isSameInstanceAs(BYTE_STRING_A);
    assertThat(list.getByteString(1)).isSameInstanceAs(BYTE_STRING_C);

    List<ByteString> byteStringList = list.asByteStringList();
    assertThat(byteStringList.get(0)).isSameInstanceAs(BYTE_STRING_A);
    assertThat(byteStringList.get(1)).isSameInstanceAs(BYTE_STRING_C);
  }

  @Test
  public void testConversionBackAndForth() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(STRING_A);
    list.add(BYTE_STRING_B);
    list.add(BYTE_STRING_C);

    // String a should be the same because it was originally a string
    assertThat(list.get(0)).isSameInstanceAs(STRING_A);

    // String b and c should be different because the string has to be computed
    // from the ByteString
    String bPrime = list.get(1);
    assertThat(bPrime).isNotSameInstanceAs(STRING_B);
    assertThat(bPrime).isEqualTo(STRING_B);
    String cPrime = list.get(2);
    assertThat(cPrime).isNotSameInstanceAs(STRING_C);
    assertThat(cPrime).isEqualTo(STRING_C);

    // String c and c should stay the same once cached.
    assertThat(list.get(1)).isSameInstanceAs(bPrime);
    assertThat(list.get(2)).isSameInstanceAs(cPrime);

    // ByteString needs to be computed from string for both a and b
    ByteString aPrimeByteString = list.getByteString(0);
    assertThat(aPrimeByteString).isEqualTo(BYTE_STRING_A);
    ByteString bPrimeByteString = list.getByteString(1);
    assertThat(bPrimeByteString).isNotSameInstanceAs(BYTE_STRING_B);
    assertThat(list.getByteString(1)).isEqualTo(BYTE_STRING_B);

    // Once cached, ByteString should stay cached.
    assertThat(list.getByteString(0)).isSameInstanceAs(aPrimeByteString);
    assertThat(list.getByteString(1)).isSameInstanceAs(bPrimeByteString);
  }

  @Test
  public void testCopyConstructorCopiesByReference() {
    LazyStringArrayList list1 = new LazyStringArrayList();
    list1.add(STRING_A);
    list1.add(BYTE_STRING_B);
    list1.add(BYTE_STRING_C);

    LazyStringArrayList list2 = new LazyStringArrayList(list1);
    assertThat(list2).hasSize(3);
    assertThat(list2.get(0)).isSameInstanceAs(STRING_A);
    assertThat(list2.getByteString(1)).isSameInstanceAs(BYTE_STRING_B);
    assertThat(list2.getByteString(2)).isSameInstanceAs(BYTE_STRING_C);
  }

  @Test
  public void testListCopyConstructor() {
    List<String> list1 = new ArrayList<>();
    list1.add(STRING_A);
    list1.add(STRING_B);
    list1.add(STRING_C);

    LazyStringArrayList list2 = new LazyStringArrayList(list1);
    assertThat(list2).hasSize(3);
    assertThat(list2.get(0)).isSameInstanceAs(STRING_A);
    assertThat(list2.get(1)).isSameInstanceAs(STRING_B);
    assertThat(list2.get(2)).isSameInstanceAs(STRING_C);
  }

  @Test
  public void testAddAllCopiesByReferenceIfPossible() {
    LazyStringArrayList list1 = new LazyStringArrayList();
    list1.add(STRING_A);
    list1.add(BYTE_STRING_B);
    list1.add(BYTE_STRING_C);

    LazyStringArrayList list2 = new LazyStringArrayList();
    list2.addAll(list1);

    assertThat(list2).hasSize(3);
    assertThat(list2.get(0)).isSameInstanceAs(STRING_A);
    assertThat(list2.getByteString(1)).isSameInstanceAs(BYTE_STRING_B);
    assertThat(list2.getByteString(2)).isSameInstanceAs(BYTE_STRING_C);
  }

  @Test
  public void testModificationWithIteration() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.addAll(asList(STRING_A, STRING_B, STRING_C));
    Iterator<String> iterator = list.iterator();
    assertThat(list).hasSize(3);
    assertThat(list.get(0)).isEqualTo(STRING_A);
    assertThat(iterator.next()).isSameInstanceAs(STRING_A);

    // Does not structurally modify.
    iterator = list.iterator();
    list.set(0, STRING_B);
    iterator.next();

    list.remove(0);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();;
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, STRING_C);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();;
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  @Test
  public void testMakeImmutable() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(STRING_A);
    list.add(STRING_B);
    list.add(STRING_C);
    list.makeImmutable();
    assertGenericListImmutable(list, STRING_A);

    // LazyStringArrayList has extra methods not covered in the generic
    // assertion.

    try {
      list.add(BYTE_STRING_A.toByteArray());
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(BYTE_STRING_A);
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAllByteArray(Collections.singletonList(BYTE_STRING_A.toByteArray()));
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAllByteString(asList(BYTE_STRING_A));
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.mergeFrom(new LazyStringArrayList());
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.set(0, BYTE_STRING_A.toByteArray());
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.set(0, BYTE_STRING_A);
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }

  @Test
  public void testImmutabilityPropagation() {
    LazyStringArrayList list = new LazyStringArrayList();
    list.add(STRING_A);
    list.makeImmutable();

    assertGenericListImmutable(list.asByteStringList(), BYTE_STRING_A);

    // Arrays use reference equality so need to retrieve the underlying value
    // to properly test deep immutability.
    List<byte[]> byteArrayList = list.asByteArrayList();
    assertGenericListImmutable(byteArrayList, byteArrayList.get(0));
  }

  private static <T> void assertGenericListImmutable(List<T> list, T value) {
    try {
      list.add(value);
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, value);
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(asList(value));
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(0, asList(value));
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.clear();
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.remove(0);
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.remove(value);
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(asList(value));
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(asList());
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(asList());
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.set(0, value);
      assertWithMessage("expected exception").fail();;
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }
}
