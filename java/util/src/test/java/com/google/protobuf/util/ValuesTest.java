// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.ListValue;
import com.google.protobuf.NullValue;
import com.google.protobuf.Struct;
import com.google.protobuf.Value;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class ValuesTest {
  @Test
  public void testOfNull_IsNullValue() throws Exception {
    assertThat(Values.ofNull())
        .isEqualTo(Value.newBuilder().setNullValue(NullValue.NULL_VALUE).build());
  }

  @Test
  public void testOfBoolean_ConstructsValue() {
    assertThat(Values.of(true)).isEqualTo(Value.newBuilder().setBoolValue(true).build());
    assertThat(Values.of(false)).isEqualTo(Value.newBuilder().setBoolValue(false).build());
  }

  @Test
  public void testOfNumeric_ConstructsValue() {
    assertThat(Values.of(100)).isEqualTo(Value.newBuilder().setNumberValue(100).build());
    assertThat(Values.of(1000L)).isEqualTo(Value.newBuilder().setNumberValue(1000).build());
    assertThat(Values.of(1000.23f)).isEqualTo(Value.newBuilder().setNumberValue(1000.23f).build());
    assertThat(Values.of(10000.23)).isEqualTo(Value.newBuilder().setNumberValue(10000.23).build());
  }

  @Test
  public void testOfString_ConstructsValue() {
    assertThat(Values.of("")).isEqualTo(Value.newBuilder().setStringValue("").build());
    assertThat(Values.of("foo")).isEqualTo(Value.newBuilder().setStringValue("foo").build());
  }

  @Test
  public void testOfStruct_ConstructsValue() {
    Struct.Builder builder = Struct.newBuilder();
    builder.putFields("a", Values.of("a"));
    builder.putFields("b", Values.of("b"));

    assertThat(Values.of(builder.build()))
        .isEqualTo(Value.newBuilder().setStructValue(builder.build()).build());
  }

  @Test
  public void testOfListValue_ConstructsInstance() {
    ListValue.Builder builder = ListValue.newBuilder();
    builder.addValues(Values.of(1));
    builder.addValues(Values.of(2));

    assertThat(Values.of(builder.build()))
        .isEqualTo(Value.newBuilder().setListValue(builder.build()).build());
  }

  @Test
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
