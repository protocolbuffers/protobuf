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

import static com.google.protobuf.Internal.checkNotNull;

import com.google.protobuf.Internal.BooleanList;
import java.util.Arrays;
import java.util.Collection;
import java.util.RandomAccess;

/**
 * An implementation of {@link BooleanList} on top of a primitive array.
 *
 * @author dweis@google.com (Daniel Weis)
 */
final class BooleanArrayList extends AbstractProtobufList<Boolean>
    implements BooleanList, RandomAccess, PrimitiveNonBoxingCollection {

  private static final BooleanArrayList EMPTY_LIST = new BooleanArrayList(new boolean[0], 0);
  static {
    EMPTY_LIST.makeImmutable();
  }

  public static BooleanArrayList emptyList() {
    return EMPTY_LIST;
  }

  /** The backing store for the list. */
  private boolean[] array;

  /**
   * The size of the list distinct from the length of the array. That is, it is the number of
   * elements set in the list.
   */
  private int size;

  /** Constructs a new mutable {@code BooleanArrayList} with default capacity. */
  BooleanArrayList() {
    this(new boolean[DEFAULT_CAPACITY], 0);
  }

  /**
   * Constructs a new mutable {@code BooleanArrayList} containing the same elements as {@code
   * other}.
   */
  private BooleanArrayList(boolean[] other, int size) {
    array = other;
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
    if (!(o instanceof BooleanArrayList)) {
      return super.equals(o);
    }
    BooleanArrayList other = (BooleanArrayList) o;
    if (size != other.size) {
      return false;
    }

    final boolean[] arr = other.array;
    for (int i = 0; i < size; i++) {
      if (array[i] != arr[i]) {
        return false;
      }
    }

    return true;
  }

  @Override
  public int hashCode() {
    int result = 1;
    for (int i = 0; i < size; i++) {
      result = (31 * result) + Internal.hashBoolean(array[i]);
    }
    return result;
  }

  @Override
  public BooleanList mutableCopyWithCapacity(int capacity) {
    if (capacity < size) {
      throw new IllegalArgumentException();
    }
    return new BooleanArrayList(Arrays.copyOf(array, capacity), size);
  }

  @Override
  public Boolean get(int index) {
    return getBoolean(index);
  }

  @Override
  public boolean getBoolean(int index) {
    ensureIndexInRange(index);
    return array[index];
  }

  @Override
  public int size() {
    return size;
  }

  @Override
  public Boolean set(int index, Boolean element) {
    return setBoolean(index, element);
  }

  @Override
  public boolean setBoolean(int index, boolean element) {
    ensureIsMutable();
    ensureIndexInRange(index);
    boolean previousValue = array[index];
    array[index] = element;
    return previousValue;
  }

  @Override
  public boolean add(Boolean element) {
    addBoolean(element);
    return true;
  }

  @Override
  public void add(int index, Boolean element) {
    addBoolean(index, element);
  }

  /** Like {@link #add(Boolean)} but more efficient in that it doesn't box the element. */
  @Override
  public void addBoolean(boolean element) {
    ensureIsMutable();
    if (size == array.length) {
      // Resize to 1.5x the size
      int length = ((size * 3) / 2) + 1;
      boolean[] newArray = new boolean[length];

      System.arraycopy(array, 0, newArray, 0, size);
      array = newArray;
    }

    array[size++] = element;
  }

  /** Like {@link #add(int, Boolean)} but more efficient in that it doesn't box the element. */
  private void addBoolean(int index, boolean element) {
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
      boolean[] newArray = new boolean[length];

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
  public boolean addAll(Collection<? extends Boolean> collection) {
    ensureIsMutable();

    checkNotNull(collection);

    // We specialize when adding another BooleanArrayList to avoid boxing elements.
    if (!(collection instanceof BooleanArrayList)) {
      return super.addAll(collection);
    }

    BooleanArrayList list = (BooleanArrayList) collection;
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
  public boolean remove(Object o) {
    ensureIsMutable();
    for (int i = 0; i < size; i++) {
      if (o.equals(array[i])) {
        System.arraycopy(array, i + 1, array, i, size - i - 1);
        size--;
        modCount++;
        return true;
      }
    }
    return false;
  }

  @Override
  public Boolean remove(int index) {
    ensureIsMutable();
    ensureIndexInRange(index);
    boolean value = array[index];
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
