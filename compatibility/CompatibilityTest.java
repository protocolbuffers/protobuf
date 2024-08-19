package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes.NestedMessage;
import protobuf_unittest.UnittestProto.TestMixedFieldsAndExtensions;

@RunWith(JUnit4.class)
public class CompatibilityTest {
    @Test
    public void testGeneratedMessage() throws Exception {
        final TestAllTypes obj = TestAllTypes.newBuilder().setOptionalInt32(1).addRepeatedInt32(1).build();
        final byte[] serialized = obj.toByteArray();
        final TestAllTypes parsed = TestAllTypes.parseFrom(serialized);
        assertThat(parsed).isEqualTo(obj);
        obj.toString();
        parsed.toString();
    }

    @Test
    public void testGetFieldBuilderWithInitializedValue() {
      Descriptor descriptor = TestAllTypes.getDescriptor();
      FieldDescriptor fieldDescriptor = descriptor.findFieldByName("optional_nested_message");
  
      // Before setting field, builder is initialized by default value.
      TestAllTypes.Builder builder = TestAllTypes.newBuilder();
      NestedMessage.Builder fieldBuilder =
          (NestedMessage.Builder) builder.getFieldBuilder(fieldDescriptor);
      assertThat(fieldBuilder.getBb()).isEqualTo(0);
  
      // Setting field value with new field builder instance.
      builder = TestAllTypes.newBuilder();
      NestedMessage.Builder newFieldBuilder = builder.getOptionalNestedMessageBuilder();
      newFieldBuilder.setBb(2);
      // Then get the field builder instance by getFieldBuilder().
      fieldBuilder = (NestedMessage.Builder) builder.getFieldBuilder(fieldDescriptor);
      // It should contain new value.
      assertThat(fieldBuilder.getBb()).isEqualTo(2);
      // These two builder should be equal.
      assertThat(fieldBuilder).isSameInstanceAs(newFieldBuilder);
    }

    @Test
    public void testExtendableMessage() throws Exception {
        final TestMixedFieldsAndExtensions obj = TestMixedFieldsAndExtensions.newBuilder().setA(1).setExtension(TestMixedFieldsAndExtensions.c, 1).build();
        assertThat((int) obj.getExtension(TestMixedFieldsAndExtensions.c)).isEqualTo(1);
        final byte[] serialized = obj.toByteArray();
        ExtensionRegistry registry = ExtensionRegistry.newInstance();
        registry.add(TestMixedFieldsAndExtensions.c);
        final TestMixedFieldsAndExtensions parsed = TestMixedFieldsAndExtensions.parseFrom(serialized, registry);
        assertThat(parsed).isEqualTo(obj);
        obj.toString();
        parsed.toString();
    }

    @Test
    public void getBuilderForType() throws Exception {
        TestUtil.MockBuilderParent mockParent = new TestUtil.MockBuilderParent();
        TestMixedFieldsAndExtensions.Builder builder =
            (TestMixedFieldsAndExtensions.Builder)
                TestMixedFieldsAndExtensions.getDefaultInstance().newBuilderForType(mockParent);
    }

    @Test
    public void testExtensionDefaults() throws Exception {
        TestUtil.assertExtensionsClear(TestAllExtensions.getDefaultInstance());
        TestUtil.assertExtensionsClear(TestAllExtensions.newBuilder().build());
    }

    @Test
    public void testExtensionReflectionSettersRejectNull() throws Exception {
        TestUtil.ReflectionTester extensionsReflectionTester =
        new TestUtil.ReflectionTester(
            TestAllExtensions.getDescriptor(), TestUtil.getFullExtensionRegistry());
        TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
        extensionsReflectionTester.assertReflectionSettersRejectNull(builder);
    }

    @Test
    public void testExtensionCopy() throws Exception {
        TestAllExtensions original = TestUtil.getAllExtensionsSet();
        TestAllExtensions copy = TestAllExtensions.newBuilder(original).build();
        TestUtil.assertAllExtensionsSet(copy);
    }

    @Test
    public void testGetParserForType() throws Exception{ 
        TestAllTypes.getDefaultInstance().getParserForType();
    }


    @Test
    public void testGetDefaultInstanceForType() throws Exception{ 
        TestAllTypes.getDefaultInstance().getDefaultInstanceForType();
    }

    @Test
    public void testMergeFrom() throws Exception{ 
        TestAllTypes.Builder obj = TestAllTypes.newBuilder().setOptionalInt32(1);
        TestAllTypes obj2 = TestAllTypes.newBuilder().setOptionalString("foo").build();
        obj.mergeFrom(obj2);
        TestAllTypes expected = TestAllTypes.newBuilder().setOptionalInt32(1).setOptionalString("foo").build();
        assertThat(obj.build()).isEqualTo(expected);
        obj.isInitialized();
    }

    @Test
    public void testFoo() throws Exception{ 
        TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
        builder.foo();
    }
}
