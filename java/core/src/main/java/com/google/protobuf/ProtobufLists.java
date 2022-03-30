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

import com.google.protobuf.Internal.BooleanList;
import com.google.protobuf.Internal.DoubleList;
import com.google.protobuf.Internal.FloatList;
import com.google.protobuf.Internal.IntList;
import com.google.protobuf.Internal.LongList;
import com.google.protobuf.Internal.ProtobufList;

/** Utility class for construction of lists that extend {@link ProtobufList}. */
@ExperimentalApi
@CheckReturnValue
final class ProtobufLists {
  private ProtobufLists() {}

  public static <E> ProtobufList<E> emptyProtobufList() {
    return ProtobufArrayList.emptyList();
  }

  public static <E> ProtobufList<E> mutableCopy(ProtobufList<E> list) {
    int size = list.size();
    return list.mutableCopyWithCapacity(
        size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
  }

  public static BooleanList emptyBooleanList() {
    return BooleanArrayList.emptyList();
  }

  public static BooleanList newBooleanList() {
    return new BooleanArrayList();
  }

  public static IntList emptyIntList() {
    return IntArrayList.emptyList();
  }

  public static IntList newIntList() {
    return new IntArrayList();
  }

  public static LongList emptyLongList() {
    return LongArrayList.emptyList();
  }

  public static LongList newLongList() {
    return new LongArrayList();
  }

  public static FloatList emptyFloatList() {
    return FloatArrayList.emptyList();
  }

  public static FloatList newFloatList() {
    return new FloatArrayList();
  }

  public static DoubleList emptyDoubleList() {
    return DoubleArrayList.emptyList();
  }

  public static DoubleList newDoubleList() {
    return new DoubleArrayList();
  }
}
