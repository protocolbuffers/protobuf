// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.TextFormat.Parser.SingularOverwritePolicy;
import protobuf_unittest.UnittestMset.TestMessageSet;
import protobuf_unittest.UnittestMset.TestMessageSetExtension1;
import protobuf_unittest.UnittestMset.TestMessageSetExtension2;
import protobuf_unittest.UnittestProto.OneString;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes.NestedMessage;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestOneof2;

import junit.framework.TestCase;

import java.io.StringReader;

/**
 * Test case for {@link TextFormat}.
 *
 * TODO(wenboz): ExtensionTest and rest of text_format_unittest.cc.
 *
 * @author wenboz@google.com (Wenbo Zhu)
 */
public class TextFormatTest extends TestCase {

  // A basic string with different escapable characters for testing.
  private final static String kEscapeTestString =
      "\"A string with ' characters \n and \r newlines and \t tabs and \001 "
          + "slashes \\";

  // A representation of the above string with all the characters escaped.
  private final static String kEscapeTestStringEscaped =
      "\\\"A string with \\' characters \\n and \\r newlines "
          + "and \\t tabs and \\001 slashes \\\\";

  private static String allFieldsSetText = TestUtil.readTextFromFile(
    "text_format_unittest_data_oneof_implemented.txt");
  private static String allExtensionsSetText = TestUtil.readTextFromFile(
    "text_format_unittest_extensions_data.txt");

  private static String exoticText =
    "repeated_int32: -1\n" +
    "repeated_int32: -2147483648\n" +
    "repeated_int64: -1\n" +
    "repeated_int64: -9223372036854775808\n" +
    "repeated_uint32: 4294967295\n" +
    "repeated_uint32: 2147483648\n" +
    "repeated_uint64: 18446744073709551615\n" +
    "repeated_uint64: 9223372036854775808\n" +
    "repeated_double: 123.0\n" +
    "repeated_double: 123.5\n" +
    "repeated_double: 0.125\n" +
    "repeated_double: .125\n" +
    "repeated_double: -.125\n" +
    "repeated_double: 1.23E17\n" +
    "repeated_double: 1.23E+17\n" +
    "repeated_double: -1.23e-17\n" +
    "repeated_double: .23e+17\n" +
    "repeated_double: -.23E17\n" +
    "repeated_double: 1.235E22\n" +
    "repeated_double: 1.235E-18\n" +
    "repeated_double: 123.456789\n" +
    "repeated_double: Infinity\n" +
    "repeated_double: -Infinity\n" +
    "repeated_double: NaN\n" +
    "repeated_string: \"\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"" +
      "\\341\\210\\264\"\n" +
    "repeated_bytes: \"\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"\\376\"\n";

  private static String canonicalExoticText =
      exoticText.replace(": .", ": 0.").replace(": -.", ": -0.")   // short-form double
      .replace("23e", "23E").replace("E+", "E").replace("0.23E17", "2.3E16");

  private String messageSetText =
    "[protobuf_unittest.TestMessageSetExtension1] {\n" +
    "  i: 123\n" +
    "}\n" +
    "[protobuf_unittest.TestMessageSetExtension2] {\n" +
    "  str: \"foo\"\n" +
    "}\n";

  private String messageSetTextWithRepeatedExtension =
      "[protobuf_unittest.TestMessageSetExtension1] {\n" +
      "  i: 123\n" +
      "}\n" +
      "[protobuf_unittest.TestMessageSetExtension1] {\n" +
      "  i: 456\n" +
      "}\n";

  private final TextFormat.Parser parserWithOverwriteForbidden =
      TextFormat.Parser.newBuilder()
          .setSingularOverwritePolicy(
              SingularOverwritePolicy.FORBID_SINGULAR_OVERWRITES)
          .build();

  private final TextFormat.Parser defaultParser =
      TextFormat.Parser.newBuilder().build();

  /** Print TestAllTypes and compare with golden file. */
  public void testPrintMessage() throws Exception {
    String javaText = TextFormat.printToString(TestUtil.getAllSet());

    // Java likes to add a trailing ".0" to floats and doubles.  C printf
    // (with %g format) does not.  Our golden files are used for both
    // C++ and Java TextFormat classes, so we need to conform.
    javaText = javaText.replace(".0\n", "\n");

    assertEquals(allFieldsSetText, javaText);
  }

