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

  // TODO: Exclude for open source tests since `option_deps` are
  // linked-in as `deps` prior to option_deps support in Bazel 7 proto_library.
}
