// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static java.lang.Math.max;

import com.google.protobuf.Internal.ProtobufList;
import java.util.Arrays;
import java.util.RandomAccess;

/** Implements {@link ProtobufList} for non-primitive and {@link String} types. */
final class ProtobufArrayList<E> extends AbstractProtobufList<E> implements RandomAccess {

  private static final Object[] EMPTY_ARRAY = new Object[0];

  private static final ProtobufArrayList<Object> EMPTY_LIST =
      new ProtobufArrayList<>(EMPTY_ARRAY, 0, false);

  @SuppressWarnings("unchecked") // Guaranteed safe by runtime.
  public static <E> ProtobufArrayList<E> emptyList() {
    return (ProtobufArrayList<E>) EMPTY_LIST;
  }

  private E[] array;
  private int size;

  @SuppressWarnings("unchecked")
  ProtobufArrayList() {
    this((E[]) EMPTY_ARRAY, 0, true);
  }

  private ProtobufArrayList(E[] array, int size, boolean isMutable) {
    super(isMutable);
    this.array = array;
    this.size = size;
  }

  @SuppressWarnings("unchecked") // Casting an empty Object[] to a generic array is safe.
  @Override
  public ProtobufArrayList<E> mutableCopyWithCapacity(int capacity) {
    if (capacity < size) {
      throw new IllegalArgumentException();
    }

    E[] newArray = capacity == 0 ? (E[]) EMPTY_ARRAY : Arrays.copyOf(array, capacity);

    return new ProtobufArrayList<E>(newArray, size, true);
  }

  @Override
  public boolean add(E element) {
    ensureIsMutable();

    if (size == array.length) {
      int length = growSize(array.length);
      E[] newArray = Arrays.copyOf(array, length);

      array = newArray;
    }

    array[size++] = element;
    modCount++;

    return true;
  }

  private static int growSize(int previousSize) {
    // Resize to 1.5x the size, rounding up to DEFAULT_CAPACITY.
    return max(((previousSize * 3) / 2) + 1, DEFAULT_CAPACITY);
  }

  @Override
  public void add(int index, E element) {
    ensureIsMutable();

    if (index < 0 || index > size) {
      throw new IndexOutOfBoundsException(makeOutOfBoundsExceptionMessage(index));
    }

    if (size < array.length) {
      // Shift everything over to make room
      System.arraycopy(array, index, array, index + 1, size - index);
    } else {
      int length = growSize(array.length);
      E[] newArray = createArray(length);

      // Copy the first part directly
      System.arraycopy(array, 0, newArray, 0, index);

      // Copy the rest shifted over by one to make room
      System.arraycopy(array, index, newArray, index + 1, size - index);
      array = newArray;
    }

    array[index] = element;
    size++;
    modCount++;
  }

  @Override
  public E get(int index) {
    ensureIndexInRange(index);
    return array[index];
  }

  @Override
  public E remove(int index) {
    ensureIsMutable();
    ensureIndexInRange(index);

    E value = array[index];
    if (index < size - 1) {
      System.arraycopy(array, index + 1, array, index, size - index - 1);
    }

    size--;
    modCount++;
    return value;
  }

  @Override
  public E set(int index, E element) {
    ensureIsMutable();
    ensureIndexInRange(index);

    E toReturn = array[index];
    array[index] = element;

    modCount++;
    return toReturn;
  }

  @Override
  public int size() {
    return size;
  }

  /** Ensures the backing array can fit at least minCapacity elements. */
  @SuppressWarnings("unchecked") // Casting an Object[] with no values inside to E[] is safe.
  void ensureCapacity(int minCapacity) {
    if (minCapacity <= array.length) {
      return;
    }
    if (array.length == 0) {
      array = (E[]) new Object[max(minCapacity, DEFAULT_CAPACITY)];
      return;
    }
    // To avoid quadratic copying when calling .addAllFoo(List) in a loop, we must not size to
    // exactly the requested capacity, but must exponentially grow instead. This is similar
    // behaviour to ArrayList.
    int n = array.length;
    while (n < minCapacity) {
      n = growSize(n);
    }
    array = Arrays.copyOf(array, n);
  }

  @SuppressWarnings("unchecked")
  private static <E> E[] createArray(int capacity) {
    return (E[]) new Object[capacity];
  }

  private void ensureIndexInRange(int index) {
    if (index < 0 || index >= size) {
      throw new IndexOutOfBoundsException(makeOutOfBoundsExceptionMessage(index));
    }
  }

  private String makeOutOfBoundsExceptionMessage(int index) {
    return "Index:" + index + ", Size:" + size;
  }
}
