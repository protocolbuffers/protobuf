// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import com.google.protobuf.ListValue;
import com.google.protobuf.NullValue;
import com.google.protobuf.Struct;
import com.google.protobuf.Value;

/** Utilities to help create {@code google.protobuf.Value} messages. */
public final class Values {

  private static final Value NULL_VALUE =
      Value.newBuilder().setNullValue(NullValue.NULL_VALUE).build();

  public static Value ofNull() {
    return NULL_VALUE;
  }

  /** Returns a Value object with number set to value. */
  public static Value of(boolean value) {
    return Value.newBuilder().setBoolValue(value).build();
  }

  /** Returns a Value object with number set to value. */
  public static Value of(double value) {
    return Value.newBuilder().setNumberValue(value).build();
  }

  /** Returns a Value object with string set to value. */
  public static Value of(String value) {
    return Value.newBuilder().setStringValue(value).build();
  }

  /** Returns a Value object with struct set to value. */
  public static Value of(Struct value) {
    return Value.newBuilder().setStructValue(value).build();
  }

  /** Returns a Value with ListValue set to value. */
  public static Value of(ListValue value) {
    return Value.newBuilder().setListValue(value).build();
  }

  /**
   * Returns a Value with ListValue set to the appending the result of calling {@link #of} on each
   * element in the iterable.
   */
  public static Value of(Iterable<Value> values) {
    return Value.newBuilder().setListValue(ListValue.newBuilder().addAllValues(values)).build();
  }

  private Values() {}
}
