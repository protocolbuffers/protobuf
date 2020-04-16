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

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.ListValue;
import com.google.protobuf.NullValue;
import com.google.protobuf.Struct;
import com.google.protobuf.Value;
import java.util.ArrayList;
import java.util.List;
import junit.framework.TestCase;

public final class ValuesTest extends TestCase {
  public void testOfNull_IsNullValue() throws Exception {
    assertThat(Values.ofNull())
        .isEqualTo(Value.newBuilder().setNullValue(NullValue.NULL_VALUE).build());
  }

  public void testOfBoolean_ConstructsValue() {
    assertThat(Values.of(true)).isEqualTo(Value.newBuilder().setBoolValue(true).build());
    assertThat(Values.of(false)).isEqualTo(Value.newBuilder().setBoolValue(false).build());
  }

  public void testOfNumeric_ConstructsValue() {
    assertThat(Values.of(100)).isEqualTo(Value.newBuilder().setNumberValue(100).build());
    assertThat(Values.of(1000L)).isEqualTo(Value.newBuilder().setNumberValue(1000).build());
    assertThat(Values.of(1000.23f)).isEqualTo(Value.newBuilder().setNumberValue(1000.23f).build());
    assertThat(Values.of(10000.23)).isEqualTo(Value.newBuilder().setNumberValue(10000.23).build());
  }

  public void testOfString_ConstructsValue() {
    assertThat(Values.of("")).isEqualTo(Value.newBuilder().setStringValue("").build());
    assertThat(Values.of("foo")).isEqualTo(Value.newBuilder().setStringValue("foo").build());
  }

  public void testOfStruct_ConstructsValue() {
    Struct.Builder builder = Struct.newBuilder();
    builder.putFields("a", Values.of("a"));
    builder.putFields("b", Values.of("b"));

    assertThat(Values.of(builder.build()))
        .isEqualTo(Value.newBuilder().setStructValue(builder.build()).build());
  }

  public void testOfListValue_ConstructsInstance() {
    ListValue.Builder builder = ListValue.newBuilder();
    builder.addValues(Values.of(1));
    builder.addValues(Values.of(2));

    assertThat(Values.of(builder.build()))
        .isEqualTo(Value.newBuilder().setListValue(builder.build()).build());
  }

  public void testOfIterable_ReturnsTheValue() {
    ListValue.Builder builder = ListValue.newBuilder();
    builder.addValues(Values.of(1));
    builder.addValues(Values.of(2));
    builder.addValues(Values.of(true));
    builder.addValues(Value.newBuilder().setListValue(builder.build()).build());

    List<Value> list = new ArrayList<>();
    list.add(Values.of(1));
    list.add(Values.of(2));
    list.add(Values.of(true));
    List<Value> copyList = new ArrayList<>(list);
    list.add(Values.of(copyList));

    assertThat(Values.of(list)).isEqualTo(Value.newBuilder().setListValue(builder).build());
    assertThat(Values.of(new ArrayList<Value>()))
        .isEqualTo(Value.newBuilder().setListValue(ListValue.getDefaultInstance()).build());
  }
}
