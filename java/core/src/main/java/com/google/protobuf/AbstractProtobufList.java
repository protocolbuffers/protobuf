// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.Internal.ProtobufList;
import java.util.AbstractList;
import java.util.Collection;
import java.util.List;
import java.util.RandomAccess;

/**
 * An abstract implementation of {@link ProtobufList} which manages mutability semantics. All mutate
 * methods must check if the list is mutable before proceeding. Subclasses must invoke {@link
 * #ensureIsMutable()} manually when overriding those methods.
 *
 * <p>This implementation assumes all subclasses are array based, supporting random access.
 */
abstract class AbstractProtobufList<E> extends AbstractList<E> implements ProtobufList<E> {

  protected static final int DEFAULT_CAPACITY = 10;

  /** Whether or not this list is modifiable. */
  private boolean isMutable;

  /** Constructs a mutable list by default. */
  AbstractProtobufList() {
    this(true);
  }

  /** Constructs an immutable list for EMPTY lists */
  AbstractProtobufList(boolean isMutable) {
    this.isMutable = isMutable;
  }

  @Override
  public boolean equals(Object o) {
    if (o == this) {
      return true;
    }
    if (!(o instanceof List)) {
      return false;
    }
    // Handle lists that do not support RandomAccess as efficiently as possible by using an iterator
    // based approach in our super class. Otherwise our index based approach will avoid those
    // allocations.
    if (!(o instanceof RandomAccess)) {
      return super.equals(o);
    }

    List<?> other = (List<?>) o;
    final int size = size();
    if (size != other.size()) {
      return false;
    }
    for (int i = 0; i < size; i++) {
      if (!get(i).equals(other.get(i))) {
        return false;
      }
    }
    return true;
  }

  @Override
  public int hashCode() {
    final int size = size();
    int hashCode = 1;
    for (int i = 0; i < size; i++) {
      hashCode = (31 * hashCode) + get(i).hashCode();
    }
    return hashCode;
  }

  @Override
  public boolean add(E e) {
    ensureIsMutable();
    return super.add(e);
  }

  @Override
  public void add(int index, E element) {
    ensureIsMutable();
    super.add(index, element);
  }

  @Override
  public boolean addAll(Collection<? extends E> c) {
    ensureIsMutable();
    return super.addAll(c);
  }

  @Override
  public boolean addAll(int index, Collection<? extends E> c) {
    ensureIsMutable();
    return super.addAll(index, c);
  }

  @Override
  public void clear() {
    ensureIsMutable();
    super.clear();
  }

  @Override
  public boolean isModifiable() {
    return isMutable;
  }

  @Override
  public final void makeImmutable() {
    if (isMutable) {
      isMutable = false;
    }
  }

  @Override
  public E remove(int index) {
    ensureIsMutable();
    return super.remove(index);
  }

  @Override
  public boolean remove(Object o) {
    ensureIsMutable();
    int index = indexOf(o);
    if (index == -1) {
      return false;
    }
    remove(index);
    return true;
  }

  @Override
  public boolean removeAll(Collection<?> c) {
    ensureIsMutable();
    return super.removeAll(c);
  }

  @Override
  public boolean retainAll(Collection<?> c) {
    ensureIsMutable();
    return super.retainAll(c);
  }

  @Override
  public E set(int index, E element) {
    ensureIsMutable();
    return super.set(index, element);
  }

  /**
   * Throws an {@link UnsupportedOperationException} if the list is immutable. Subclasses are
   * responsible for invoking this method on mutate operations.
   */
  protected void ensureIsMutable() {
    if (!isMutable) {
      throw new UnsupportedOperationException();
    }
  }
}
