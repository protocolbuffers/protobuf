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

import com.google.protobuf.Internal.LongList;

import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.RandomAccess;

/**
 * An implementation of {@link LongList} on top of a primitive array.
 * 
 * @author dweis@google.com (Daniel Weis)
 */
final class LongArrayList extends AbstractProtobufList<Long> implements LongList, RandomAccess {
  
  private static final int DEFAULT_CAPACITY = 10;
  
  private static final LongArrayList EMPTY_LIST = new LongArrayList();
  static {
    EMPTY_LIST.makeImmutable();
  }
  
  public static LongArrayList emptyList() {
    return EMPTY_LIST;
  }
  
  /**
   * The backing store for the list.
   */
  private long[] array;
  
  /**
   * The size of the list distinct from the length of the array. That is, it is the number of
   * elements set in the list.
   */
  private int size;

  /**
   * Constructs a new mutable {@code LongArrayList} with default capacity.
   */
  LongArrayList() {
    this(DEFAULT_CAPACITY);
  }

  /**
   * Constructs a new mutable {@code LongArrayList} with the provided capacity.
   */
  LongArrayList(int capacity) {
    array = new long[capacity];
    size = 0;
  }

  /**
   * Constructs a new mutable {@code LongArrayList} containing the same elements as {@code other}.
   */
  LongArrayList(List<Long> other) {
    if (other instanceof LongArrayList) {
      LongArrayList list = (LongArrayList) other;
      array = list.array.clone();
      size = list.size;
    } else {
      size = other.size();
      array = new long[size];
      for (int i = 0; i < size; i++) {
        array[i] = other.get(i);
      }
    }
  }
  
  @Override
  public Long get(int index) {
    return getLong(index);
  }

  @Override
  public long getLong(int index) {
    ensureIndexInRange(index);
    return array[index];
  }

  @Override
  public int size() {
    return size;
  }

  @Override
  public Long set(int index, Long element) {
    return setLong(index, element);
  }

  @Override
  public long setLong(int index, long element) {
    ensureIsMutable();
    ensureIndexInRange(index);
    long previousValue = array[index];
    array[index] = element;
    return previousValue;
  }

  @Override
  public void add(int index, Long element) {
    addLong(index, element);
  }

  /**
   * Like {@link #add(Long)} but more efficient in that it doesn't box the element.
   */
  @Override
  public void addLong(long element) {
    addLong(size, element);
  }

  /**
   * Like {@link #add(int, Long)} but more efficient in that it doesn't box the element.
   */
  private void addLong(int index, long element) {
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
      long[] newArray = new long[length];
      
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
  public boolean addAll(Collection<? extends Long> collection) {
    ensureIsMutable();
    
    if (collection == null) {
      throw new NullPointerException();
    }
    
    // We specialize when adding another LongArrayList to avoid boxing elements.
    if (!(collection instanceof LongArrayList)) {
      return super.addAll(collection);
    }
    
    LongArrayList list = (LongArrayList) collection;
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
        System.arraycopy(array, i + 1, array, i, size - i);
        size--;
        modCount++;
        return true;
      }
    }
    return false;
  }

  @Override
  public Long remove(int index) {
    ensureIsMutable();
    ensureIndexInRange(index);
    long value = array[index];
    System.arraycopy(array, index + 1, array, index, size - index);
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
