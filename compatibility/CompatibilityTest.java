package com.google.protobuf;

import protobuf_unittest.UnittestProto.Int64ParseTester;
import protobuf_unittest.UnittestProto.TestAllTypes;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class CompatibilityTest {
    @Test
    public void testFoo() throws Exception {
        final TestAllTypes obj = TestAllTypes.newBuilder().setOptionalInt32(1).build();
        final byte[] serialized = obj.toByteArray();
        final TestAllTypes parsed = TestAllTypes.parseFrom(serialized);

        obj.toString();
        parsed.toString();

        final Int64ParseTester obj2 = Int64ParseTester.newBuilder().setOptionalInt64Hifield(0).build();
        final byte[] serialized2 = obj2.toByteArray();
        final Int64ParseTester parsed2 = Int64ParseTester.parseFrom(serialized2);
        obj2.toString();
        parsed2.toString();
    }
}