  /** Print TestAllTypes as Builder and compare with golden file. */
  public void testPrintMessageBuilder() throws Exception {
    String javaText = TextFormat.printToString(TestUtil.getAllSetBuilder());

    // Java likes to add a trailing ".0" to floats and doubles.  C printf
    // (with %g format) does not.  Our golden files are used for both
    // C++ and Java TextFormat classes, so we need to conform.
    javaText = javaText.replace(".0\n", "\n");

    assertEquals(allFieldsSetText, javaText);
  }

  /** Print TestAllExtensions and compare with golden file. */
  public void testPrintExtensions() throws Exception {
    String javaText = TextFormat.printToString(TestUtil.getAllExtensionsSet());

    // Java likes to add a trailing ".0" to floats and doubles.  C printf
    // (with %g format) does not.  Our golden files are used for both
    // C++ and Java TextFormat classes, so we need to conform.
    javaText = javaText.replace(".0\n", "\n");

    assertEquals(allExtensionsSetText, javaText);
  }

  // Creates an example unknown field set.
  private UnknownFieldSet makeUnknownFieldSet() {
    return UnknownFieldSet.newBuilder()
        .addField(5,
            UnknownFieldSet.Field.newBuilder()
            .addVarint(1)
            .addFixed32(2)
            .addFixed64(3)
            .addLengthDelimited(ByteString.copyFromUtf8("4"))
            .addGroup(
                UnknownFieldSet.newBuilder()
                .addField(10,
                    UnknownFieldSet.Field.newBuilder()
                    .addVarint(5)
                    .build())
                .build())
            .build())
        .addField(8,
            UnknownFieldSet.Field.newBuilder()
            .addVarint(1)
            .addVarint(2)
            .addVarint(3)
            .build())
        .addField(15,
            UnknownFieldSet.Field.newBuilder()
            .addVarint(0xABCDEF1234567890L)
            .addFixed32(0xABCD1234)
            .addFixed64(0xABCDEF1234567890L)
            .build())
        .build();
  }

  public void testPrintUnknownFields() throws Exception {
    // Test printing of unknown fields in a message.

    TestEmptyMessage message =
      TestEmptyMessage.newBuilder()
        .setUnknownFields(makeUnknownFieldSet())
        .build();

    assertEquals(
      "5: 1\n" +
      "5: 0x00000002\n" +
      "5: 0x0000000000000003\n" +
      "5: \"4\"\n" +
      "5 {\n" +
      "  10: 5\n" +
      "}\n" +
      "8: 1\n" +
      "8: 2\n" +
      "8: 3\n" +
      "15: 12379813812177893520\n" +
      "15: 0xabcd1234\n" +
      "15: 0xabcdef1234567890\n",
      TextFormat.printToString(message));
  }

  public void testPrintField() throws Exception {
    final FieldDescriptor dataField =
      OneString.getDescriptor().findFieldByName("data");
    assertEquals(
      "data: \"test data\"\n",
      TextFormat.printFieldToString(dataField, "test data"));

    final FieldDescriptor optionalField =
      TestAllTypes.getDescriptor().findFieldByName("optional_nested_message");
    final Object value = NestedMessage.newBuilder().setBb(42).build();

    assertEquals(
      "optional_nested_message {\n  bb: 42\n}\n",
      TextFormat.printFieldToString(optionalField, value));
  }

  /**
   * Helper to construct a ByteString from a String containing only 8-bit
   * characters.  The characters are converted directly to bytes, *not*
   * encoded using UTF-8.
   */
  private ByteString bytes(String str) throws Exception {
    return ByteString.copyFrom(str.getBytes("ISO-8859-1"));
  }

  /**
   * Helper to construct a ByteString from a bunch of bytes.  The inputs are
   * actually ints so that I can use hex notation and not get stupid errors
   * about precision.
   */
  private ByteString bytes(int... bytesAsInts) {
    byte[] bytes = new byte[bytesAsInts.length];
    for (int i = 0; i < bytesAsInts.length; i++) {
      bytes[i] = (byte) bytesAsInts[i];
    }
    return ByteString.copyFrom(bytes);
  }

