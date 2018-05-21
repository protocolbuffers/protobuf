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

import com.google.protobuf.Internal.FloatList;
import java.util.Collections;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import junit.framework.TestCase;

/**
 * Tests for {@link FloatArrayList}.
 *
 * @author dweis@google.com (Daniel Weis)
 */
public class FloatArrayListTest extends TestCase {

  private static final FloatArrayList UNARY_LIST =
      newImmutableFloatArrayList(1);
  private static final FloatArrayList TERTIARY_LIST =
      newImmutableFloatArrayList(1, 2, 3);

  private FloatArrayList list;

  @Override
  protected void setUp() throws Exception {
    list = new FloatArrayList();
  }

  public void testEmptyListReturnsSameInstance() {
    assertSame(FloatArrayList.emptyList(), FloatArrayList.emptyList());
  }

  public void testEmptyListIsImmutable() {
    assertImmutable(FloatArrayList.emptyList());
  }

  public void testMakeImmutable() {
    list.addFloat(3);
    list.addFloat(4);
    list.addFloat(5);
    list.addFloat(7);
    list.makeImmutable();
    assertImmutable(list);
  }

  public void testModificationWithIteration() {
    list.addAll(asList(1F, 2F, 3F, 4F));
    Iterator<Float> iterator = list.iterator();
    assertEquals(4, list.size());
    assertEquals(1F, (float) list.get(0));
    assertEquals(1F, (float) iterator.next());
    list.set(0, 1F);
    assertEquals(2F, (float) iterator.next());

    list.remove(0);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }

    iterator = list.iterator();
    list.add(0, 0F);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }

  public void testGet() {
    assertEquals(1F, (float) TERTIARY_LIST.get(0));
    assertEquals(2F, (float) TERTIARY_LIST.get(1));
    assertEquals(3F, (float) TERTIARY_LIST.get(2));

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

  public void testGetFloat() {
    assertEquals(1F, TERTIARY_LIST.getFloat(0));
    assertEquals(2F, TERTIARY_LIST.getFloat(1));
    assertEquals(3F, TERTIARY_LIST.getFloat(2));

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
    assertEquals(0, FloatArrayList.emptyList().size());
    assertEquals(1, UNARY_LIST.size());
    assertEquals(3, TERTIARY_LIST.size());

    list.addFloat(3);
    list.addFloat(4);
    list.addFloat(6);
    list.addFloat(8);
    assertEquals(4, list.size());

    list.remove(0);
    assertEquals(3, list.size());

    list.add(17F);
    assertEquals(4, list.size());
  }

  public void testSet() {
    list.addFloat(2);
    list.addFloat(4);

    assertEquals(2F, (float) list.set(0, 3F));
    assertEquals(3F, list.getFloat(0));

    assertEquals(4F, (float) list.set(1, 0F));
    assertEquals(0F, list.getFloat(1));

    try {
      list.set(-1, 0F);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, 0F);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testSetFloat() {
    list.addFloat(1);
    list.addFloat(3);

    assertEquals(1F, list.setFloat(0, 0));
    assertEquals(0F, list.getFloat(0));

    assertEquals(3F, list.setFloat(1, 0));
    assertEquals(0F, list.getFloat(1));

    try {
      list.setFloat(-1, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setFloat(2, 0);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }

  public void testAdd() {
    assertEquals(0, list.size());

    assertTrue(list.add(2F));
    assertEquals(asList(2F), list);

    assertTrue(list.add(3F));
    list.add(0, 4F);
    assertEquals(asList(4F, 2F, 3F), list);

    list.add(0, 1F);
    list.add(0, 0F);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(Float.valueOf(5 + i));
    }
    assertEquals(
        asList(0F, 1F, 4F, 2F, 3F, 5F, 6F, 7F, 8F, 9F, 10F),
        list);

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

  public void testAddFloat() {
    assertEquals(0, list.size());

    list.addFloat(2);
    assertEquals(asList(2F), list);

    list.addFloat(3);
    assertEquals(asList(2F, 3F), list);
  }

  public void testAddAll() {
    assertEquals(0, list.size());

    assertTrue(list.addAll(Collections.singleton(1F)));
    assertEquals(1, list.size());
    assertEquals(1F, (float) list.get(0));
    assertEquals(1F, list.getFloat(0));

    assertTrue(list.addAll(asList(2F, 3F, 4F, 5F, 6F)));
    assertEquals(asList(1F, 2F, 3F, 4F, 5F, 6F), list);

    assertTrue(list.addAll(TERTIARY_LIST));
    assertEquals(asList(1F, 2F, 3F, 4F, 5F, 6F, 1F, 2F, 3F), list);

    assertFalse(list.addAll(Collections.<Float>emptyList()));
    assertFalse(list.addAll(FloatArrayList.emptyList()));
  }

  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertEquals(1F, (float) list.remove(0));
    assertEquals(asList(2F, 3F), list);

    assertTrue(list.remove(Float.valueOf(3)));
    assertEquals(asList(2F), list);

    assertFalse(list.remove(Float.valueOf(3)));
    assertEquals(asList(2F), list);

    assertEquals(2F, (float) list.remove(0));
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
    FloatList toRemove = FloatArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addFloat(3);
    toRemove.remove(0);
    assertEquals(0, toRemove.size());
  }

  public void testSublistRemoveEndOfCapacity() {
    FloatList toRemove = FloatArrayList.emptyList().mutableCopyWithCapacity(1);
    toRemove.addFloat(3);
    toRemove.subList(0, 1).clear();
    assertEquals(0, toRemove.size());
  }

  private void assertImmutable(FloatArrayList list) {
    if (list.contains(1F)) {
      throw new RuntimeException("Cannot test the immutability of lists that contain 1.");
    }

    try {
      list.add(1F);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.add(0, 1F);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.<Float>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(Collections.singletonList(1F));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addAll(new FloatArrayList());
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
      list.addAll(0, Collections.singleton(1F));
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
      list.addAll(0, Collections.<Float>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.addFloat(0);
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
      list.removeAll(Collections.<Float>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.removeAll(Collections.singleton(1F));
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
      list.retainAll(Collections.<Float>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.retainAll(Collections.singleton(1F));
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
      list.set(0, 0F);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }

    try {
      list.setFloat(0, 0);
      fail();
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
