// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static com.google.protobuf.TestUtil.TEST_REQUIRED_INITIALIZED;
import static com.google.protobuf.TestUtil.TEST_REQUIRED_UNINITIALIZED;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.TextFormat.InvalidEscapeSequenceException;
import com.google.protobuf.TextFormat.Parser.SingularOverwritePolicy;
import com.google.protobuf.testing.proto.TestProto3Optional;
import com.google.protobuf.testing.proto.TestProto3Optional.NestedEnum;
import any_test.AnyTestProto.TestAny;
import map_test.MapTestProto.TestMap;
import protobuf_unittest.UnittestMset.TestMessageSetExtension1;
import protobuf_unittest.UnittestMset.TestMessageSetExtension2;
import protobuf_unittest.UnittestProto.OneString;
import protobuf_unittest.UnittestProto.TestAllExtensions;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestAllTypes.NestedMessage;
import protobuf_unittest.UnittestProto.TestEmptyMessage;
import protobuf_unittest.UnittestProto.TestOneof2;
import protobuf_unittest.UnittestProto.TestRecursiveMessage;
import protobuf_unittest.UnittestProto.TestRequired;
import protobuf_unittest.UnittestProto.TestReservedFields;
import proto2_wireformat_unittest.UnittestMsetWireFormat.TestMessageSet;
import java.io.StringReader;
import java.util.Arrays;
import java.util.List;
import java.util.logging.Logger;
import org.junit.Test;
import org.junit.function.ThrowingRunnable;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test case for {@link TextFormat}. */
@RunWith(JUnit4.class)
public class TextFormatTest {

  // A basic string with different escapable characters for testing.
  private static final String ESCAPE_TEST_STRING =
      "\"A string with ' characters \n and \r newlines and \t tabs and \001 slashes \\";

  // A representation of the above string with all the characters escaped.
  private static final String ESCAPE_TEST_STRING_ESCAPED =
      "\\\"A string with \\' characters \\n and \\r newlines "
          + "and \\t tabs and \\001 slashes \\\\";

  private static final String ALL_FIELDS_SET_TEXT =
      TestUtil.readTextFromFile("text_format_unittest_data_oneof_implemented.txt");
  private static final String ALL_EXTENSIONS_SET_TEXT =
      TestUtil.readTextFromFile("text_format_unittest_extensions_data.txt");

  private static final String EXOTIC_TEXT =
      ""
          + "repeated_int32: -1\n"
          + "repeated_int32: -2147483648\n"
          + "repeated_int64: -1,\n"
          + "repeated_int64: -9223372036854775808\n"
          + "repeated_uint32: 4294967295\n"
          + "repeated_uint32: 2147483648\n"
          + "repeated_uint64: 18446744073709551615\n"
          + "repeated_uint64: 9223372036854775808\n"
          + "repeated_double: 123.0\n"
          + "repeated_double: 123.5\n"
          + "repeated_double: 0.125\n"
          + "repeated_double: .125\n"
          + "repeated_double: -.125\n"
          + "repeated_double: 1.23E17\n"
          + "repeated_double: 1.23E+17\n"
          + "repeated_double: -1.23e-17\n"
          + "repeated_double: .23e+17\n"
          + "repeated_double: -.23E17\n"
          + "repeated_double: 1.235E22\n"
          + "repeated_double: 1.235E-18\n"
          + "repeated_double: 123.456789\n"
          + "repeated_double: Infinity\n"
          + "repeated_double: -Infinity\n"
          + "repeated_double: NaN\n"
          + "repeated_string: \"\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\""
          + "\\341\\210\\264\"\n"
          + "repeated_bytes: \"\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"\\376\"\n";

  private static final String CANONICAL_EXOTIC_TEXT =
      EXOTIC_TEXT
          .replace(": .", ": 0.")
          .replace(": -.", ": -0.") // short-form double
          .replace("23e", "23E")
          .replace("E+", "E")
          .replace("0.23E17", "2.3E16")
          .replace(",", "");

  private static final String MESSAGE_SET_TEXT =
      ""
          + "[protobuf_unittest.TestMessageSetExtension1] {\n"
          + "  i: 123\n"
          + "}\n"
          + "[protobuf_unittest.TestMessageSetExtension2] {\n"
          + "  str: \"foo\"\n"
          + "}\n";

  private static final String MESSAGE_SET_TEXT_WITH_REPEATED_EXTENSION =
      ""
          + "[protobuf_unittest.TestMessageSetExtension1] {\n"
          + "  i: 123\n"
          + "}\n"
          + "[protobuf_unittest.TestMessageSetExtension1] {\n"
          + "  i: 456\n"
          + "}\n";

  private static final TextFormat.Parser PARSER_ALLOWING_UNKNOWN_FIELDS =
      TextFormat.Parser.newBuilder().setAllowUnknownFields(true).build();

  private static final TextFormat.Parser PARSER_ALLOWING_UNKNOWN_EXTENSIONS =
      TextFormat.Parser.newBuilder().setAllowUnknownExtensions(true).build();

  private static final TextFormat.Parser PARSER_WITH_OVERWRITE_FORBIDDEN =
      TextFormat.Parser.newBuilder()
          .setSingularOverwritePolicy(SingularOverwritePolicy.FORBID_SINGULAR_OVERWRITES)
          .build();

  private static final TextFormat.Parser DEFAULT_PARSER = TextFormat.Parser.newBuilder().build();

  /** Print TestAllTypes and compare with golden file. */
  @Test
  public void testPrintMessage() throws Exception {
    String javaText = TextFormat.printer().printToString(TestUtil.getAllSet());

    // Java likes to add a trailing ".0" to floats and doubles.  C printf
    // (with %g format) does not.  Our golden files are used for both
    // C++ and Java TextFormat classes, so we need to conform.
    javaText = javaText.replace(".0\n", "\n");

    assertThat(javaText).isEqualTo(ALL_FIELDS_SET_TEXT);
  }

  @Test
  // https://github.com/protocolbuffers/protobuf/issues/9447
  public void testCharacterNotInUnicodeBlock() throws TextFormat.InvalidEscapeSequenceException {
    ByteString actual = TextFormat.unescapeBytes("\\U000358da");
    assertThat(actual.size()).isEqualTo(4);
  }

  /** Print TestAllTypes as Builder and compare with golden file. */
  @Test
  public void testPrintMessageBuilder() throws Exception {
    String javaText = TextFormat.printer().printToString(TestUtil.getAllSetBuilder());

    // Java likes to add a trailing ".0" to floats and doubles.  C printf
    // (with %g format) does not.  Our golden files are used for both
    // C++ and Java TextFormat classes, so we need to conform.
    javaText = javaText.replace(".0\n", "\n");

    assertThat(javaText).isEqualTo(ALL_FIELDS_SET_TEXT);
  }

  /** Print TestAllExtensions and compare with golden file. */
  @Test
  public void testPrintExtensions() throws Exception {
    String javaText = TextFormat.printer().printToString(TestUtil.getAllExtensionsSet());

    // Java likes to add a trailing ".0" to floats and doubles.  C printf
    // (with %g format) does not.  Our golden files are used for both
    // C++ and Java TextFormat classes, so we need to conform.
    javaText = javaText.replace(".0\n", "\n");

    assertThat(javaText).isEqualTo(ALL_EXTENSIONS_SET_TEXT);
  }

  // Creates an example unknown field set.
  private UnknownFieldSet makeUnknownFieldSet() {

    return UnknownFieldSet.newBuilder()
        .addField(
            5,
            UnknownFieldSet.Field.newBuilder()
                .addVarint(1)
                .addFixed32(2)
                .addFixed64(3)
                .addLengthDelimited(ByteString.copyFromUtf8("4"))
                .addLengthDelimited(
                    UnknownFieldSet.newBuilder()
                        .addField(12, UnknownFieldSet.Field.newBuilder().addVarint(6).build())
                        .build()
                        .toByteString())
                .addGroup(
                    UnknownFieldSet.newBuilder()
                        .addField(10, UnknownFieldSet.Field.newBuilder().addVarint(5).build())
                        .build())
                .build())
        .addField(
            8, UnknownFieldSet.Field.newBuilder().addVarint(1).addVarint(2).addVarint(3).build())
        .addField(
            15,
            UnknownFieldSet.Field.newBuilder()
                .addVarint(0xABCDEF1234567890L)
                .addFixed32(0xABCD1234)
                .addFixed64(0xABCDEF1234567890L)
                .build())
        .build();
  }

  @Test
  public void testPrintUnknownFields() throws Exception {
    // Test printing of unknown fields in a message.

    TestEmptyMessage message =
        TestEmptyMessage.newBuilder().setUnknownFields(makeUnknownFieldSet()).build();

    assertThat(TextFormat.printer().printToString(message))
        .isEqualTo(
            "5: 1\n"
                + "5: 0x00000002\n"
                + "5: 0x0000000000000003\n"
                + "5: \"4\"\n"
                + "5: {\n"
                + "  12: 6\n"
                + "}\n"
                + "5 {\n"
                + "  10: 5\n"
                + "}\n"
                + "8: 1\n"
                + "8: 2\n"
                + "8: 3\n"
                + "15: 12379813812177893520\n"
                + "15: 0xabcd1234\n"
                + "15: 0xabcdef1234567890\n");
  }

