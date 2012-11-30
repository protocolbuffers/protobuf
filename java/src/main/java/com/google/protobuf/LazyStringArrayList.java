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

import java.util.List;
import java.util.AbstractList;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.RandomAccess;

/**
 * An implementation of {@link LazyStringList} that wraps an ArrayList. Each
 * element is either a ByteString or a String. It caches the last one requested
 * which is most likely the one needed next. This minimizes memory usage while
 * satisfying the most common use cases.
 * <p>
 * <strong>Note that this implementation is not synchronized.</strong>
 * If multiple threads access an <tt>ArrayList</tt> instance concurrently,
 * and at least one of the threads modifies the list structurally, it
 * <i>must</i> be synchronized externally.  (A structural modification is
 * any operation that adds or deletes one or more elements, or explicitly
 * resizes the backing array; merely setting the value of an element is not
 * a structural modification.)  This is typically accomplished by
 * synchronizing on some object that naturally encapsulates the list.
 * <p>
 * If the implementation is accessed via concurrent reads, this is thread safe.
 * Conversions are done in a thread safe manner. It's possible that the
 * conversion may happen more than once if two threads attempt to access the
 * same element and the modifications were not visible to each other, but this
 * will not result in any corruption of the list or change in behavior other
 * than performance.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class LazyStringArrayList extends AbstractList<String>
    implements LazyStringList, RandomAccess {

  public final static LazyStringList EMPTY = new UnmodifiableLazyStringList(
      new LazyStringArrayList());

  private final List<Object> list;

  public LazyStringArrayList() {
    list = new ArrayList<Object>();
  }

  public LazyStringArrayList(LazyStringList from) {
    list = new ArrayList<Object>(from.size());
    addAll(from);
  }

  public LazyStringArrayList(List<String> from) {
    list = new ArrayList<Object>(from);
  }

  @Override
  public String get(int index) {
    Object o = list.get(index);
    if (o instanceof String) {
      return (String) o;
    } else {
      ByteString bs = (ByteString) o;
      String s = bs.toStringUtf8();
      if (bs.isValidUtf8()) {
        list.set(index, s);
      }
      return s;
    }
  }

  @Override
  public int size() {
    return list.size();
  }

  @Override
  public String set(int index, String s) {
    Object o = list.set(index, s);
    return asString(o);
  }

  @Override
  public void add(int index, String element) {
    list.add(index, element);
    modCount++;
  }

  @Override
  public boolean addAll(Collection<? extends String> c) {
    // The default implementation of AbstractCollection.addAll(Collection)
    // delegates to add(Object). This implementation instead delegates to
    // addAll(int, Collection), which makes a special case for Collections
    // which are instances of LazyStringList.
    return addAll(size(), c);
  }

  @Override
  public boolean addAll(int index, Collection<? extends String> c) {
    // When copying from another LazyStringList, directly copy the underlying
    // elements rather than forcing each element to be decoded to a String.
    Collection<?> collection = c instanceof LazyStringList
        ? ((LazyStringList) c).getUnderlyingElements() : c;
    boolean ret = list.addAll(index, collection);
    modCount++;
    return ret;
  }

  @Override
  public String remove(int index) {
    Object o = list.remove(index);
    modCount++;
    return asString(o);
  }

  public void clear() {
    list.clear();
    modCount++;
  }

  // @Override
  public void add(ByteString element) {
    list.add(element);
    modCount++;
  }

  // @Override
  public ByteString getByteString(int index) {
    Object o = list.get(index);
    if (o instanceof String) {
      ByteString b = ByteString.copyFromUtf8((String) o);
      list.set(index, b);
      return b;
    } else {
      return (ByteString) o;
    }
  }

  private String asString(Object o) {
    if (o instanceof String) {
      return (String) o;
    } else {
      return ((ByteString) o).toStringUtf8();
    }
  }

  public List<?> getUnderlyingElements() {
    return Collections.unmodifiableList(list);
  }
}
