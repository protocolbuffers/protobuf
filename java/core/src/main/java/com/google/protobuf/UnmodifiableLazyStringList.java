// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.AbstractList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.RandomAccess;

/**
 * An implementation of {@link LazyStringList} that wraps another {@link LazyStringList} such that
 * it cannot be modified via the wrapper.
 *
 * @author jonp@google.com (Jon Perlow)
 * @deprecated use {@link LazyStringArrayList#makeImmutable} instead.
 */
@Deprecated
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
  public Object getRaw(int index) {
    return list.getRaw(index);
  }

  @Override
  public int size() {
    return list.size();
  }

  @Override
  public ByteString getByteString(int index) {
    return list.getByteString(index);
  }

  @Override
  public void add(ByteString element) {
    throw new UnsupportedOperationException();
  }

  @Override
  public void set(int index, ByteString element) {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean addAllByteString(Collection<? extends ByteString> element) {
    throw new UnsupportedOperationException();
  }

  @Override
  public byte[] getByteArray(int index) {
    return list.getByteArray(index);
  }

  @Override
  public void add(byte[] element) {
    throw new UnsupportedOperationException();
  }

  @Override
  public void set(int index, byte[] element) {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean addAllByteArray(Collection<byte[]> element) {
    throw new UnsupportedOperationException();
  }

  @Override
  public ListIterator<String> listIterator(final int index) {
    return new ListIterator<String>() {
      ListIterator<String> iter = list.listIterator(index);

      @Override
      public boolean hasNext() {
        return iter.hasNext();
      }

      @Override
      public String next() {
        return iter.next();
      }

      @Override
      public boolean hasPrevious() {
        return iter.hasPrevious();
      }

      @Override
      public String previous() {
        return iter.previous();
      }

      @Override
      public int nextIndex() {
        return iter.nextIndex();
      }

      @Override
      public int previousIndex() {
        return iter.previousIndex();
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }

      @Override
      public void set(String o) {
        throw new UnsupportedOperationException();
      }

      @Override
      public void add(String o) {
        throw new UnsupportedOperationException();
      }
    };
  }

  @Override
  public Iterator<String> iterator() {
    return new Iterator<String>() {
      Iterator<String> iter = list.iterator();

      @Override
      public boolean hasNext() {
        return iter.hasNext();
      }

      @Override
      public String next() {
        return iter.next();
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }

  @Override
  public List<?> getUnderlyingElements() {
    // The returned value is already unmodifiable.
    return list.getUnderlyingElements();
  }

  @Override
  public void mergeFrom(LazyStringList other) {
    throw new UnsupportedOperationException();
  }

  @Override
  public List<byte[]> asByteArrayList() {
    return Collections.unmodifiableList(list.asByteArrayList());
  }

  @Override
  public List<ByteString> asByteStringList() {
    return Collections.unmodifiableList(list.asByteStringList());
  }

  @Override
  public LazyStringList getUnmodifiableView() {
    return this;
  }
}
