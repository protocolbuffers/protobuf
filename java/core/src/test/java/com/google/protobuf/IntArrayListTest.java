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

import static java.util.Arrays.asList;

import com.google.protobuf.Internal.IntList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import junit.framework.TestCase;

/**
 * Tests for {@link IntArrayList}.
 *
 * @author dweis@google.com (Daniel Weis)
 */
public class IntArrayListTest extends TestCase {

  private static final IntArrayList UNARY_LIST =
      newImmutableIntArrayList(1);
  private static final IntArrayList TERTIARY_LIST =
      newImmutableIntArrayList(1, 2, 3);

  private IntArrayList list;

  @Override
  protected void setUp() throws Exception {
    list = new IntArrayList();
  }

  public void testEmptyListReturnsSameInstance() {
    assertSame(IntArrayList.emptyList(), IntArrayList.emptyList());
  }

  public void testEmptyListIsImmutable() {
    assertImmutable(IntArrayList.emptyList());
  }

  public void testMakeImmutable() {
    list.addInt(3);
    list.addInt(4);
    list.addInt(5);
    list.addInt(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  public void testModificationWithIteration() {
    list.addAll(asList(1, 2, 3, 4));
    Iterator<Integer> iterator = list.iterator();
    assertEquals(4, list.size());
    assertEquals(1, (int) list.get(0));
    assertEquals(1, (int) iterator.next());
    list.set(0, 1);
    assertEquals(2, (int) iterator.next());

    list.remove(0);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  public void testGet() {
    assertEquals(1, (int) TERTIARY_LIST.get(0));
    assertEquals(2, (int) TERTIARY_LIST.get(1));
    assertEquals(3, (int) TERTIARY_LIST.get(2));

    try {
      TERTIARY_LIST.get(-1);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      TERTIARY_LIST.get(3);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testGetInt() {
    assertEquals(1, TERTIARY_LIST.getInt(0));
    assertEquals(2, TERTIARY_LIST.getInt(1));
    assertEquals(3, TERTIARY_LIST.getInt(2));

    try {
      TERTIARY_LIST.get(-1);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      TERTIARY_LIST.get(3);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testSize() {
    assertEquals(0, IntArrayList.emptyList().size());
    assertEquals(1, UNARY_LIST.size());
    assertEquals(3, TERTIARY_LIST.size());

    list.addInt(3);
    list.addInt(4);
    list.addInt(6);
    list.addInt(8);
    assertEquals(4, list.size());

    list.remove(0);
    assertEquals(3, list.size());

    list.add(17);
    assertEquals(4, list.size());
  }

  public void testSet() {
    list.addInt(2);
    list.addInt(4);

    assertEquals(2, (int) list.set(0, 3));
    assertEquals(3, list.getInt(0));

    assertEquals(4, (int) list.set(1, 0));
    assertEquals(0, list.getInt(1));

    try {
      list.set(-1, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testSetInt() {
    list.addInt(1);
    list.addInt(3);

    assertEquals(1, list.setInt(0, 0));
    assertEquals(0, list.getInt(0));

    assertEquals(3, list.setInt(1, 0));
    assertEquals(0, list.getInt(1));

    try {
      list.setInt(-1, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setInt(2, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testAdd() {
    assertEquals(0, list.size());

    assertTrue(list.add(2));
    assertEquals(asList(2), list);

    assertTrue(list.add(3));
    list.add(0, 4);
    assertEquals(asList(4, 2, 3), list);

    list.add(0, 1);
    list.add(0, 0);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Integer.valueOf(5 + i));
    }
    assertEquals(
        asList(0, 1, 4, 2, 3, 5, 6, 7, 8, 9, 10),
        list);

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

  public void testAddInt() {
    assertEquals(0, list.size());

    list.addInt(2);
    assertEquals(asList(2), list);

    list.addInt(3);
    assertEquals(asList(2, 3), list);
  }

  public void testAddAll() {
    assertEquals(0, list.size());

    assertTrue(list.addAll(Collections.singleton(1)));
    assertEquals(1, list.size());
    assertEquals(1, (int) list.get(0));
    assertEquals(1, list.getInt(0));

    assertTrue(list.addAll(asList(2, 3, 4, 5, 6)));
    assertEquals(asList(1, 2, 3, 4, 5, 6), list);

    assertTrue(list.addAll(TERTIARY_LIST));
    assertEquals(asList(1, 2, 3, 4, 5, 6, 1, 2, 3), list);

    assertFalse(list.addAll(Collections.<Integer>emptyList()));
    assertFalse(list.addAll(IntArrayList.emptyList()));
  }

  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertEquals(1, (int) list.remove(0));
    assertEquals(asList(2, 3), list);

    assertTrue(list.remove(Integer.valueOf(3)));
    assertEquals(asList(2), list);

    assertFalse(list.remove(Integer.valueOf(3)));
    assertEquals(asList(2), list);

    assertEquals(2, (int) list.remove(0));
    assertEquals(asList(), list);

    try {
      list.remove(-1);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.remove(0);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testRemoveEndOfCapacity() {
    IntList toRemove =
        IntArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addInt(3);
    toRemove.remove(0);
    assertEquals(0, toRemove.size());
  }

  public void testSublistRemoveEndOfCapacity() {
    IntList toRemove =
        IntArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addInt(3);
    toRemove.subList(0, 1).clear();
    assertEquals(0, toRemove.size());
  }

  private void assertImmutable(IntList list) {
    if (list.contains(1)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Integer>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new IntArrayList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(UNARY_LIST);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(0, Collections.singleton(1));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(0, UNARY_LIST);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(0, Collections.<Integer>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addInt(0);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.clear();
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.remove(1);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.remove(new Object());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.<Integer>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(UNARY_LIST);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.<Integer>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(UNARY_LIST);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.set(0, 0);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setInt(0, 0);
      fail();
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
