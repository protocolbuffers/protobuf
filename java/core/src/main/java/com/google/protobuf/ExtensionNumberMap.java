// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;
import static com.google.protobuf.Internal.checkState;
import static com.google.protobuf.Internal.findNextNonNull;

import java.util.Iterator;
import java.util.function.Function;
import java.util.function.ToIntFunction;

/**
 * More efficient alternative to {@code java.util.HashMap<Pair<O, Integer>, T>} which does not use
 * primitive boxing, is flat, does not use intermediate nodes or linked lists for buckets, and uses
 * linear probing.
 *
 * <p>This implementation abuses the fact that things like {@link Descriptors.FieldDescriptor} store
 * both their full name and number which are accessible using simple getters.
 */
final class ExtensionNumberMap<T, O> implements Iterable<T> {

  private final Function<? super T, O> containingTypeFunction;
  private final ToIntFunction<? super T> numberFunction;
  private Object[] entries = null;
  private int size = 0;
  private int threshold = 0;

  ExtensionNumberMap(
      Function<? super T, O> containingTypeFunction, ToIntFunction<? super T> numberFunction) {
    this.containingTypeFunction = checkNotNull(containingTypeFunction);
    this.numberFunction = checkNotNull(numberFunction);
  }

  int size() {
    return size;
  }

  boolean isEmpty() {
    return size() == 0;
  }

  @CanIgnoreReturnValue
  T put(T extension) {
    return putInternal(extension, true);
  }

  @CanIgnoreReturnValue
  T putIfAbsent(T extension) {
    return putInternal(extension, false);
  }

  @SuppressWarnings("unchecked") // Generic arrays.
  @CanIgnoreReturnValue
  private T putInternal(T extension, boolean replace) {
    checkNotNull(extension);
    // Copy members to stack, this is faster for the interpreter and this path is hot.
    final Object[] entries = beforePut();
    final Function<? super T, O> containingTypeFunction = this.containingTypeFunction;
    final ToIntFunction<? super T> numberFunction = this.numberFunction;
    final O containingType = checkNotNull(containingTypeFunction.apply(extension));
    final int number = numberFunction.applyAsInt(extension);
    final int mask = entries.length - 1;
    int slot = hash(containingType, number) & mask;
    while (true) {
      T otherExtension = (T) entries[slot];
      if (otherExtension == null) {
        entries[slot] = extension;
        ++size;
        afterPut(slot);
        return null;
      }
      if (containingType.equals(checkNotNull(containingTypeFunction.apply(otherExtension)))
          && numberFunction.applyAsInt(otherExtension) == number) {
        if (replace) {
          entries[slot] = extension;
        }
        return otherExtension;
      }
      slot = (slot + 1) & mask;
    }
  }

  @SuppressWarnings("unchecked") // Generic arrays.
  T get(O extendee, int number) {
    checkNotNull(extendee);
    if (isEmpty()) {
      return null;
    }
    // Copy members to stack, this is faster for the interpreter and this path is hot.
    final Object[] entries = this.entries;
    final Function<? super T, O> containingTypeFunction = this.containingTypeFunction;
    final ToIntFunction<? super T> numberFunction = this.numberFunction;
    final int mask = entries.length - 1;
    int slot = hash(extendee, number) & mask;
    while (true) {
      T extension = (T) entries[slot];
      if (extension == null) {
        return null;
      }
      if (extendee.equals(checkNotNull(containingTypeFunction.apply(extension)))
          && numberFunction.applyAsInt(extension) == number) {
        return extension;
      }
      slot = (slot + 1) & mask;
    }
  }

  boolean containsKey(O extendee, int number) {
    return get(extendee, number) != null;
  }

