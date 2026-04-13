package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import proto2_unittest.UnittestProto.TestAllExtensions;
import proto2_unittest.UnittestProto.TestAllTypes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class InternalLazyFieldTest {

  private static final ExtensionRegistryLite EXTENSION_REGISTRY = TestUtil.getExtensionRegistry();

  @Test
  public void testGetValue() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.getValue()).isEqualTo(message);
  }

  @Test
  public void testGetValueContainingExtensions() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.getValue()).isEqualTo(message);
  }

  @Test
  public void testGetValueMemoized() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    MessageLite lazyFieldMessage1 = lazyField.getValue();
    MessageLite lazyFieldMessage2 = lazyField.getValue();

    assertThat(lazyFieldMessage1).isEqualTo(message);
    assertThat(lazyFieldMessage1).isSameInstanceAs(lazyFieldMessage2);
  }

  @Test
  public void testGetValueOnInvalidData_defaultInstance() {
    InternalLazyField lazyField =
        new InternalLazyField(
            TestAllTypes.getDefaultInstance(),
            EXTENSION_REGISTRY,
            ByteString.copyFromUtf8("invalid"));

    assertThat(lazyField.getValue()).isEqualTo(TestAllTypes.getDefaultInstance());
  }

  @Test
  public void testGetSerializedSize() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(message.getSerializedSize()).isEqualTo(lazyField.getSerializedSize());
  }

  @Test
  public void testGetSerializedSizeContainingExtensions() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.getSerializedSize()).isEqualTo(message.getSerializedSize());
  }

  @Test
  public void testGetByteString() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.toByteString()).isEqualTo(message.toByteString());
  }

  @Test
  public void testGetByteStringContainingExtensions() {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);

    assertThat(lazyField.toByteString()).isEqualTo(message.toByteString());
  }

  @Test
  public void testGetByteStringMemoized() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    ByteString lazyFieldByteString1 = lazyField.toByteString();
    ByteString lazyFieldByteString2 = lazyField.toByteString();

    assertThat(lazyFieldByteString1).isEqualTo(message.toByteString());
    assertThat(lazyFieldByteString1).isSameInstanceAs(lazyFieldByteString2);
  }

  @Test
  public void testHashCode() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    MessageLite unused = lazyField.getValue();
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
  }

  @Test
  public void testHashCodeContainingExtensions() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
    MessageLite unused = lazyField.getValue();
    assertThat(message.hashCode()).isEqualTo(lazyField.hashCode());
  }

  @Test
  public void testEqualsObject() {
    MessageLite message = TestUtil.getAllSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(lazyField).isEqualTo(message);
  }

  @Test
  @SuppressWarnings("TruthIncompatibleType")
  public void testEqualsObjectContainingExtensions() throws Exception {
    TestAllExtensions message = TestUtil.getAllExtensionsSet();
    InternalLazyField lazyField = createLazyFieldWithBytesFromMessage(message);
    assertThat(lazyField).isEqualTo(message);
  }

  private InternalLazyField createLazyFieldWithBytesFromMessage(MessageLite message) {
    return new InternalLazyField(
        message.getDefaultInstanceForType(), EXTENSION_REGISTRY, message.toByteString());
  }
}
