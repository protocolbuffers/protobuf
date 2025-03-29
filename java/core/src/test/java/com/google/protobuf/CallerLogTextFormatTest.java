package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import proto2_unittest.UnittestProto.TestAllTypes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class CallerLogTextFormatTest {

  @Test
  public void toStringParser_showsCallerImplicit() throws Exception {
    TestAllTypes proto = TestAllTypes.newBuilder().setOptionalInt32(123).build();
    String result = String.format("%s", proto);
    assertThat(result).contains("orrery trace");
    TextFormat.ParseException e =
        assertThrows(
            TextFormat.ParseException.class, () -> TextFormat.parse(result, TestAllTypes.class));
    assertThat(e).hasMessageThat().contains("toString Deserialization# orrery trace@");
  }

  @Test
  public void toStringParser_showsCallerMultiline() throws Exception {
    TestAllTypes proto =
        TestAllTypes.newBuilder().setOptionalInt32(123).setOptionalString("hello world").build();
    String result = proto.toString();
    assertThat(result).contains("orrery traceeee");
    // TextFormat.ParseException e =
    //     assertThrows(
    //         TextFormat.ParseException.class, () -> TextFormat.parse(result, TestAllTypes.class));
    // assertThat(e).hasMessageThat().contains("toString Deserialization# orrery trace@");
    // assertThat(e).hasMessageThat().contains("com.google.protobuf.TestAllTypes@");
  }

  @Test
  public void toStringParser_showsCallerNestedToString() throws Exception {
    // In the future, we may want to mark when message.toString() is called as a nested call to
    // toString.
    class NestedDeserialization {
      TestAllTypes proto;

      NestedDeserialization() {
        this.proto =
            TestAllTypes.newBuilder()
                .setOptionalInt32(123)
                .setOptionalString("hello world")
                .build();
      }

      @Override
      public String toString() {
        return proto.toString();
      }
    }
    String result = new NestedDeserialization().toString();
    System.out.println("result: " + result);
    assertThat(result).contains("orrery trace");
    TextFormat.ParseException e =
        assertThrows(
            TextFormat.ParseException.class, () -> TextFormat.parse(result, TestAllTypes.class));
    assertThat(e).hasMessageThat().contains("toString Deserialization# orrery trace@");
  }

  @Test
  public void toStringParser_showsCallerNestedToStringImplicit() throws Exception {
    // In the future, we may want to mark when message.toString() is called as a nested call to
    // toString.
    class NestedDeserialization {
      TestAllTypes proto;

      NestedDeserialization() {
        this.proto =
            TestAllTypes.newBuilder()
                .setOptionalInt32(123)
                .setOptionalString("hello world")
                .build();
      }

      @Override
      public String toString() {
        return String.format("%s", proto);
      }
    }
    String result = new NestedDeserialization().toString();
    System.out.println("result: " + result);
    assertThat(result).contains("orrery trace");
    TextFormat.ParseException e =
        assertThrows(
            TextFormat.ParseException.class, () -> TextFormat.parse(result, TestAllTypes.class));
    assertThat(e).hasMessageThat().contains("toString Deserialization# orrery trace@");
  }
}
