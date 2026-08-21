package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import proto2_unittest.UnittestProto;
import proto2_unittest.UnittestProto.TestAllExtensions;
import java.io.ByteArrayOutputStream;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit test for GeneratedMessage lazy equals behavior. This is only in a separate file from
 * GeneratedMessageTest so it can be excluded from the v25 compat test.
 */
@RunWith(JUnit4.class)
public class GeneratedMessageLazyEqualsTest {

  @Test
  public void testLazyExtensionEqualsAvoidsResolution() throws Exception {
    ExtensionRegistryLite.LazyExtensionMode originalMode =
        ExtensionRegistryLite.getLazyExtensionMode();
    ExtensionRegistryLite.setLazyExtensionMode(
        ExtensionRegistryLite.LazyExtensionMode.LAZY_VERIFY_ON_ACCESS);
    try {
      // Create bytes for TestAllExtensions with optionalLazyMessageExtension (field 27)
      // set to invalid bytes (0xFF).
      ByteArrayOutputStream baos = new ByteArrayOutputStream();
      CodedOutputStream cos = CodedOutputStream.newInstance(baos);
      cos.writeTag(27, WireFormat.WIRETYPE_LENGTH_DELIMITED);
      cos.writeUInt32NoTag(1); // Length of NestedMessage
      cos.writeRawByte((byte) 0xFF); // Invalid payload
      cos.flush();
      byte[] bytes = baos.toByteArray();

      ExtensionRegistry registry = ExtensionRegistry.newInstance();
      registry.add(UnittestProto.optionalLazyMessageExtension);

      // Parsing should succeed because the extension is lazy and not eagerly parsed.
      TestAllExtensions message = TestAllExtensions.parseFrom(bytes, registry);

      // Comparing to default instance should return false and NOT throw an exception
      // because it shouldn't need to resolve the lazy extension (since it's not present in
      // default).
      assertThat(message.equals(TestAllExtensions.getDefaultInstance())).isFalse();
    } finally {
      ExtensionRegistryLite.setLazyExtensionMode(originalMode);
    }
  }
}