  @Test
  public void testPrintField() throws Exception {
    final FieldDescriptor dataField = OneString.getDescriptor().findFieldByName("data");
    assertThat(TextFormat.printer().printFieldToString(dataField, "test data"))
        .isEqualTo("data: \"test data\"\n");

    final FieldDescriptor optionalField =
        TestAllTypes.getDescriptor().findFieldByName("optional_nested_message");
    final Object value = NestedMessage.newBuilder().setBb(42).build();

    assertThat(TextFormat.printer().printFieldToString(optionalField, value))
        .isEqualTo("optional_nested_message {\n  bb: 42\n}\n");
  }

  /**
   * Helper to construct a ByteString from a String containing only 8-bit characters. The characters
   * are converted directly to bytes, *not* encoded using UTF-8.
   */
  private ByteString bytes(String str) {
    return ByteString.copyFrom(str.getBytes(Internal.ISO_8859_1));
  }

  /**
   * Helper to construct a ByteString from a bunch of bytes. The inputs are actually ints so that I
   * can use hex notation and not get stupid errors about precision.
   */
  private ByteString bytes(int... bytesAsInts) {
    byte[] bytes = new byte[bytesAsInts.length];
    for (int i = 0; i < bytesAsInts.length; i++) {
      bytes[i] = (byte) bytesAsInts[i];
    }
    return ByteString.copyFrom(bytes);
  }

  @Test
  public void testPrintExotic() throws Exception {
    Message message =
        TestAllTypes.newBuilder()
            // Signed vs. unsigned numbers.
            .addRepeatedInt32(-1)
            .addRepeatedUint32(-1)
            .addRepeatedInt64(-1)
            .addRepeatedUint64(-1)
            .addRepeatedInt32(1 << 31)
            .addRepeatedUint32(1 << 31)
            .addRepeatedInt64(1L << 63)
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

    assertThat(message.toString()).isEqualTo(CANONICAL_EXOTIC_TEXT);
  }

  @Test
  public void testRoundtripProto3Optional() throws Exception {
    Message message =
        TestProto3Optional.newBuilder()
            .setOptionalInt32(1)
            .setOptionalInt64(2)
            .setOptionalNestedEnum(NestedEnum.BAZ)
            .build();
    TestProto3Optional.Builder message2 = TestProto3Optional.newBuilder();
    TextFormat.merge(message.toString(), message2);

    assertThat(message2.hasOptionalInt32()).isTrue();
    assertThat(message2.hasOptionalInt64()).isTrue();
    assertThat(message2.hasOptionalNestedEnum()).isTrue();
    assertThat(message2.getOptionalInt32()).isEqualTo(1);
    assertThat(message2.getOptionalInt64()).isEqualTo(2);
    assertThat(message2.getOptionalNestedEnum()).isEqualTo(NestedEnum.BAZ);
  }

  @Test
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

