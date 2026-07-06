package com.google.protobuf.utf8validation;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.DynamicMessage;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.InvalidProtocolBufferException;
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class Utf8ValidationDynamicMessageTest {
  enum Validates {
    NEVER,
    ALWAYS
  }

  private final Case testCase;

  public Utf8ValidationDynamicMessageTest(Case testCase) {
    this.testCase = testCase;
  }

  @Parameters(name = "{0}")
  public static List<Object[]> data() {
    List<Object[]> p = new ArrayList<>();
    Descriptor d;

    d = Utf8TestProto2.getDescriptor();
    Case.create(p, d, "file_unset_field_unset", Validates.NEVER);
    Case.create(p, d, "file_unset_field_enforced", Validates.NEVER);
    Case.create(p, d, "ext_file_unset_field_unset", Validates.NEVER);
    Case.create(p, d, "ext_file_unset_field_enforced", Validates.NEVER);

    d = Utf8TestProto2Unchecked.getDescriptor();
    Case.create(p, d, "file_unchecked_field_unset", Validates.NEVER);
    Case.create(p, d, "file_unchecked_field_enforced", Validates.NEVER);
    Case.create(p, d, "ext_file_unchecked_field_unset", Validates.NEVER);
    Case.create(p, d, "ext_file_unchecked_field_enforced", Validates.NEVER);

    d = Utf8TestProto2Checked.getDescriptor();
    Case.create(p, d, "file_checked_field_unset", Validates.ALWAYS);
    Case.create(p, d, "file_checked_field_enforced", Validates.ALWAYS);
    Case.create(p, d, "ext_file_checked_field_unset", Validates.ALWAYS);
    Case.create(p, d, "ext_file_checked_field_enforced", Validates.ALWAYS);

    d = Utf8TestProto3.getDescriptor();
    Case.create(p, d, "file_unset_field_unset", Validates.ALWAYS);
    Case.create(p, d, "file_unset_field_enforced", Validates.ALWAYS);

    d = Utf8TestProto3Unchecked.getDescriptor();
    Case.create(p, d, "file_unchecked_field_unset", Validates.ALWAYS);
    Case.create(p, d, "file_unchecked_field_enforced", Validates.ALWAYS);

    d = Utf8TestProto3Checked.getDescriptor();
    Case.create(p, d, "file_checked_field_unset", Validates.ALWAYS);
    Case.create(p, d, "file_checked_field_enforced", Validates.ALWAYS);

    d = Utf8TestEdition2023.getDescriptor();
    Case.create(p, d, "global_unset_java_unset", Validates.ALWAYS);
    Case.create(p, d, "global_unset_java_default", Validates.ALWAYS);
    Case.create(p, d, "global_unset_java_verify", Validates.ALWAYS);
    Case.create(p, d, "global_verify_java_unset", Validates.ALWAYS);
    Case.create(p, d, "global_verify_java_default", Validates.ALWAYS);
    Case.create(p, d, "global_verify_java_verify", Validates.ALWAYS);
    Case.create(p, d, "global_none_java_unset", Validates.NEVER);
    Case.create(p, d, "global_none_java_default", Validates.NEVER);
    Case.create(p, d, "global_none_java_verify", Validates.ALWAYS);
    Case.create(p, d, "ext_global_unset_java_unset", Validates.ALWAYS);
    Case.create(p, d, "ext_global_unset_java_default", Validates.ALWAYS);
    Case.create(p, d, "ext_global_unset_java_verify", Validates.ALWAYS);
    Case.create(p, d, "ext_global_verify_java_unset", Validates.ALWAYS);
    Case.create(p, d, "ext_global_verify_java_default", Validates.ALWAYS);
    Case.create(p, d, "ext_global_verify_java_verify", Validates.ALWAYS);
    Case.create(p, d, "ext_global_none_java_unset", Validates.NEVER);
    Case.create(p, d, "ext_global_none_java_default", Validates.NEVER);
    Case.create(p, d, "ext_global_none_java_verify", Validates.ALWAYS);

    d = Utf8TestEdition2024.getDescriptor();
    Case.create(p, d, "global_unset_java_unset", Validates.ALWAYS);
    Case.create(p, d, "global_unset_java_default", Validates.ALWAYS);
    Case.create(p, d, "global_unset_java_verify", Validates.ALWAYS);
    Case.create(p, d, "global_verify_java_unset", Validates.ALWAYS);
    Case.create(p, d, "global_verify_java_default", Validates.ALWAYS);
    Case.create(p, d, "global_verify_java_verify", Validates.ALWAYS);
    Case.create(p, d, "global_none_java_unset", Validates.NEVER);
    Case.create(p, d, "global_none_java_default", Validates.NEVER);
    Case.create(p, d, "global_none_java_verify", Validates.ALWAYS);
    Case.create(p, d, "ext_global_unset_java_unset", Validates.ALWAYS);
    Case.create(p, d, "ext_global_unset_java_default", Validates.ALWAYS);
    Case.create(p, d, "ext_global_unset_java_verify", Validates.ALWAYS);
    Case.create(p, d, "ext_global_verify_java_unset", Validates.ALWAYS);
    Case.create(p, d, "ext_global_verify_java_default", Validates.ALWAYS);
    Case.create(p, d, "ext_global_verify_java_verify", Validates.ALWAYS);
    Case.create(p, d, "ext_global_none_java_unset", Validates.NEVER);
    Case.create(p, d, "ext_global_none_java_default", Validates.NEVER);
    Case.create(p, d, "ext_global_none_java_verify", Validates.ALWAYS);

    return p;
  }

  @Test
  public void test() throws Exception {
    FieldDescriptor field = testCase.getFieldDescriptor();

    byte[] serializedPayload = buildSerialized(field.getNumber());

    ExtensionRegistry registry = ExtensionRegistry.newInstance();
    if (field.isExtension()) {
      registry.add(field);
    }

    if (testCase.expectedToValidate()) {
      assertThrows(
          InvalidProtocolBufferException.class,
          () -> DynamicMessage.parseFrom(testCase.descriptor, serializedPayload, registry));
    } else {
      DynamicMessage msg =
          DynamicMessage.parseFrom(testCase.descriptor, serializedPayload, registry);
      assertTrue(msg.hasField(field));
      assertEquals("\uFFFD\uFFFD", msg.getField(field));
    }
  }

  private static byte[] buildSerialized(int fieldNumber) throws Exception {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    CodedOutputStream codedOut = CodedOutputStream.newInstance(out);
    codedOut.writeByteArray(fieldNumber, new byte[] {(byte) 0xC0, (byte) 0x80});
    codedOut.flush();
    return out.toByteArray();
  }

  private static class Case {
    final Descriptor descriptor;
    final String fieldName;
    final Validates expected;

    static void create(
        List<Object[]> params, Descriptor descriptor, String fieldName, Validates expected) {
      params.add(new Object[] {new Case(descriptor, fieldName, expected)});
    }

    private Case(Descriptor descriptor, String fieldName, Validates expected) {
      this.descriptor = descriptor;
      this.fieldName = fieldName;
      this.expected = expected;
    }

    @Override
    public String toString() {
      return String.format(
          "%s#%s-->%s",
          descriptor.getName(),
          fieldName,
          expected == Validates.ALWAYS ? "validates" : "doesNotValidate");
    }

    FieldDescriptor getFieldDescriptor() {
      FieldDescriptor fieldDescriptor = descriptor.findFieldByName(fieldName);
      if (fieldDescriptor == null) {
        fieldDescriptor = descriptor.getFile().findExtensionByName(fieldName);
      }
      return fieldDescriptor;
    }

    boolean expectedToValidate() {
      return expected == Validates.ALWAYS;
    }
  }
}
