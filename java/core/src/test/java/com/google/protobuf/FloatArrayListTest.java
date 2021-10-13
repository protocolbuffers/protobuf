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

import com.google.protobuf.Internal.FloatList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link FloatArrayList}. */
@RunWith(JUnit4.class)
public class FloatArrayListTest {

  private static final FloatArrayList UNARY_LIST = newImmutableFloatArrayList(1);
  private static final FloatArrayList TERTIARY_LIST = newImmutableFloatArrayList(1, 2, 3);

  private FloatArrayList list;

  @Before
  public void setUp() throws Exception {
    list = new FloatArrayList();
  }

  @Test
  public void testEmptyListReturnsSameInstance() {
    assertThat(FloatArrayList.emptyList()).isSameInstanceAs(FloatArrayList.emptyList());
  }

  @Test
  public void testEmptyListIsImmutable() {
    assertImmutable(FloatArrayList.emptyList());
  }

  @Test
  public void testMakeImmutable() {
    list.addFloat(3);
    list.addFloat(4);
    list.addFloat(5);
    list.addFloat(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  @Test
  public void testModificationWithIteration() {
    list.addAll(asList(1F, 2F, 3F, 4F));
    Iterator<Float> iterator = list.iterator();
    assertThat(list).hasSize(4);
    assertThat((float) list.get(0)).isEqualTo(1F);
    assertThat((float) iterator.next()).isEqualTo(1F);
    list.set(0, 1F);
    assertThat((float) iterator.next()).isEqualTo(2F);

    list.remove(0);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0F);
    try {
      iterator.next();
      assertWithMessage("expected exception").fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  @Test
  public void testGet() {
    assertThat((float) TERTIARY_LIST.get(0)).isEqualTo(1F);
    assertThat((float) TERTIARY_LIST.get(1)).isEqualTo(2F);
    assertThat((float) TERTIARY_LIST.get(2)).isEqualTo(3F);

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
  public void testGetFloat() {
    assertThat(TERTIARY_LIST.getFloat(0)).isEqualTo(1F);
    assertThat(TERTIARY_LIST.getFloat(1)).isEqualTo(2F);
    assertThat(TERTIARY_LIST.getFloat(2)).isEqualTo(3F);

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
    assertThat(UNARY_LIST.indexOf(2F)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_notInListWithDuplicates() {
    FloatArrayList listWithDupes = newImmutableFloatArrayList(1F, 1F);
    assertThat(listWithDupes.indexOf(2F)).isEqualTo(-1);
  }

  @Test
  public void testIndexOf_inList() {
    assertThat(TERTIARY_LIST.indexOf(2F)).isEqualTo(1);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchAtHead() {
    FloatArrayList listWithDupes = newImmutableFloatArrayList(1F, 1F, 2F);
    assertThat(listWithDupes.indexOf(1F)).isEqualTo(0);
  }

  @Test
  public void testIndexOf_inListWithDuplicates_matchMidList() {
    FloatArrayList listWithDupes = newImmutableFloatArrayList(2F, 1F, 1F, 2F);
    assertThat(listWithDupes.indexOf(1F)).isEqualTo(1);
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
    assertThat(UNARY_LIST).doesNotContain(2F);
  }

  @Test
  public void testContains_notInListWithDuplicates() {
    FloatArrayList listWithDupes = newImmutableFloatArrayList(1F, 1F);
    assertThat(listWithDupes).doesNotContain(2F);
  }

  @Test
  public void testContains_inList() {
    assertThat(TERTIARY_LIST).contains(2F);
  }

  @Test
  public void testContains_inListWithDuplicates_matchAtHead() {
    FloatArrayList listWithDupes = newImmutableFloatArrayList(1F, 1F, 2F);
    assertThat(listWithDupes).contains(1F);
  }

  @Test
  public void testContains_inListWithDuplicates_matchMidList() {
    FloatArrayList listWithDupes = newImmutableFloatArrayList(2F, 1F, 1F, 2F);
    assertThat(listWithDupes).contains(1F);
  }

  @Test
  public void testSize() {
    assertThat(FloatArrayList.emptyList()).isEmpty();
    assertThat(UNARY_LIST).hasSize(1);
    assertThat(TERTIARY_LIST).hasSize(3);

    list.addFloat(3);
    list.addFloat(4);
    list.addFloat(6);
    list.addFloat(8);
    assertThat(list).hasSize(4);

    list.remove(0);
    assertThat(list).hasSize(3);

    list.add(17F);
    assertThat(list).hasSize(4);
  }

  @Test
  public void testSet() {
    list.addFloat(2);
    list.addFloat(4);

    assertThat((float) list.set(0, 3F)).isEqualTo(2F);
    assertThat(list.getFloat(0)).isEqualTo(3F);

    assertThat((float) list.set(1, 0F)).isEqualTo(4F);
    assertThat(list.getFloat(1)).isEqualTo(0F);

    try {
      list.set(-1, 0F);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0F);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testSetFloat() {
    list.addFloat(1);
    list.addFloat(3);

    assertThat(list.setFloat(0, 0)).isEqualTo(1F);
    assertThat(list.getFloat(0)).isEqualTo(0F);

    assertThat(list.setFloat(1, 0)).isEqualTo(3F);
    assertThat(list.getFloat(1)).isEqualTo(0F);

    try {
      list.setFloat(-1, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setFloat(2, 0);
      assertWithMessage("expected exception").fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAdd() {
    assertThat(list).isEmpty();

    assertThat(list.add(2F)).isTrue();
    assertThat(list).containsExactly(2F);

    assertThat(list.add(3F)).isTrue();
    list.add(0, 4F);
    assertThat(list).containsExactly(4F, 2F, 3F).inOrder();

    list.add(0, 1F);
    list.add(0, 0F);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Float.valueOf(5 + i));
    }
    assertThat(list).containsExactly(0F, 1F, 4F, 2F, 3F, 5F, 6F, 7F, 8F, 9F, 10F).inOrder();

    try {
      list.add(-1, 5F);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.add(4, 5F);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  @Test
  public void testAddFloat() {
    assertThat(list).isEmpty();

    list.addFloat(2);
    assertThat(list).containsExactly(2F);

    list.addFloat(3);
    assertThat(list).containsExactly(2F, 3F).inOrder();
  }

  @Test
  public void testAddAll() {
    assertThat(list).isEmpty();

    assertThat(list.addAll(Collections.singleton(1F))).isTrue();
    assertThat(list).hasSize(1);
    assertThat((float) list.get(0)).isEqualTo(1F);
    assertThat(list.getFloat(0)).isEqualTo(1F);

    assertThat(list.addAll(asList(2F, 3F, 4F, 5F, 6F))).isTrue();
    assertThat(list).containsExactly(1F, 2F, 3F, 4F, 5F, 6F).inOrder();

    assertThat(list.addAll(TERTIARY_LIST)).isTrue();
    assertThat(list).containsExactly(1F, 2F, 3F, 4F, 5F, 6F, 1F, 2F, 3F).inOrder();

    assertThat(list.addAll(Collections.<Float>emptyList())).isFalse();
    assertThat(list.addAll(FloatArrayList.emptyList())).isFalse();
  }

  @Test
  public void testEquals() {
    FloatArrayList list1 = new FloatArrayList();
    FloatArrayList list2 = new FloatArrayList();

    list1.addFloat(Float.intBitsToFloat(0xff800001));
    list2.addFloat(Float.intBitsToFloat(0xff800002));
    assertThat(list1).isEqualTo(list2);
  }

  @Test
  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertThat((float) list.remove(0)).isEqualTo(1F);
    assertThat(list).containsExactly(2F, 3F).inOrder();

    assertThat(list.remove(Float.valueOf(3))).isTrue();
    assertThat(list).containsExactly(2F);

    assertThat(list.remove(Float.valueOf(3))).isFalse();
    assertThat(list).containsExactly(2F);

    assertThat((float) list.remove(0)).isEqualTo(2F);
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
    FloatList toRemove = FloatArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addFloat(3);
    toRemove.remove(0);
    assertThat(toRemove).isEmpty();
  }

  @Test
  public void testRemove_listAtCapacity() {
    FloatList toRemove = FloatArrayList.emptyList().mutableCopyWithCapacity(2);
    toRemove.addFloat(3);
    toRemove.addFloat(4);
    toRemove.remove(0);
    assertThat(toRemove).hasSize(1);
    assertThat((float) toRemove.get(0)).isEqualTo(4F);
  }

  @Test
  public void testSublistRemoveEndOfCapacity() {
    FloatList toRemove = FloatArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addFloat(3);
    toRemove.subList(0, 1).clear();
    assertThat(toRemove).isEmpty();
  }

  private void assertImmutable(FloatList list) {
    if (list.contains(1F)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1F);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1F);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Float>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1F));
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new FloatArrayList());
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
      list.addAll(0, Collections.singleton(1F));
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
      list.addAll(0, Collections.<Float>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addFloat(0);
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
      list.removeAll(Collections.<Float>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1F));
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
      list.retainAll(Collections.<Float>emptyList());
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1F));
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
      list.set(0, 0F);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setFloat(0, 0);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
  }

  private static FloatArrayList newImmutableFloatArrayList(float... elements) {
    FloatArrayList list = new FloatArrayList();
    for (float element : elements) {
      list.addFloat(element);
    }
    list.makeImmutable();
    return list;
  }
}
