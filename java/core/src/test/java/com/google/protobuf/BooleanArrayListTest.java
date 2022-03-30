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

import com.google.protobuf.Internal.BooleanList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link BooleanArrayList}. */
@RunWith(JUnit4.class)
public class BooleanArrayListTest {

  private static final BooleanArrayList UNARY_LIST = newImmutableBooleanArrayList(true);
  private static final BooleanArrayList TERTIARY_LIST =
      newImmutableBooleanArrayList(true, false, true);

  private BooleanArrayList list;

  @Before
  public void setUp() throws Exception {
    list = new BooleanArrayList();
  }

  @Test
  public void testEmptyListReturnsSameInstance() {
    assertThat(BooleanArrayList.emptyList()).isSameInstanceAs(BooleanArrayList.emptyList());
  }

  @Test
  public void testEmptyListIsImmutable() {
    assertImmutable(BooleanArrayList.emptyList());
  }

  @Test
  public void testMakeImmutable() {
    list.addBoolean(true);
    list.addBoolean(false);
    list.addBoolean(true);
    list.addBoolean(true);
    list.makeImmutable();
    assertImmutable(list);
  }

  @Test
  public void testModificationWithIteration() {
    list.addAll(asList(true, false, true, false));
    Iterator<Boolean> iterator = list.iterator();
    assertThat(list).hasSize(4);
    assertThat((boolean) list.get(0)).isEqualTo(true);
    assertThat((boolean) iterator.next()).isEqualTo(true);
    list.set(0, true);
    assertThat((boolean) iterator.next()).isEqualTo(false);

    list.remove(0);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, false);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  @Test
  public void testGet() {
    assertThat((boolean) TERTIARY_LIST.get(0)).isEqualTo(true);
    assertThat((boolean) TERTIARY_LIST.get(1)).isEqualTo(false);
    assertThat((boolean) TERTIARY_LIST.get(2)).isEqualTo(true);

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
  public void testGetBoolean() {
    assertThat(TERTIARY_LIST.getBoolean(0)).isEqualTo(true);
    assertThat(TERTIARY_LIST.getBoolean(1)).isEqualTo(false);
    assertThat(TERTIARY_LIST.getBoolean(2)).isEqualTo(true);

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
    assertThat(UNARY_LIST.indexOf(false)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_notInListWithDuplicates() {
    BooleanArrayList listWithDupes = newImmutableBooleanArrayList(true, true);
    assertThat(listWithDupes.indexOf(false)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_inList() {
    assertThat(TERTIARY_LIST.indexOf(false)).isEqualTo(1);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchAtHead() {
    BooleanArrayList listWithDupes = newImmutableBooleanArrayList(true, true, false);
    assertThat(listWithDupes.indexOf(true)).isEqualTo(0);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchMidList() {
    BooleanArrayList listWithDupes = newImmutableBooleanArrayList(false, true, true, false);
    assertThat(listWithDupes.indexOf(true)).isEqualTo(1);
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
    assertThat(UNARY_LIST).doesNotContain(false);
  }

  @Test
  public void testContains_notInListWithDuplicates() {
    BooleanArrayList listWithDupes = newImmutableBooleanArrayList(true, true);
    assertThat(listWithDupes).doesNotContain(false);
  }

  @Test
  public void testContains_inList() {
    assertThat(TERTIARY_LIST).contains(false);
  }

  @Test
  public void testContains_inListWithDuplicates_matchAtHead() {
    BooleanArrayList listWithDupes = newImmutableBooleanArrayList(true, true, false);
    assertThat(listWithDupes).contains(true);
  }

  @Test
  public void testContains_inListWithDuplicates_matchMidList() {
    BooleanArrayList listWithDupes = newImmutableBooleanArrayList(false, true, true, false);
    assertThat(listWithDupes).contains(true);
  }

  @Test
  public void testSize() {
    assertThat(BooleanArrayList.emptyList()).isEmpty();
    assertThat(UNARY_LIST).hasSize(1);
    assertThat(TERTIARY_LIST).hasSize(3);

    list.addBoolean(true);
    list.addBoolean(false);
    list.addBoolean(false);
    list.addBoolean(false);
    assertThat(list).hasSize(4);

    list.remove(0);
    assertThat(list).hasSize(3);

    list.add(true);
    assertThat(list).hasSize(4);
  }

  @Test
  public void testSet() {
    list.addBoolean(false);
    list.addBoolean(false);

    assertThat((boolean) list.set(0, true)).isEqualTo(false);
    assertThat(list.getBoolean(0)).isEqualTo(true);

    assertThat((boolean) list.set(1, false)).isEqualTo(false);
    assertThat(list.getBoolean(1)).isEqualTo(false);

    try {
      list.set(-1, false);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, false);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testSetBoolean() {
    list.addBoolean(true);
    list.addBoolean(true);

    assertThat(list.setBoolean(0, false)).isEqualTo(true);
    assertThat(list.getBoolean(0)).isEqualTo(false);

    assertThat(list.setBoolean(1, false)).isEqualTo(true);
    assertThat(list.getBoolean(1)).isEqualTo(false);

    try {
      list.setBoolean(-1, false);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setBoolean(2, false);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAdd() {
    assertThat(list).isEmpty();

    assertThat(list.add(false)).isTrue();
    assertThat(list).containsExactly(false);

    assertThat(list.add(true)).isTrue();
    list.add(0, false);
    assertThat(list).containsExactly(false, false, true).inOrder();

    list.add(0, true);
    list.add(0, false);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(i % 2 == 0);
    }
    assertThat(list)
        .containsExactly(false, true, false, false, true, true, false, true, false, true, false)
        .inOrder();

    try {
      list.add(-1, true);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.add(4, true);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAddBoolean() {
    assertThat(list).isEmpty();

    list.addBoolean(false);
    assertThat(list).containsExactly(false);

    list.addBoolean(true);
    assertThat(list).containsExactly(false, true).inOrder();
  }

  @Test
  public void testAddAll() {
    assertThat(list).isEmpty();

    assertThat(list.addAll(Collections.singleton(true))).isTrue();
    assertThat(list).hasSize(1);
    assertThat((boolean) list.get(0)).isEqualTo(true);
    assertThat(list.getBoolean(0)).isEqualTo(true);

    assertThat(list.addAll(asList(false, true, false, true, false))).isTrue();
    assertThat(list).containsExactly(true, false, true, false, true, false).inOrder();

    assertThat(list.addAll(TERTIARY_LIST)).isTrue();
    assertThat(list)
        .containsExactly(true, false, true, false, true, false, true, false, true)
        .inOrder();

    assertThat(list.addAll(Collections.<Boolean>emptyList())).isFalse();
    assertThat(list.addAll(BooleanArrayList.emptyList())).isFalse();
  }

  @Test
  public void testEquals() {
    BooleanArrayList list1 = new BooleanArrayList();
    BooleanArrayList list2 = new BooleanArrayList();

    assertThat(list1).isEqualTo(list2);
  }

  @Test
  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertThat((boolean) list.remove(0)).isEqualTo(true);
    assertThat(list).containsExactly(false, true).inOrder();

    assertThat(list.remove(Boolean.TRUE)).isTrue();
    assertThat(list).containsExactly(false);

    assertThat(list.remove(Boolean.TRUE)).isFalse();
    assertThat(list).containsExactly(false);

    assertThat((boolean) list.remove(0)).isEqualTo(false);
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
    BooleanList toRemove = BooleanArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addBoolean(true);
    toRemove.remove(0);
    assertThat(toRemove).isEmpty();
  }

  @Test
  public void testRemove_listAtCapacity() {
    BooleanList toRemove = BooleanArrayList.emptyList().mutableCopyWithCapacity(2);
    toRemove.addBoolean(true);
    toRemove.addBoolean(false);
    toRemove.remove(0);
    assertThat(toRemove).hasSize(1);
    assertThat((boolean) toRemove.get(0)).isEqualTo(false);
  }

  @Test
  public void testSublistRemoveEndOfCapacity() {
    BooleanList toRemove = BooleanArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addBoolean(true);
    toRemove.subList(0, 1).clear();
    assertThat(toRemove).isEmpty();
  }

  private void assertImmutable(BooleanList list) {

    try {
      list.add(true);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, true);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Boolean>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(true));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new BooleanArrayList());
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
      list.addAll(0, Collections.singleton(true));
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
      list.addAll(0, Collections.<Boolean>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addBoolean(false);
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
      list.removeAll(Collections.<Boolean>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(Boolean.TRUE));
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
      list.retainAll(Collections.<Boolean>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(Boolean.TRUE));
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
      list.set(0, false);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setBoolean(0, false);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }

  private static BooleanArrayList newImmutableBooleanArrayList(boolean... elements) {
    BooleanArrayList list = new BooleanArrayList();
    for (boolean element : elements) {
      list.addBoolean(element);
    }
    list.makeImmutable();
    return list;
  }
}
