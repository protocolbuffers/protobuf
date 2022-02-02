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

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static java.util.Arrays.asList;

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
    IntArrayList list = new IntArrayList();
    for (int element : elements) {
      list.addInt(element);
    }
    list.makeImmutable();
    return list;
  }
}