  public void testPrintExotic() throws Exception {
    Message message = TestAllTypes.newBuilder()
      // Signed vs. unsigned numbers.
      .addRepeatedInt32 (-1)
      .addRepeatedUint32(-1)
      .addRepeatedInt64 (-1)
      .addRepeatedUint64(-1)

      .addRepeatedInt32 (1  << 31)
      .addRepeatedUint32(1  << 31)
      .addRepeatedInt64 (1L << 63)
      .addRepeatedUint64(1L << 63)

      // Floats of various precisions and exponents.
      .addRepeatedDouble(123)
      .addRepeatedDouble(123.5)
      .addRepeatedDouble(0.125)
      .addRepeatedDouble(.125)
      .addRepeatedDouble(-.125)
      .addRepeatedDouble(123e15)
      .addRepeatedDouble(123e15)
      .addRepeatedDouble(-1.23e-17)
      .addRepeatedDouble(.23e17)
      .addRepeatedDouble(-23e15)
      .addRepeatedDouble(123.5e20)
      .addRepeatedDouble(123.5e-20)
      .addRepeatedDouble(123.456789)
      .addRepeatedDouble(Double.POSITIVE_INFINITY)
      .addRepeatedDouble(Double.NEGATIVE_INFINITY)
      .addRepeatedDouble(Double.NaN)

      // Strings and bytes that needing escaping.
      .addRepeatedString("\0\001\007\b\f\n\r\t\013\\\'\"\u1234")
      .addRepeatedBytes(bytes("\0\001\007\b\f\n\r\t\013\\\'\"\u00fe"))
      .build();

    assertEquals(canonicalExoticText, message.toString());
  }

  public void testPrintMessageSet() throws Exception {
    TestMessageSet messageSet =
      TestMessageSet.newBuilder()
        .setExtension(
          TestMessageSetExtension1.messageSetExtension,
          TestMessageSetExtension1.newBuilder().setI(123).build())
        .setExtension(
          TestMessageSetExtension2.messageSetExtension,
          TestMessageSetExtension2.newBuilder().setStr("foo").build())
        .build();

    assertEquals(messageSetText, messageSet.toString());
  }

  // =================================================================

