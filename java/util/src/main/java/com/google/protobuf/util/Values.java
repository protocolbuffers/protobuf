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
import java.util.Map;

/** Utilities to help create {@code google.protobuf.Value} messages. */
public final class Values {

  private static final Value NULL_VALUE =
      Value.newBuilder().setNullValue(NullValue.NULL_VALUE).build();
  private static final Value TRUE_VALUE = Value.newBuilder().setBoolValue(true).build();
  private static final Value FALSE_VALUE = Value.newBuilder().setBoolValue(false).build();
  private static final Value EMPTY_STR_VALUE = Value.newBuilder().setStringValue("").build();

  public static Value ofNull() {
    return NULL_VALUE;
  }

  /** Returns a {@link Value} object with number set to value. */
  public static Value of(boolean value) {
    return value ? TRUE_VALUE : FALSE_VALUE;
  }

  /** Returns a {@link Value} object with number set to value. */
  public static Value of(double value) {
    return Value.newBuilder().setNumberValue(value).build();
  }

  /** Returns a {@link Value} object with string set to value. */
  public static Value of(String value) {
    return value.isEmpty() ? EMPTY_STR_VALUE : Value.newBuilder().setStringValue(value).build();
  }

  /** Returns a {@link Value} object with struct set to value. */
  public static Value of(Struct value) {
    return Value.newBuilder().setStructValue(value).build();
  }

  /** Returns a {@link Value} with a {@link ListValue} set to value. */
  public static Value of(ListValue value) {
    return Value.newBuilder().setListValue(value).build();
  }

  /**
   * Returns a {@link Value} with a {@link ListValue} whose values are set to the result of calling
   * {@link #of} on each element in the iterable.
   */
  public static Value of(Iterable<Value> values) {
    return Value.newBuilder().setListValue(ListValue.newBuilder().addAllValues(values)).build();
  }

  /**
   * Returns a {@link Value} with a {@link Struct} whose fields are set to the result of calling
   * {@link #of} on each value in the map, preserving keys.
   */
  public static Value of(Map<String, Value> values) {
    return Value.newBuilder().setStructValue(Struct.newBuilder().putAllFields(values)).build();
  }

  private Values() {}
}
