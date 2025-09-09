// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.Internal.ProtobufList;
import java.util.List;

/**
 * Utility class that aids in properly manipulating list fields for either the lite or full runtime.
 */
final class ListFieldSchemaLite implements ListFieldSchema {

  @Override
  public <L> List<L> mutableListAt(Object message, long offset) {
    ProtobufList<L> list = getProtobufList(message, offset);
    if (!list.isModifiable()) {
      int size = list.size();
      list =
          list.mutableCopyWithCapacity(
              size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
      UnsafeUtil.putObject(message, offset, list);
    }
    return list;
  }

  @Override
  public void makeImmutableListAt(Object message, long offset) {
    ProtobufList<?> list = getProtobufList(message, offset);
    list.makeImmutable();
  }

  @Override
  public <E> void mergeListsAt(Object msg, Object otherMsg, long offset) {
    ProtobufList<E> mine = getProtobufList(msg, offset);
    ProtobufList<E> other = getProtobufList(otherMsg, offset);

    int size = mine.size();
    int otherSize = other.size();
    if (size > 0 && otherSize > 0) {
      if (!mine.isModifiable()) {
        mine = mine.mutableCopyWithCapacity(size + otherSize);
      }
      mine.addAll(other);
    }

    ProtobufList<E> merged = size > 0 ? mine : other;
    UnsafeUtil.putObject(msg, offset, merged);
  }

  @SuppressWarnings("unchecked")
  static <E> ProtobufList<E> getProtobufList(Object message, long offset) {
    return (ProtobufList<E>) UnsafeUtil.getObject(message, offset);
  }
}