  public void testParse() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(allFieldsSetText, builder);
    TestUtil.assertAllFieldsSet(builder.build());
  }

  public void testParseReader() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(new StringReader(allFieldsSetText), builder);
    TestUtil.assertAllFieldsSet(builder.build());
  }

  public void testParseExtensions() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TextFormat.merge(allExtensionsSetText,
                     TestUtil.getExtensionRegistry(),
                     builder);
    TestUtil.assertAllExtensionsSet(builder.build());
  }

  public void testParseCompatibility() throws Exception {
    String original = "repeated_float: inf\n" +
                      "repeated_float: -inf\n" +
                      "repeated_float: nan\n" +
                      "repeated_float: inff\n" +
                      "repeated_float: -inff\n" +
                      "repeated_float: nanf\n" +
                      "repeated_float: 1.0f\n" +
                      "repeated_float: infinityf\n" +
                      "repeated_float: -Infinityf\n" +
                      "repeated_double: infinity\n" +
                      "repeated_double: -infinity\n" +
                      "repeated_double: nan\n";
    String canonical =  "repeated_float: Infinity\n" +
                        "repeated_float: -Infinity\n" +
                        "repeated_float: NaN\n" +
                        "repeated_float: Infinity\n" +
                        "repeated_float: -Infinity\n" +
                        "repeated_float: NaN\n" +
                        "repeated_float: 1.0\n" +
                        "repeated_float: Infinity\n" +
                        "repeated_float: -Infinity\n" +
                        "repeated_double: Infinity\n" +
                        "repeated_double: -Infinity\n" +
                        "repeated_double: NaN\n";
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(original, builder);
    assertEquals(canonical, builder.build().toString());
  }

  public void testParseExotic() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(exoticText, builder);

    // Too lazy to check things individually.  Don't try to debug this
    // if testPrintExotic() is failing.
    assertEquals(canonicalExoticText, builder.build().toString());
  }

  public void testParseMessageSet() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);

    TestMessageSet.Builder builder = TestMessageSet.newBuilder();
    TextFormat.merge(messageSetText, extensionRegistry, builder);
    TestMessageSet messageSet = builder.build();

    assertTrue(messageSet.hasExtension(
      TestMessageSetExtension1.messageSetExtension));
    assertEquals(123, messageSet.getExtension(
      TestMessageSetExtension1.messageSetExtension).getI());
    assertTrue(messageSet.hasExtension(
      TestMessageSetExtension2.messageSetExtension));
    assertEquals("foo", messageSet.getExtension(
      TestMessageSetExtension2.messageSetExtension).getStr());

    builder = TestMessageSet.newBuilder();
    TextFormat.merge(messageSetTextWithRepeatedExtension, extensionRegistry,
        builder);
    messageSet = builder.build();
    assertEquals(456, messageSet.getExtension(
      TestMessageSetExtension1.messageSetExtension).getI());
  }

  public void testParseMessageSetWithOverwriteForbidden() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);

    TestMessageSet.Builder builder = TestMessageSet.newBuilder();
    parserWithOverwriteForbidden.merge(
        messageSetText, extensionRegistry, builder);
    TestMessageSet messageSet = builder.build();
    assertEquals(123, messageSet.getExtension(
        TestMessageSetExtension1.messageSetExtension).getI());
    assertEquals("foo", messageSet.getExtension(
      TestMessageSetExtension2.messageSetExtension).getStr());

    builder = TestMessageSet.newBuilder();
    try {
      parserWithOverwriteForbidden.merge(
          messageSetTextWithRepeatedExtension, extensionRegistry, builder);
      fail("expected parse exception");
    } catch (TextFormat.ParseException e) {
      assertEquals("6:1: Non-repeated field "
          + "\"protobuf_unittest.TestMessageSetExtension1.message_set_extension\""
          + " cannot be overwritten.",
          e.getMessage());
    }
  }

  public void testParseNumericEnum() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_nested_enum: 2", builder);
    assertEquals(TestAllTypes.NestedEnum.BAR, builder.getOptionalNestedEnum());
  }

  public void testParseAngleBrackets() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("OptionalGroup: < a: 1 >", builder);
    assertTrue(builder.hasOptionalGroup());
    assertEquals(1, builder.getOptionalGroup().getA());
  }

  public void testParseComment() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(
      "# this is a comment\n" +
      "optional_int32: 1  # another comment\n" +
      "optional_int64: 2\n" +
      "# EOF comment", builder);
    assertEquals(1, builder.getOptionalInt32());
    assertEquals(2, builder.getOptionalInt64());
  }

  private void assertParseError(String error, String text) {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      TextFormat.merge(text, TestUtil.getExtensionRegistry(), builder);
      fail("Expected parse exception.");
    } catch (TextFormat.ParseException e) {
      assertEquals(error, e.getMessage());
    }
  }

  private void assertParseErrorWithOverwriteForbidden(String error,
      String text) {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      parserWithOverwriteForbidden.merge(
          text, TestUtil.getExtensionRegistry(), builder);
      fail("Expected parse exception.");
    } catch (TextFormat.ParseException e) {
      assertEquals(error, e.getMessage());
    }
  }

  private TestAllTypes assertParseSuccessWithOverwriteForbidden(
      String text) throws TextFormat.ParseException {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    parserWithOverwriteForbidden.merge(
        text, TestUtil.getExtensionRegistry(), builder);
    return builder.build();
  }

  public void testParseErrors() throws Exception {
    assertParseError(
      "1:16: Expected \":\".",
      "optional_int32 123");
    assertParseError(
      "1:23: Expected identifier. Found '?'",
      "optional_nested_enum: ?");
    assertParseError(
      "1:18: Couldn't parse integer: Number must be positive: -1",
      "optional_uint32: -1");
    assertParseError(
      "1:17: Couldn't parse integer: Number out of range for 32-bit signed " +
        "integer: 82301481290849012385230157",
      "optional_int32: 82301481290849012385230157");
    assertParseError(
      "1:16: Expected \"true\" or \"false\".",
      "optional_bool: maybe");
    assertParseError(
      "1:16: Expected \"true\" or \"false\".",
      "optional_bool: 2");
    assertParseError(
      "1:18: Expected string.",
      "optional_string: 123");
    assertParseError(
      "1:18: String missing ending quote.",
      "optional_string: \"ueoauaoe");
    assertParseError(
      "1:18: String missing ending quote.",
      "optional_string: \"ueoauaoe\n" +
      "optional_int32: 123");
    assertParseError(
      "1:18: Invalid escape sequence: '\\z'",
      "optional_string: \"\\z\"");
    assertParseError(
      "1:18: String missing ending quote.",
      "optional_string: \"ueoauaoe\n" +
      "optional_int32: 123");
    assertParseError(
      "1:2: Extension \"nosuchext\" not found in the ExtensionRegistry.",
      "[nosuchext]: 123");
    assertParseError(
      "1:20: Extension \"protobuf_unittest.optional_int32_extension\" does " +
        "not extend message type \"protobuf_unittest.TestAllTypes\".",
      "[protobuf_unittest.optional_int32_extension]: 123");
    assertParseError(
      "1:1: Message type \"protobuf_unittest.TestAllTypes\" has no field " +
        "named \"nosuchfield\".",
      "nosuchfield: 123");
    assertParseError(
      "1:21: Expected \">\".",
      "OptionalGroup < a: 1");
    assertParseError(
      "1:23: Enum type \"protobuf_unittest.TestAllTypes.NestedEnum\" has no " +
        "value named \"NO_SUCH_VALUE\".",
      "optional_nested_enum: NO_SUCH_VALUE");
    assertParseError(
      "1:23: Enum type \"protobuf_unittest.TestAllTypes.NestedEnum\" has no " +
        "value with number 123.",
      "optional_nested_enum: 123");

    // Delimiters must match.
    assertParseError(
      "1:22: Expected identifier. Found '}'",
      "OptionalGroup < a: 1 }");
    assertParseError(
      "1:22: Expected identifier. Found '>'",
      "OptionalGroup { a: 1 >");
  }

  // =================================================================

  public void testEscape() throws Exception {
    // Escape sequences.
    assertEquals("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"",
      TextFormat.escapeBytes(bytes("\0\001\007\b\f\n\r\t\013\\\'\"")));
    assertEquals("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"",
      TextFormat.escapeText("\0\001\007\b\f\n\r\t\013\\\'\""));
    assertEquals(bytes("\0\001\007\b\f\n\r\t\013\\\'\""),
      TextFormat.unescapeBytes("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\""));
    assertEquals("\0\001\007\b\f\n\r\t\013\\\'\"",
      TextFormat.unescapeText("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\""));
    assertEquals(kEscapeTestStringEscaped,
      TextFormat.escapeText(kEscapeTestString));
    assertEquals(kEscapeTestString,
      TextFormat.unescapeText(kEscapeTestStringEscaped));

    // Unicode handling.
    assertEquals("\\341\\210\\264", TextFormat.escapeText("\u1234"));
    assertEquals("\\341\\210\\264",
                 TextFormat.escapeBytes(bytes(0xe1, 0x88, 0xb4)));
    assertEquals("\u1234", TextFormat.unescapeText("\\341\\210\\264"));
    assertEquals(bytes(0xe1, 0x88, 0xb4),
                 TextFormat.unescapeBytes("\\341\\210\\264"));
    assertEquals("\u1234", TextFormat.unescapeText("\\xe1\\x88\\xb4"));
    assertEquals(bytes(0xe1, 0x88, 0xb4),
                 TextFormat.unescapeBytes("\\xe1\\x88\\xb4"));

    // Handling of strings with unescaped Unicode characters > 255.
    final String zh = "\u9999\u6e2f\u4e0a\u6d77\ud84f\udf80\u8c50\u9280\u884c";
    ByteString zhByteString = ByteString.copyFromUtf8(zh);
    assertEquals(zhByteString, TextFormat.unescapeBytes(zh));

    // Errors.
    try {
      TextFormat.unescapeText("\\x");
      fail("Should have thrown an exception.");
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      // success
    }

    try {
      TextFormat.unescapeText("\\z");
      fail("Should have thrown an exception.");
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      // success
    }

    try {
      TextFormat.unescapeText("\\");
      fail("Should have thrown an exception.");
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      // success
    }
  }

  public void testParseInteger() throws Exception {
    assertEquals(          0, TextFormat.parseInt32(          "0"));
    assertEquals(          1, TextFormat.parseInt32(          "1"));
    assertEquals(         -1, TextFormat.parseInt32(         "-1"));
    assertEquals(      12345, TextFormat.parseInt32(      "12345"));
    assertEquals(     -12345, TextFormat.parseInt32(     "-12345"));
    assertEquals( 2147483647, TextFormat.parseInt32( "2147483647"));
    assertEquals(-2147483648, TextFormat.parseInt32("-2147483648"));

    assertEquals(                0, TextFormat.parseUInt32(         "0"));
    assertEquals(                1, TextFormat.parseUInt32(         "1"));
    assertEquals(            12345, TextFormat.parseUInt32(     "12345"));
    assertEquals(       2147483647, TextFormat.parseUInt32("2147483647"));
    assertEquals((int) 2147483648L, TextFormat.parseUInt32("2147483648"));
    assertEquals((int) 4294967295L, TextFormat.parseUInt32("4294967295"));

    assertEquals(          0L, TextFormat.parseInt64(          "0"));
    assertEquals(          1L, TextFormat.parseInt64(          "1"));
    assertEquals(         -1L, TextFormat.parseInt64(         "-1"));
    assertEquals(      12345L, TextFormat.parseInt64(      "12345"));
    assertEquals(     -12345L, TextFormat.parseInt64(     "-12345"));
    assertEquals( 2147483647L, TextFormat.parseInt64( "2147483647"));
    assertEquals(-2147483648L, TextFormat.parseInt64("-2147483648"));
    assertEquals( 4294967295L, TextFormat.parseInt64( "4294967295"));
    assertEquals( 4294967296L, TextFormat.parseInt64( "4294967296"));
    assertEquals(9223372036854775807L,
                 TextFormat.parseInt64("9223372036854775807"));
    assertEquals(-9223372036854775808L,
                 TextFormat.parseInt64("-9223372036854775808"));

    assertEquals(          0L, TextFormat.parseUInt64(          "0"));
    assertEquals(          1L, TextFormat.parseUInt64(          "1"));
    assertEquals(      12345L, TextFormat.parseUInt64(      "12345"));
    assertEquals( 2147483647L, TextFormat.parseUInt64( "2147483647"));
    assertEquals( 4294967295L, TextFormat.parseUInt64( "4294967295"));
    assertEquals( 4294967296L, TextFormat.parseUInt64( "4294967296"));
    assertEquals(9223372036854775807L,
                 TextFormat.parseUInt64("9223372036854775807"));
    assertEquals(-9223372036854775808L,
                 TextFormat.parseUInt64("9223372036854775808"));
    assertEquals(-1L, TextFormat.parseUInt64("18446744073709551615"));

    // Hex
    assertEquals(0x1234abcd, TextFormat.parseInt32("0x1234abcd"));
    assertEquals(-0x1234abcd, TextFormat.parseInt32("-0x1234abcd"));
    assertEquals(-1, TextFormat.parseUInt64("0xffffffffffffffff"));
    assertEquals(0x7fffffffffffffffL,
                 TextFormat.parseInt64("0x7fffffffffffffff"));

    // Octal
    assertEquals(01234567, TextFormat.parseInt32("01234567"));

    // Out-of-range
    try {
      TextFormat.parseInt32("2147483648");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseInt32("-2147483649");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt32("4294967296");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt32("-1");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseInt64("9223372036854775808");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseInt64("-9223372036854775809");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt64("18446744073709551616");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt64("-1");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }

    // Not a number.
    try {
      TextFormat.parseInt32("abcd");
      fail("Should have thrown an exception.");
    } catch (NumberFormatException e) {
      // success
    }
  }

  public void testParseString() throws Exception {
    final String zh = "\u9999\u6e2f\u4e0a\u6d77\ud84f\udf80\u8c50\u9280\u884c";
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_string: \"" + zh + "\"", builder);
    assertEquals(zh, builder.getOptionalString());
  }

  public void testParseLongString() throws Exception {
    String longText =
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890" +
      "123456789012345678901234567890123456789012345678901234567890";

    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_string: \"" + longText + "\"", builder);
    assertEquals(longText, builder.getOptionalString());
  }

  public void testParseBoolean() throws Exception {
    String goodText =
        "repeated_bool: t  repeated_bool : 0\n" +
        "repeated_bool :f repeated_bool:1";
    String goodTextCanonical =
        "repeated_bool: true\n" +
        "repeated_bool: false\n" +
        "repeated_bool: false\n" +
        "repeated_bool: true\n";
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(goodText, builder);
    assertEquals(goodTextCanonical, builder.build().toString());

    try {
      TestAllTypes.Builder badBuilder = TestAllTypes.newBuilder();
      TextFormat.merge("optional_bool:2", badBuilder);
      fail("Should have thrown an exception.");
    } catch (TextFormat.ParseException e) {
      // success
    }
    try {
      TestAllTypes.Builder badBuilder = TestAllTypes.newBuilder();
      TextFormat.merge("optional_bool: foo", badBuilder);
      fail("Should have thrown an exception.");
    } catch (TextFormat.ParseException e) {
      // success
    }
  }

  public void testParseAdjacentStringLiterals() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_string: \"foo\" 'corge' \"grault\"", builder);
    assertEquals("foocorgegrault", builder.getOptionalString());
  }

  public void testPrintFieldValue() throws Exception {
    assertPrintFieldValue("\"Hello\"", "Hello", "repeated_string");
    assertPrintFieldValue("123.0",  123f, "repeated_float");
    assertPrintFieldValue("123.0",  123d, "repeated_double");
    assertPrintFieldValue("123",  123, "repeated_int32");
    assertPrintFieldValue("123",  123L, "repeated_int64");
    assertPrintFieldValue("true",  true, "repeated_bool");
    assertPrintFieldValue("4294967295", 0xFFFFFFFF, "repeated_uint32");
    assertPrintFieldValue("18446744073709551615",  0xFFFFFFFFFFFFFFFFL,
        "repeated_uint64");
    assertPrintFieldValue("\"\\001\\002\\003\"",
        ByteString.copyFrom(new byte[] {1, 2, 3}), "repeated_bytes");
  }

  private void assertPrintFieldValue(String expect, Object value,
      String fieldName) throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    StringBuilder sb = new StringBuilder();
    TextFormat.printFieldValue(
        TestAllTypes.getDescriptor().findFieldByName(fieldName),
        value, sb);
    assertEquals(expect, sb.toString());
  }

  public void testShortDebugString() {
    assertEquals("optional_nested_message { bb: 42 } repeated_int32: 1"
        + " repeated_uint32: 2",
        TextFormat.shortDebugString(TestAllTypes.newBuilder()
            .addRepeatedInt32(1)
            .addRepeatedUint32(2)
            .setOptionalNestedMessage(
                NestedMessage.newBuilder().setBb(42).build())
            .build()));
  }

  public void testShortDebugString_unknown() {
    assertEquals("5: 1 5: 0x00000002 5: 0x0000000000000003 5: \"4\" 5 { 10: 5 }"
        + " 8: 1 8: 2 8: 3 15: 12379813812177893520 15: 0xabcd1234 15:"
        + " 0xabcdef1234567890",
        TextFormat.shortDebugString(makeUnknownFieldSet()));
  }

  public void testPrintToUnicodeString() throws Exception {
    assertEquals(
        "optional_string: \"abc\u3042efg\"\n" +
        "optional_bytes: \"\\343\\201\\202\"\n" +
        "repeated_string: \"\u3093XYZ\"\n",
        TextFormat.printToUnicodeString(TestAllTypes.newBuilder()
            .setOptionalString("abc\u3042efg")
            .setOptionalBytes(bytes(0xe3, 0x81, 0x82))
            .addRepeatedString("\u3093XYZ")
            .build()));

    // Double quotes and backslashes should be escaped
    assertEquals(
        "optional_string: \"a\\\\bc\\\"ef\\\"g\"\n",
        TextFormat.printToUnicodeString(TestAllTypes.newBuilder()
            .setOptionalString("a\\bc\"ef\"g")
            .build()));

    // Test escaping roundtrip
    TestAllTypes message = TestAllTypes.newBuilder()
        .setOptionalString("a\\bc\\\"ef\"g")
        .build();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(TextFormat.printToUnicodeString(message), builder);
    assertEquals(message.getOptionalString(), builder.getOptionalString());
  }
  
  public void testPrintToUnicodeStringWithNewlines() {
    // No newlines at start and end
    assertEquals("optional_string: \"test newlines\n\nin\nstring\"\n",
        TextFormat.printToUnicodeString(TestAllTypes.newBuilder()
            .setOptionalString("test newlines\n\nin\nstring")
            .build()));

    // Newlines at start and end
    assertEquals("optional_string: \"\ntest\nnewlines\n\nin\nstring\n\"\n",
        TextFormat.printToUnicodeString(TestAllTypes.newBuilder()
            .setOptionalString("\ntest\nnewlines\n\nin\nstring\n")
            .build()));

    // Strings with 0, 1 and 2 newlines.
    assertEquals("optional_string: \"\"\n",
        TextFormat.printToUnicodeString(TestAllTypes.newBuilder()
            .setOptionalString("")
            .build()));
    assertEquals("optional_string: \"\n\"\n",
        TextFormat.printToUnicodeString(TestAllTypes.newBuilder()
            .setOptionalString("\n")
            .build()));
    assertEquals("optional_string: \"\n\n\"\n",
        TextFormat.printToUnicodeString(TestAllTypes.newBuilder()
            .setOptionalString("\n\n")
            .build()));
  }

  public void testPrintToUnicodeString_unknown() {
    assertEquals(
        "1: \"\\343\\201\\202\"\n",
        TextFormat.printToUnicodeString(UnknownFieldSet.newBuilder()
            .addField(1,
                UnknownFieldSet.Field.newBuilder()
                .addLengthDelimited(bytes(0xe3, 0x81, 0x82)).build())
            .build()));
  }

  public void testParseNonRepeatedFields() throws Exception {
    assertParseSuccessWithOverwriteForbidden(
        "repeated_int32: 1\n" +
        "repeated_int32: 2\n");
    assertParseSuccessWithOverwriteForbidden(
        "RepeatedGroup { a: 1 }\n" +
        "RepeatedGroup { a: 2 }\n");
    assertParseSuccessWithOverwriteForbidden(
        "repeated_nested_message { bb: 1 }\n" +
        "repeated_nested_message { bb: 2 }\n");
    assertParseErrorWithOverwriteForbidden(
        "3:17: Non-repeated field " +
        "\"protobuf_unittest.TestAllTypes.optional_int32\" " +
        "cannot be overwritten.",
        "optional_int32: 1\n" +
        "optional_bool: true\n" +
        "optional_int32: 1\n");
    assertParseErrorWithOverwriteForbidden(
        "2:17: Non-repeated field " +
        "\"protobuf_unittest.TestAllTypes.optionalgroup\" " +
        "cannot be overwritten.",
        "OptionalGroup { a: 1 }\n" +
        "OptionalGroup { }\n");
    assertParseErrorWithOverwriteForbidden(
        "2:33: Non-repeated field " +
        "\"protobuf_unittest.TestAllTypes.optional_nested_message\" " +
        "cannot be overwritten.",
        "optional_nested_message { }\n" +
        "optional_nested_message { bb: 3 }\n");
    assertParseErrorWithOverwriteForbidden(
        "2:16: Non-repeated field " +
        "\"protobuf_unittest.TestAllTypes.default_int32\" " +
        "cannot be overwritten.",
        "default_int32: 41\n" +  // the default value
        "default_int32: 41\n");
    assertParseErrorWithOverwriteForbidden(
        "2:17: Non-repeated field " +
        "\"protobuf_unittest.TestAllTypes.default_string\" " +
        "cannot be overwritten.",
        "default_string: \"zxcv\"\n" +
        "default_string: \"asdf\"\n");
  }

  public void testParseShortRepeatedFormOfRepeatedFields() throws Exception {
    assertParseSuccessWithOverwriteForbidden("repeated_foreign_enum: [FOREIGN_FOO, FOREIGN_BAR]");
    assertParseSuccessWithOverwriteForbidden("repeated_int32: [ 1, 2 ]\n");
    assertParseSuccessWithOverwriteForbidden("RepeatedGroup [{ a: 1 },{ a: 2 }]\n");
    assertParseSuccessWithOverwriteForbidden("repeated_nested_message [{ bb: 1 }, { bb: 2 }]\n");
  }

  public void testParseShortRepeatedFormOfNonRepeatedFields() throws Exception {
    assertParseErrorWithOverwriteForbidden(
        "1:17: Couldn't parse integer: For input string: \"[\"",
        "optional_int32: [1]\n");
  }

  // =======================================================================
  // test oneof

  public void testOneofTextFormat() throws Exception {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    TestUtil.setOneof(builder);
    TestOneof2 message = builder.build();
    TestOneof2.Builder dest = TestOneof2.newBuilder();
    TextFormat.merge(TextFormat.printToUnicodeString(message), dest);
    TestUtil.assertOneofSet(dest.build());
  }

  public void testOneofOverwriteForbidden() throws Exception {
    String input = "foo_string: \"stringvalue\" foo_int: 123";
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    try {
      parserWithOverwriteForbidden.merge(
          input, TestUtil.getExtensionRegistry(), builder);
      fail("Expected parse exception.");
    } catch (TextFormat.ParseException e) {
      assertEquals("1:36: Field \"protobuf_unittest.TestOneof2.foo_int\""
                   + " is specified along with field \"protobuf_unittest.TestOneof2.foo_string\","
                   + " another member of oneof \"foo\".", e.getMessage());
    }
  }

  public void testOneofOverwriteAllowed() throws Exception {
    String input = "foo_string: \"stringvalue\" foo_int: 123";
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    defaultParser.merge(input, TestUtil.getExtensionRegistry(), builder);
    // Only the last value sticks.
    TestOneof2 oneof = builder.build();
    assertFalse(oneof.hasFooString());
    assertTrue(oneof.hasFooInt());
  }
}
