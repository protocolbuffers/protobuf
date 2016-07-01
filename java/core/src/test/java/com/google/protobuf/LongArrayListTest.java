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

import junit.framework.TestCase;

import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;

/**
 * Tests for {@link LongArrayList}.
 *
 * @author dweis@google.com (Daniel Weis)
 */
public class LongArrayListTest extends TestCase {

  private static final LongArrayList UNARY_LIST =
      newImmutableLongArrayList(1);
  private static final LongArrayList TERTIARY_LIST =
      newImmutableLongArrayList(1, 2, 3);

  private LongArrayList list;

  @Override
  protected void setUp() throws Exception {
    list = new LongArrayList();
  }

  public void testEmptyListReturnsSameInstance() {
    assertSame(LongArrayList.emptyList(), LongArrayList.emptyList());
  }

  public void testEmptyListIsImmutable() {
    assertImmutable(LongArrayList.emptyList());
  }

  public void testMakeImmutable() {
    list.addLong(3);
    list.addLong(4);
    list.addLong(5);
    list.addLong(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  public void testModificationWithIteration() {
    list.addAll(asList(1L, 2L, 3L, 4L));
    Iterator<Long> iterator = list.iterator();
    assertEquals(4, list.size());
    assertEquals(1L, (long) list.get(0));
    assertEquals(1L, (long) iterator.next());
    list.set(0, 1L);
    assertEquals(2L, (long) iterator.next());

    list.remove(0);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0L);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  public void testGet() {
    assertEquals(1L, (long) TERTIARY_LIST.get(0));
    assertEquals(2L, (long) TERTIARY_LIST.get(1));
    assertEquals(3L, (long) TERTIARY_LIST.get(2));

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

  public void testGetLong() {
    assertEquals(1L, TERTIARY_LIST.getLong(0));
    assertEquals(2L, TERTIARY_LIST.getLong(1));
    assertEquals(3L, TERTIARY_LIST.getLong(2));

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
    assertEquals(0, LongArrayList.emptyList().size());
    assertEquals(1, UNARY_LIST.size());
    assertEquals(3, TERTIARY_LIST.size());

    list.addLong(3);
    list.addLong(4);
    list.addLong(6);
    list.addLong(8);
    assertEquals(4, list.size());

    list.remove(0);
    assertEquals(3, list.size());

    list.add(17L);
    assertEquals(4, list.size());
  }

  public void testSet() {
    list.addLong(2);
    list.addLong(4);

    assertEquals(2L, (long) list.set(0, 3L));
    assertEquals(3L, list.getLong(0));

    assertEquals(4L, (long) list.set(1, 0L));
    assertEquals(0L, list.getLong(1));

    try {
      list.set(-1, 0L);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0L);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testSetLong() {
    list.addLong(1);
    list.addLong(3);

    assertEquals(1L, list.setLong(0, 0));
    assertEquals(0L, list.getLong(0));

    assertEquals(3L, list.setLong(1, 0));
    assertEquals(0L, list.getLong(1));

    try {
      list.setLong(-1, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setLong(2, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testAdd() {
    assertEquals(0, list.size());

    assertTrue(list.add(2L));
    assertEquals(asList(2L), list);

    assertTrue(list.add(3L));
    list.add(0, 4L);
    assertEquals(asList(4L, 2L, 3L), list);

    list.add(0, 1L);
    list.add(0, 0L);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Long.valueOf(5 + i));
    }
    assertEquals(
        asList(0L, 1L, 4L, 2L, 3L, 5L, 6L, 7L, 8L, 9L, 10L),
        list);

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

  public void testAddLong() {
    assertEquals(0, list.size());

    list.addLong(2);
    assertEquals(asList(2L), list);

    list.addLong(3);
    assertEquals(asList(2L, 3L), list);
  }

  public void testAddAll() {
    assertEquals(0, list.size());

    assertTrue(list.addAll(Collections.singleton(1L)));
    assertEquals(1, list.size());
    assertEquals(1L, (long) list.get(0));
    assertEquals(1L, list.getLong(0));

    assertTrue(list.addAll(asList(2L, 3L, 4L, 5L, 6L)));
    assertEquals(asList(1L, 2L, 3L, 4L, 5L, 6L), list);

    assertTrue(list.addAll(TERTIARY_LIST));
    assertEquals(asList(1L, 2L, 3L, 4L, 5L, 6L, 1L, 2L, 3L), list);

    assertFalse(list.addAll(Collections.<Long>emptyList()));
    assertFalse(list.addAll(LongArrayList.emptyList()));
  }

  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertEquals(1L, (long) list.remove(0));
    assertEquals(asList(2L, 3L), list);

    assertTrue(list.remove(Long.valueOf(3)));
    assertEquals(asList(2L), list);

    assertFalse(list.remove(Long.valueOf(3)));
    assertEquals(asList(2L), list);

    assertEquals(2L, (long) list.remove(0));
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

  private void assertImmutable(LongArrayList list) {
    if (list.contains(1L)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1L);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1L);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Long>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1L));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new LongArrayList());
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
      list.addAll(0, Collections.singleton(1L));
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
      list.addAll(0, Collections.<Long>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addLong(0);
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
      list.removeAll(Collections.<Long>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1L));
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
      list.retainAll(Collections.<Long>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1L));
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
      list.set(0, 0L);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setLong(0, 0);
      fail();
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
