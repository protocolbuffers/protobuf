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
import java.util.List;

/**
 * Tests for {@link ProtobufArrayList}.
 */
public class ProtobufArrayListTest extends TestCase {
  
  private static final ProtobufArrayList<Integer> UNARY_LIST = newImmutableProtoArrayList(1);
  private static final ProtobufArrayList<Integer> TERTIARY_LIST =
      newImmutableProtoArrayList(1, 2, 3);
  
  private ProtobufArrayList<Integer> list;
  
  @Override
  protected void setUp() throws Exception {
    list = new ProtobufArrayList<Integer>();
  }
  
  public void testEmptyListReturnsSameInstance() {
    assertSame(ProtobufArrayList.emptyList(), ProtobufArrayList.emptyList());
  }
  
  public void testEmptyListIsImmutable() {
    assertImmutable(ProtobufArrayList.<Integer>emptyList());
  }
  
  public void testModificationWithIteration() {
    list.addAll(asList(1, 2, 3, 4));
    Iterator<Integer> iterator = list.iterator();
    assertEquals(4, list.size());
    assertEquals(1, (int) list.get(0));
    assertEquals(1, (int) iterator.next());
    
    list.remove(0);
    try {
      iterator.next();
      fail();
    } catch (ConcurrentModificationException e) {
      // expected
    }
    
    iterator = list.iterator();
    list.set(0, 1);
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
  
  public void testMakeImmutable() {
    list.add(2);
    list.add(4);
    list.add(6);
    list.add(8);
    list.makeImmutable();
    assertImmutable(list);
  }
  
  public void testRemove() {
    list.add(2);
    list.add(4);
    list.add(6);

    list.remove(1);
    assertEquals(asList(2, 6), list);

    list.remove(1);
    assertEquals(asList(2), list);

    list.remove(0);
    assertEquals(asList(), list);
  }
  
  public void testGet() {
    list.add(2);
    list.add(6);
    
    assertEquals(2, (int) list.get(0));
    assertEquals(6, (int) list.get(1));
  }
  
  public void testSet() {
    list.add(2);
    list.add(6);
    
    list.set(0, 1);
    assertEquals(1, (int) list.get(0));
    list.set(1, 2);
    assertEquals(2, (int) list.get(1));
  }

  private void assertImmutable(List<Integer> list) {
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
      list.addAll(new ProtobufArrayList<Integer>());
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
      list.removeAll(Collections.emptyList());
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
      list.retainAll(Collections.emptyList());
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
  }
  
  private static ProtobufArrayList<Integer> newImmutableProtoArrayList(int... elements) {
    ProtobufArrayList<Integer> list = new ProtobufArrayList<Integer>();
    for (int element : elements) {
      list.add(element);
    }
    list.makeImmutable();
    return list;
  }
}
