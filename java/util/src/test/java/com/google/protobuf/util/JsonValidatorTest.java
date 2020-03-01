package com.google.protobuf.util;

import com.google.common.collect.Sets;
import com.google.protobuf.Message;
import com.google.protobuf.Value;
import com.google.protobuf.util.proto.JsonTestProto.TestComplicatedType;
import junit.framework.TestCase;

import java.io.IOException;
import java.util.HashSet;
import java.util.Set;


public final class JsonValidatorTest extends TestCase
{
  private void mergeFromJson(String json, Message.Builder builder)
          throws IOException
  {
    JsonFormat.parser().merge(json, builder);
  }

  public void test()
          throws IOException
  {
    Value.Builder builder = Value.newBuilder();
    mergeFromJson("{\n" +
            "  \"type_string\": \"\",\n" +
            "  \"type_TestStruct\": 1\n" +
            "}", builder);
    Set<String> expected = new HashSet<String>()
    {{
      add("type_int64 is not included.");
      add("type_int32 is not included.");
      add("type_uint32 is not included.");
      add("type_uint64 is not included.");
      add("type_float is not included.");
      add("type_double is not included.");
      add("type_bool is not included.");
      add("type_TestStruct supposed to be object but not.");
    }};
    Set<String> principal = new HashSet<String>(JsonValidator.validate(builder.build(), TestComplicatedType.class));
    assertEquals(0, Sets.difference(expected, principal).size());
  }
}
