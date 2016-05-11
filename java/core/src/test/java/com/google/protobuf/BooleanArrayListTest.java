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
 * Tests for {@link BooleanArrayList}.
 * 
 * @author dweis@google.com (Daniel Weis)
 */
public class BooleanArrayListTest extends TestCase {
  
  private static final BooleanArrayList UNARY_LIST = newImmutableBooleanArrayList(true);
  private static final BooleanArrayList TERTIARY_LIST =
      newImmutableBooleanArrayList(true, true, false);
  
  private BooleanArrayList list;
  
  @Override
  protected void setUp() throws Exception {
    list = new BooleanArrayList();
  }
  
  public void testEmptyListReturnsSameInstance() {
    assertSame(BooleanArrayList.emptyList(), BooleanArrayList.emptyList());
  }
  
  public void testEmptyListIsImmutable() {
    assertImmutable(BooleanArrayList.emptyList());
  }
  
  public void testMakeImmutable() {
    list.addBoolean(true);
    list.addBoolean(false);
    list.addBoolean(true);
    list.addBoolean(true);
    list.makeImmutable();
    assertImmutable(list);
  }
  
  public void testModificationWithIteration() {
    list.addAll(asList(true, false, false, true));
    Iterator<Boolean> iterator = list.iterator();
    assertEquals(4, list.size());
    assertEquals(true, (boolean) list.get(0));
    assertEquals(true, (boolean) iterator.next());
    list.set(0, true);
    assertEquals(false, (boolean) iterator.next());
    
    list.remove(0);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
    
    iterator = list.iterator();
    list.add(0, false);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
  }
  
  public void testGet() {
    assertEquals(true, (boolean) TERTIARY_LIST.get(0));
    assertEquals(true, (boolean) TERTIARY_LIST.get(1));
    assertEquals(false, (boolean) TERTIARY_LIST.get(2));
    
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
    assertEquals(true, TERTIARY_LIST.getBoolean(0));
    assertEquals(true, TERTIARY_LIST.getBoolean(1));
    assertEquals(false, TERTIARY_LIST.getBoolean(2));
    
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
    assertEquals(0, BooleanArrayList.emptyList().size());
    assertEquals(1, UNARY_LIST.size());
    assertEquals(3, TERTIARY_LIST.size());

    list.addBoolean(true);
    list.addBoolean(false);
    list.addBoolean(false);
    list.addBoolean(false);
    assertEquals(4, list.size());
    
    list.remove(0);
    assertEquals(3, list.size());
    
    list.add(true);
    assertEquals(4, list.size());
  }
  
  public void testSet() {
    list.addBoolean(false);
    list.addBoolean(false);
    
    assertEquals(false, (boolean) list.set(0, true));
    assertEquals(true, list.getBoolean(0));

    assertEquals(false, (boolean) list.set(1, false));
    assertEquals(false, list.getBoolean(1));
    
    try {
      list.set(-1, true);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.set(2, false);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }
  
  public void testSetInt() {
    list.addBoolean(true);
    list.addBoolean(true);
    
    assertEquals(true, list.setBoolean(0, false));
    assertEquals(false, list.getBoolean(0));

    assertEquals(true, list.setBoolean(1, false));
    assertEquals(false, list.getBoolean(1));
    
    try {
      list.setBoolean(-1, false);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }

    try {
      list.setBoolean(2, true);
      fail();
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }
  
  public void testAdd() {
    assertEquals(0, list.size());

    assertTrue(list.add(true));
    assertEquals(asList(true), list);

    assertTrue(list.add(false));
    list.add(0, false);
    assertEquals(asList(false, true, false), list);
    
    list.add(0, false);
    list.add(0, true);
    // Force a resize by getting up to 11 elements.
    for (int i = 0; i < 6; i++) {
      list.add(true);
    }
    assertEquals(asList(true, false, false, true, false, true, true, true, true, true, true), list);
    
    try {
      list.add(-1, false);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
    
    try {
      list.add(4, true);
    } catch (IndexOutOfBoundsException e) {
      // expected
    }
  }
  
  public void testAddInt() {
    assertEquals(0, list.size());

    list.addBoolean(true);
    assertEquals(asList(true), list);

    list.addBoolean(false);
    assertEquals(asList(true, false), list);
  }
  
  public void testAddAll() {
    assertEquals(0, list.size());

    assertTrue(list.addAll(Collections.singleton(false)));
    assertEquals(1, list.size());
    assertEquals(false, (boolean) list.get(0));
    assertEquals(false, list.getBoolean(0));
    
    assertTrue(list.addAll(asList(true, false, false, false, true)));
    assertEquals(asList(false, true, false, false, false, true), list);
    
    assertTrue(list.addAll(TERTIARY_LIST));
    assertEquals(asList(false, true, false, false, false, true, true, true, false), list);

    assertFalse(list.addAll(Collections.<Boolean>emptyList()));
    assertFalse(list.addAll(BooleanArrayList.emptyList()));
  }
  
  public void testRemove() {
    list.addAll(TERTIARY_LIST);
    assertEquals(true, (boolean) list.remove(0));
    assertEquals(asList(true, false), list);

    assertTrue(list.remove(Boolean.TRUE));
    assertEquals(asList(false), list);

    assertFalse(list.remove(Boolean.TRUE));
    assertEquals(asList(false), list);

    assertEquals(false, (boolean) list.remove(0));
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
  
  private void assertImmutable(BooleanArrayList list) {
    try {
      list.add(false);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.add(0, true);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.addAll(Collections.<Boolean>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.addAll(Collections.singletonList(false));
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.addAll(new BooleanArrayList());
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
      list.addAll(0, Collections.singleton(true));
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
      list.addAll(0, Collections.<Boolean>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.addBoolean(true);
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
      list.removeAll(Collections.<Boolean>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.removeAll(Collections.singleton(Boolean.TRUE));
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
      list.retainAll(Collections.<Boolean>emptyList());
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.retainAll(Collections.singleton(Boolean.TRUE));
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
      list.set(0, true);
      fail();
    } catch (UnsupportedOperationException e) {
      // expected
    }
    
    try {
      list.setBoolean(0, false);
      fail();
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
