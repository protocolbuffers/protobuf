// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.Internal.ProtobufList;
import java.util.Arrays;
import java.util.RandomAccess;

/** Implements {@link ProtobufList} for non-primitive and {@link String} types. */
final class ProtobufArrayList<E> extends AbstractProtobufList<E> implements RandomAccess {

  private static final ProtobufArrayList<Object> EMPTY_LIST =
      new ProtobufArrayList<Object>(new Object[0], 0, false);

  @SuppressWarnings("unchecked") // Guaranteed safe by runtime.
  public static <E> ProtobufArrayList<E> emptyList() {
    return (ProtobufArrayList<E>) EMPTY_LIST;
  }

  private E[] array;
  private int size;

  @SuppressWarnings("unchecked")
  ProtobufArrayList() {
    this((E[]) new Object[DEFAULT_CAPACITY], 0, true);
  }

  private ProtobufArrayList(E[] array, int size, boolean isMutable) {
    super(isMutable);
    this.array = array;
    this.size = size;
  }

  @Override
  public ProtobufArrayList<E> mutableCopyWithCapacity(int capacity) {
    if (capacity < size) {
      throw new IllegalArgumentException();
    }

    E[] newArray = Arrays.copyOf(array, capacity);

    return new ProtobufArrayList<E>(newArray, size, true);
  }

  @Override
  public boolean add(E element) {
    ensureIsMutable();

    if (size == array.length) {
      // Resize to 1.5x the size
      int length = ((size * 3) / 2) + 1;
      E[] newArray = Arrays.copyOf(array, length);

      array = newArray;
    }

    array[size++] = element;
    modCount++;

    return true;
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
      // Resize to 1.5x the size
      int length = ((size * 3) / 2) + 1;
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