  @SuppressWarnings("unchecked") // Generic arrays.
  @CanIgnoreReturnValue
  T remove(O extendee, int number) {
    checkNotNull(extendee);
    if (isEmpty()) {
      return null;
    }
    // Copy members to stack, this is faster for the interpreter and this path is hot.
    final Object[] entries = this.entries;
    final Function<? super T, O> containingTypeFunction = this.containingTypeFunction;
    final ToIntFunction<? super T> numberFunction = this.numberFunction;
    final int mask = entries.length - 1;
    int slot = hash(extendee, number) & mask;
    while (true) {
      T otherExtension = (T) entries[slot];
      if (otherExtension == null) {
        return null;
      }
      if (extendee.equals(checkNotNull(containingTypeFunction.apply(otherExtension)))
          && numberFunction.applyAsInt(otherExtension) == number) {
        entries[slot] = null;
        --size;
        int i = slot;
        int j = slot;
        while (true) {
          j = (j + 1) & mask;
          if (entries[j] == null) {
            break;
          }
          final int k =
              hash(
                      checkNotNull(containingTypeFunction.apply((T) entries[j])),
                      numberFunction.applyAsInt((T) entries[j]))
                  & mask;
          if (i <= j) {
            if (i < k && k <= j) {
              continue;
            }
          } else {
            if (k <= j || i < k) {
              continue;
            }
          }
          entries[i] = entries[j];
          entries[j] = null;
          i = j;
        }
        return otherExtension;
      }
      slot = (slot + 1) & mask;
    }
  }

  @CanIgnoreReturnValue
  T remove(T extension) {
    return remove(
        checkNotNull(containingTypeFunction.apply(extension)),
        numberFunction.applyAsInt(extension));
  }

  @Override
  public Iterator<T> iterator() {
    return new ExtensionIterator();
  }

  private Object[] beforePut() {
    Object[] entries = this.entries;
    if (entries == null) {
      this.entries = entries = new Object[16];
      threshold = calculateThreshold(entries);
    }
    return entries;
  }

  @SuppressWarnings("unchecked") // Generic arrays.
  private void afterPut(int oldSlot) {
    if (size < threshold) {
      return;
    }
    // Copy members to stack, this is faster for the interpreter and this path is hot.
    final Object[] entries = this.entries;
    final Function<? super T, O> containingTypeFunction = this.containingTypeFunction;
    final ToIntFunction<? super T> numberFunction = this.numberFunction;
    int newLength;
    try {
      newLength = Math.multiplyExact(entries.length, 2);
    } catch (ArithmeticException e) {
      entries[oldSlot] = null;
      --size;
      throw e;
    }
    Object[] newEntries = new Object[newLength];
    final int newMask = newEntries.length - 1;

    try {
      for (int index = 0; index < entries.length; ++index) {
        T entry = (T) entries[index];
        if (entry == null) {
          continue;
        }
        int newSlot =
            hash(
                    checkNotNull(containingTypeFunction.apply(entry)),
                    numberFunction.applyAsInt(entry))
                & newMask;
        while (true) {
          if (newEntries[newSlot] == null) {
            newEntries[newSlot] = entry;
            break;
          }
          newSlot = (newSlot + 1) & newMask;
        }
      }
    } catch (RuntimeException | Error e) {
      entries[oldSlot] = null;
      --size;
      throw e;
    }

    this.entries = newEntries;
    threshold = calculateThreshold(newEntries);
  }

  private static int calculateThreshold(Object[] entries) {
    return (int) (((long) entries.length) * 2L / 3L);
  }

  private static <O> int hash(O extendee, int number) {
    return Hash.combine(extendee.hashCode(), Integer.hashCode(number));
  }

  private static boolean hasNext(Object[] entries, int size, int index, int count) {
    return count < size && index < entries.length;
  }

  private final class ExtensionIterator implements Iterator<T> {

    private int index = 0;
    private int count = 0;

    ExtensionIterator() {
      if (!isEmpty()) {
        index = findNextNonNull(entries, -1);
      }
    }

    @SuppressWarnings("unchecked") // Generic arrays.
    @Override
    public T next() {
      // Copy members to stack, this is faster for the interpreter and this path is hot.
      final Object[] entries = ExtensionNumberMap.this.entries;
      final int size = ExtensionNumberMap.this.size;
      final int index = this.index;
      int count = this.count;
      checkState(ExtensionNumberMap.hasNext(entries, size, index, count));
      T t = (T) entries[index];
      this.count = count = count + 1;
      if (count < size) {
        this.index = findNextNonNull(entries, index);
      }
      return t;
    }

    @Override
    public boolean hasNext() {
      return ExtensionNumberMap.hasNext(entries, size, index, count);
    }
  }
}
