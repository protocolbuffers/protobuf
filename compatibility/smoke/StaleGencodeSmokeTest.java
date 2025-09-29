// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package smoke;

import static com.google.common.truth.Truth.assertThat;

import legacy_gencode_test.proto3.Proto3GencodeTestProto.TestMessage;

import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.util.JsonFormat;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test that touches a few behaviors on old versions of generated code. */
@RunWith(JUnit4.class)
public class StaleGencodeSmokeTest {
  @Test
  public void testBuilder() {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    assertThat(b.getX()).isEqualTo("hello");
    assertThat(b.getY().getZCount()).isEqualTo(0);
    b.getYBuilder().addZ(4);
    assertThat(b.getY().getZCount()).isEqualTo(1);
    assertThat(b.getY().getZList().get(0)).isEqualTo(4);

    TestMessage msg = b.build();
    assertThat(msg.getX()).isEqualTo("hello");
    assertThat(msg.getY().getZList().get(0)).isEqualTo(4);
  }

  @Test
  public void testReflection() {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();

    Map<FieldDescriptor, Object> fields = msg.getAllFields();
    assertThat(fields.size()).isEqualTo(1);
    assertThat(fields.values().contains("hello")).isTrue();
  }

  @Test
  public void testSerializeParse() throws Exception {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();
    TestMessage roundTrip = TestMessage.parseFrom(msg.toByteArray());
    assertThat(roundTrip).isEqualTo(msg);
  }

  @Test
  public void testSerializeParseJson() throws Exception {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();
    String json = JsonFormat.printer().print(msg);

    TestMessage.Builder roundTrip = TestMessage.newBuilder();
    JsonFormat.parser().merge(json, roundTrip);
    assertThat(roundTrip.build()).isEqualTo(msg);
  }
}
