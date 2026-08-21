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

import com.google.protobuf.Internal.DoubleList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link DoubleArrayList}. */
@RunWith(JUnit4.class)
public class DoubleArrayListTest {

  private static final DoubleArrayList UNARY_LIST = newImmutableDoubleArrayList(1);
  private static final DoubleArrayList TERTIARY_LIST = newImmutableDoubleArrayList(1, 2, 3);

  private DoubleArrayList list;

  @Before
  public void setUp() throws Exception {
    list = new DoubleArrayList();
  }

  @Test
  public void testEmptyListReturnsSameInstance() {
    assertThat(DoubleArrayList.emptyList()).isSameInstanceAs(DoubleArrayList.emptyList());
  }

  @Test
  public void testEmptyListIsImmutable() {
    assertImmutable(DoubleArrayList.emptyList());
  }

  @Test
  public void testMakeImmutable() {
    list.addDouble(3);
    list.addDouble(4);
    list.addDouble(5);
    list.addDouble(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  @Test
  public void testModificationWithIteration() {
    list.addAll(asList(1D, 2D, 3D, 4D));
    Iterator<Double> iterator = list.iterator();
    assertThat(list).hasSize(4);
    assertThat((double) list.get(0)).isEqualTo(1D);
    assertThat((double) iterator.next()).isEqualTo(1D);
    list.set(0, 1D);
    assertThat((double) iterator.next()).isEqualTo(2D);

    list.remove(0);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0D);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  @Test
  public void testGet() {
    assertThat((double) TERTIARY_LIST.get(0)).isEqualTo(1D);
    assertThat((double) TERTIARY_LIST.get(1)).isEqualTo(2D);
    assertThat((double) TERTIARY_LIST.get(2)).isEqualTo(3D);

    try {
      TERTIARY_LIST.get(-1);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      TERTIARY_LIST.get(3);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testGetDouble() {
    assertThat(TERTIARY_LIST.getDouble(0)).isEqualTo(1D);
    assertThat(TERTIARY_LIST.getDouble(1)).isEqualTo(2D);
    assertThat(TERTIARY_LIST.getDouble(2)).isEqualTo(3D);

    try {
      TERTIARY_LIST.get(-1);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      TERTIARY_LIST.get(3);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testIndexOf_nullElement() {
    assertThat(TERTIARY_LIST.indexOf(null)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_incompatibleElementType() {
    assertThat(TERTIARY_LIST.indexOf(new Object())).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_notInList() {
    assertThat(UNARY_LIST.indexOf(2D)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_notInListWithDuplicates() {
    DoubleArrayList listWithDupes = newImmutableDoubleArrayList(1D, 1D);
    assertThat(listWithDupes.indexOf(2D)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_inList() {
    assertThat(TERTIARY_LIST.indexOf(2D)).isEqualTo(1);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchAtHead() {
    DoubleArrayList listWithDupes = newImmutableDoubleArrayList(1D, 1D, 2D);
    assertThat(listWithDupes.indexOf(1D)).isEqualTo(0);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchMidList() {
    DoubleArrayList listWithDupes = newImmutableDoubleArrayList(2D, 1D, 1D, 2D);
    assertThat(listWithDupes.indexOf(1D)).isEqualTo(1);
  }

  @Test
  public void testContains_nullElement() {
    assertThat(TERTIARY_LIST).doesNotContain(null);
  }

  @Test
  public void testContains_incompatibleElementType() {
    assertThat(TERTIARY_LIST).doesNotContain(new Object());
  }

  @Test
  public void testContains_notInList() {
    assertThat(UNARY_LIST).doesNotContain(2D);
  }

  @Test
  public void testContains_notInListWithDuplicates() {
    DoubleArrayList listWithDupes = newImmutableDoubleArrayList(1D, 1D);
    assertThat(listWithDupes).doesNotContain(2D);
  }

  @Test
  public void testContains_inList() {
    assertThat(TERTIARY_LIST).contains(2D);
  }

  @Test
  public void testContains_inListWithDuplicates_matchAtHead() {
    DoubleArrayList listWithDupes = newImmutableDoubleArrayList(1D, 1D, 2D);
    assertThat(listWithDupes).contains(1D);
  }

  @Test
  public void testContains_inListWithDuplicates_matchMidList() {
    DoubleArrayList listWithDupes = newImmutableDoubleArrayList(2D, 1D, 1D, 2D);
    assertThat(listWithDupes).contains(1D);
  }

  @Test
  public void testSize() {
    assertThat(DoubleArrayList.emptyList()).isEmpty();
    assertThat(UNARY_LIST).hasSize(1);
    assertThat(TERTIARY_LIST).hasSize(3);

    list.addDouble(3);
    list.addDouble(4);
    list.addDouble(6);
    list.addDouble(8);
    assertThat(list).hasSize(4);

    list.remove(0);
    assertThat(list).hasSize(3);

    list.add(17D);
    assertThat(list).hasSize(4);
  }

  @Test
  public void testSet() {
    list.addDouble(2);
    list.addDouble(4);

    assertThat((double) list.set(0, 3D)).isEqualTo(2D);
    assertThat(list.getDouble(0)).isEqualTo(3D);

    assertThat((double) list.set(1, 0D)).isEqualTo(4D);
    assertThat(list.getDouble(1)).isEqualTo(0D);

    try {
      list.set(-1, 0D);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0D);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testSetDouble() {
    list.addDouble(1);
    list.addDouble(3);

    assertThat(list.setDouble(0, 0)).isEqualTo(1D);
    assertThat(list.getDouble(0)).isEqualTo(0D);

    assertThat(list.setDouble(1, 0)).isEqualTo(3D);
    assertThat(list.getDouble(1)).isEqualTo(0D);

    try {
      list.setDouble(-1, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setDouble(2, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAdd() {
    assertThat(list).isEmpty();

    assertThat(list.add(2D)).isTrue();
    assertThat(list).containsExactly(2D);

    assertThat(list.add(3D)).isTrue();
    list.add(0, 4D);
    assertThat(list).containsExactly(4D, 2D, 3D).inOrder();

    list.add(0, 1D);
    list.add(0, 0D);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Double.valueOf(5 + i));
    }
    assertThat(list).containsExactly(0D, 1D, 4D, 2D, 3D, 5D, 6D, 7D, 8D, 9D, 10D).inOrder();

    try {
      list.add(-1, 5D);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.add(4, 5D);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAddDouble() {
    assertThat(list).isEmpty();

    list.addDouble(2);
    assertThat(list).containsExactly(2D);

    list.addDouble(3);
    assertThat(list).containsExactly(2D, 3D).inOrder();
  }

  @Test
  public void testAddAll() {
    assertThat(list).isEmpty();

    assertThat(list.addAll(Collections.singleton(1D))).isTrue();
    assertThat(list).hasSize(1);
    assertThat((double) list.get(0)).isEqualTo(1D);
    assertThat(list.getDouble(0)).isEqualTo(1D);

    assertThat(list.addAll(asList(2D, 3D, 4D, 5D, 6D))).isTrue();
    assertThat(list).containsExactly(1D, 2D, 3D, 4D, 5D, 6D).inOrder();

    assertThat(list.addAll(TERTIARY_LIST)).isTrue();
    assertThat(list).containsExactly(1D, 2D, 3D, 4D, 5D, 6D, 1D, 2D, 3D).inOrder();

    assertThat(list.addAll(Collections.<Double>emptyList())).isFalse();
    assertThat(list.addAll(DoubleArrayList.emptyList())).isFalse();
  }

  @Test
  public void testEquals() {
    DoubleArrayList list1 = new DoubleArrayList();
    DoubleArrayList list2 = new DoubleArrayList();

    list1.addDouble(Double.longBitsToDouble(0x7ff0000000000001L));
    list2.addDouble(Double.longBitsToDouble(0x7ff0000000000002L));
    assertThat(list1).isEqualTo(list2);
  }

  @Test
  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertThat((double) list.remove(0)).isEqualTo(1D);
    assertThat(list).containsExactly(2D, 3D).inOrder();

    assertThat(list.remove(Double.valueOf(3))).isTrue();
    assertThat(list).containsExactly(2D);

    assertThat(list.remove(Double.valueOf(3))).isFalse();
    assertThat(list).containsExactly(2D);

    assertThat((double) list.remove(0)).isEqualTo(2D);
    assertThat(list).isEmpty();

    try {
      list.remove(-1);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.remove(0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testRemoveEnd_listAtCapacity() {
    DoubleList toRemove = DoubleArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addDouble(3);
    toRemove.remove(0);
    assertThat(toRemove).isEmpty();
  }

  @Test
  public void testRemove_listAtCapacity() {
    DoubleList toRemove = DoubleArrayList.emptyList().mutableCopyWithCapacity(2);
    toRemove.addDouble(3);
    toRemove.addDouble(4);
    toRemove.remove(0);
    assertThat(toRemove).hasSize(1);
    assertThat((double) toRemove.get(0)).isEqualTo(4D);
  }

  @Test
  public void testSublistRemoveEndOfCapacity() {
    DoubleList toRemove = DoubleArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addDouble(3);
    toRemove.subList(0, 1).clear();
    assertThat(toRemove).isEmpty();
  }

  private void assertImmutable(DoubleList list) {
    if (list.contains(1D)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1D);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1D);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Double>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1D));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new DoubleArrayList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(UNARY_LIST);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(0, Collections.singleton(1D));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(0, UNARY_LIST);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(0, Collections.<Double>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addDouble(0);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.clear();
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.remove(1);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.remove(new Object());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.<Double>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1D));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(UNARY_LIST);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.<Double>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1D));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(UNARY_LIST);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.set(0, 0D);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setDouble(0, 0);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }

  private static DoubleArrayList newImmutableDoubleArrayList(double... elements) {
    DoubleArrayList list = new DoubleArrayList();
    for (double element : elements) {
      list.addDouble(element);
    }
    list.makeImmutable();
    return list;
  }
}
