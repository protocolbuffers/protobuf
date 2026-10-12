// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static proto2_unittest.UnittestProto.optionalInt32Extension;

import map_test.MapTestProto.TestMap;
import proto2_unittest.UnittestProto.TestAllExtensions;
import proto2_unittest.UnittestProto.TestAllTypes;
import proto2_unittest.UnittestProto.TestAllTypes.NestedMessage;
import proto2_unittest.UnittestProto.TestEmptyMessage;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class SilentMarkerPrintingTest {

  @Test
  public void testPrintsSilentMarkerNonMessage() throws Exception {
    TestAllTypes proto = TestAllTypes.newBuilder().addRepeatedInt32(1).build();
    assertThat(proto.toString()).isEqualTo("repeated_int32: \t 1\n");
  }

  @Test
  public void testPrintsSilentMarkerMessageField() throws Exception {
    TestAllTypes proto =
        TestAllTypes.newBuilder()
            .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(1))
            .addRepeatedInt32(1)
            .build();
    assertThat(proto.toString())
        .isEqualTo("optional_nested_message \t {\n  bb: 1\n}\nrepeated_int32: 1\n");
  }

  @Test
  public void testPrintsSilentMarkerUnknownField() throws Exception {
    UnknownFieldSet unknownFields =
        UnknownFieldSet.newBuilder()
            .addField(5, UnknownFieldSet.Field.newBuilder().addVarint(1).build())
            .build();
    TestEmptyMessage proto = TestEmptyMessage.newBuilder().setUnknownFields(unknownFields).build();
    assertThat(proto.toString()).isEqualTo("5: \t 1\n");
  }

  @Test
  public void testPrintsSilentMarkerUnknownFieldGroup() throws Exception {
    UnknownFieldSet unknownFields =
        UnknownFieldSet.newBuilder()
            .addField(
                5,
                UnknownFieldSet.Field.newBuilder()
                    .addGroup(
                        UnknownFieldSet.newBuilder()
                            .addField(10, UnknownFieldSet.Field.newBuilder().addVarint(5).build())
                            .build())
                    .build())
            .build();
    TestEmptyMessage proto = TestEmptyMessage.newBuilder().setUnknownFields(unknownFields).build();
    assertThat(proto.toString()).isEqualTo("5 \t {\n  10: 5\n}\n");
  }

  @Test
  public void testPrintsSilentMarkerAnyField() throws Exception {
    Any any =
        Any.newBuilder()
            .setTypeUrl("type.googleapis.com/" + TestAllTypes.getDescriptor().getFullName())
            .setValue(TestAllTypes.newBuilder().setOptionalInt32(1).build().toByteString())
            .build();
    assertThat(any.toString())
        .isEqualTo(
            "type_url: \t \"type.googleapis.com/proto2_unittest.TestAllTypes\"\n"
                + "value: \"\\b\\001\"\n");
  }

  @Test
  public void testPrintsSilentMarkerExtensionField() throws Exception {
    TestAllExtensions proto =
        TestAllExtensions.newBuilder().setExtension(optionalInt32Extension, 1).build();
    assertThat(proto.toString()).isEqualTo("[proto2_unittest.optional_int32_extension]: \t 1\n");
  }

  @Test
  public void testPrintsSilentMarkerMapField() throws Exception {
    TestMap proto = TestMap.newBuilder().putInt32ToInt32Field(1, 2).build();
    assertThat(proto.toString()).isEqualTo("int32_to_int32_field \t {\n  key: 1\n  value: 2\n}\n");
  }

  @Test
  public void testPrintsSilentMarkerFromBuilder() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder().addRepeatedInt32(1);
    assertThat(builder.toString()).isEqualTo("repeated_int32: \t 1\n");
  }

  @Test
  public void testPrintsSilentMarkerPrintToStringShortDebugStringUnchanged() throws Exception {
    TestAllTypes proto = TestAllTypes.newBuilder().setOptionalInt32(1).build();
    assertThat(TextFormat.printer().printToString(proto)).isEqualTo("optional_int32: 1\n");
    assertThat(TextFormat.printer().shortDebugString(proto)).isEqualTo("optional_int32: 1");
  }
}
