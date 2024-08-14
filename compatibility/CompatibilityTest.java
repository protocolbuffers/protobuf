package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestMixedFieldsAndExtensions;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

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
}
