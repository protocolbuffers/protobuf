package com.google.protobuf;

import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestMixedFieldsAndExtensions;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class CompatibilityTest {
    @Test
    public void testFoo() throws Exception {
        final TestAllTypes obj = TestAllTypes.newBuilder().setOptionalInt32(1).addRepeatedInt32(1).build();
        final byte[] serialized = obj.toByteArray();
        final TestAllTypes parsed = TestAllTypes.parseFrom(serialized);

        obj.toString();
        parsed.toString();

        final TestMixedFieldsAndExtensions obj2 = TestMixedFieldsAndExtensions.newBuilder().setA(1).setExtension(TestMixedFieldsAndExtensions.c, 1).build();
        final byte[] serialized2 = obj2.toByteArray();
        final TestMixedFieldsAndExtensions parsed2 = TestMixedFieldsAndExtensions.parseFrom(serialized2);
        obj2.toString();
        parsed2.toString();
    }
}
