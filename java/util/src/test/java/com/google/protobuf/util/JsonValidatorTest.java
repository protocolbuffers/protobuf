package com.google.protobuf.util;

import com.google.common.collect.Sets;
import com.google.protobuf.Message;
import com.google.protobuf.Value;
import com.google.protobuf.util.proto.JsonTestProto.TestJsonValidator;
import com.google.protobuf.util.proto.JsonTestProto.TestStruct;
import junit.framework.TestCase;

import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Set;


public final class JsonValidatorTest extends TestCase
{
  private void mergeFromJson(String json, Message.Builder builder)
          throws IOException
  {
    JsonFormat.parser().merge(json, builder);
  }

  public void testBasic()
          throws IOException
  {
    Value.Builder builder = Value.newBuilder();
    mergeFromJson("{\n" +
            "  \"type_string\": \"\",\n" +
            "  \"type_map_string_string\": 1\n" +
            "}", builder);
    Set<String> expected = new HashSet<String>()
    {{
      add("type_map_string_string supposed to be object but not.");
    }};
    Set<String> principal = new HashSet<String>(JsonValidator.validate(builder.build(), TestJsonValidator.class));
    assertEquals(0, Sets.difference(expected, principal).size());
  }

  public void testComplexSchema()
          throws IOException
  {
    Value.Builder builder = Value.newBuilder();
    mergeFromJson("{\n" +
            "  \"type_string\": \"\",\n" +
            "  \"type_int64\": 9223372036854775806,\n" +
            "  \"type_int32\": 2147483647,\n" +
            "  \"type_uint32\": 4294967295,\n" +
            "  \"type_uint64\": 18446744073709551615,\n" +
            "  \"type_float\": 123.45,\n" +
            "  \"type_double\": 123312.123213,\n" +
            "  \"type_bool\": true,\n" +
            "  \"repeated_type_TestRecursive\": [\n" +
            "    {\n" +
            "      \"value\": 1,\n" +
            "      \"nested\": {\n" +
            "        \"value\": 2,\n" +
            "        \"nested\": {\n" +
            "          \"value\": 3,\n" +
            "          \"nested\": {\n" +
            "            \"value\": 4,\n" +
            "            \"nested\": null\n" +
            "          }\n" +
            "        }\n" +
            "      }\n" +
            "    }\n" +
            "  ],\n" +
            "  \"type_map_string_string\": {\n" +
            "    \"k1\": \"v1\",\n" +
            "    \"k2\": \"v2\"\n" +
            "  }\n" +
            "}\n", builder);
    List<String> result = JsonValidator.validate(builder.build(), TestJsonValidator.class);
    System.out.println(result);
    assertEquals(0, result.size());
  }
}
