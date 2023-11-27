// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import com.google.protobuf.Internal.DoubleList;
import java.util.Arrays;
import java.util.Collection;
import java.util.RandomAccess;

/**
 * An implementation of {@link DoubleList} on top of a primitive array.
 *
 * @author dweis@google.com (Daniel Weis)
 */
final class DoubleArrayList extends AbstractProtobufList<Double>
    implements DoubleList, RandomAccess, PrimitiveNonBoxingCollection {

  private static final DoubleArrayList EMPTY_LIST = new DoubleArrayList(new double[0], 0, false);

  public static DoubleArrayList emptyList() {
    return EMPTY_LIST;
  }

  /** The backing store for the list. */
  private double[] array;

  /**
   * The size of the list distinct from the length of the array. That is, it is the number of
   * elements set in the list.
   */
  private int size;

  /** Constructs a new mutable {@code DoubleArrayList} with default capacity. */
  DoubleArrayList() {
    this(new double[DEFAULT_CAPACITY], 0, true);
  }

  /**
   * Constructs a new mutable {@code DoubleArrayList} containing the same elements as {@code other}.
   */
  private DoubleArrayList(double[] other, int size, boolean isMutable) {
    super(isMutable);
    this.array = other;
    this.size = size;
  }

  @Override
  protected void removeRange(int fromIndex, int toIndex) {
    ensureIsMutable();
    if (toIndex < fromIndex) {
      throw new IndexOutOfBoundsException("toIndex < fromIndex");
    }

    System.arraycopy(array, toIndex, array, fromIndex, size - toIndex);
    size -= (toIndex - fromIndex);
    modCount++;
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }
    if (!(o instanceof DoubleArrayList)) {
      return super.equals(o);
    }
    DoubleArrayList other = (DoubleArrayList) o;
    if (size != other.size) {
      return false;
    }

    final double[] arr = other.array;
    for (int i = 0; i < size; i++) {
      if (Double.doubleToLongBits(array[i]) != Double.doubleToLongBits(arr[i])) {
        return false;
      }
    }

    return true;
  }

  @Override
  public int hashCode() {
    int result = 1;
    for (int i = 0; i < size; i++) {
      long bits = Double.doubleToLongBits(array[i]);
      result = (31 * result) + Internal.hashLong(bits);
    }
    return result;
  }

  @Override
  public DoubleList mutableCopyWithCapacity(int capacity) {
    if (capacity < size) {
      throw new IllegalArgumentException();
    }
    return new DoubleArrayList(Arrays.copyOf(array, capacity), size, true);
  }

  @Override
  public Double get(int index) {
    return getDouble(index);
  }

  @Override
  public double getDouble(int index) {
    ensureIndexInRange(index);
    return array[index];
  }

  @Override
  public int indexOf(Object element) {
    if (!(element instanceof Double)) {
      return -1;
    }
    double unboxedElement = (Double) element;
    int numElems = size();
    for (int i = 0; i < numElems; i++) {
      if (array[i] == unboxedElement) {
        return i;
      }
    }
    return -1;
  }

  @Override
  public boolean contains(Object element) {
    return indexOf(element) != -1;
  }

  @Override
  public int size() {
    return size;
  }

  @Override
  public Double set(int index, Double element) {
    return setDouble(index, element);
  }

  @Override
  public double setDouble(int index, double element) {
    ensureIsMutable();
    ensureIndexInRange(index);
    double previousValue = array[index];
    array[index] = element;
    return previousValue;
  }

  @Override
  public boolean add(Double element) {
    addDouble(element);
    return true;
  }

  @Override
  public void add(int index, Double element) {
    addDouble(index, element);
  }

  /** Like {@link #add(Double)} but more efficient in that it doesn't box the element. */
  @Override
  public void addDouble(double element) {
    ensureIsMutable();
    if (size == array.length) {
      // Resize to 1.5x the size
      int length = ((size * 3) / 2) + 1;
      double[] newArray = new double[length];

      System.arraycopy(array, 0, newArray, 0, size);
      array = newArray;
    }

    array[size++] = element;
  }

  /** Like {@link #add(int, Double)} but more efficient in that it doesn't box the element. */
  private void addDouble(int index, double element) {
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
      double[] newArray = new double[length];

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
  public boolean addAll(Collection<? extends Double> collection) {
    ensureIsMutable();

    checkNotNull(collection);

    // We specialize when adding another DoubleArrayList to avoid boxing elements.
    if (!(collection instanceof DoubleArrayList)) {
      return super.addAll(collection);
    }

    DoubleArrayList list = (DoubleArrayList) collection;
    if (list.size == 0) {
      return false;
    }

    int overflow = Integer.MAX_VALUE - size;
    if (overflow < list.size) {
      // We can't actually represent a list this large.
      throw new OutOfMemoryError();
    }

    int newSize = size + list.size;
    if (newSize > array.length) {
      array = Arrays.copyOf(array, newSize);
    }

    System.arraycopy(list.array, 0, array, size, list.size);
    size = newSize;
    modCount++;
    return true;
  }

  @Override
  public Double remove(int index) {
    ensureIsMutable();
    ensureIndexInRange(index);
    double value = array[index];
    if (index < size - 1) {
      System.arraycopy(array, index + 1, array, index, size - index - 1);
    }
    size--;
    modCount++;
    return value;
  }

  /**
   * Ensures that the provided {@code index} is within the range of {@code [0, size]}. Throws an
   * {@link IndexOutOfBoundsException} if it is not.
   *
   * @param index the index to verify is in range
   */
  private void ensureIndexInRange(int index) {
    if (index < 0 || index >= size) {
      throw new IndexOutOfBoundsException(makeOutOfBoundsExceptionMessage(index));
    }
  }

  private String makeOutOfBoundsExceptionMessage(int index) {
    return "Index:" + index + ", Size:" + size;
  }
}
