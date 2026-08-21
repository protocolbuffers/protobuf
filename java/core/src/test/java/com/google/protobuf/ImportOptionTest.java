package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import proto2_unittest.UnittestCustomOptions;
import proto2_unittest_import_option.TestMessage;
import proto2_unittest_import_option.UnittestImportOptionProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for import option. */
@RunWith(JUnit4.class)
public final class ImportOptionTest {

  @Test
  public void testImportOption() throws Exception {
    FileDescriptor fileDescriptor = UnittestImportOptionProto.getDescriptor();
    Descriptor messageDescriptor = TestMessage.getDescriptor();
    FieldDescriptor fieldDescriptor = messageDescriptor.findFieldByName("field1");

    UnknownFieldSet unknownFieldsFile = fileDescriptor.getOptions().getUnknownFields();
    UnknownFieldSet unknownFieldsMessage = messageDescriptor.getOptions().getUnknownFields();
    UnknownFieldSet unknownFieldsField = fieldDescriptor.getOptions().getUnknownFields();

    // Using the extension ensures UnittestCustomOptions is linked in and referenced
    assertThat(fileDescriptor.getOptions().getExtension(UnittestCustomOptions.fileOpt1))
        .isEqualTo(1);
    assertThat(messageDescriptor.getOptions().getExtension(UnittestCustomOptions.messageOpt1))
        .isEqualTo(2);
    assertThat(fieldDescriptor.getOptions().getExtension(UnittestCustomOptions.fieldOpt1))
        .isEqualTo(3);

    // Options from import option that are not linked in should be in unknown fields.
    assertThat(unknownFieldsFile.getField(7736975).getVarintList()).containsExactly(1L);
    assertThat(unknownFieldsMessage.getField(7739037).getVarintList()).containsExactly(2L);
    assertThat(unknownFieldsField.getField(7740937).getFixed64List()).containsExactly(3L);

    assertThat(unknownFieldsFile.asMap()).hasSize(1);
    assertThat(unknownFieldsMessage.asMap()).hasSize(1);
    assertThat(unknownFieldsField.asMap()).hasSize(1);
  }
}
