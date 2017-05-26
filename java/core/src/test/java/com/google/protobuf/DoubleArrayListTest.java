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

import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import junit.framework.TestCase;

/**
 * Tests for {@link DoubleArrayList}.
 *
 * @author dweis@google.com (Daniel Weis)
 */
public class DoubleArrayListTest extends TestCase {

  private static final DoubleArrayList UNARY_LIST =
      newImmutableDoubleArrayList(1);
  private static final DoubleArrayList TERTIARY_LIST =
      newImmutableDoubleArrayList(1, 2, 3);

  private DoubleArrayList list;

  @Override
  protected void setUp() throws Exception {
    list = new DoubleArrayList();
  }

  public void testEmptyListReturnsSameInstance() {
    assertSame(DoubleArrayList.emptyList(), DoubleArrayList.emptyList());
  }

  public void testEmptyListIsImmutable() {
    assertImmutable(DoubleArrayList.emptyList());
  }

  public void testMakeImmutable() {
    list.addDouble(3);
    list.addDouble(4);
    list.addDouble(5);
    list.addDouble(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  public void testModificationWithIteration() {
    list.addAll(asList(1D, 2D, 3D, 4D));
    Iterator<Double> iterator = list.iterator();
    assertEquals(4, list.size());
    assertEquals(1D, (double) list.get(0));
    assertEquals(1D, (double) iterator.next());
    list.set(0, 1D);
    assertEquals(2D, (double) iterator.next());

    list.remove(0);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0D);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  public void testGet() {
    assertEquals(1D, (double) TERTIARY_LIST.get(0));
    assertEquals(2D, (double) TERTIARY_LIST.get(1));
    assertEquals(3D, (double) TERTIARY_LIST.get(2));

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

  public void testGetDouble() {
    assertEquals(1D, TERTIARY_LIST.getDouble(0));
    assertEquals(2D, TERTIARY_LIST.getDouble(1));
    assertEquals(3D, TERTIARY_LIST.getDouble(2));

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
    assertEquals(0, DoubleArrayList.emptyList().size());
    assertEquals(1, UNARY_LIST.size());
    assertEquals(3, TERTIARY_LIST.size());

    list.addDouble(3);
    list.addDouble(4);
    list.addDouble(6);
    list.addDouble(8);
    assertEquals(4, list.size());

    list.remove(0);
    assertEquals(3, list.size());

    list.add(17D);
    assertEquals(4, list.size());
  }

  public void testSet() {
    list.addDouble(2);
    list.addDouble(4);

    assertEquals(2D, (double) list.set(0, 3D));
    assertEquals(3D, list.getDouble(0));

    assertEquals(4D, (double) list.set(1, 0D));
    assertEquals(0D, list.getDouble(1));

    try {
      list.set(-1, 0D);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0D);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testSetDouble() {
    list.addDouble(1);
    list.addDouble(3);

    assertEquals(1D, list.setDouble(0, 0));
    assertEquals(0D, list.getDouble(0));

    assertEquals(3D, list.setDouble(1, 0));
    assertEquals(0D, list.getDouble(1));

    try {
      list.setDouble(-1, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setDouble(2, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testAdd() {
    assertEquals(0, list.size());

    assertTrue(list.add(2D));
    assertEquals(asList(2D), list);

    assertTrue(list.add(3D));
    list.add(0, 4D);
    assertEquals(asList(4D, 2D, 3D), list);

    list.add(0, 1D);
    list.add(0, 0D);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Double.valueOf(5 + i));
    }
    assertEquals(
        asList(0D, 1D, 4D, 2D, 3D, 5D, 6D, 7D, 8D, 9D, 10D),
        list);

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

  public void testAddDouble() {
    assertEquals(0, list.size());

    list.addDouble(2);
    assertEquals(asList(2D), list);

    list.addDouble(3);
    assertEquals(asList(2D, 3D), list);
  }

  public void testAddAll() {
    assertEquals(0, list.size());

    assertTrue(list.addAll(Collections.singleton(1D)));
    assertEquals(1, list.size());
    assertEquals(1D, (double) list.get(0));
    assertEquals(1D, list.getDouble(0));

    assertTrue(list.addAll(asList(2D, 3D, 4D, 5D, 6D)));
    assertEquals(asList(1D, 2D, 3D, 4D, 5D, 6D), list);

    assertTrue(list.addAll(TERTIARY_LIST));
    assertEquals(asList(1D, 2D, 3D, 4D, 5D, 6D, 1D, 2D, 3D), list);

    assertFalse(list.addAll(Collections.<Double>emptyList()));
    assertFalse(list.addAll(DoubleArrayList.emptyList()));
  }

  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertEquals(1D, (double) list.remove(0));
    assertEquals(asList(2D, 3D), list);

    assertTrue(list.remove(Double.valueOf(3)));
    assertEquals(asList(2D), list);

    assertFalse(list.remove(Double.valueOf(3)));
    assertEquals(asList(2D), list);

    assertEquals(2D, (double) list.remove(0));
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

  private void assertImmutable(DoubleArrayList list) {
    if (list.contains(1D)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1D);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1D);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Double>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1D));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new DoubleArrayList());
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
      list.addAll(0, Collections.singleton(1D));
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
      list.addAll(0, Collections.<Double>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addDouble(0);
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
      list.removeAll(Collections.<Double>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1D));
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
      list.retainAll(Collections.<Double>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1D));
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
      list.set(0, 0D);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setDouble(0, 0);
      fail();
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
