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
    Value.Builder valueBuilder = Value.newBuilder();
    ListValue.Builder listValue = valueBuilder.getListValueBuilder();
    listValue.addAllValues(values);
    return valueBuilder.build();
  }

  private Values() {}
}
