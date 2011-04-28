// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

import java.util.AbstractList;
import java.util.RandomAccess;
import java.util.ListIterator;
import java.util.Iterator;

/**
 * An implementation of {@link LazyStringList} that wraps another
 * {@link LazyStringList} such that it cannot be modified via the wrapper.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class UnmodifiableLazyStringList extends AbstractList<String>
    implements LazyStringList, RandomAccess {

  private final LazyStringList list;

  public UnmodifiableLazyStringList(LazyStringList list) {
    this.list = list;
  }

  @Override
  public String get(int index) {
    return list.get(index);
  }

  @Override
  public int size() {
    return list.size();
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public ByteString getByteString(int index) {
    return list.getByteString(index);
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public void add(ByteString element) {
    throw new UnsupportedOperationException();
  }

  //@Override (Java 1.6 override semantics, but we must support 1.5)
  public ListIterator<String> listIterator(final int index) {
    return new ListIterator<String>() {
      ListIterator<String> iter = list.listIterator(index);

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public boolean hasNext() {
        return iter.hasNext();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public String next() {
        return iter.next();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public boolean hasPrevious() {
        return iter.hasPrevious();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public String previous() {
        return iter.previous();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public int nextIndex() {
        return iter.nextIndex();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public int previousIndex() {
        return iter.previousIndex();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public void remove() {
        throw new UnsupportedOperationException();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public void set(String o) {
        throw new UnsupportedOperationException();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public void add(String o) {
        throw new UnsupportedOperationException();
      }
    };
  }

  @Override
  public Iterator<String> iterator() {
    return new Iterator<String>() {
      Iterator<String> iter = list.iterator();

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public boolean hasNext() {
        return iter.hasNext();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public String next() {
        return iter.next();
      }

      //@Override (Java 1.6 override semantics, but we must support 1.5)
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }
}
