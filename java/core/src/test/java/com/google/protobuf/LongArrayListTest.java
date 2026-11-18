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

import com.google.protobuf.Internal.LongList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link LongArrayList}. */
@RunWith(JUnit4.class)
public class LongArrayListTest {

  private static final LongArrayList UNARY_LIST = newImmutableLongArrayList(1);
  private static final LongArrayList TERTIARY_LIST = newImmutableLongArrayList(1, 2, 3);

  private LongArrayList list;

  @Before
  public void setUp() throws Exception {
    list = new LongArrayList();
  }

  @Test
  public void testEmptyListReturnsSameInstance() {
    assertThat(LongArrayList.emptyList()).isSameInstanceAs(LongArrayList.emptyList());
  }

  @Test
  public void testEmptyListIsImmutable() {
    assertImmutable(LongArrayList.emptyList());
  }

  @Test
  public void testMakeImmutable() {
    list.addLong(3);
    list.addLong(4);
    list.addLong(5);
    list.addLong(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  @Test
  public void testModificationWithIteration() {
    list.addAll(asList(1L, 2L, 3L, 4L));
    Iterator<Long> iterator = list.iterator();
    assertThat(list).hasSize(4);
    assertThat((long) list.get(0)).isEqualTo(1L);
    assertThat((long) iterator.next()).isEqualTo(1L);
    list.set(0, 1L);
    assertThat((long) iterator.next()).isEqualTo(2L);

    list.remove(0);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0L);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  @Test
  public void testGet() {
    assertThat((long) TERTIARY_LIST.get(0)).isEqualTo(1L);
    assertThat((long) TERTIARY_LIST.get(1)).isEqualTo(2L);
    assertThat((long) TERTIARY_LIST.get(2)).isEqualTo(3L);

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
  public void testGetLong() {
    assertThat(TERTIARY_LIST.getLong(0)).isEqualTo(1L);
    assertThat(TERTIARY_LIST.getLong(1)).isEqualTo(2L);
    assertThat(TERTIARY_LIST.getLong(2)).isEqualTo(3L);

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
    assertThat(UNARY_LIST.indexOf(2L)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_notInListWithDuplicates() {
    LongArrayList listWithDupes = newImmutableLongArrayList(1L, 1L);
    assertThat(listWithDupes.indexOf(2L)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_inList() {
    assertThat(TERTIARY_LIST.indexOf(2L)).isEqualTo(1);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchAtHead() {
    LongArrayList listWithDupes = newImmutableLongArrayList(1L, 1L, 2L);
    assertThat(listWithDupes.indexOf(1L)).isEqualTo(0);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchMidList() {
    LongArrayList listWithDupes = newImmutableLongArrayList(2L, 1L, 1L, 2L);
    assertThat(listWithDupes.indexOf(1L)).isEqualTo(1);
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
    assertThat(UNARY_LIST).doesNotContain(2L);
  }

  @Test
  public void testContains_notInListWithDuplicates() {
    LongArrayList listWithDupes = newImmutableLongArrayList(1L, 1L);
    assertThat(listWithDupes).doesNotContain(2L);
  }

  @Test
  public void testContains_inList() {
    assertThat(TERTIARY_LIST).contains(2L);
  }

  @Test
  public void testContains_inListWithDuplicates_matchAtHead() {
    LongArrayList listWithDupes = newImmutableLongArrayList(1L, 1L, 2L);
    assertThat(listWithDupes).contains(1L);
  }

  @Test
  public void testContains_inListWithDuplicates_matchMidList() {
    LongArrayList listWithDupes = newImmutableLongArrayList(2L, 1L, 1L, 2L);
    assertThat(listWithDupes).contains(1L);
  }

  @Test
  public void testSize() {
    assertThat(LongArrayList.emptyList()).isEmpty();
    assertThat(UNARY_LIST).hasSize(1);
    assertThat(TERTIARY_LIST).hasSize(3);

    list.addLong(3);
    list.addLong(4);
    list.addLong(6);
    list.addLong(8);
    assertThat(list).hasSize(4);

    list.remove(0);
    assertThat(list).hasSize(3);

    list.add(17L);
    assertThat(list).hasSize(4);
  }

  @Test
  public void testSet() {
    list.addLong(2);
    list.addLong(4);

    assertThat((long) list.set(0, 3L)).isEqualTo(2L);
    assertThat(list.getLong(0)).isEqualTo(3L);

    assertThat((long) list.set(1, 0L)).isEqualTo(4L);
    assertThat(list.getLong(1)).isEqualTo(0L);

    try {
      list.set(-1, 0L);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0L);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testSetLong() {
    list.addLong(1);
    list.addLong(3);

    assertThat(list.setLong(0, 0)).isEqualTo(1L);
    assertThat(list.getLong(0)).isEqualTo(0L);

    assertThat(list.setLong(1, 0)).isEqualTo(3L);
    assertThat(list.getLong(1)).isEqualTo(0L);

    try {
      list.setLong(-1, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setLong(2, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAdd() {
    assertThat(list).isEmpty();

    assertThat(list.add(2L)).isTrue();
    assertThat(list).containsExactly(2L);

    assertThat(list.add(3L)).isTrue();
    list.add(0, 4L);
    assertThat(list).containsExactly(4L, 2L, 3L).inOrder();

    list.add(0, 1L);
    list.add(0, 0L);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Long.valueOf(5 + i));
    }
    assertThat(list).containsExactly(0L, 1L, 4L, 2L, 3L, 5L, 6L, 7L, 8L, 9L, 10L).inOrder();

    try {
      list.add(-1, 5L);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.add(4, 5L);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAddLong() {
    assertThat(list).isEmpty();

    list.addLong(2);
    assertThat(list).containsExactly(2L);

    list.addLong(3);
    assertThat(list).containsExactly(2L, 3L).inOrder();
  }

  @Test
  public void testAddAll() {
    assertThat(list).isEmpty();

    assertThat(list.addAll(Collections.singleton(1L))).isTrue();
    assertThat(list).hasSize(1);
    assertThat((long) list.get(0)).isEqualTo(1L);
    assertThat(list.getLong(0)).isEqualTo(1L);

    assertThat(list.addAll(asList(2L, 3L, 4L, 5L, 6L))).isTrue();
    assertThat(list).containsExactly(1L, 2L, 3L, 4L, 5L, 6L).inOrder();

    assertThat(list.addAll(TERTIARY_LIST)).isTrue();
    assertThat(list).containsExactly(1L, 2L, 3L, 4L, 5L, 6L, 1L, 2L, 3L).inOrder();

    assertThat(list.addAll(Collections.<Long>emptyList())).isFalse();
    assertThat(list.addAll(LongArrayList.emptyList())).isFalse();
  }

  @Test
  public void testEquals() {
    LongArrayList list1 = new LongArrayList();
    LongArrayList list2 = new LongArrayList();

    assertThat(list1).isEqualTo(list2);
  }

  @Test
  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertThat((long) list.remove(0)).isEqualTo(1L);
    assertThat(list).containsExactly(2L, 3L).inOrder();

    assertThat(list.remove(Long.valueOf(3))).isTrue();
    assertThat(list).containsExactly(2L);

    assertThat(list.remove(Long.valueOf(3))).isFalse();
    assertThat(list).containsExactly(2L);

    assertThat((long) list.remove(0)).isEqualTo(2L);
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
    LongList toRemove = LongArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addLong(3);
    toRemove.remove(0);
    assertThat(toRemove).isEmpty();
  }

  @Test
  public void testRemove_listAtCapacity() {
    LongList toRemove = LongArrayList.emptyList().mutableCopyWithCapacity(2);
    toRemove.addLong(3);
    toRemove.addLong(4);
    toRemove.remove(0);
    assertThat(toRemove).hasSize(1);
    assertThat((long) toRemove.get(0)).isEqualTo(4L);
  }

  @Test
  public void testSublistRemoveEndOfCapacity() {
    LongList toRemove = LongArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addLong(3);
    toRemove.subList(0, 1).clear();
    assertThat(toRemove).isEmpty();
  }

  private void assertImmutable(LongList list) {
    if (list.contains(1L)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1L);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1L);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Long>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1L));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new LongArrayList());
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
      list.addAll(0, Collections.singleton(1L));
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
      list.addAll(0, Collections.<Long>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addLong(0);
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
      list.removeAll(Collections.<Long>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1L));
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
      list.retainAll(Collections.<Long>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1L));
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
      list.set(0, 0L);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setLong(0, 0);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }

  private static LongArrayList newImmutableLongArrayList(long... elements) {
    LongArrayList list = new LongArrayList();
    for (long element : elements) {
      list.addLong(element);
    }
    list.makeImmutable();
    return list;
  }
}
