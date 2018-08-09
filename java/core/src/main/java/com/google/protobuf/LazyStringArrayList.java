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

import java.util.AbstractList;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.RandomAccess;

/**
 * An implementation of {@link LazyStringList} that wraps an ArrayList. Each
 * element is one of String, ByteString, or byte[]. It caches the last one
 * requested which is most likely the one needed next. This minimizes memory
 * usage while satisfying the most common use cases.
 * <p>
 * <strong>Note that this implementation is not synchronized.</strong>
 * If multiple threads access an <tt>ArrayList</tt> instance concurrently,
 * and at least one of the threads modifies the list structurally, it
 * <i>must</i> be synchronized externally.  (A structural modification is
 * any operation that adds or deletes one or more elements, or explicitly
 * resizes the backing array; merely setting the value of an element is not
 * a structural modification.)  This is typically accomplished by
 * synchronizing on some object that naturally encapsulates the list.
 * <p>
 * If the implementation is accessed via concurrent reads, this is thread safe.
 * Conversions are done in a thread safe manner. It's possible that the
 * conversion may happen more than once if two threads attempt to access the
 * same element and the modifications were not visible to each other, but this
 * will not result in any corruption of the list or change in behavior other
 * than performance.
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class LazyStringArrayList extends AbstractProtobufList<String>
    implements LazyStringList, RandomAccess {

  private static final LazyStringArrayList EMPTY_LIST = new LazyStringArrayList();
  static {
    EMPTY_LIST.makeImmutable();
  }

  static LazyStringArrayList emptyList() {
    return EMPTY_LIST;
  }

  // For compatibility with older runtimes.
  public static final LazyStringList EMPTY = EMPTY_LIST;

  private final List<Object> list;

  public LazyStringArrayList() {
    this(DEFAULT_CAPACITY);
  }

  public LazyStringArrayList(int initialCapacity) {
    this(new ArrayList<Object>(initialCapacity));
  }

  public LazyStringArrayList(LazyStringList from) {
    list = new ArrayList<Object>(from.size());
    addAll(from);
  }

  public LazyStringArrayList(List<String> from) {
    this(new ArrayList<Object>(from));
  }

  private LazyStringArrayList(ArrayList<Object> list) {
    this.list = list;
  }

  @Override
  public LazyStringArrayList mutableCopyWithCapacity(int capacity) {
    if (capacity < size()) {
      throw new IllegalArgumentException();
    }
    ArrayList<Object> newList = new ArrayList<Object>(capacity);
    newList.addAll(list);
    return new LazyStringArrayList(newList);
  }

  @Override
  public String get(int index) {
    Object o = list.get(index);
    if (o instanceof String) {
      return (String) o;
    } else if (o instanceof ByteString) {
      ByteString bs = (ByteString) o;
      String s = bs.toStringUtf8();
      if (bs.isValidUtf8()) {
        list.set(index, s);
      }
      return s;
    } else {
      byte[] ba = (byte[]) o;
      String s = Internal.toStringUtf8(ba);
      if (Internal.isValidUtf8(ba)) {
        list.set(index, s);
      }
      return s;
    }
  }

  @Override
  public int size() {
    return list.size();
  }

  @Override
  public String set(int index, String s) {
    ensureIsMutable();
    Object o = list.set(index, s);
    return asString(o);
  }

  @Override
  public void add(int index, String element) {
    ensureIsMutable();
    list.add(index, element);
    modCount++;
  }

  private void add(int index, ByteString element) {
    ensureIsMutable();
    list.add(index, element);
    modCount++;
  }

  private void add(int index, byte[] element) {
    ensureIsMutable();
    list.add(index, element);
    modCount++;
  }

  @Override
  public boolean addAll(Collection<? extends String> c) {
    // The default implementation of AbstractCollection.addAll(Collection)
    // delegates to add(Object). This implementation instead delegates to
    // addAll(int, Collection), which makes a special case for Collections
    // which are instances of LazyStringList.
    return addAll(size(), c);
  }

  @Override
  public boolean addAll(int index, Collection<? extends String> c) {
    ensureIsMutable();
    // When copying from another LazyStringList, directly copy the underlying
    // elements rather than forcing each element to be decoded to a String.
    Collection<?> collection = c instanceof LazyStringList
        ? ((LazyStringList) c).getUnderlyingElements() : c;
    boolean ret = list.addAll(index, collection);
    modCount++;
    return ret;
  }

  @Override
  public boolean addAllByteString(Collection<? extends ByteString> values) {
    ensureIsMutable();
    boolean ret = list.addAll(values);
    modCount++;
    return ret;
  }

  @Override
  public boolean addAllByteArray(Collection<byte[]> c) {
    ensureIsMutable();
    boolean ret = list.addAll(c);
    modCount++;
    return ret;
  }

  @Override
  public String remove(int index) {
    ensureIsMutable();
    Object o = list.remove(index);
    modCount++;
    return asString(o);
  }

  @Override
  public void clear() {
    ensureIsMutable();
    list.clear();
    modCount++;
  }

  @Override
  public void add(ByteString element) {
    ensureIsMutable();
    list.add(element);
    modCount++;
  }

  @Override
  public void add(byte[] element) {
    ensureIsMutable();
    list.add(element);
    modCount++;
  }

  @Override
  public Object getRaw(int index) {
    return list.get(index);
  }

  @Override
  public ByteString getByteString(int index) {
    Object o = list.get(index);
    ByteString b = asByteString(o);
    if (b != o) {
      list.set(index, b);
    }
    return b;
  }

  @Override
  public byte[] getByteArray(int index) {
    Object o = list.get(index);
    byte[] b = asByteArray(o);
    if (b != o) {
      list.set(index, b);
    }
    return b;
  }

  @Override
  public void set(int index, ByteString s) {
    setAndReturn(index, s);
  }

  private Object setAndReturn(int index, ByteString s) {
    ensureIsMutable();
    return list.set(index, s);
  }

  @Override
  public void set(int index, byte[] s) {
    setAndReturn(index, s);
  }

  private Object setAndReturn(int index, byte[] s) {
    ensureIsMutable();
    return list.set(index, s);
  }

  private static String asString(Object o) {
    if (o instanceof String) {
      return (String) o;
    } else if (o instanceof ByteString) {
      return ((ByteString) o).toStringUtf8();
    } else {
      return Internal.toStringUtf8((byte[]) o);
    }
  }

  private static ByteString asByteString(Object o) {
    if (o instanceof ByteString) {
      return (ByteString) o;
    } else if (o instanceof String) {
      return ByteString.copyFromUtf8((String) o);
    } else {
      return ByteString.copyFrom((byte[]) o);
    }
  }

  private static byte[] asByteArray(Object o) {
    if (o instanceof byte[]) {
      return (byte[]) o;
    } else if (o instanceof String) {
      return Internal.toByteArray((String) o);
    } else {
      return ((ByteString) o).toByteArray();
    }
  }

  @Override
  public List<?> getUnderlyingElements() {
    return Collections.unmodifiableList(list);
  }

  @Override
  public void mergeFrom(LazyStringList other) {
    ensureIsMutable();
    for (Object o : other.getUnderlyingElements()) {
      if (o instanceof byte[]) {
        byte[] b = (byte[]) o;
        // Byte array's content is mutable so they should be copied rather than
        // shared when merging from one message to another.
        list.add(Arrays.copyOf(b, b.length));
      } else {
        list.add(o);
      }
    }
  }

  private static class ByteArrayListView extends AbstractList<byte[]>
      implements RandomAccess {
    private final LazyStringArrayList list;

    ByteArrayListView(LazyStringArrayList list) {
      this.list = list;
    }

    @Override
    public byte[] get(int index) {
      return list.getByteArray(index);
    }

    @Override
    public int size() {
      return list.size();
    }

    @Override
    public byte[] set(int index, byte[] s) {
      Object o = list.setAndReturn(index, s);
      modCount++;
      return asByteArray(o);
    }

    @Override
    public void add(int index, byte[] s) {
      list.add(index, s);
      modCount++;
    }

    @Override
    public byte[] remove(int index) {
      Object o = list.remove(index);
      modCount++;
      return asByteArray(o);
    }
  }

  @Override
  public List<byte[]> asByteArrayList() {
    return new ByteArrayListView(this);
  }

  private static class ByteStringListView extends AbstractList<ByteString>
      implements RandomAccess {
    private final LazyStringArrayList list;

    ByteStringListView(LazyStringArrayList list) {
      this.list = list;
    }

    @Override
    public ByteString get(int index) {
      return list.getByteString(index);
    }

    @Override
    public int size() {
      return list.size();
    }

    @Override
    public ByteString set(int index, ByteString s) {
      Object o = list.setAndReturn(index, s);
      modCount++;
      return asByteString(o);
    }

    @Override
    public void add(int index, ByteString s) {
      list.add(index, s);
      modCount++;
    }

    @Override
    public ByteString remove(int index) {
      Object o = list.remove(index);
      modCount++;
      return asByteString(o);
    }
  }

  @Override
  public List<ByteString> asByteStringList() {
    return new ByteStringListView(this);
  }

  @Override
  public LazyStringList getUnmodifiableView() {
    if (isModifiable()) {
      return new UnmodifiableLazyStringList(this);
    }
    return this;
  }

}
