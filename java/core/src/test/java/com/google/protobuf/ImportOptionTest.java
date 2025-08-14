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
    // Ensure that UnittestCustomOptions is linked in and referenced.
    FileDescriptor unused = UnittestCustomOptions.getDescriptor();

    FileDescriptor fileDescriptor = UnittestImportOptionProto.getDescriptor();
    Descriptor messageDescriptor = TestMessage.getDescriptor();
    FieldDescriptor fieldDescriptor = messageDescriptor.findFieldByName("field1");

    UnknownFieldSet unknownFieldsFile = fileDescriptor.getOptions().getUnknownFields();
    UnknownFieldSet unknownFieldsMessage = messageDescriptor.getOptions().getUnknownFields();
    UnknownFieldSet unknownFieldsField = fieldDescriptor.getOptions().getUnknownFields();

    // TODO: Currently linked in options also end up in unknown fields.
    // TODO: Exclude for open source tests once linked in options are treated
    // differently, since `option_deps` are treated as `deps` in Bazel 7.
    // assertThat(fileDescriptor.getOptions().getExtension(UnittestCustomOptions.fileOpt1))
    //     .isEqualTo(1);
    // assertThat(messageDescriptor.getOptions().getExtension(UnittestCustomOptions.messageOpt1))
    //     .isEqualTo(2);
    // assertThat(fieldDescriptor.getOptions().getExtension(UnittestCustomOptions.fieldOpt1))
    //     .isEqualTo(3);
    assertThat(unknownFieldsFile.getField(7736974).getVarintList()).containsExactly(1L);
    assertThat(unknownFieldsMessage.getField(7739036).getVarintList()).containsExactly(2L);
    assertThat(unknownFieldsField.getField(7740936).getFixed64List()).containsExactly(3L);

    // Options from import option that are not linked in should be in unknown fields.
    assertThat(unknownFieldsFile.getField(7736975).getVarintList()).containsExactly(1L);
    assertThat(unknownFieldsMessage.getField(7739037).getVarintList()).containsExactly(2L);
    assertThat(unknownFieldsField.getField(7740937).getFixed64List()).containsExactly(3L);

    assertThat(unknownFieldsFile.asMap()).hasSize(2);
    assertThat(unknownFieldsMessage.asMap()).hasSize(2);
    assertThat(unknownFieldsField.asMap()).hasSize(2);
  }
}
