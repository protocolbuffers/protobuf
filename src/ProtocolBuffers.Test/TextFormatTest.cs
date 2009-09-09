#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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
#endregion

using System;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;
using System.Globalization;
using System.Threading;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class TextFormatTest {

    private static readonly string AllFieldsSetText = TestUtil.ReadTextFromFile("text_format_unittest_data.txt");
    private static readonly string AllExtensionsSetText = TestUtil.ReadTextFromFile("text_format_unittest_extensions_data.txt");

    /// <summary>
    /// Note that this is slightly different to the Java - 123.0 becomes 123, and 1.23E17 becomes 1.23E+17.
    /// Both of these differences can be parsed by the Java and the C++, and we can parse their output too.
    /// </summary>
    private const string ExoticText =
      "repeated_int32: -1\n" +
      "repeated_int32: -2147483648\n" +
      "repeated_int64: -1\n" +
      "repeated_int64: -9223372036854775808\n" +
      "repeated_uint32: 4294967295\n" +
      "repeated_uint32: 2147483648\n" +
      "repeated_uint64: 18446744073709551615\n" +
      "repeated_uint64: 9223372036854775808\n" +
      "repeated_double: 123\n" +
      "repeated_double: 123.5\n" +
      "repeated_double: 0.125\n" +
      "repeated_double: 1.23E+17\n" +
      "repeated_double: 1.235E+22\n" +
      "repeated_double: 1.235E-18\n" +
      "repeated_double: 123.456789\n" +
      "repeated_double: Infinity\n" +
      "repeated_double: -Infinity\n" +
      "repeated_double: NaN\n" +
      "repeated_string: \"\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"" +
        "\\341\\210\\264\"\n" +
      "repeated_bytes: \"\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"\\376\"\n";

    private const string MessageSetText =
      "[protobuf_unittest.TestMessageSetExtension1] {\n" +
      "  i: 123\n" +
      "}\n" +
      "[protobuf_unittest.TestMessageSetExtension2] {\n" +
      "  str: \"foo\"\n" +
      "}\n";
    
    /// <summary>
    /// Print TestAllTypes and compare with golden file. 
    /// </summary>
    [Test]
    public void PrintMessage() {
      TestUtil.TestInMultipleCultures(() => {
        string text = TextFormat.PrintToString(TestUtil.GetAllSet());
        Assert.AreEqual(AllFieldsSetText.Replace("\r\n", "\n"), text.Replace("\r\n", "\n"));
      });
    }

    /// <summary>
    /// Print TestAllExtensions and compare with golden file.
    /// </summary>
    [Test]
    public void PrintExtensions() {
      string text = TextFormat.PrintToString(TestUtil.GetAllExtensionsSet());

      Assert.AreEqual(AllExtensionsSetText.Replace("\r\n", "\n"), text.Replace("\r\n", "\n"));
    }

    /// <summary>
    /// Test printing of unknown fields in a message.
    /// </summary>
    [Test]
    public void PrintUnknownFields() {
      TestEmptyMessage message =
        TestEmptyMessage.CreateBuilder()
          .SetUnknownFields(
            UnknownFieldSet.CreateBuilder()
              .AddField(5,
                UnknownField.CreateBuilder()
                  .AddVarint(1)
                  .AddFixed32(2)
                  .AddFixed64(3)
                  .AddLengthDelimited(ByteString.CopyFromUtf8("4"))
                  .AddGroup(
                    UnknownFieldSet.CreateBuilder()
                      .AddField(10,
                        UnknownField.CreateBuilder()
                          .AddVarint(5)
                          .Build())
                      .Build())
                  .Build())
              .AddField(8,
                UnknownField.CreateBuilder()
                  .AddVarint(1)
                  .AddVarint(2)
                  .AddVarint(3)
                  .Build())
              .AddField(15,
                UnknownField.CreateBuilder()
                  .AddVarint(0xABCDEF1234567890L)
                  .AddFixed32(0xABCD1234)
                  .AddFixed64(0xABCDEF1234567890L)
                  .Build())
              .Build())
          .Build();

      Assert.AreEqual(
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
        TextFormat.PrintToString(message));
    }

    /// <summary>
    /// Helper to construct a ByteString from a string containing only 8-bit
    /// characters. The characters are converted directly to bytes, *not*
    /// encoded using UTF-8.
    /// </summary>
    private static ByteString Bytes(string str) {
      return ByteString.CopyFrom(Encoding.GetEncoding(28591).GetBytes(str));
    }   

    [Test]
    public void PrintExotic() {
      IMessage message = TestAllTypes.CreateBuilder()
        // Signed vs. unsigned numbers.
        .AddRepeatedInt32 (-1)
        .AddRepeatedUint32(uint.MaxValue)
        .AddRepeatedInt64 (-1)
        .AddRepeatedUint64(ulong.MaxValue)

        .AddRepeatedInt32 (1  << 31)
        .AddRepeatedUint32(1U  << 31)
        .AddRepeatedInt64 (1L << 63)
        .AddRepeatedUint64(1UL << 63)

        // Floats of various precisions and exponents.
        .AddRepeatedDouble(123)
        .AddRepeatedDouble(123.5)
        .AddRepeatedDouble(0.125)
        .AddRepeatedDouble(123e15)
        .AddRepeatedDouble(123.5e20)
        .AddRepeatedDouble(123.5e-20)
        .AddRepeatedDouble(123.456789)
        .AddRepeatedDouble(Double.PositiveInfinity)
        .AddRepeatedDouble(Double.NegativeInfinity)
        .AddRepeatedDouble(Double.NaN)

        // Strings and bytes that needing escaping.
        .AddRepeatedString("\0\u0001\u0007\b\f\n\r\t\v\\\'\"\u1234")
        .AddRepeatedBytes(Bytes("\0\u0001\u0007\b\f\n\r\t\v\\\'\"\u00fe"))
        .Build();

      Assert.AreEqual(ExoticText, message.ToString());
    }

    [Test]
    public void PrintMessageSet() {
      TestMessageSet messageSet =
        TestMessageSet.CreateBuilder()
          .SetExtension(
            TestMessageSetExtension1.MessageSetExtension,
            TestMessageSetExtension1.CreateBuilder().SetI(123).Build())
          .SetExtension(
            TestMessageSetExtension2.MessageSetExtension,
            TestMessageSetExtension2.CreateBuilder().SetStr("foo").Build())
          .Build();

      Assert.AreEqual(MessageSetText, messageSet.ToString());
    }

    // =================================================================

    [Test]
    public void Parse() {
      TestUtil.TestInMultipleCultures(() => {
        TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
        TextFormat.Merge(AllFieldsSetText, builder);
        TestUtil.AssertAllFieldsSet(builder.Build());
      });
    }

    [Test]
    public void ParseReader() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TextFormat.Merge(new StringReader(AllFieldsSetText), builder);
      TestUtil.AssertAllFieldsSet(builder.Build());
    }

    [Test]
    public void ParseExtensions() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      TextFormat.Merge(AllExtensionsSetText,
                       TestUtil.CreateExtensionRegistry(),
                       builder);
      TestUtil.AssertAllExtensionsSet(builder.Build());
    }

    [Test]
    public void ParseCompatibility() {
      string original = "repeated_float: inf\n" +
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
      string canonical = "repeated_float: Infinity\n" +
                          "repeated_float: -Infinity\n" +
                          "repeated_float: NaN\n" +
                          "repeated_float: Infinity\n" +
                          "repeated_float: -Infinity\n" +
                          "repeated_float: NaN\n" +
                          "repeated_float: 1\n" + // Java has 1.0; this is fine
                          "repeated_float: Infinity\n" +
                          "repeated_float: -Infinity\n" +
                          "repeated_double: Infinity\n" +
                          "repeated_double: -Infinity\n" +
                          "repeated_double: NaN\n";
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TextFormat.Merge(original, builder);
      Assert.AreEqual(canonical, builder.Build().ToString());
    }

    [Test]
    public void ParseExotic() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TextFormat.Merge(ExoticText, builder);

      // Too lazy to check things individually.  Don't try to debug this
      // if testPrintExotic() is Assert.Failing.
      Assert.AreEqual(ExoticText, builder.Build().ToString());
    }

    [Test]
    public void ParseMessageSet() {
      ExtensionRegistry extensionRegistry = ExtensionRegistry.CreateInstance();
      extensionRegistry.Add(TestMessageSetExtension1.MessageSetExtension);
      extensionRegistry.Add(TestMessageSetExtension2.MessageSetExtension);

      TestMessageSet.Builder builder = TestMessageSet.CreateBuilder();
      TextFormat.Merge(MessageSetText, extensionRegistry, builder);
      TestMessageSet messageSet = builder.Build();

      Assert.IsTrue(messageSet.HasExtension(TestMessageSetExtension1.MessageSetExtension));
      Assert.AreEqual(123, messageSet.GetExtension(TestMessageSetExtension1.MessageSetExtension).I);
      Assert.IsTrue(messageSet.HasExtension(TestMessageSetExtension2.MessageSetExtension));
      Assert.AreEqual("foo", messageSet.GetExtension(TestMessageSetExtension2.MessageSetExtension).Str);
    }

    [Test]
    public void ParseNumericEnum() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TextFormat.Merge("optional_nested_enum: 2", builder);
      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR, builder.OptionalNestedEnum);
    }

    [Test]
    public void ParseAngleBrackets() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TextFormat.Merge("OptionalGroup: < a: 1 >", builder);
      Assert.IsTrue(builder.HasOptionalGroup);
      Assert.AreEqual(1, builder.OptionalGroup.A);
    }

    [Test]
    public void ParseComment() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TextFormat.Merge(
        "# this is a comment\n" +
        "optional_int32: 1  # another comment\n" +
        "optional_int64: 2\n" +
        "# EOF comment", builder);
      Assert.AreEqual(1, builder.OptionalInt32);
      Assert.AreEqual(2, builder.OptionalInt64);
    }


    private static void AssertParseError(string error, string text) {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      try {
        TextFormat.Merge(text, TestUtil.CreateExtensionRegistry(), builder);
        Assert.Fail("Expected parse exception.");
      } catch (FormatException e) {
        Assert.AreEqual(error, e.Message);
      }
    }

    [Test]
    public void ParseErrors() {
      AssertParseError(
        "1:16: Expected \":\".",
        "optional_int32 123");
      AssertParseError(
        "1:23: Expected identifier.",
        "optional_nested_enum: ?");
      AssertParseError(
        "1:18: Couldn't parse integer: Number must be positive: -1",
        "optional_uint32: -1");
      AssertParseError(
        "1:17: Couldn't parse integer: Number out of range for 32-bit signed " +
          "integer: 82301481290849012385230157",
        "optional_int32: 82301481290849012385230157");
      AssertParseError(
        "1:16: Expected \"true\" or \"false\".",
        "optional_bool: maybe");
      AssertParseError(
        "1:18: Expected string.",
        "optional_string: 123");
      AssertParseError(
        "1:18: String missing ending quote.",
        "optional_string: \"ueoauaoe");
      AssertParseError(
        "1:18: String missing ending quote.",
        "optional_string: \"ueoauaoe\n" +
        "optional_int32: 123");
      AssertParseError(
        "1:18: Invalid escape sequence: '\\z'",
        "optional_string: \"\\z\"");
      AssertParseError(
        "1:18: String missing ending quote.",
        "optional_string: \"ueoauaoe\n" +
        "optional_int32: 123");
      AssertParseError(
        "1:2: Extension \"nosuchext\" not found in the ExtensionRegistry.",
        "[nosuchext]: 123");
      AssertParseError(
        "1:20: Extension \"protobuf_unittest.optional_int32_extension\" does " +
          "not extend message type \"protobuf_unittest.TestAllTypes\".",
        "[protobuf_unittest.optional_int32_extension]: 123");
      AssertParseError(
        "1:1: Message type \"protobuf_unittest.TestAllTypes\" has no field " +
          "named \"nosuchfield\".",
        "nosuchfield: 123");
      AssertParseError(
        "1:21: Expected \">\".",
        "OptionalGroup < a: 1");
      AssertParseError(
        "1:23: Enum type \"protobuf_unittest.TestAllTypes.NestedEnum\" has no " +
          "value named \"NO_SUCH_VALUE\".",
        "optional_nested_enum: NO_SUCH_VALUE");
      AssertParseError(
        "1:23: Enum type \"protobuf_unittest.TestAllTypes.NestedEnum\" has no " +
          "value with number 123.",
        "optional_nested_enum: 123");

      // Delimiters must match.
      AssertParseError(
        "1:22: Expected identifier.",
        "OptionalGroup < a: 1 }");
      AssertParseError(
        "1:22: Expected identifier.",
        "OptionalGroup { a: 1 >");
    }

    // =================================================================

    private static ByteString Bytes(params byte[] bytes) {
      return ByteString.CopyFrom(bytes);
    }

    private delegate void FormattingAction();

    private static void AssertFormatException(FormattingAction action) {
      try {
        action();
        Assert.Fail("Should have thrown an exception.");
      } catch (FormatException) {
        // success
      }
    }

    [Test]
    public void Escape() {
      // Escape sequences.
      Assert.AreEqual("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"",
        TextFormat.EscapeBytes(Bytes("\0\u0001\u0007\b\f\n\r\t\v\\\'\"")));
      Assert.AreEqual("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"",
        TextFormat.EscapeText("\0\u0001\u0007\b\f\n\r\t\v\\\'\""));
      Assert.AreEqual(Bytes("\0\u0001\u0007\b\f\n\r\t\v\\\'\""),
        TextFormat.UnescapeBytes("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\""));
      Assert.AreEqual("\0\u0001\u0007\b\f\n\r\t\v\\\'\"",
        TextFormat.UnescapeText("\\000\\001\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\""));

      // Unicode handling.
      Assert.AreEqual("\\341\\210\\264", TextFormat.EscapeText("\u1234"));
      Assert.AreEqual("\\341\\210\\264", TextFormat.EscapeBytes(Bytes(0xe1, 0x88, 0xb4)));
      Assert.AreEqual("\u1234", TextFormat.UnescapeText("\\341\\210\\264"));
      Assert.AreEqual(Bytes(0xe1, 0x88, 0xb4), TextFormat.UnescapeBytes("\\341\\210\\264"));
      Assert.AreEqual("\u1234", TextFormat.UnescapeText("\\xe1\\x88\\xb4"));
      Assert.AreEqual(Bytes(0xe1, 0x88, 0xb4), TextFormat.UnescapeBytes("\\xe1\\x88\\xb4"));

      // Errors.
      AssertFormatException(() => TextFormat.UnescapeText("\\x"));
      AssertFormatException(() => TextFormat.UnescapeText("\\z"));
      AssertFormatException(() => TextFormat.UnescapeText("\\"));
    }

    [Test]
    public void ParseInteger() {
      Assert.AreEqual(          0, TextFormat.ParseInt32(          "0"));
      Assert.AreEqual(          1, TextFormat.ParseInt32(          "1"));
      Assert.AreEqual(         -1, TextFormat.ParseInt32(         "-1"));
      Assert.AreEqual(      12345, TextFormat.ParseInt32(      "12345"));
      Assert.AreEqual(     -12345, TextFormat.ParseInt32(     "-12345"));
      Assert.AreEqual( 2147483647, TextFormat.ParseInt32( "2147483647"));
      Assert.AreEqual(-2147483648, TextFormat.ParseInt32("-2147483648"));

      Assert.AreEqual(          0, TextFormat.ParseUInt32(         "0"));
      Assert.AreEqual(          1, TextFormat.ParseUInt32(         "1"));
      Assert.AreEqual(      12345, TextFormat.ParseUInt32(     "12345"));
      Assert.AreEqual( 2147483647, TextFormat.ParseUInt32("2147483647"));
      Assert.AreEqual(2147483648U, TextFormat.ParseUInt32("2147483648"));
      Assert.AreEqual(4294967295U, TextFormat.ParseUInt32("4294967295"));

      Assert.AreEqual(          0L, TextFormat.ParseInt64(          "0"));
      Assert.AreEqual(          1L, TextFormat.ParseInt64(          "1"));
      Assert.AreEqual(         -1L, TextFormat.ParseInt64(         "-1"));
      Assert.AreEqual(      12345L, TextFormat.ParseInt64(      "12345"));
      Assert.AreEqual(     -12345L, TextFormat.ParseInt64(     "-12345"));
      Assert.AreEqual( 2147483647L, TextFormat.ParseInt64( "2147483647"));
      Assert.AreEqual(-2147483648L, TextFormat.ParseInt64("-2147483648"));
      Assert.AreEqual( 4294967295L, TextFormat.ParseInt64( "4294967295"));
      Assert.AreEqual( 4294967296L, TextFormat.ParseInt64( "4294967296"));
      Assert.AreEqual(9223372036854775807L, TextFormat.ParseInt64("9223372036854775807"));
      Assert.AreEqual(-9223372036854775808L, TextFormat.ParseInt64("-9223372036854775808"));

      Assert.AreEqual(          0L, TextFormat.ParseUInt64(          "0"));
      Assert.AreEqual(          1L, TextFormat.ParseUInt64(          "1"));
      Assert.AreEqual(      12345L, TextFormat.ParseUInt64(      "12345"));
      Assert.AreEqual( 2147483647L, TextFormat.ParseUInt64( "2147483647"));
      Assert.AreEqual( 4294967295L, TextFormat.ParseUInt64( "4294967295"));
      Assert.AreEqual( 4294967296L, TextFormat.ParseUInt64( "4294967296"));
      Assert.AreEqual(9223372036854775807UL, TextFormat.ParseUInt64("9223372036854775807"));
      Assert.AreEqual(9223372036854775808UL, TextFormat.ParseUInt64("9223372036854775808"));
      Assert.AreEqual(18446744073709551615UL, TextFormat.ParseUInt64("18446744073709551615"));

      // Hex
      Assert.AreEqual(0x1234abcd, TextFormat.ParseInt32("0x1234abcd"));
      Assert.AreEqual(-0x1234abcd, TextFormat.ParseInt32("-0x1234abcd"));
      Assert.AreEqual(0xffffffffffffffffUL, TextFormat.ParseUInt64("0xffffffffffffffff"));
      Assert.AreEqual(0x7fffffffffffffffL,
                   TextFormat.ParseInt64("0x7fffffffffffffff"));

      // Octal
      Assert.AreEqual(342391, TextFormat.ParseInt32("01234567"));

      // Out-of-range
      AssertFormatException(() => TextFormat.ParseInt32("2147483648"));
      AssertFormatException(() => TextFormat.ParseInt32("-2147483649"));
      AssertFormatException(() => TextFormat.ParseUInt32("4294967296"));
      AssertFormatException(() => TextFormat.ParseUInt32("-1"));
      AssertFormatException(() => TextFormat.ParseInt64("9223372036854775808"));
      AssertFormatException(() => TextFormat.ParseInt64("-9223372036854775809"));
      AssertFormatException(() => TextFormat.ParseUInt64("18446744073709551616"));
      AssertFormatException(() => TextFormat.ParseUInt64("-1"));
      AssertFormatException(() => TextFormat.ParseInt32("abcd"));
    }

    [Test]
    public void ParseLongString() {
      string longText =
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
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TextFormat.Merge("optional_string: \"" + longText + "\"", builder);
      Assert.AreEqual(longText, builder.OptionalString);
    }
  }
}
