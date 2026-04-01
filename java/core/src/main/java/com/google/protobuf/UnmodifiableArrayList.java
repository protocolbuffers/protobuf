// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.AbstractList;
import java.util.RandomAccess;

/**
 * An unmodifiable list optimized for fast, devirtualized reads.
 * Mutator methods (add, set, remove) throw UnsupportedOperationException
 * by virtue of inheriting from AbstractList without overriding them.
 */
public class UnmodifiableArrayList<E> extends AbstractList<E> implements RandomAccess {
  
  private static final UnmodifiableArrayList<Object> EMPTY = new UnmodifiableArrayList<>(new Object[0], 0);

  @SuppressWarnings("unchecked")
  public static <E> UnmodifiableArrayList<E> emptyList() {
    return (UnmodifiableArrayList<E>) EMPTY;
  }

  protected Object[] elements;
  protected int size;

  UnmodifiableArrayList(Object[] elements, int size) {
    this.elements = elements;
    this.size = size;
  }

  @SuppressWarnings("unchecked")
  @Override
  public final E get(int index) {
    if (index < 0 || index >= size) {
      throw new IndexOutOfBoundsException("Index: " + index + ", Size: " + size);
    }
    return (E) elements[index];
  }

  @Override
  public final int size() {
    return size;
  }
}