    assertThat(messageSet.toString()).isEqualTo(MESSAGE_SET_TEXT);
  }

  // =================================================================

  @Test
  public void testMerge() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(ALL_FIELDS_SET_TEXT, builder);
    TestUtil.assertAllFieldsSet(builder.build());
  }

  @Test
  public void testParse() throws Exception {
    TestUtil.assertAllFieldsSet(TextFormat.parse(ALL_FIELDS_SET_TEXT, TestAllTypes.class));
  }

  @Test
  public void testMergeInitialized() throws Exception {
    TestRequired.Builder builder = TestRequired.newBuilder();
    TextFormat.merge(TEST_REQUIRED_INITIALIZED.toString(), builder);
    assertThat(builder.buildPartial().toString()).isEqualTo(TEST_REQUIRED_INITIALIZED.toString());
    assertThat(builder.isInitialized()).isTrue();
  }

  @Test
  public void testParseInitialized() throws Exception {
    TestRequired parsed =
        TextFormat.parse(TEST_REQUIRED_INITIALIZED.toString(), TestRequired.class);
    assertThat(parsed.toString()).isEqualTo(TEST_REQUIRED_INITIALIZED.toString());
    assertThat(parsed.isInitialized()).isTrue();
  }

  @Test
  public void testMergeUninitialized() throws Exception {
    TestRequired.Builder builder = TestRequired.newBuilder();
    TextFormat.merge(TEST_REQUIRED_UNINITIALIZED.toString(), builder);
    assertThat(builder.buildPartial().toString()).isEqualTo(TEST_REQUIRED_UNINITIALIZED.toString());
    assertThat(builder.isInitialized()).isFalse();
  }

  @Test
  public void testParseUninitialized() throws Exception {
    try {
      TextFormat.parse(TEST_REQUIRED_UNINITIALIZED.toString(), TestRequired.class);
      assertWithMessage("Expected UninitializedMessageException.").fail();
    } catch (UninitializedMessageException e) {
      assertThat(e).hasMessageThat().isEqualTo("Message missing required fields: b, c");
    }
  }

  @Test
  public void testMergeReader() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(new StringReader(ALL_FIELDS_SET_TEXT), builder);
    TestUtil.assertAllFieldsSet(builder.build());
  }

  @Test
  public void testMergeExtensions() throws Exception {
    TestAllExtensions.Builder builder = TestAllExtensions.newBuilder();
    TextFormat.merge(ALL_EXTENSIONS_SET_TEXT, TestUtil.getFullExtensionRegistry(), builder);
    TestUtil.assertAllExtensionsSet(builder.build());
  }

  @Test
  public void testParseExtensions() throws Exception {
    TestUtil.assertAllExtensionsSet(
        TextFormat.parse(
            ALL_EXTENSIONS_SET_TEXT, TestUtil.getFullExtensionRegistry(), TestAllExtensions.class));
  }

  @Test
  public void testMergeAndParseCompatibility() throws Exception {
    String original =
        "repeated_float: inf\n"
            + "repeated_float: -inf\n"
            + "repeated_float: nan\n"
            + "repeated_float: inff\n"
            + "repeated_float: -inff\n"
            + "repeated_float: nanf\n"
            + "repeated_float: 1.0f\n"
            + "repeated_float: infinityf\n"
            + "repeated_float: -Infinityf\n"
            + "repeated_double: infinity\n"
            + "repeated_double: -infinity\n"
            + "repeated_double: nan\n";
    String canonical =
        "repeated_float: Infinity\n"
            + "repeated_float: -Infinity\n"
            + "repeated_float: NaN\n"
            + "repeated_float: Infinity\n"
            + "repeated_float: -Infinity\n"
            + "repeated_float: NaN\n"
            + "repeated_float: 1.0\n"
            + "repeated_float: Infinity\n"
            + "repeated_float: -Infinity\n"
            + "repeated_double: Infinity\n"
            + "repeated_double: -Infinity\n"
            + "repeated_double: NaN\n";

    // Test merge().
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(original, builder);
    assertThat(builder.build().toString()).isEqualTo(canonical);

    // Test parse().
    assertThat(TextFormat.parse(original, TestAllTypes.class).toString()).isEqualTo(canonical);
  }

  @Test
  public void testMergeAndParseExotic() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(EXOTIC_TEXT, builder);

    // Too lazy to check things individually.  Don't try to debug this
    // if testPrintExotic() is failing.
    assertThat(builder.build().toString()).isEqualTo(CANONICAL_EXOTIC_TEXT);
    assertThat(TextFormat.parse(EXOTIC_TEXT, TestAllTypes.class).toString())
        .isEqualTo(CANONICAL_EXOTIC_TEXT);
  }

  @Test
  public void testMergeMessageSet() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);

    TestMessageSet.Builder builder = TestMessageSet.newBuilder();
    TextFormat.merge(MESSAGE_SET_TEXT, extensionRegistry, builder);
    TestMessageSet messageSet = builder.build();

    assertThat(messageSet.hasExtension(TestMessageSetExtension1.messageSetExtension)).isTrue();
    assertThat(messageSet.getExtension(TestMessageSetExtension1.messageSetExtension).getI())
        .isEqualTo(123);
    assertThat(messageSet.hasExtension(TestMessageSetExtension2.messageSetExtension)).isTrue();
    assertThat(messageSet.getExtension(TestMessageSetExtension2.messageSetExtension).getStr())
        .isEqualTo("foo");

    builder = TestMessageSet.newBuilder();
    TextFormat.merge(MESSAGE_SET_TEXT_WITH_REPEATED_EXTENSION, extensionRegistry, builder);
    messageSet = builder.build();
    assertThat(messageSet.getExtension(TestMessageSetExtension1.messageSetExtension).getI())
        .isEqualTo(456);
  }

  @Test
  public void testMergeMessageSetWithOverwriteForbidden() throws Exception {
    ExtensionRegistry extensionRegistry = ExtensionRegistry.newInstance();
    extensionRegistry.add(TestMessageSetExtension1.messageSetExtension);
    extensionRegistry.add(TestMessageSetExtension2.messageSetExtension);

    TestMessageSet.Builder builder = TestMessageSet.newBuilder();
    PARSER_WITH_OVERWRITE_FORBIDDEN.merge(MESSAGE_SET_TEXT, extensionRegistry, builder);
    TestMessageSet messageSet = builder.build();
    assertThat(messageSet.getExtension(TestMessageSetExtension1.messageSetExtension).getI())
        .isEqualTo(123);
    assertThat(messageSet.getExtension(TestMessageSetExtension2.messageSetExtension).getStr())
        .isEqualTo("foo");

    builder = TestMessageSet.newBuilder();
    try {
      PARSER_WITH_OVERWRITE_FORBIDDEN.merge(
          MESSAGE_SET_TEXT_WITH_REPEATED_EXTENSION, extensionRegistry, builder);
      assertWithMessage("expected parse exception").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo(
              "4:44: Non-repeated field "
                  + "\"protobuf_unittest.TestMessageSetExtension1.message_set_extension\""
                  + " cannot be overwritten.");
    }
  }

  @Test
  public void testMergeNumericEnum() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_nested_enum: 2", builder);
    assertThat(builder.getOptionalNestedEnum()).isEqualTo(TestAllTypes.NestedEnum.BAR);
  }

  @Test
  public void testMergeAngleBrackets() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("OptionalGroup: < a: 1 >", builder);
    assertThat(builder.hasOptionalGroup()).isTrue();
    assertThat(builder.getOptionalGroup().getA()).isEqualTo(1);
  }

  @Test
  public void testMergeComment() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(
        "# this is a comment\n"
            + "optional_int32: 1  # another comment\n"
            + "optional_int64: 2\n"
            + "# EOF comment",
        builder);
    assertThat(builder.getOptionalInt32()).isEqualTo(1);
    assertThat(builder.getOptionalInt64()).isEqualTo(2);
  }

  @Test
  public void testPrintAny_customBuiltTypeRegistry() throws Exception {
    TestAny testAny =
        TestAny.newBuilder()
            .setValue(
                Any.newBuilder()
                    .setTypeUrl("type.googleapis.com/" + TestAllTypes.getDescriptor().getFullName())
                    .setValue(
                        TestAllTypes.newBuilder().setOptionalInt32(12345).build().toByteString())
                    .build())
            .build();
    String actual =
        TextFormat.printer()
            .usingTypeRegistry(TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build())
            .printToString(testAny);
    String expected =
        "value {\n"
            + "  [type.googleapis.com/protobuf_unittest.TestAllTypes] {\n"
            + "    optional_int32: 12345\n"
            + "  }\n"
            + "}\n";
    assertThat(actual).isEqualTo(expected);
  }

  private static Descriptor createDescriptorForAny(FieldDescriptorProto... fields)
      throws Exception {
    FileDescriptor fileDescriptor =
        FileDescriptor.buildFrom(
            FileDescriptorProto.newBuilder()
                .setName("any.proto")
                .setPackage("google.protobuf")
                .setSyntax("proto3")
                .addMessageType(
                    DescriptorProto.newBuilder().setName("Any").addAllField(Arrays.asList(fields)))
                .build(),
            new FileDescriptor[0]);
    return fileDescriptor.getMessageTypes().get(0);
  }

  @Test
  public void testPrintAny_anyWithDynamicMessage() throws Exception {
    Descriptor descriptor =
        createDescriptorForAny(
            FieldDescriptorProto.newBuilder()
                .setName("type_url")
                .setNumber(1)
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_STRING)
                .build(),
            FieldDescriptorProto.newBuilder()
                .setName("value")
                .setNumber(2)
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_BYTES)
                .build());
    DynamicMessage testAny =
        DynamicMessage.newBuilder(descriptor)
            .setField(
                descriptor.findFieldByNumber(1),
                "type.googleapis.com/" + TestAllTypes.getDescriptor().getFullName())
            .setField(
                descriptor.findFieldByNumber(2),
                TestAllTypes.newBuilder().setOptionalInt32(12345).build().toByteString())
            .build();
    String actual =
        TextFormat.printer()
            .usingTypeRegistry(TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build())
            .printToString(testAny);
    String expected =
        "[type.googleapis.com/protobuf_unittest.TestAllTypes] {\n"
            + "  optional_int32: 12345\n"
            + "}\n";
    assertThat(actual).isEqualTo(expected);
  }

  @Test
  public void testPrintAny_anyFromWithNoValueField() throws Exception {
    Descriptor descriptor =
        createDescriptorForAny(
            FieldDescriptorProto.newBuilder()
                .setName("type_url")
                .setNumber(1)
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_STRING)
                .build());
    DynamicMessage testAny =
        DynamicMessage.newBuilder(descriptor)
            .setField(
                descriptor.findFieldByNumber(1),
                "type.googleapis.com/" + TestAllTypes.getDescriptor().getFullName())
            .build();
    String actual =
        TextFormat.printer()
            .usingTypeRegistry(TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build())
            .printToString(testAny);
    String expected = "type_url: \"type.googleapis.com/protobuf_unittest.TestAllTypes\"\n";
    assertThat(actual).isEqualTo(expected);
  }

  @Test
  public void testPrintAny_anyFromWithNoTypeUrlField() throws Exception {
    Descriptor descriptor =
        createDescriptorForAny(
            FieldDescriptorProto.newBuilder()
                .setName("value")
                .setNumber(2)
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_BYTES)
                .build());
    DynamicMessage testAny =
        DynamicMessage.newBuilder(descriptor)
            .setField(
                descriptor.findFieldByNumber(2),
                TestAllTypes.newBuilder().setOptionalInt32(12345).build().toByteString())
            .build();
    String actual =
        TextFormat.printer()
            .usingTypeRegistry(TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build())
            .printToString(testAny);
    String expected = "value: \"\\b\\271`\"\n";
    assertThat(actual).isEqualTo(expected);
  }

  @Test
  public void testPrintAny_anyWithInvalidFieldType() throws Exception {
    Descriptor descriptor =
        createDescriptorForAny(
            FieldDescriptorProto.newBuilder()
                .setName("type_url")
                .setNumber(1)
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_STRING)
                .build(),
            FieldDescriptorProto.newBuilder()
                .setName("value")
                .setNumber(2)
                .setLabel(FieldDescriptorProto.Label.LABEL_OPTIONAL)
                .setType(FieldDescriptorProto.Type.TYPE_STRING)
                .build());
    DynamicMessage testAny =
        DynamicMessage.newBuilder(descriptor)
            .setField(
                descriptor.findFieldByNumber(1),
                "type.googleapis.com/" + TestAllTypes.getDescriptor().getFullName())
            .setField(descriptor.findFieldByNumber(2), "test")
            .build();
    String actual =
        TextFormat.printer()
            .usingTypeRegistry(TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build())
            .printToString(testAny);
    String expected =
        "type_url: \"type.googleapis.com/protobuf_unittest.TestAllTypes\"\n" + "value: \"test\"\n";
    assertThat(actual).isEqualTo(expected);
  }

  @Test
  public void testMergeAny_customBuiltTypeRegistry() throws Exception {
    TestAny.Builder builder = TestAny.newBuilder();
    TextFormat.Parser.newBuilder()
        .setTypeRegistry(TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build())
        .build()
        .merge(
            "value: {\n"
                + "[type.googleapis.com/protobuf_unittest.TestAllTypes] {\n"
                + "optional_int32: 12345\n"
                + "optional_nested_message {\n"
                + "  bb: 123\n"
                + "}\n"
                + "}\n"
                + "}",
            builder);
    assertThat(builder.build())
        .isEqualTo(
            TestAny.newBuilder()
                .setValue(
                    Any.newBuilder()
                        .setTypeUrl(
                            "type.googleapis.com/" + TestAllTypes.getDescriptor().getFullName())
                        .setValue(
                            TestAllTypes.newBuilder()
                                .setOptionalInt32(12345)
                                .setOptionalNestedMessage(
                                    TestAllTypes.NestedMessage.newBuilder().setBb(123))
                                .build()
                                .toByteString())
                        .build())
                .build());
  }

  private void assertParseError(String error, String text) {
    // Test merge().
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      TextFormat.merge(text, TestUtil.getFullExtensionRegistry(), builder);
      assertWithMessage("Expected parse exception.").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().isEqualTo(error);
    }

    // Test parse().
    try {
      TextFormat.parse(text, TestUtil.getFullExtensionRegistry(), TestAllTypes.class);
      assertWithMessage("Expected parse exception.").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().isEqualTo(error);
    }
  }

  private void assertParseErrorWithUnknownFields(String error, String text) {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      PARSER_ALLOWING_UNKNOWN_FIELDS.merge(text, TestUtil.getFullExtensionRegistry(), builder);
      assertWithMessage("Expected parse exception.").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().isEqualTo(error);
    }
  }

  private TestAllTypes assertParseSuccessWithUnknownFields(String text)
      throws TextFormat.ParseException {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    PARSER_ALLOWING_UNKNOWN_FIELDS.merge(text, TestUtil.getFullExtensionRegistry(), builder);
    return builder.build();
  }

  private void assertParseErrorWithUnknownExtensions(String error, String text) {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      PARSER_ALLOWING_UNKNOWN_EXTENSIONS.merge(text, builder);
      assertWithMessage("Expected parse exception.").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().isEqualTo(error);
    }
  }

  private TestAllTypes assertParseSuccessWithUnknownExtensions(String text)
      throws TextFormat.ParseException {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    PARSER_ALLOWING_UNKNOWN_EXTENSIONS.merge(text, builder);
    return builder.build();
  }

  private void assertParseErrorWithOverwriteForbidden(String error, String text) {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    try {
      PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, TestUtil.getFullExtensionRegistry(), builder);
      assertWithMessage("Expected parse exception.").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().isEqualTo(error);
    }
  }

  @CanIgnoreReturnValue
  private TestAllTypes assertParseSuccessWithOverwriteForbidden(String text)
      throws TextFormat.ParseException {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, TestUtil.getFullExtensionRegistry(), builder);
    return builder.build();
  }

  @Test
  public void testParseErrors() throws Exception {
    assertParseError("1:16: Expected \":\".", "optional_int32 123");
    assertParseError("1:23: Expected identifier. Found '?'", "optional_nested_enum: ?");
    assertParseError(
        "1:18: Couldn't parse integer: Number must be positive: -1", "optional_uint32: -1");
    assertParseError(
        "1:17: Couldn't parse integer: Number out of range for 32-bit signed "
            + "integer: 82301481290849012385230157",
        "optional_int32: 82301481290849012385230157");
    assertParseError(
        "1:16: Expected \"true\" or \"false\". Found \"maybe\".", "optional_bool: maybe");
    assertParseError("1:16: Expected \"true\" or \"false\". Found \"2\".", "optional_bool: 2");
    assertParseError("1:18: Expected string.", "optional_string: 123");
    assertParseError("1:18: String missing ending quote.", "optional_string: \"ueoauaoe");
    assertParseError(
        "1:18: String missing ending quote.", "optional_string: \"ueoauaoe\noptional_int32: 123");
    assertParseError("1:18: Invalid escape sequence: '\\z'", "optional_string: \"\\z\"");
    assertParseError(
        "1:18: String missing ending quote.", "optional_string: \"ueoauaoe\noptional_int32: 123");
    assertParseError(
        "1:2: Input contains unknown fields and/or extensions:\n"
            + "1:2:\tprotobuf_unittest.TestAllTypes.[nosuchext]",
        "[nosuchext]: 123");
    assertParseError(
        "1:20: Extension \"protobuf_unittest.optional_int32_extension\" does "
            + "not extend message type \"protobuf_unittest.TestAllTypes\".",
        "[protobuf_unittest.optional_int32_extension]: 123");
    assertParseError(
        "1:1: Input contains unknown fields and/or extensions:\n"
            + "1:1:\tprotobuf_unittest.TestAllTypes.nosuchfield",
        "nosuchfield: 123");
    assertParseError("1:21: Expected \">\".", "OptionalGroup < a: 1");
    assertParseError(
        "1:23: Enum type \"protobuf_unittest.TestAllTypes.NestedEnum\" has no "
            + "value named \"NO_SUCH_VALUE\".",
        "optional_nested_enum: NO_SUCH_VALUE");
    assertParseError(
        "1:23: Enum type \"protobuf_unittest.TestAllTypes.NestedEnum\" has no "
            + "value with number 123.",
        "optional_nested_enum: 123");

    // Delimiters must match.
    assertParseError("1:22: Expected identifier. Found '}'", "OptionalGroup < a: 1 }");
    assertParseError("1:22: Expected identifier. Found '>'", "OptionalGroup { a: 1 >");
  }

  // =================================================================
  @Test
  public void testEscapeQuestionMark() throws InvalidEscapeSequenceException {
    assertThat(TextFormat.unescapeText("?")).isEqualTo("?");
    assertThat(TextFormat.unescapeText("\\?")).isEqualTo("?");
  }

  @Test
  public void testEscape() throws Exception {
    // Escape sequences.
    assertThat(TextFormat.escapeBytes(bytes("\0\001\007\b\f\n\r\t\013\\\'\"\177")))
        .isEqualTo("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"\\177");
    assertThat(TextFormat.escapeText("\0\001\007\b\f\n\r\t\013\\\'\"\177"))
        .isEqualTo("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"\\177");
    assertThat(TextFormat.unescapeBytes("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\""))
        .isEqualTo(bytes("\0\001\007\b\f\n\r\t\013\\\'\""));
    assertThat(TextFormat.unescapeText("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\""))
        .isEqualTo("\0\001\007\b\f\n\r\t\013\\\'\"");
    assertThat(TextFormat.escapeText(ESCAPE_TEST_STRING)).isEqualTo(ESCAPE_TEST_STRING_ESCAPED);
    assertThat(TextFormat.unescapeText(ESCAPE_TEST_STRING_ESCAPED)).isEqualTo(ESCAPE_TEST_STRING);

    // Invariant
    assertThat(TextFormat.escapeBytes(bytes("hello"))).isEqualTo("hello");
    assertThat(TextFormat.escapeText("hello")).isEqualTo("hello");
    assertThat(TextFormat.unescapeBytes("hello")).isEqualTo(bytes("hello"));
    assertThat(TextFormat.unescapeText("hello")).isEqualTo("hello");

    // Unicode handling.
    assertThat(TextFormat.escapeText("\u1234")).isEqualTo("\\341\\210\\264");
    assertThat(TextFormat.escapeBytes(bytes(0xe1, 0x88, 0xb4))).isEqualTo("\\341\\210\\264");
    assertThat(TextFormat.unescapeText("\\341\\210\\264")).isEqualTo("\u1234");
    assertThat(TextFormat.unescapeBytes("\\341\\210\\264")).isEqualTo(bytes(0xe1, 0x88, 0xb4));
    assertThat(TextFormat.unescapeText("\\xe1\\x88\\xb4")).isEqualTo("\u1234");
    assertThat(TextFormat.unescapeBytes("\\xe1\\x88\\xb4")).isEqualTo(bytes(0xe1, 0x88, 0xb4));
    assertThat(TextFormat.unescapeText("\\u1234")).isEqualTo("\u1234");
    assertThat(TextFormat.unescapeBytes("\\u1234")).isEqualTo(bytes(0xe1, 0x88, 0xb4));
    assertThat(TextFormat.unescapeBytes("\\U00001234")).isEqualTo(bytes(0xe1, 0x88, 0xb4));
    assertThat(TextFormat.unescapeText("\\xf0\\x90\\x90\\xb7"))
        .isEqualTo(new String(new int[] {0x10437}, 0, 1));
    assertThat(TextFormat.unescapeBytes("\\U00010437")).isEqualTo(bytes(0xf0, 0x90, 0x90, 0xb7));

    // Handling of strings with unescaped Unicode characters > 255.
    final String zh = "\u9999\u6e2f\u4e0a\u6d77\ud84f\udf80\u8c50\u9280\u884c";
    ByteString zhByteString = ByteString.copyFromUtf8(zh);
    assertThat(TextFormat.unescapeBytes(zh)).isEqualTo(zhByteString);

    // Errors.
    try {
      TextFormat.unescapeText("\\x");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      // success
    }

    try {
      TextFormat.unescapeText("\\z");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      // success
    }

    try {
      TextFormat.unescapeText("\\");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      // success
    }

    try {
      TextFormat.unescapeText("\\u");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\u' with too few hex chars");
    }

    try {
      TextFormat.unescapeText("\\ud800");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\u' refers to a surrogate");
    }

    try {
      TextFormat.unescapeText("\\ud800\\u1234");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\u' refers to a surrogate");
    }

    try {
      TextFormat.unescapeText("\\udc00");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\u' refers to a surrogate");
    }

    try {
      TextFormat.unescapeText("\\ud801\\udc37");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\u' refers to a surrogate");
    }

    try {
      TextFormat.unescapeText("\\U1234");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\U' with too few hex chars");
    }

    try {
      TextFormat.unescapeText("\\U1234no more hex");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\U' with too few hex chars");
    }

    try {
      TextFormat.unescapeText("\\U00110000");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\U00110000' is not a valid code point value");
    }

    try {
      TextFormat.unescapeText("\\U0000d801\\U00000dc37");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.InvalidEscapeSequenceException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo("Invalid escape sequence: '\\U0000d801' refers to a surrogate code unit");
    }
  }

  @Test
  public void testParseInteger() throws Exception {
    assertThat(TextFormat.parseInt32("0")).isEqualTo(0);
    assertThat(TextFormat.parseInt32("1")).isEqualTo(1);
    assertThat(TextFormat.parseInt32("-1")).isEqualTo(-1);
    assertThat(TextFormat.parseInt32("12345")).isEqualTo(12345);
    assertThat(TextFormat.parseInt32("-12345")).isEqualTo(-12345);
    assertThat(TextFormat.parseInt32("2147483647")).isEqualTo(2147483647);
    assertThat(TextFormat.parseInt32("-2147483648")).isEqualTo(-2147483648);

    assertThat(TextFormat.parseUInt32("0")).isEqualTo(0);
    assertThat(TextFormat.parseUInt32("1")).isEqualTo(1);
    assertThat(TextFormat.parseUInt32("12345")).isEqualTo(12345);
    assertThat(TextFormat.parseUInt32("2147483647")).isEqualTo(2147483647);
    assertThat(TextFormat.parseUInt32("2147483648")).isEqualTo((int) 2147483648L);
    assertThat(TextFormat.parseUInt32("4294967295")).isEqualTo((int) 4294967295L);

    assertThat(TextFormat.parseInt64("0")).isEqualTo(0L);
    assertThat(TextFormat.parseInt64("1")).isEqualTo(1L);
    assertThat(TextFormat.parseInt64("-1")).isEqualTo(-1L);
    assertThat(TextFormat.parseInt64("12345")).isEqualTo(12345L);
    assertThat(TextFormat.parseInt64("-12345")).isEqualTo(-12345L);
    assertThat(TextFormat.parseInt64("2147483647")).isEqualTo(2147483647L);
    assertThat(TextFormat.parseInt64("-2147483648")).isEqualTo(-2147483648L);
    assertThat(TextFormat.parseInt64("4294967295")).isEqualTo(4294967295L);
    assertThat(TextFormat.parseInt64("4294967296")).isEqualTo(4294967296L);
    assertThat(TextFormat.parseInt64("9223372036854775807")).isEqualTo(9223372036854775807L);
    assertThat(TextFormat.parseInt64("-9223372036854775808")).isEqualTo(-9223372036854775808L);

    assertThat(TextFormat.parseUInt64("0")).isEqualTo(0L);
    assertThat(TextFormat.parseUInt64("1")).isEqualTo(1L);
    assertThat(TextFormat.parseUInt64("12345")).isEqualTo(12345L);
    assertThat(TextFormat.parseUInt64("2147483647")).isEqualTo(2147483647L);
    assertThat(TextFormat.parseUInt64("4294967295")).isEqualTo(4294967295L);
    assertThat(TextFormat.parseUInt64("4294967296")).isEqualTo(4294967296L);
    assertThat(TextFormat.parseUInt64("9223372036854775807")).isEqualTo(9223372036854775807L);
    assertThat(TextFormat.parseUInt64("9223372036854775808")).isEqualTo(-9223372036854775808L);
    assertThat(TextFormat.parseUInt64("18446744073709551615")).isEqualTo(-1L);

    // Hex
    assertThat(TextFormat.parseInt32("0x1234abcd")).isEqualTo(0x1234abcd);
    assertThat(TextFormat.parseInt32("-0x1234abcd")).isEqualTo(-0x1234abcd);
    assertThat(TextFormat.parseUInt64("0xffffffffffffffff")).isEqualTo(-1);
    assertThat(TextFormat.parseInt64("0x7fffffffffffffff")).isEqualTo(0x7fffffffffffffffL);

    // Octal
    assertThat(TextFormat.parseInt32("01234567")).isEqualTo(01234567);

    // Out-of-range
    try {
      TextFormat.parseInt32("2147483648");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseInt32("-2147483649");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt32("4294967296");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt32("-1");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseInt64("9223372036854775808");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseInt64("-9223372036854775809");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt64("18446744073709551616");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    try {
      TextFormat.parseUInt64("-1");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }

    // Not a number.
    try {
      TextFormat.parseInt32("abcd");
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (NumberFormatException e) {
      // success
    }
  }

  @Test
  public void testParseString() throws Exception {
    final String zh = "\u9999\u6e2f\u4e0a\u6d77\ud84f\udf80\u8c50\u9280\u884c";
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_string: \"" + zh + "\"", builder);
    assertThat(builder.getOptionalString()).isEqualTo(zh);
  }

  @Test
  public void testParseLongString() throws Exception {
    String longText =
        "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890"
            + "123456789012345678901234567890123456789012345678901234567890";

    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_string: \"" + longText + "\"", builder);
    assertThat(builder.getOptionalString()).isEqualTo(longText);
  }

  @Test
  public void testParseBoolean() throws Exception {
    String goodText =
        "repeated_bool: t  repeated_bool : 0\n"
            + "repeated_bool :f repeated_bool:1\n"
            + "repeated_bool: False repeated_bool: True";
    String goodTextCanonical =
        "repeated_bool: true\n"
            + "repeated_bool: false\n"
            + "repeated_bool: false\n"
            + "repeated_bool: true\n"
            + "repeated_bool: false\n"
            + "repeated_bool: true\n";
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(goodText, builder);
    assertThat(builder.build().toString()).isEqualTo(goodTextCanonical);

    try {
      TestAllTypes.Builder badBuilder = TestAllTypes.newBuilder();
      TextFormat.merge("optional_bool:2", badBuilder);
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.ParseException e) {
      // success
    }
    try {
      TestAllTypes.Builder badBuilder = TestAllTypes.newBuilder();
      TextFormat.merge("optional_bool: foo", badBuilder);
      assertWithMessage("Should have thrown an exception.").fail();
    } catch (TextFormat.ParseException e) {
      // success
    }
  }

  @Test
  public void testParseAdjacentStringLiterals() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge("optional_string: \"foo\" 'corge' \"grault\"", builder);
    assertThat(builder.getOptionalString()).isEqualTo("foocorgegrault");
  }

  @Test
  public void testPrintFieldValue() throws Exception {
    assertPrintFieldValue("\"Hello\"", "Hello", "repeated_string");
    assertPrintFieldValue("123.0", 123f, "repeated_float");
    assertPrintFieldValue("123.0", 123d, "repeated_double");
    assertPrintFieldValue("123", 123, "repeated_int32");
    assertPrintFieldValue("123", 123L, "repeated_int64");
    assertPrintFieldValue("true", true, "repeated_bool");
    assertPrintFieldValue("4294967295", 0xFFFFFFFF, "repeated_uint32");
    assertPrintFieldValue("18446744073709551615", 0xFFFFFFFFFFFFFFFFL, "repeated_uint64");
    assertPrintFieldValue(
        "\"\\001\\002\\003\"", ByteString.copyFrom(new byte[] {1, 2, 3}), "repeated_bytes");
  }

  private void assertPrintFieldValue(String expect, Object value, String fieldName)
      throws Exception {
    StringBuilder sb = new StringBuilder();
    TextFormat.printer()
        .printFieldValue(TestAllTypes.getDescriptor().findFieldByName(fieldName), value, sb);
    assertThat(sb.toString()).isEqualTo(expect);
  }

  @Test
  public void testPrintFieldValueThrows() throws Exception {
    assertPrintFieldThrowsClassCastException(5, "repeated_string");
    assertPrintFieldThrowsClassCastException(5L, "repeated_string");
    assertPrintFieldThrowsClassCastException(ByteString.EMPTY, "repeated_string");
    assertPrintFieldThrowsClassCastException(5, "repeated_float");
    assertPrintFieldThrowsClassCastException(5D, "repeated_float");
    assertPrintFieldThrowsClassCastException("text", "repeated_float");
    assertPrintFieldThrowsClassCastException(5, "repeated_double");
    assertPrintFieldThrowsClassCastException(5F, "repeated_double");
    assertPrintFieldThrowsClassCastException("text", "repeated_double");
    assertPrintFieldThrowsClassCastException(123L, "repeated_int32");
    assertPrintFieldThrowsClassCastException(123, "repeated_int64");
    assertPrintFieldThrowsClassCastException(1, "repeated_bytes");
  }

  private void assertPrintFieldThrowsClassCastException(final Object value, String fieldName)
      throws Exception {
    final StringBuilder stringBuilder = new StringBuilder();
    final FieldDescriptor fieldDescriptor = TestAllTypes.getDescriptor().findFieldByName(fieldName);
    assertThrows(
        ClassCastException.class,
        new ThrowingRunnable() {
          @Override
          public void run() throws Throwable {
            TextFormat.printer().printFieldValue(fieldDescriptor, value, stringBuilder);
          }
        });
  }

  @Test
  public void testShortDebugString() {
    assertThat(
            TextFormat.shortDebugString(
                TestAllTypes.newBuilder()
                    .addRepeatedInt32(1)
                    .addRepeatedUint32(2)
                    .setOptionalNestedMessage(NestedMessage.newBuilder().setBb(42).build())
                    .build()))
        .isEqualTo("optional_nested_message { bb: 42 } repeated_int32: 1 repeated_uint32: 2");
  }

  @Test
  public void testShortDebugString_field() {
    final FieldDescriptor dataField = OneString.getDescriptor().findFieldByName("data");
    assertThat(TextFormat.printer().shortDebugString(dataField, "test data"))
        .isEqualTo("data: \"test data\"");

    final FieldDescriptor optionalField =
        TestAllTypes.getDescriptor().findFieldByName("optional_nested_message");
    final Object value = NestedMessage.newBuilder().setBb(42).build();

    assertThat(TextFormat.printer().shortDebugString(optionalField, value))
        .isEqualTo("optional_nested_message { bb: 42 }");
  }

  @Test
  public void testShortDebugString_unknown() {
    assertThat(TextFormat.printer().shortDebugString(makeUnknownFieldSet()))
        .isEqualTo(
            "5: 1 5: 0x00000002 5: 0x0000000000000003 5: \"4\" 5: { 12: 6 } 5 { 10: 5 }"
                + " 8: 1 8: 2 8: 3 15: 12379813812177893520 15: 0xabcd1234 15:"
                + " 0xabcdef1234567890");
  }

  @Test
  public void testPrintToUnicodeString() throws Exception {
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(
                    TestAllTypes.newBuilder()
                        .setOptionalString("abc\u3042efg")
                        .setOptionalBytes(bytes(0xe3, 0x81, 0x82))
                        .addRepeatedString("\u3093XYZ")
                        .build()))
        .isEqualTo(
            "optional_string: \"abc\u3042efg\"\n"
                + "optional_bytes: \"\\343\\201\\202\"\n"
                + "repeated_string: \"\u3093XYZ\"\n");

    // Double quotes and backslashes should be escaped
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(TestAllTypes.newBuilder().setOptionalString("a\\bc\"ef\"g").build()))
        .isEqualTo("optional_string: \"a\\\\bc\\\"ef\\\"g\"\n");

    // Test escaping roundtrip
    TestAllTypes message = TestAllTypes.newBuilder().setOptionalString("a\\bc\\\"ef\"g").build();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(TextFormat.printer().escapingNonAscii(false).printToString(message), builder);
    assertThat(builder.getOptionalString()).isEqualTo(message.getOptionalString());
  }

  @Test
  public void testPrintToUnicodeStringWithNewlines() throws Exception {
    // No newlines at start and end
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(
                    TestAllTypes.newBuilder()
                        .setOptionalString("test newlines\n\nin\nstring")
                        .build()))
        .isEqualTo("optional_string: \"test newlines\\n\\nin\\nstring\"\n");

    // Newlines at start and end
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(
                    TestAllTypes.newBuilder()
                        .setOptionalString("\ntest\nnewlines\n\nin\nstring\n")
                        .build()))
        .isEqualTo("optional_string: \"\\ntest\\nnewlines\\n\\nin\\nstring\\n\"\n");

    // Strings with 0, 1 and 2 newlines.
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(TestAllTypes.newBuilder().setOptionalString("").build()))
        .isEqualTo("optional_string: \"\"\n");
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(TestAllTypes.newBuilder().setOptionalString("\n").build()))
        .isEqualTo("optional_string: \"\\n\"\n");
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(TestAllTypes.newBuilder().setOptionalString("\n\n").build()))
        .isEqualTo("optional_string: \"\\n\\n\"\n");

    // Test escaping roundtrip
    TestAllTypes message =
        TestAllTypes.newBuilder().setOptionalString("\ntest\nnewlines\n\nin\nstring\n").build();
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();
    TextFormat.merge(TextFormat.printer().escapingNonAscii(false).printToString(message), builder);
    assertThat(builder.getOptionalString()).isEqualTo(message.getOptionalString());
  }

  @Test
  public void testPrintToUnicodeString_unknown() {
    assertThat(
            TextFormat.printer()
                .escapingNonAscii(false)
                .printToString(
                    UnknownFieldSet.newBuilder()
                        .addField(
                            1,
                            UnknownFieldSet.Field.newBuilder()
                                .addLengthDelimited(bytes(0xe3, 0x81, 0x82))
                                .build())
                        .build()))
        .isEqualTo("1: \"\\343\\201\\202\"\n");
  }

  @Test
  public void testParseUnknownExtensions() throws Exception {
    TestUtil.TestLogHandler logHandler = new TestUtil.TestLogHandler();
    Logger logger = Logger.getLogger(TextFormat.class.getName());
    logger.addHandler(logHandler);
    // Test unknown extension can pass.
    assertParseSuccessWithUnknownExtensions("[unknown_extension]: 123");
    assertParseSuccessWithUnknownExtensions(
        "[unknown_extension]: 123\n" + "[unknown_ext]: inf\n" + "[unknown]: 1.234");
    // Test warning messages.
    assertThat(logHandler.getStoredLogRecords().get(0).getMessage())
        .isEqualTo(
            "Input contains unknown fields and/or extensions:\n"
                + "1:2:\tprotobuf_unittest.TestAllTypes.[unknown_extension]");
    assertThat(logHandler.getStoredLogRecords().get(1).getMessage())
        .isEqualTo(
            "Input contains unknown fields and/or extensions:\n"
                + "1:2:\tprotobuf_unittest.TestAllTypes.[unknown_extension]\n"
                + "2:2:\tprotobuf_unittest.TestAllTypes.[unknown_ext]\n"
                + "3:2:\tprotobuf_unittest.TestAllTypes.[unknown]");

    // Test unknown field can not pass.
    assertParseErrorWithUnknownExtensions(
        "2:1: Input contains unknown fields and/or extensions:\n"
            + "1:2:\tprotobuf_unittest.TestAllTypes.[unknown_extension]\n"
            + "2:1:\tprotobuf_unittest.TestAllTypes.unknown_field",
        "[unknown_extension]: 1\n" + "unknown_field: 12345");
    assertParseErrorWithUnknownExtensions(
        "3:1: Input contains unknown fields and/or extensions:\n"
            + "1:2:\tprotobuf_unittest.TestAllTypes.[unknown_extension1]\n"
            + "2:2:\tprotobuf_unittest.TestAllTypes.[unknown_extension2]\n"
            + "3:1:\tprotobuf_unittest.TestAllTypes.unknown_field\n"
            + "4:2:\tprotobuf_unittest.TestAllTypes.[unknown_extension3]",
        "[unknown_extension1]: 1\n"
            + "[unknown_extension2]: 2\n"
            + "unknown_field: 12345\n"
            + "[unknown_extension3]: 3\n");
    assertParseErrorWithUnknownExtensions(
        "1:1: Input contains unknown fields and/or extensions:\n"
            + "1:1:\tprotobuf_unittest.TestAllTypes.unknown_field1\n"
            + "2:1:\tprotobuf_unittest.TestAllTypes.unknown_field2\n"
            + "3:2:\tprotobuf_unittest.TestAllTypes.[unknown_extension]\n"
            + "4:1:\tprotobuf_unittest.TestAllTypes.unknown_field3",
        "unknown_field1: 1\n"
            + "unknown_field2: 2\n"
            + "[unknown_extension]: 12345\n"
            + "unknown_field3: 3\n");
  }

  @Test
  public void testParseUnknownExtensionWithAnyMessage() throws Exception {
    assertParseSuccessWithUnknownExtensions(
        "[unknown_extension]: { "
            + "  any_value { "
            + "    [type.googleapis.com/protobuf_unittest.OneString] { "
            + "      data: 123 "
            + "    } "
            + " } "
            + "}");
  }

  // See additional coverage in testOneofOverwriteForbidden and testMapOverwriteForbidden.
  @Test
  public void testParseNonRepeatedFields() throws Exception {
    assertParseSuccessWithOverwriteForbidden("repeated_int32: 1\nrepeated_int32: 2\n");
    assertParseSuccessWithOverwriteForbidden("RepeatedGroup { a: 1 }\nRepeatedGroup { a: 2 }\n");
    assertParseSuccessWithOverwriteForbidden(
        "repeated_nested_message { bb: 1 }\nrepeated_nested_message { bb: 2 }\n");

    assertParseErrorWithOverwriteForbidden(
        "3:15: Non-repeated field "
            + "\"protobuf_unittest.TestAllTypes.optional_int32\" "
            + "cannot be overwritten.",
        "optional_int32: 1\noptional_bool: true\noptional_int32: 1\n");
    assertParseErrorWithOverwriteForbidden(
        "2:1: Non-repeated field "
            + "\"protobuf_unittest.TestAllTypes.optionalgroup\" "
            + "cannot be overwritten.",
        "OptionalGroup { a: 1 }\nOptionalGroup { }\n");
    assertParseErrorWithOverwriteForbidden(
        "2:1: Non-repeated field "
            + "\"protobuf_unittest.TestAllTypes.optional_nested_message\" "
            + "cannot be overwritten.",
        "optional_nested_message { }\noptional_nested_message { bb: 3 }\n");
    assertParseErrorWithOverwriteForbidden(
        "2:14: Non-repeated field "
            + "\"protobuf_unittest.TestAllTypes.default_int32\" "
            + "cannot be overwritten.",
        "default_int32: 41\n"
            + // the default value
            "default_int32: 41\n");
    assertParseErrorWithOverwriteForbidden(
        "2:15: Non-repeated field "
            + "\"protobuf_unittest.TestAllTypes.default_string\" "
            + "cannot be overwritten.",
        "default_string: \"zxcv\"\ndefault_string: \"asdf\"\n");
  }

  @Test
  public void testParseShortRepeatedFormOfRepeatedFields() throws Exception {
    assertParseSuccessWithOverwriteForbidden("repeated_foreign_enum: [FOREIGN_FOO, FOREIGN_BAR]");
    assertParseSuccessWithOverwriteForbidden("repeated_int32: [ 1, 2 ]\n");
    assertParseSuccessWithOverwriteForbidden("RepeatedGroup [{ a: 1 },{ a: 2 }]\n");
    assertParseSuccessWithOverwriteForbidden("repeated_nested_message [{ bb: 1 }, { bb: 2 }]\n");
    // See also testMapShortForm.
  }

  @Test
  public void testParseShortRepeatedFormOfEmptyRepeatedFields() throws Exception {
    assertParseSuccessWithOverwriteForbidden("repeated_foreign_enum: []");
    assertParseSuccessWithOverwriteForbidden("repeated_int32: []\n");
    assertParseSuccessWithOverwriteForbidden("RepeatedGroup []\n");
    assertParseSuccessWithOverwriteForbidden("repeated_nested_message []\n");
    // See also testMapShortFormEmpty.
  }

  @Test
  public void testParseShortRepeatedFormWithTrailingComma() throws Exception {
    assertParseErrorWithOverwriteForbidden(
        "1:38: Expected identifier. Found \']\'", "repeated_foreign_enum: [FOREIGN_FOO, ]\n");
    assertParseErrorWithOverwriteForbidden(
        "1:22: Couldn't parse integer: For input string: \"]\"", "repeated_int32: [ 1, ]\n");
    assertParseErrorWithOverwriteForbidden("1:25: Expected \"{\".", "RepeatedGroup [{ a: 1 },]\n");
    assertParseErrorWithOverwriteForbidden(
        "1:37: Expected \"{\".", "repeated_nested_message [{ bb: 1 }, ]\n");
    // See also testMapShortFormTrailingComma.
  }

  @Test
  public void testParseShortRepeatedFormOfNonRepeatedFields() throws Exception {
    assertParseErrorWithOverwriteForbidden(
        "1:17: Couldn't parse integer: For input string: \"[\"", "optional_int32: [1]\n");
    assertParseErrorWithOverwriteForbidden(
        "1:17: Couldn't parse integer: For input string: \"[\"", "optional_int32: []\n");
  }

  // =======================================================================
  // test oneof

  @Test
  public void testOneofTextFormat() throws Exception {
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    TestUtil.setOneof(builder);
    TestOneof2 message = builder.build();
    TestOneof2.Builder dest = TestOneof2.newBuilder();
    TextFormat.merge(TextFormat.printer().escapingNonAscii(false).printToString(message), dest);
    TestUtil.assertOneofSet(dest.build());
  }

  @Test
  public void testOneofOverwriteForbidden() throws Exception {
    String input = "foo_string: \"stringvalue\" foo_int: 123";
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    try {
      PARSER_WITH_OVERWRITE_FORBIDDEN.merge(input, TestUtil.getFullExtensionRegistry(), builder);
      assertWithMessage("Expected parse exception.").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e)
          .hasMessageThat()
          .isEqualTo(
              "1:34: Field \"protobuf_unittest.TestOneof2.foo_int\""
                  + " is specified along with field \"protobuf_unittest.TestOneof2.foo_string\","
                  + " another member of oneof \"foo\".");
    }
  }

  @Test
  public void testOneofOverwriteAllowed() throws Exception {
    String input = "foo_string: \"stringvalue\" foo_int: 123";
    TestOneof2.Builder builder = TestOneof2.newBuilder();
    DEFAULT_PARSER.merge(input, TestUtil.getFullExtensionRegistry(), builder);
    // Only the last value sticks.
    TestOneof2 oneof = builder.build();
    assertThat(oneof.hasFooString()).isFalse();
    assertThat(oneof.hasFooInt()).isTrue();
  }

  // =======================================================================
  // test map

  @Test
  public void testMapTextFormat() throws Exception {
    TestMap message =
        TestMap.newBuilder()
            .putInt32ToStringField(10, "apple")
            .putInt32ToStringField(20, "banana")
            .putInt32ToStringField(30, "cherry")
            .build();
    String text = TextFormat.printer().escapingNonAscii(false).printToString(message);
    {
      TestMap.Builder dest = TestMap.newBuilder();
      TextFormat.merge(text, dest);
      assertThat(dest.build()).isEqualTo(message);
    }
    {
      TestMap.Builder dest = TestMap.newBuilder();
      PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, dest);
      assertThat(dest.build()).isEqualTo(message);
    }
  }

  @Test
  public void testMapDuplicateKeys() throws Exception {
    String input =
        "int32_to_int32_field: {\n"
            + "  key: 1\n"
            + "  value: 1\n"
            + "}\n"
            + "int32_to_int32_field: {\n"
            + "  key: -2147483647\n"
            + "  value: 5\n"
            + "}\n"
            + "int32_to_int32_field: {\n"
            + "  key: 1\n"
            + "  value: -1\n"
            + "}\n";
    TestMap msg = TextFormat.parse(input, TestMap.class);
    int i1 = msg.getInt32ToInt32FieldMap().get(1);
    TestMap msg2 = TextFormat.parse(msg.toString(), TestMap.class);
    int i2 = msg2.getInt32ToInt32FieldMap().get(1);
    assertThat(i1).isEqualTo(i2);
  }

  @Test
  public void testMapShortForm() throws Exception {
    String text =
        "string_to_int32_field [{ key: 'x' value: 10 }, { key: 'y' value: 20 }]\n"
            + "int32_to_message_field "
            + "[{ key: 1 value { value: 100 } }, { key: 2 value: { value: 200 } }]\n";
    TestMap.Builder dest = TestMap.newBuilder();
    PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, dest);
    TestMap message = dest.build();
    assertThat(message.getStringToInt32FieldMap()).hasSize(2);
    assertThat(message.getInt32ToMessageFieldMap()).hasSize(2);
    assertThat(message.getStringToInt32FieldMap().get("x").intValue()).isEqualTo(10);
    assertThat(message.getInt32ToMessageFieldMap().get(2).getValue()).isEqualTo(200);
  }

  @Test
  public void testMapShortFormEmpty() throws Exception {
    String text = "string_to_int32_field []\nint32_to_message_field: []\n";
    TestMap.Builder dest = TestMap.newBuilder();
    PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, dest);
    TestMap message = dest.build();
    assertThat(message.getStringToInt32FieldMap()).isEmpty();
    assertThat(message.getInt32ToMessageFieldMap()).isEmpty();
  }

  @Test
  public void testMapShortFormTrailingComma() throws Exception {
    String text = "string_to_int32_field [{ key: 'x' value: 10 }, ]\n";
    TestMap.Builder dest = TestMap.newBuilder();
    try {
      PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, dest);
      assertWithMessage("Expected parse exception.").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().isEqualTo("1:48: Expected \"{\".");
    }
  }

  @Test
  public void testMapOverwrite() throws Exception {
    String text =
        "int32_to_int32_field { key: 1 value: 10 }\n"
            + "int32_to_int32_field { key: 2 value: 20 }\n"
            + "int32_to_int32_field { key: 1 value: 30 }\n";

    {
      // With default parser, last value set for the key holds.
      TestMap.Builder builder = TestMap.newBuilder();
      DEFAULT_PARSER.merge(text, builder);
      TestMap map = builder.build();
      assertThat(map.getInt32ToInt32FieldMap()).hasSize(2);
      assertThat(map.getInt32ToInt32FieldMap().get(1).intValue()).isEqualTo(30);
    }

    {
      // With overwrite forbidden, same behavior.
      // TODO: Expect parse exception here.
      TestMap.Builder builder = TestMap.newBuilder();
      PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, builder);
      TestMap map = builder.build();
      assertThat(map.getInt32ToInt32FieldMap()).hasSize(2);
      assertThat(map.getInt32ToInt32FieldMap().get(1).intValue()).isEqualTo(30);
    }

    {
      // With overwrite forbidden and a dynamic message, same behavior.
      // TODO: Expect parse exception here.
      Message.Builder builder = DynamicMessage.newBuilder(TestMap.getDescriptor());
      PARSER_WITH_OVERWRITE_FORBIDDEN.merge(text, builder);
      TestMap map =
          TestMap.parseFrom(
              builder.build().toByteString(), ExtensionRegistryLite.getEmptyRegistry());
      assertThat(map.getInt32ToInt32FieldMap()).hasSize(2);
      assertThat(map.getInt32ToInt32FieldMap().get(1).intValue()).isEqualTo(30);
    }
  }

  // =======================================================================
  // test location information

  @Test
  public void testParseInfoTreeBuilding() throws Exception {
    TestAllTypes.Builder builder = TestAllTypes.newBuilder();

    Descriptor descriptor = TestAllTypes.getDescriptor();
    TextFormatParseInfoTree.Builder treeBuilder = TextFormatParseInfoTree.builder();
    // Set to allow unknown fields
    TextFormat.Parser parser =
        TextFormat.Parser.newBuilder()
            .setAllowUnknownFields(true)
            .setParseInfoTreeBuilder(treeBuilder)
            .build();

    final String stringData =
        "optional_int32: 1\n"
            + "optional_int64: 2\n"
            + "  optional_double: 2.4\n"
            + "repeated_int32: 5\n"
            + "repeated_int32: 10\n"
            + "optional_nested_message <\n"
            + "  bb: 78\n"
            + ">\n"
            + "repeated_nested_message <\n"
            + "  bb: 79\n"
            + ">\n"
            + "repeated_nested_message <\n"
            + "  bb: 80\n"
            + ">";

    parser.merge(stringData, builder);
    TextFormatParseInfoTree tree = treeBuilder.build();

    // Verify that the tree has the correct positions.
    assertLocation(tree, descriptor, "optional_int32", 0, 0, 0);
    assertLocation(tree, descriptor, "optional_int64", 0, 1, 0);
    assertLocation(tree, descriptor, "optional_double", 0, 2, 2);

    assertLocation(tree, descriptor, "repeated_int32", 0, 3, 0);
    assertLocation(tree, descriptor, "repeated_int32", 1, 4, 0);

    assertLocation(tree, descriptor, "optional_nested_message", 0, 5, 0);
    assertLocation(tree, descriptor, "repeated_nested_message", 0, 8, 0);
    assertLocation(tree, descriptor, "repeated_nested_message", 1, 11, 0);

    // Check for fields not set. For an invalid field, the location returned should be -1, -1.
    assertLocation(tree, descriptor, "repeated_int64", 0, -1, -1);
    assertLocation(tree, descriptor, "repeated_int32", 6, -1, -1);

    // Verify inside the nested message.
    FieldDescriptor nestedField = descriptor.findFieldByName("optional_nested_message");

    TextFormatParseInfoTree nestedTree = tree.getNestedTrees(nestedField).get(0);
    assertLocation(nestedTree, nestedField.getMessageType(), "bb", 0, 6, 2);

    // Verify inside another nested message.
    nestedField = descriptor.findFieldByName("repeated_nested_message");
    nestedTree = tree.getNestedTrees(nestedField).get(0);
    assertLocation(nestedTree, nestedField.getMessageType(), "bb", 0, 9, 2);

    nestedTree = tree.getNestedTrees(nestedField).get(1);
    assertLocation(nestedTree, nestedField.getMessageType(), "bb", 0, 12, 2);

    // Verify a NULL tree for an unknown nested field.
    try {
      tree.getNestedTree(nestedField, 2);
      assertWithMessage("unknown nested field should throw").fail();
    } catch (IllegalArgumentException unused) {
      // pass
    }
  }

  @SuppressWarnings("LenientFormatStringValidation")
  private void assertLocation(
      TextFormatParseInfoTree tree,
      final Descriptor descriptor,
      final String fieldName,
      int index,
      int line,
      int column) {
    List<TextFormatParseLocation> locs = tree.getLocations(descriptor.findFieldByName(fieldName));
    if (index < locs.size()) {
      TextFormatParseLocation location = locs.get(index);
      TextFormatParseLocation expected = TextFormatParseLocation.create(line, column);
      assertThat(location).isEqualTo(expected);
    } else if (line != -1 && column != -1) {
      // Expected 0 args, but got 3.
      assertWithMessage(
              "Tree/descriptor/fieldname did not contain index %d, line %d column %d expected",
              index, line, column)
          .fail();
    }
  }

  @Test
  public void testSortMapFields() throws Exception {
    TestMap message =
        TestMap.newBuilder()
            .putStringToInt32Field("cherry", 30)
            .putStringToInt32Field("banana", 20)
            .putStringToInt32Field("apple", 10)
            .putInt32ToStringField(30, "cherry")
            .putInt32ToStringField(20, "banana")
            .putInt32ToStringField(10, "apple")
            .build();
    String text =
        "int32_to_string_field {\n"
            + "  key: 10\n"
            + "  value: \"apple\"\n"
            + "}\n"
            + "int32_to_string_field {\n"
            + "  key: 20\n"
            + "  value: \"banana\"\n"
            + "}\n"
            + "int32_to_string_field {\n"
            + "  key: 30\n"
            + "  value: \"cherry\"\n"
            + "}\n"
            + "string_to_int32_field {\n"
            + "  key: \"apple\"\n"
            + "  value: 10\n"
            + "}\n"
            + "string_to_int32_field {\n"
            + "  key: \"banana\"\n"
            + "  value: 20\n"
            + "}\n"
            + "string_to_int32_field {\n"
            + "  key: \"cherry\"\n"
            + "  value: 30\n"
            + "}\n";
    assertThat(TextFormat.printer().printToString(message)).isEqualTo(text);
  }

  @Test
  public void testPreservesFloatingPointNegative0() throws Exception {
    proto3_unittest.UnittestProto3.TestAllTypes message =
        proto3_unittest.UnittestProto3.TestAllTypes.newBuilder()
            .setOptionalFloat(-0.0f)
            .setOptionalDouble(-0.0)
            .build();
    assertThat(TextFormat.printer().printToString(message))
        .isEqualTo("optional_float: -0.0\noptional_double: -0.0\n");
  }

  private TestRecursiveMessage makeRecursiveMessage(int depth) {
    if (depth == 0) {
      return TestRecursiveMessage.newBuilder().setI(5).build();
    } else {
      return TestRecursiveMessage.newBuilder().setA(makeRecursiveMessage(depth - 1)).build();
    }
  }

  @Test
  public void testDefaultRecursionLimit() throws Exception {
    String depth100 = TextFormat.printer().printToString(makeRecursiveMessage(100));
    String depth101 = TextFormat.printer().printToString(makeRecursiveMessage(101));
    TextFormat.parse(depth100, TestRecursiveMessage.class);
    try {
      TextFormat.parse(depth101, TestRecursiveMessage.class);
      assertWithMessage("Parsing deep message should have failed").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().contains("too deep");
    }
  }

  @Test
  public void testRecursionLimitWithUnknownFields() throws Exception {
    TextFormat.Parser parser =
        TextFormat.Parser.newBuilder().setAllowUnknownFields(true).setRecursionLimit(2).build();
    TestRecursiveMessage.Builder depth2 = TestRecursiveMessage.newBuilder();
    parser.merge("u { u { i: 0 } }", depth2);
    try {
      TestRecursiveMessage.Builder depth3 = TestRecursiveMessage.newBuilder();
      parser.merge("u { u { u { } } }", depth3);
      assertWithMessage("Parsing deep message should have failed").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().contains("too deep");
    }
  }

  @Test
  public void testRecursionLimitWithKnownAndUnknownFields() throws Exception {
    TextFormat.Parser parser =
        TextFormat.Parser.newBuilder().setAllowUnknownFields(true).setRecursionLimit(2).build();
    TestRecursiveMessage.Builder depth2 = TestRecursiveMessage.newBuilder();
    parser.merge("a { u { i: 0 } }", depth2);
    try {
      TestRecursiveMessage.Builder depth3 = TestRecursiveMessage.newBuilder();
      parser.merge("a { u { u { } } }", depth3);
      assertWithMessage("Parsing deep message should have failed").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().contains("too deep");
    }
  }

  @Test
  public void testRecursionLimitWithAny() throws Exception {
    TextFormat.Parser parser =
        TextFormat.Parser.newBuilder()
            .setRecursionLimit(2)
            .setTypeRegistry(TypeRegistry.newBuilder().add(TestAllTypes.getDescriptor()).build())
            .build();
    TestAny.Builder depth2 = TestAny.newBuilder();
    parser.merge(
        "value { [type.googleapis.com/protobuf_unittest.TestAllTypes] { optional_int32: 1 } }",
        depth2);
    try {
      TestAny.Builder depth3 = TestAny.newBuilder();
      parser.merge(
          "value { [type.googleapis.com/protobuf_unittest.TestAllTypes] { optional_nested_message {"
              + "} } }",
          depth3);
      assertWithMessage("Parsing deep message should have failed").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().contains("too deep");
    }
  }

  @Test
  public void testRecursionLimitWithTopLevelAny() throws Exception {
    TextFormat.Parser parser =
        TextFormat.Parser.newBuilder()
            .setRecursionLimit(2)
            .setTypeRegistry(
                TypeRegistry.newBuilder().add(TestRecursiveMessage.getDescriptor()).build())
            .build();
    Any.Builder depth2 = Any.newBuilder();
    parser.merge(
        "[type.googleapis.com/protobuf_unittest.TestRecursiveMessage] { a { i: 0 } }", depth2);
    try {
      Any.Builder depth3 = Any.newBuilder();
      parser.merge(
          "[type.googleapis.com/protobuf_unittest.TestRecursiveMessage] { a { a { i: 0 } } }",
          depth3);
      assertWithMessage("Parsing deep message should have failed").fail();
    } catch (TextFormat.ParseException e) {
      assertThat(e).hasMessageThat().contains("too deep");
    }
  }
}
