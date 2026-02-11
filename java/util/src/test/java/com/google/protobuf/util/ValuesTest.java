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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class ValuesTest {
  @Test
  public void testOfNull_isNullValue() throws Exception {
    assertThat(Values.ofNull())
        .isEqualTo(Value.newBuilder().setNullValue(NullValue.NULL_VALUE).build());
  }

  @Test
  public void testOfBoolean_constructsValue() {
    assertThat(Values.of(true)).isEqualTo(Value.newBuilder().setBoolValue(true).build());
    assertThat(Values.of(false)).isEqualTo(Value.newBuilder().setBoolValue(false).build());
  }

  @Test
  public void testOfNumeric_constructsValue() {
    assertThat(Values.of(100)).isEqualTo(Value.newBuilder().setNumberValue(100).build());
    assertThat(Values.of(1000L)).isEqualTo(Value.newBuilder().setNumberValue(1000).build());
    assertThat(Values.of(1000.23f)).isEqualTo(Value.newBuilder().setNumberValue(1000.23f).build());
    assertThat(Values.of(10000.23)).isEqualTo(Value.newBuilder().setNumberValue(10000.23).build());
  }

  @Test
  public void testOfString_constructsValue() {
    assertThat(Values.of("")).isEqualTo(Value.newBuilder().setStringValue("").build());
    assertThat(Values.of("foo")).isEqualTo(Value.newBuilder().setStringValue("foo").build());
  }

  @Test
  public void testOfStruct_constructsValue() {
    Struct.Builder builder = Struct.newBuilder();
    builder.putFields("a", Values.of("a"));
    builder.putFields("b", Values.of("b"));

    assertThat(Values.of(builder.build()))
        .isEqualTo(Value.newBuilder().setStructValue(builder.build()).build());
  }

  @Test
  public void testOfListValue_constructsInstance() {
    ListValue.Builder builder = ListValue.newBuilder();
    builder.addValues(Values.of(1));
    builder.addValues(Values.of(2));

    assertThat(Values.of(builder.build()))
        .isEqualTo(Value.newBuilder().setListValue(builder.build()).build());
  }

  @Test
  public void testOfIterable_returnsTheValue() {
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

  @Test
  public void testOfMap_returnsTheValue() {
    Struct.Builder builder = Struct.newBuilder();
    builder.putFields("a", Values.of(1));
    builder.putFields("b", Values.of(2));
    builder.putFields("c", Values.of(true));
    builder.putFields("d", Value.newBuilder().setStructValue(builder).build());

    Map<String, Value> map = new HashMap<>();
    map.put("a", Values.of(1));
    map.put("b", Values.of(2));
    map.put("c", Values.of(true));
    Map<String, Value> copyMap = new HashMap<>(map);
    map.put("d", Values.of(copyMap));

    assertThat(Values.of(map)).isEqualTo(Value.newBuilder().setStructValue(builder).build());
    assertThat(Values.of(new HashMap<String, Value>()))
        .isEqualTo(Value.newBuilder().setStructValue(Struct.getDefaultInstance()).build());
  }
}
