// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.Arrays;
import java.util.Collection;

/**
 * A mutable list used internally by Protobuf Builders.
 */
public final class ModifiableArrayList<E> extends UnmodifiableArrayList<E> {
  private boolean shared;

  public ModifiableArrayList(int capacity) {
    super(new Object[capacity], 0);
  }

  public ModifiableArrayList(ModifiableArrayList<E> other) {
    super(Arrays.copyOf(other.elements, other.size), other.size);
  }

  public ModifiableArrayList(Collection<? extends E> other) {
    super(other.toArray(), other.size());
  }

  /**
   * Triggers Copy-on-Write if the array has been shared with an UnmodifiableArrayList.
   */
  private void ensureCapacityAndUnshared(int minCapacity) {
    if (shared || minCapacity > elements.length) {
      int newCapacity = Math.max(elements.length * 2, minCapacity);
      elements = Arrays.copyOf(elements, newCapacity);
      shared = false;
    }
  }

  @Override
  public boolean add(E element) {
    ensureCapacityAndUnshared(size + 1);
    elements[size++] = element;
    modCount++;
    return true;
  }

  @Override
  public void add(int index, E element) {
    if (index < 0 || index > size) {
      throw new IndexOutOfBoundsException("Index: " + index + ", Size: " + size);
    }
    ensureCapacityAndUnshared(size + 1);
    System.arraycopy(elements, index, elements, index + 1, size - index);
    elements[index] = element;
    size++;
    modCount++;
  }

  @Override
  public E set(int index, E element) {
    if (index < 0 || index >= size) {
      throw new IndexOutOfBoundsException("Index: " + index + ", Size: " + size);
    }
    ensureCapacityAndUnshared(size);
    @SuppressWarnings("unchecked")
    E oldValue = (E) elements[index];
    elements[index] = element;
    return oldValue;
  }

  @Override
  public E remove(int index) {
    if (index < 0 || index >= size) {
      throw new IndexOutOfBoundsException("Index: " + index + ", Size: " + size);
    }
    ensureCapacityAndUnshared(size);
    @SuppressWarnings("unchecked")
    E oldValue = (E) elements[index];
    int numMoved = size - index - 1;
    if (numMoved > 0) {
      System.arraycopy(elements, index + 1, elements, index, numMoved);
    }
    elements[--size] = null; // clear to let GC do its work
    modCount++;
    return oldValue;
  }

  @Override
  public void clear() {
    ensureCapacityAndUnshared(size);
    for (int i = 0; i < size; i++) {
      elements[i] = null;
    }
    size = 0;
    modCount++;
  }

  /**
   * Freezes the current contents and returns an unmodifiable view sharing the SAME array.
   */
  public UnmodifiableArrayList<E> toUnmodifiable() {
    this.shared = true;
    return new UnmodifiableArrayList<>(this.elements, this.size);
  }
}
