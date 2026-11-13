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
import static org.junit.Assert.assertThrows;

import com.google.protobuf.Internal.IntList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link IntArrayList}. */
@RunWith(JUnit4.class)
public class IntArrayListTest {

  private static final IntArrayList UNARY_LIST = newImmutableIntArrayList(1);
  private static final IntArrayList TERTIARY_LIST = newImmutableIntArrayList(1, 2, 3);

  private IntArrayList list;

  @Before
  public void setUp() throws Exception {
    list = new IntArrayList();
  }

  @Test
  public void testEmptyListReturnsSameInstance() {
    assertThat(IntArrayList.emptyList()).isSameInstanceAs(IntArrayList.emptyList());
  }

  @Test
  public void testEmptyListIsImmutable() {
    assertImmutable(IntArrayList.emptyList());
  }

  @Test
  public void testMakeImmutable() {
    list.addInt(3);
    list.addInt(4);
    list.addInt(5);
    list.addInt(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  @Test
  public void testModificationWithIteration() {
    list.addAll(asList(1, 2, 3, 4));
    Iterator<Integer> iterator = list.iterator();
    assertThat(list).hasSize(4);
    assertThat((int) list.get(0)).isEqualTo(1);
    assertThat((int) iterator.next()).isEqualTo(1);
    list.set(0, 1);
    assertThat((int) iterator.next()).isEqualTo(2);

    list.remove(0);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  @Test
  public void testGet() {
    assertThat((int) TERTIARY_LIST.get(0)).isEqualTo(1);
    assertThat((int) TERTIARY_LIST.get(1)).isEqualTo(2);
    assertThat((int) TERTIARY_LIST.get(2)).isEqualTo(3);

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
  public void testGetInt() {
    assertThat(TERTIARY_LIST.getInt(0)).isEqualTo(1);
    assertThat(TERTIARY_LIST.getInt(1)).isEqualTo(2);
    assertThat(TERTIARY_LIST.getInt(2)).isEqualTo(3);

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
    assertThat(UNARY_LIST.indexOf(2)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_notInListWithDuplicates() {
    IntArrayList listWithDupes = newImmutableIntArrayList(1, 1);
    assertThat(listWithDupes.indexOf(2)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_inList() {
    assertThat(TERTIARY_LIST.indexOf(2)).isEqualTo(1);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchAtHead() {
    IntArrayList listWithDupes = newImmutableIntArrayList(1, 1, 2);
    assertThat(listWithDupes.indexOf(1)).isEqualTo(0);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchMidList() {
    IntArrayList listWithDupes = newImmutableIntArrayList(2, 1, 1, 2);
    assertThat(listWithDupes.indexOf(1)).isEqualTo(1);
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
    assertThat(UNARY_LIST).doesNotContain(2);
  }

  @Test
  public void testContains_notInListWithDuplicates() {
    IntArrayList listWithDupes = newImmutableIntArrayList(1, 1);
    assertThat(listWithDupes).doesNotContain(2);
  }

  @Test
  public void testContains_inList() {
    assertThat(TERTIARY_LIST).contains(2);
  }

  @Test
  public void testContains_inListWithDuplicates_matchAtHead() {
    IntArrayList listWithDupes = newImmutableIntArrayList(1, 1, 2);
    assertThat(listWithDupes).contains(1);
  }

  @Test
  public void testContains_inListWithDuplicates_matchMidList() {
    IntArrayList listWithDupes = newImmutableIntArrayList(2, 1, 1, 2);
    assertThat(listWithDupes).contains(1);
  }

  @Test
  public void testSize() {
    assertThat(IntArrayList.emptyList()).isEmpty();
    assertThat(UNARY_LIST).hasSize(1);
    assertThat(TERTIARY_LIST).hasSize(3);

    list.addInt(3);
    list.addInt(4);
    list.addInt(6);
    list.addInt(8);
    assertThat(list).hasSize(4);

    list.remove(0);
    assertThat(list).hasSize(3);

    list.add(17);
    assertThat(list).hasSize(4);
  }

  @Test
  public void testSet() {
    list.addInt(2);
    list.addInt(4);

    assertThat((int) list.set(0, 3)).isEqualTo(2);
    assertThat(list.getInt(0)).isEqualTo(3);

    assertThat((int) list.set(1, 0)).isEqualTo(4);
    assertThat(list.getInt(1)).isEqualTo(0);

    try {
      list.set(-1, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testSetInt() {
    list.addInt(1);
    list.addInt(3);

    assertThat(list.setInt(0, 0)).isEqualTo(1);
    assertThat(list.getInt(0)).isEqualTo(0);

    assertThat(list.setInt(1, 0)).isEqualTo(3);
    assertThat(list.getInt(1)).isEqualTo(0);

    try {
      list.setInt(-1, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setInt(2, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAdd() {
    assertThat(list).isEmpty();

    assertThat(list.add(2)).isTrue();
    assertThat(list).containsExactly(2);

    assertThat(list.add(3)).isTrue();
    list.add(0, 4);
    assertThat(list).containsExactly(4, 2, 3).inOrder();

    list.add(0, 1);
    list.add(0, 0);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Integer.valueOf(5 + i));
    }
    assertThat(list).containsExactly(0, 1, 4, 2, 3, 5, 6, 7, 8, 9, 10).inOrder();

    try {
      list.add(-1, 5);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.add(4, 5);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAddInt() {
    assertThat(list).isEmpty();

    list.addInt(2);
    assertThat(list).containsExactly(2);

    list.addInt(3);
    assertThat(list).containsExactly(2, 3).inOrder();
  }

  @Test
  public void testAddAll() {
    assertThat(list).isEmpty();

    assertThat(list.addAll(Collections.singleton(1))).isTrue();
    assertThat(list).hasSize(1);
    assertThat((int) list.get(0)).isEqualTo(1);
    assertThat(list.getInt(0)).isEqualTo(1);

    assertThat(list.addAll(asList(2, 3, 4, 5, 6))).isTrue();
    assertThat(list).containsExactly(1, 2, 3, 4, 5, 6).inOrder();

    assertThat(list.addAll(TERTIARY_LIST)).isTrue();
    assertThat(list).containsExactly(1, 2, 3, 4, 5, 6, 1, 2, 3).inOrder();

    assertThat(list.addAll(Collections.<Integer>emptyList())).isFalse();
    assertThat(list.addAll(IntArrayList.emptyList())).isFalse();
  }

  @Test
  public void testEquals() {
    IntArrayList list1 = new IntArrayList();
    IntArrayList list2 = new IntArrayList();

    assertThat(list1).isEqualTo(list2);
  }

  @Test
  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertThat((int) list.remove(0)).isEqualTo(1);
    assertThat(list).containsExactly(2, 3).inOrder();

    assertThat(list.remove(Integer.valueOf(3))).isTrue();
    assertThat(list).containsExactly(2);

    assertThat(list.remove(Integer.valueOf(3))).isFalse();
    assertThat(list).containsExactly(2);

    assertThat((int) list.remove(0)).isEqualTo(2);
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
    IntList toRemove = IntArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addInt(3);
    toRemove.remove(0);
    assertThat(toRemove).isEmpty();
  }

  @Test
  public void testRemove_listAtCapacity() {
    IntList toRemove = IntArrayList.emptyList().mutableCopyWithCapacity(2);
    toRemove.addInt(3);
    toRemove.addInt(4);
    toRemove.remove(0);
    assertThat(toRemove).hasSize(1);
    assertThat((int) toRemove.get(0)).isEqualTo(4);
  }

  @Test
  public void testSublistRemoveEndOfCapacity() {
    IntList toRemove = IntArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addInt(3);
    toRemove.subList(0, 1).clear();
    assertThat(toRemove).isEmpty();
  }

  @Test
  public void makeImmutable_singleElement_trimsIntArrayListCapacityAndFreezes() {
    IntArrayList intList = new IntArrayList();
    intList.addInt(42);

    intList.makeImmutable();

    assertThat(intList.isModifiable()).isFalse();
    assertThat(intList.getCapacityForTesting()).isEqualTo(1);
    assertThat(intList).containsExactly(42);
    assertThat(intList).isEqualTo(newImmutableIntArrayList(42));
    assertThat(intList.hashCode()).isEqualTo(newImmutableIntArrayList(42).hashCode());
    assertThrows(UnsupportedOperationException.class, () -> intList.addInt(99));
    assertThrows(UnsupportedOperationException.class, () -> intList.setInt(0, 99));
    assertThrows(UnsupportedOperationException.class, intList::clear);
  }

  @Test
  public void makeImmutable_singleElement_trimsLongArrayListCapacityAndFreezes() {
    LongArrayList longList = new LongArrayList();
    longList.addLong(42000000000L);

    longList.makeImmutable();

    assertThat(longList.isModifiable()).isFalse();
    assertThat(longList.getCapacityForTesting()).isEqualTo(1);
    assertThat(longList).containsExactly(42000000000L);
    assertThat(longList).isEqualTo(newImmutableLongArrayList(42000000000L));
    assertThat(longList.hashCode()).isEqualTo(newImmutableLongArrayList(42000000000L).hashCode());
    assertThrows(UnsupportedOperationException.class, () -> longList.addLong(99L));
    assertThrows(UnsupportedOperationException.class, () -> longList.setLong(0, 99L));
    assertThrows(UnsupportedOperationException.class, longList::clear);
  }

  @Test
  public void makeImmutable_grownBeyondDefaultCapacity_trimsAndRemainsIdempotent() {
    IntArrayList intList = newMutableIntArrayList(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
    LongArrayList longList =
        newMutableLongArrayList(10L, 11L, 12L, 13L, 14L, 15L, 16L, 17L, 18L, 19L, 20L);

    intList.makeImmutable();
    intList.makeImmutable();
    longList.makeImmutable();
    longList.makeImmutable();

    assertThat(intList.getCapacityForTesting()).isEqualTo(11);
    assertThat(intList).containsExactly(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20).inOrder();
    assertThat(longList.getCapacityForTesting()).isEqualTo(11);
    assertThat(longList)
        .containsExactly(10L, 11L, 12L, 13L, 14L, 15L, 16L, 17L, 18L, 19L, 20L)
        .inOrder();
  }

  @Test
  public void makeImmutable_emptyEnsuredAndClearedLists_resetBackingArrayToEmpty() {
    IntArrayList ensuredIntList = new IntArrayList();
    ensuredIntList.ensureCapacity(5000);
    IntArrayList clearedIntList = newMutableIntArrayList(7, 8);
    clearedIntList.clear();
    LongArrayList ensuredLongList = new LongArrayList();
    ensuredLongList.ensureCapacity(5000);
    LongArrayList clearedLongList = newMutableLongArrayList(7L, 8L);
    clearedLongList.clear();

    ensuredIntList.makeImmutable();
    clearedIntList.makeImmutable();
    ensuredLongList.makeImmutable();
    clearedLongList.makeImmutable();
    IntArrayList.emptyList().makeImmutable();
    LongArrayList.emptyList().makeImmutable();

    assertThat(ensuredIntList.isModifiable()).isFalse();
    assertThat(ensuredIntList.getCapacityForTesting()).isEqualTo(0);
    assertThat(ensuredIntList).isEmpty();
    assertThat(clearedIntList.isModifiable()).isFalse();
    assertThat(clearedIntList.getCapacityForTesting()).isEqualTo(0);
    assertThat(clearedIntList).isEmpty();
    assertThat(ensuredLongList.isModifiable()).isFalse();
    assertThat(ensuredLongList.getCapacityForTesting()).isEqualTo(0);
    assertThat(ensuredLongList).isEmpty();
    assertThat(clearedLongList.isModifiable()).isFalse();
    assertThat(clearedLongList.getCapacityForTesting()).isEqualTo(0);
    assertThat(clearedLongList).isEmpty();
    assertThat(IntArrayList.emptyList().isModifiable()).isFalse();
    assertThat(IntArrayList.emptyList().getCapacityForTesting()).isEqualTo(0);
    assertThat(LongArrayList.emptyList().isModifiable()).isFalse();
    assertThat(LongArrayList.emptyList().getCapacityForTesting()).isEqualTo(0);
  }

  @Test
  public void makeImmutable_atAndAboveMaxTrimCapacity_trimsAtBoundaryAndPreservesAbove() {
    IntArrayList atLimitInt =
        (IntArrayList) newImmutableIntArrayList(5, 6).mutableCopyWithCapacity(4096);
    IntArrayList aboveLimitInt =
        (IntArrayList) newImmutableIntArrayList(5, 6).mutableCopyWithCapacity(4097);
    IntArrayList largeLadderInt = newMutableIntSequence(4097);
    LongArrayList atLimitLong =
        (LongArrayList) newImmutableLongArrayList(5L, 6L).mutableCopyWithCapacity(4096);
    LongArrayList aboveLimitLong =
        (LongArrayList) newImmutableLongArrayList(5L, 6L).mutableCopyWithCapacity(4097);
    LongArrayList largeLadderLong = newMutableLongSequence(4097);

    atLimitInt.makeImmutable();
    aboveLimitInt.makeImmutable();
    largeLadderInt.makeImmutable();
    atLimitLong.makeImmutable();
    aboveLimitLong.makeImmutable();
    largeLadderLong.makeImmutable();

    assertThat(atLimitInt.getCapacityForTesting()).isEqualTo(2);
    assertThat(atLimitInt).containsExactly(5, 6).inOrder();
    assertThat(aboveLimitInt.getCapacityForTesting()).isEqualTo(4097);
    assertThat(aboveLimitInt).containsExactly(5, 6).inOrder();
    assertThat(largeLadderInt.getCapacityForTesting()).isEqualTo(5164);
    assertThat(largeLadderInt).hasSize(4097);
    assertThat(largeLadderInt.getInt(0)).isEqualTo(1000);
    assertThat(largeLadderInt.getInt(4096)).isEqualTo(5096);
    assertThat(atLimitLong.getCapacityForTesting()).isEqualTo(2);
    assertThat(atLimitLong).containsExactly(5L, 6L).inOrder();
    assertThat(aboveLimitLong.getCapacityForTesting()).isEqualTo(4097);
    assertThat(aboveLimitLong).containsExactly(5L, 6L).inOrder();
    assertThat(largeLadderLong.getCapacityForTesting()).isEqualTo(5164);
    assertThat(largeLadderLong).hasSize(4097);
    assertThat(largeLadderLong.getLong(0)).isEqualTo(1000L);
    assertThat(largeLadderLong.getLong(4096)).isEqualTo(5096L);
  }

  @Test
  public void makeImmutable_exactCapacityMutableLists_preservesCapacityAndFreezes() {
    IntArrayList wrappedInt = IntArrayList.unsafeWrap(new int[] {4, 5, 6});
    IntArrayList copiedInt =
        (IntArrayList) newImmutableIntArrayList(4, 5, 6).mutableCopyWithCapacity(3);
    LongArrayList wrappedLong = LongArrayList.unsafeWrap(new long[] {4L, 5L, 6L});
    LongArrayList copiedLong =
        (LongArrayList) newImmutableLongArrayList(4L, 5L, 6L).mutableCopyWithCapacity(3);

    wrappedInt.makeImmutable();
    copiedInt.makeImmutable();
    wrappedLong.makeImmutable();
    copiedLong.makeImmutable();

    assertThat(wrappedInt.isModifiable()).isFalse();
    assertThat(wrappedInt.getCapacityForTesting()).isEqualTo(3);
    assertThat(wrappedInt).containsExactly(4, 5, 6).inOrder();
    assertThrows(UnsupportedOperationException.class, () -> wrappedInt.addInt(7));
    assertThat(copiedInt.isModifiable()).isFalse();
    assertThat(copiedInt.getCapacityForTesting()).isEqualTo(3);
    assertThat(copiedInt).containsExactly(4, 5, 6).inOrder();
    assertThrows(UnsupportedOperationException.class, () -> copiedInt.addInt(7));
    assertThat(wrappedLong.isModifiable()).isFalse();
    assertThat(wrappedLong.getCapacityForTesting()).isEqualTo(3);
    assertThat(wrappedLong).containsExactly(4L, 5L, 6L).inOrder();
    assertThrows(UnsupportedOperationException.class, () -> wrappedLong.addLong(7L));
    assertThat(copiedLong.isModifiable()).isFalse();
    assertThat(copiedLong.getCapacityForTesting()).isEqualTo(3);
    assertThat(copiedLong).containsExactly(4L, 5L, 6L).inOrder();
    assertThrows(UnsupportedOperationException.class, () -> copiedLong.addLong(7L));
  }

  private void assertImmutable(IntList list) {
    if (list.contains(1)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Integer>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new IntArrayList());
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
      list.addAll(0, Collections.singleton(1));
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
      list.addAll(0, Collections.<Integer>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addInt(0);
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
      list.removeAll(Collections.<Integer>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1));
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
      list.retainAll(Collections.<Integer>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1));
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
      list.set(0, 0);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setInt(0, 0);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }

  private static IntArrayList newImmutableIntArrayList(int... elements) {
    IntArrayList list = newMutableIntArrayList(elements);
    list.makeImmutable();
    return list;
  }

  private static IntArrayList newMutableIntArrayList(int... elements) {
    IntArrayList list = new IntArrayList();
    for (int element : elements) {
      list.addInt(element);
    }
    return list;
  }

  private static LongArrayList newImmutableLongArrayList(long... elements) {
    LongArrayList list = newMutableLongArrayList(elements);
    list.makeImmutable();
    return list;
  }

  private static LongArrayList newMutableLongArrayList(long... elements) {
    LongArrayList list = new LongArrayList();
    for (long element : elements) {
      list.addLong(element);
    }
    return list;
  }

  private static IntArrayList newMutableIntSequence(int count) {
    IntArrayList list = new IntArrayList();
    for (int i = 0; i < count; i++) {
      list.addInt(1000 + i);
    }
    return list;
  }

  private static LongArrayList newMutableLongSequence(int count) {
    LongArrayList list = new LongArrayList();
    for (int i = 0; i < count; i++) {
      list.addLong(1000L + i);
    }
    return list;
  }
}
