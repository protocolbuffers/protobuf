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
import com.google.protobuf.TextFormat;
import com.google.protobuf.util.JsonFormat;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Smoke test for old versions of full (non-lite) generated code.
 *
 * <p>Inherits all of the tests from StaleGencodeSmokeTest, and additionally covers functionality that
 * is only available on full generated code.
 */
@RunWith(JUnit4.class)
public class StaleGencodeFullSmokeTest extends StaleGencodeSmokeTest {
  @Test
  public void testNestedBuilder() {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.getYBuilder().addZ(4);
    assertThat(b.getY().getZCount()).isEqualTo(1);
    assertThat(b.build().getY().getZList().get(0)).isEqualTo(4);
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
  public void testSerializeParseJson() throws Exception {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();
    String json = JsonFormat.printer().print(msg);

    TestMessage.Builder roundTrip = TestMessage.newBuilder();
    JsonFormat.parser().merge(json, roundTrip);
    assertThat(roundTrip.build()).isEqualTo(msg);
  }

  @Test
  public void testSerializeParseText() throws Exception {
    TestMessage.Builder b = TestMessage.newBuilder();
    b.setX("hello");
    TestMessage msg = b.build();
    String text = TextFormat.printer().printToString(msg);

    TestMessage.Builder roundTrip = TestMessage.newBuilder();
    TextFormat.getParser().merge(text, roundTrip);
    assertThat(roundTrip.build()).isEqualTo(msg);
  }
}
