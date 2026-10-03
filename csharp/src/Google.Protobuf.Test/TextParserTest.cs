#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.TestProtos;
using Google.Protobuf.WellKnownTypes;
using NUnit.Framework;
using System;
using System.IO;
using System.Text;
using Proto2 = Google.Protobuf.TestProtos.Proto2;

namespace Google.Protobuf
{
    /// <summary>
    /// Unit tests for text format parsing.
    /// </summary>
    public class TextParserTest
    {
        private static TestAllTypes Parse(string text) => TextParser.Default.Parse<TestAllTypes>(text);

        private static void AssertInvalid<T>(string text) where T : IMessage, new() =>
            Assert.Throws<InvalidProtocolBufferException>(() => TextParser.Default.Parse<T>(text));

        private static void AssertInvalid(string text) => AssertInvalid<TestAllTypes>(text);

        #region Overall structure and entry points

        [Test]
        public void AllSingularScalarFields()
        {
            string text = @"
                single_int32: 100
                single_int64: 3210987654321
                single_uint32: 4294967295
                single_uint64: 18446744073709551615
                single_sint32: -456
                single_sint64: -12345678901235
                single_fixed32: 23
                single_fixed64: 1234567890123
                single_sfixed32: -123
                single_sfixed64: -12345678901234
                single_float: 12.25
                single_double: 23.5
                single_bool: true
                single_string: ""test""
                single_bytes: ""\001\002\003\004""
                single_nested_enum: FOO
                single_foreign_enum: FOREIGN_BAR
                single_nested_message { bb: 35 }
                single_foreign_message: { c: 10 }
            ";
            var expected = new TestAllTypes
            {
                SingleInt32 = 100,
                SingleInt64 = 3210987654321,
                SingleUint32 = uint.MaxValue,
                SingleUint64 = ulong.MaxValue,
                SingleSint32 = -456,
                SingleSint64 = -12345678901235,
                SingleFixed32 = 23,
                SingleFixed64 = 1234567890123,
                SingleSfixed32 = -123,
                SingleSfixed64 = -12345678901234,
                SingleFloat = 12.25f,
                SingleDouble = 23.5,
                SingleBool = true,
                SingleString = "test",
                SingleBytes = ByteString.CopyFrom(1, 2, 3, 4),
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.Foo,
                SingleForeignEnum = ForeignEnum.ForeignBar,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 35 },
                SingleForeignMessage = new ForeignMessage { C = 10 },
            };
            Assert.AreEqual(expected, Parse(text));
        }

        [Test]
        [TestCase("")]
        [TestCase("   \t\r\n  ")]
        [TestCase("# just a comment")]
        public void EmptyInput(string text)
        {
            Assert.AreEqual(new TestAllTypes(), Parse(text));
        }

        [Test]
        public void MessageParser_ParseText()
        {
            Assert.AreEqual(new TestAllTypes { SingleInt32 = 7 }, TestAllTypes.Parser.ParseText("single_int32: 7"));
            MessageParser untyped = TestAllTypes.Parser;
            Assert.AreEqual(new TestAllTypes { SingleInt32 = 7 }, untyped.ParseText("single_int32: 7"));
        }

        [Test]
        public void ParseWithDescriptor()
        {
            var message = TextParser.Default.Parse("single_int32: 7", TestAllTypes.Descriptor);
            Assert.AreEqual(new TestAllTypes { SingleInt32 = 7 }, message);
        }

        [Test]
        public void ParseFromTextReader()
        {
            using var reader = new StringReader("single_string: 'abc'");
            Assert.AreEqual(new TestAllTypes { SingleString = "abc" }, TextParser.Default.Parse<TestAllTypes>(reader));
        }

        [Test]
        public void NullArguments()
        {
            Assert.Throws<ArgumentNullException>(() => TextParser.Default.Parse<TestAllTypes>((string) null));
            Assert.Throws<ArgumentNullException>(() => TextParser.Default.Parse<TestAllTypes>((TextReader) null));
            Assert.Throws<ArgumentNullException>(() => TextParser.Default.Parse("", null));
            Assert.Throws<ArgumentNullException>(() => new TextParser(null));
        }

        [Test]
        public void CommentsAndSeparators()
        {
            string text = @"
                # Leading comment
                single_int32: 1,   # trailing comment with a 'quote' and { brace
                single_string: 'a'; single_bool: true
                single_nested_message { bb: 2, }
                single_foreign_message < c: 3; >
                # Trailing comment without newline";
            var expected = new TestAllTypes
            {
                SingleInt32 = 1,
                SingleString = "a",
                SingleBool = true,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 2 },
                SingleForeignMessage = new ForeignMessage { C = 3 },
            };
            Assert.AreEqual(expected, Parse(text));
        }

        [Test]
        [TestCase("single_int32: 1,,")]
        [TestCase("single_int32: 1;;")]
        [TestCase("single_int32: 1,;")]
        [TestCase(",single_int32: 1")]
        [TestCase("single_nested_message { bb: 1,, }")]
        [TestCase("single_nested_message { bb: 1;; }")]
        public void DuplicateSeparatorsRejected(string text)
        {
            AssertInvalid(text);
        }

        [Test]
        [TestCase("single_int32")]
        [TestCase("single_int32:")]
        [TestCase("single_int32 1")]
        [TestCase("single_int32: : 1")]
        [TestCase("single_int32: 1 }")]
        [TestCase("single_int32: 1 >")]
        [TestCase("}")]
        [TestCase("]")]
        [TestCase("1: 2")]
        [TestCase("\"single_int32\": 1")]
        [TestCase("single_int32: 1 @")]
        [TestCase("single_nested_message")]
        [TestCase("single_nested_message {")]
        [TestCase("single_nested_message { bb: 1")]
        [TestCase("single_nested_message { bb: 1 >")]
        [TestCase("single_nested_message < bb: 1 }")]
        [TestCase("single_nested_message: 1")]
        [TestCase("single_nested_message: \"x\"")]
        [TestCase("single_nested_message: [ bb: 1 ]")]
        public void MalformedStructure(string text)
        {
            AssertInvalid(text);
        }

        #endregion

        #region Integers

        [Test]
        [TestCase("0", 0)]
        [TestCase("-0", 0)]
        [TestCase("42", 42)]
        [TestCase("-42", -42)]
        [TestCase("2147483647", int.MaxValue)]
        [TestCase("-2147483648", int.MinValue)]
        [TestCase("0x7fffffff", int.MaxValue)]
        [TestCase("0X7FFFFFFF", int.MaxValue)]
        [TestCase("-0x80000000", int.MinValue)]
        [TestCase("0x0", 0)]
        [TestCase("017", 15)]
        [TestCase("-017", -15)]
        [TestCase("00", 0)]
        [TestCase("017777777777", int.MaxValue)]
        [TestCase("-020000000000", int.MinValue)]
        public void Int32Literals(string literal, int expected)
        {
            Assert.AreEqual(expected, Parse("single_int32: " + literal).SingleInt32);
        }

        [Test]
        [TestCase("single_int32: 2147483648")]
        [TestCase("single_int32: -2147483649")]
        [TestCase("single_int32: 0x80000000")]
        [TestCase("single_int32: -0x80000001")]
        [TestCase("single_int32: 020000000000")]
        [TestCase("single_int32: -020000000001")]
        [TestCase("single_uint32: 4294967296")]
        [TestCase("single_uint32: 0x100000000")]
        [TestCase("single_uint32: 040000000000")]
        [TestCase("single_uint32: -1")]
        [TestCase("single_uint32: -0")]
        [TestCase("single_int64: 9223372036854775808")]
        [TestCase("single_int64: -9223372036854775809")]
        [TestCase("single_int64: 0x8000000000000000")]
        [TestCase("single_int64: -0x8000000000000001")]
        [TestCase("single_int64: 01000000000000000000000")]
        [TestCase("single_int64: -01000000000000000000001")]
        [TestCase("single_uint64: 18446744073709551616")]
        [TestCase("single_uint64: 0x10000000000000000")]
        [TestCase("single_uint64: 02000000000000000000000")]
        [TestCase("single_uint64: 99999999999999999999999999999")]
        [TestCase("single_uint64: -1")]
        [TestCase("single_fixed32: -1")]
        [TestCase("single_fixed64: -1")]
        public void IntegerOutOfRange(string text)
        {
            AssertInvalid(text);
        }

        [Test]
        [TestCase("single_int32: 1.0")]
        [TestCase("single_int32: 1e2")]
        [TestCase("single_int32: 1f")]
        [TestCase("single_int32: 08")]
        [TestCase("single_int32: 0x")]
        [TestCase("single_int32: 0xg")]
        [TestCase("single_int32: 1foo")]
        [TestCase("single_int32: 0x1.5")]
        [TestCase("single_int32: 1_000")]
        [TestCase("single_int32: +1")]
        [TestCase("single_int32: --1")]
        [TestCase("single_int32: -")]
        [TestCase("single_int32: true")]
        [TestCase("single_int32: \"1\"")]
        [TestCase("single_int32: inf")]
        [TestCase("single_int32: -inf")]
        public void InvalidIntegerLiterals(string text)
        {
            AssertInvalid(text);
        }

        [Test]
        public void IntegerBoundaries64Bit()
        {
            var message = Parse(@"
                single_int64: 9223372036854775807
                single_sint64: -9223372036854775808
                single_uint64: 18446744073709551615
                single_fixed64: 0xffffffffffffffff
                single_sfixed64: -0x8000000000000000");
            Assert.AreEqual(long.MaxValue, message.SingleInt64);
            Assert.AreEqual(long.MinValue, message.SingleSint64);
            Assert.AreEqual(ulong.MaxValue, message.SingleUint64);
            Assert.AreEqual(ulong.MaxValue, message.SingleFixed64);
            Assert.AreEqual(long.MinValue, message.SingleSfixed64);
        }

        [Test]
        public void WhitespaceAllowedAfterMinusSign()
        {
            Assert.AreEqual(-5, Parse("single_int32: - 5").SingleInt32);
            Assert.AreEqual(-1.5f, Parse("single_float: -\n1.5").SingleFloat);
        }

        #endregion

        #region Floating point

        [Test]
        [TestCase("1.5", 1.5f)]
        [TestCase("1.5f", 1.5f)]
        [TestCase("1.5F", 1.5f)]
        [TestCase("-1.5", -1.5f)]
        [TestCase("1", 1f)]
        [TestCase("1f", 1f)]
        [TestCase("1.", 1f)]
        [TestCase(".5", 0.5f)]
        [TestCase("-.5", -0.5f)]
        [TestCase(".5e1", 5f)]
        [TestCase("1e2", 100f)]
        [TestCase("1E+2", 100f)]
        [TestCase("1.5e-1", 0.15f)]
        [TestCase("3.192837", 3.192837f)]
        [TestCase("3.4028235e+38", float.MaxValue)]
        [TestCase("1.17549e-38", 1.17549e-38f)]
        [TestCase("4294967296", 4294967296f)]
        [TestCase("18446744073709551616", 18446744073709551616f)]
        [TestCase("inf", float.PositiveInfinity)]
        [TestCase("INF", float.PositiveInfinity)]
        [TestCase("iNf", float.PositiveInfinity)]
        [TestCase("infinity", float.PositiveInfinity)]
        [TestCase("Infinity", float.PositiveInfinity)]
        [TestCase("-inf", float.NegativeInfinity)]
        [TestCase("-INFINITY", float.NegativeInfinity)]
        [TestCase("- inf", float.NegativeInfinity)]
        [TestCase("1e50", float.PositiveInfinity)]
        [TestCase("-1e50", float.NegativeInfinity)]
        [TestCase("3.4028235e+39", float.PositiveInfinity)]
        [TestCase("1e18446744073709551616", float.PositiveInfinity)]
        [TestCase("1e-50", 0f)]
        [TestCase("1e-18446744073709551616", 0f)]
        public void FloatLiterals(string literal, float expected)
        {
            Assert.AreEqual(expected, Parse("single_float: " + literal).SingleFloat);
        }

        [Test]
        [TestCase("1.5", 1.5)]
        [TestCase("1.5f", 1.5)]
        [TestCase("-0.25", -0.25)]
        [TestCase("1e300", 1e300)]
        [TestCase("1e9999", double.PositiveInfinity)]
        [TestCase("-1e9999", double.NegativeInfinity)]
        [TestCase("1e18446744073709551616", double.PositiveInfinity)]
        [TestCase("1e-18446744073709551616", 0.0)]
        [TestCase("inf", double.PositiveInfinity)]
        [TestCase("-Infinity", double.NegativeInfinity)]
        [TestCase("9223372036854775808", 9223372036854775808.0)]
        public void DoubleLiterals(string literal, double expected)
        {
            Assert.AreEqual(expected, Parse("single_double: " + literal).SingleDouble);
        }

        [Test]
        [TestCase("nan")]
        [TestCase("NaN")]
        [TestCase("nAn")]
        [TestCase("-nan")]
        public void NanLiterals(string literal)
        {
            Assert.IsTrue(float.IsNaN(Parse("single_float: " + literal).SingleFloat));
            Assert.IsTrue(double.IsNaN(Parse("single_double: " + literal).SingleDouble));
        }

        [Test]
        [TestCase("-0")]
        [TestCase("-0.0")]
        [TestCase("-0f")]
        [TestCase("-1e-50")]
        [TestCase("-1e-18446744073709551616")]
        public void NegativeZeroFloat(string literal)
        {
            // 1 / -0.0 is negative infinity, which distinguishes -0 from +0.
            Assert.IsTrue(float.IsNegativeInfinity(1f / Parse("single_float: " + literal).SingleFloat));
        }

        [Test]
        [TestCase("-0")]
        [TestCase("-0.0")]
        [TestCase("-0f")]
        [TestCase("-1e-400")]
        [TestCase("-1e-18446744073709551616")]
        public void NegativeZeroDouble(string literal)
        {
            Assert.IsTrue(double.IsNegativeInfinity(1.0 / Parse("single_double: " + literal).SingleDouble));
        }

        [Test]
        [TestCase("single_float: 0x1")]
        [TestCase("single_float: -0x1")]
        [TestCase("single_float: 012")]
        [TestCase("single_float: -012")]
        [TestCase("single_double: 0x10")]
        [TestCase("single_double: 010")]
        [TestCase("single_float: 1.5.2")]
        [TestCase("single_float: 1e")]
        [TestCase("single_float: 1e+")]
        [TestCase("single_float: 1.5ff")]
        [TestCase("single_float: 1.5d")]
        [TestCase("single_float: .")]
        [TestCase("single_float: -")]
        [TestCase("single_float: infinite")]
        [TestCase("single_float: -foo")]
        [TestCase("single_float: true")]
        [TestCase("single_float: \"1.5\"")]
        public void InvalidFloatLiterals(string text)
        {
            AssertInvalid(text);
        }

        #endregion

        #region Booleans and enums

        [Test]
        [TestCase("true", true)]
        [TestCase("True", true)]
        [TestCase("t", true)]
        [TestCase("1", true)]
        [TestCase("0x1", true)]
        [TestCase("false", false)]
        [TestCase("False", false)]
        [TestCase("f", false)]
        [TestCase("0", false)]
        public void BoolLiterals(string literal, bool expected)
        {
            Assert.AreEqual(expected, Parse("single_bool: " + literal).SingleBool);
        }

        [Test]
        [TestCase("single_bool: TRUE")]
        [TestCase("single_bool: yes")]
        [TestCase("single_bool: 2")]
        [TestCase("single_bool: -1")]
        [TestCase("single_bool: 1.0")]
        [TestCase("single_bool: \"true\"")]
        public void InvalidBoolLiterals(string text)
        {
            AssertInvalid(text);
        }

        [Test]
        [TestCase("FOO", TestAllTypes.Types.NestedEnum.Foo)]
        [TestCase("BAZ", TestAllTypes.Types.NestedEnum.Baz)]
        [TestCase("NEG", TestAllTypes.Types.NestedEnum.Neg)]
        [TestCase("2", TestAllTypes.Types.NestedEnum.Bar)]
        [TestCase("-1", TestAllTypes.Types.NestedEnum.Neg)]
        [TestCase("0x3", TestAllTypes.Types.NestedEnum.Baz)]
        [TestCase("100", (TestAllTypes.Types.NestedEnum) 100)]
        public void EnumValues(string literal, TestAllTypes.Types.NestedEnum expected)
        {
            Assert.AreEqual(expected, Parse("single_nested_enum: " + literal).SingleNestedEnum);
        }

        [Test]
        [TestCase("single_nested_enum: foo")]
        [TestCase("single_nested_enum: UNKNOWN_VALUE")]
        [TestCase("single_nested_enum: \"FOO\"")]
        [TestCase("single_nested_enum: 1.0")]
        [TestCase("single_nested_enum: 2147483648")]
        [TestCase("single_nested_enum: true")]
        public void InvalidEnumValues(string text)
        {
            AssertInvalid(text);
        }

        #endregion

        #region Strings and bytes

        [Test]
        [TestCase("\"double\"", "double")]
        [TestCase("'single'", "single")]
        [TestCase("\"it's\"", "it's")]
        [TestCase("'say \"hi\"'", "say \"hi\"")]
        [TestCase("''", "")]
        [TestCase("\"a\" \"b\"", "ab")]
        [TestCase("'a' \"b\" 'c'", "abc")]
        [TestCase("\"first\"\n  # comment\n  'second'", "firstsecond")]
        [TestCase("\"\\a\\b\\f\\n\\r\\t\\v\"", "\a\b\f\n\r\t\v")]
        [TestCase("\"\\\\\\'\\\"\\?\"", "\\'\"?")]
        [TestCase("\"\\101\\102\\103\"", "ABC")]
        [TestCase("\"\\1\\12\"", "\u0001\n")]
        [TestCase("\"\\0017\"", "\u00017")]
        [TestCase("\"\\x41\\x4a\\x4B\"", "AJK")]
        [TestCase("\"\\X41\"", "A")]
        [TestCase("\"\\x4\"", "\u0004")]
        [TestCase("\"\\x414\"", "A4")]
        [TestCase("\"\\xe1\\x88\\xb4\"", "\u1234")]
        [TestCase("\"\\341\\210\\264\"", "\u1234")]
        [TestCase("\"\\u1234\"", "\u1234")]
        [TestCase("\"\\u00e9\"", "\u00e9")]
        [TestCase("\"\\U00001234\\U00010437\"", "\u1234\U00010437")]
        [TestCase("\"\u00e9\u1234\U00010437\"", "\u00e9\u1234\U00010437")]
        public void StringLiterals(string literal, string expected)
        {
            Assert.AreEqual(expected, Parse("single_string: " + literal).SingleString);
        }

        [Test]
        public void BytesLiterals()
        {
            var message = Parse("single_bytes: \"\\000\\001\\377\\xff\\xC0abc\\n\" '\\x00'");
            Assert.AreEqual(ByteString.CopyFrom(0, 1, 0xff, 0xff, 0xc0, (byte) 'a', (byte) 'b', (byte) 'c', (byte) '\n', 0), message.SingleBytes);
            // Non-ASCII source characters are stored as UTF-8.
            Assert.AreEqual(ByteString.CopyFromUtf8("\u00e9\u1234"), Parse("single_bytes: \"\u00e9\\u1234\"").SingleBytes);
        }

        [Test]
        public void StringFieldRejectsInvalidUtf8ButBytesFieldAccepts()
        {
            AssertInvalid("single_string: '\\300'");
            AssertInvalid("single_string: '\\xc0'");
            AssertInvalid("single_string: '\\xed\\xa0\\x80'"); // Encoded surrogate
            Assert.AreEqual(ByteString.CopyFrom(0xc0), Parse("single_bytes: '\\300'").SingleBytes);
            Assert.AreEqual(ByteString.CopyFrom(0xc0), Parse("single_bytes: '\\xc0'").SingleBytes);
        }

        [Test]
        [TestCase("single_string: \"unterminated")]
        [TestCase("single_string: 'unterminated")]
        [TestCase("single_string: \"mismatched'")]
        [TestCase("single_string: \"first line\nsecond line\"")]
        [TestCase("single_string: \"bad \\q escape\"")]
        [TestCase("single_string: \"\\x\"")]
        [TestCase("single_string: \"\\xg\"")]
        [TestCase("single_string: \"\\400\"")]
        [TestCase("single_string: \"\\u12\"")]
        [TestCase("single_string: \"\\u12345\\\"")]
        [TestCase("single_string: \"\\U1234\"")]
        [TestCase("single_string: \"\\U00110000\"")]
        [TestCase("single_string: \"\\ud800\"")]
        [TestCase("single_string: \"\\udc00\"")]
        [TestCase("single_string: \"\\ud801\\udc37\"")]
        [TestCase("single_string: \"\\U0000d800\"")]
        [TestCase("single_string: \"\\U0000d801\\U0000dc37\"")]
        [TestCase("single_string: \"trailing backslash\\")]
        [TestCase("single_string: 123")]
        [TestCase("single_string: abc")]
        [TestCase("single_bytes: 123")]
        public void InvalidStringLiterals(string text)
        {
            AssertInvalid(text);
        }

        #endregion

        #region Nested messages and groups

        [Test]
        [TestCase("single_nested_message { bb: 1 }")]
        [TestCase("single_nested_message: { bb: 1 }")]
        [TestCase("single_nested_message < bb: 1 >")]
        [TestCase("single_nested_message: < bb: 1 >")]
        [TestCase("single_nested_message{bb:1}")]
        [TestCase("single_nested_message {\n  bb: 1\n}\n")]
        public void NestedMessageDelimiters(string text)
        {
            Assert.AreEqual(new TestAllTypes { SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 1 } }, Parse(text));
        }

        [Test]
        public void EmptyNestedMessageIsPresent()
        {
            var message = Parse("single_nested_message {}");
            Assert.IsNotNull(message.SingleNestedMessage);
            Assert.AreEqual(new TestAllTypes.Types.NestedMessage(), message.SingleNestedMessage);
        }

        [Test]
        public void RepeatedSingularMessageBlocksAreMerged()
        {
            var message = TextParser.Default.Parse<NestedTestAllTypes>(@"
                child { payload { single_int32: 1 } }
                child { payload { single_string: 'x' } child { payload { single_bool: true } } }");
            Assert.AreEqual(1, message.Child.Payload.SingleInt32);
            Assert.AreEqual("x", message.Child.Payload.SingleString);
            Assert.IsTrue(message.Child.Child.Payload.SingleBool);
        }

        [Test]
        public void DeeplyNestedMessages()
        {
            var message = TextParser.Default.Parse<NestedTestAllTypes>(
                "child < child { child < payload { single_int32: 1 } > } >");
            Assert.AreEqual(1, message.Child.Child.Child.Payload.SingleInt32);
        }

        [Test]
        [TestCase("OptionalGroup { a: 1 }")]
        [TestCase("OptionalGroup: { a: 1 }")]
        [TestCase("OptionalGroup < a: 1 >")]
        [TestCase("optionalgroup { a: 1 }")]
        public void Proto2Groups(string text)
        {
            var expected = new Proto2.TestAllTypes
            {
                OptionalGroup = new Proto2.TestAllTypes.Types.OptionalGroup { A = 1 }
            };
            Assert.AreEqual(expected, TextParser.Default.Parse<Proto2.TestAllTypes>(text));
        }

        [Test]
        public void Proto2RepeatedGroups()
        {
            var message = TextParser.Default.Parse<Proto2.TestAllTypes>(
                "RepeatedGroup { a: 1 } repeatedgroup { a: 2 } RepeatedGroup: [{ a: 3 }, < a: 4 >]");
            Assert.AreEqual(4, message.RepeatedGroup.Count);
            Assert.AreEqual(1, message.RepeatedGroup[0].A);
            Assert.AreEqual(2, message.RepeatedGroup[1].A);
            Assert.AreEqual(3, message.RepeatedGroup[2].A);
            Assert.AreEqual(4, message.RepeatedGroup[3].A);
        }

        [Test]
        public void Proto2ScalarsAndDefaults()
        {
            var message = TextParser.Default.Parse<Proto2.TestAllTypes>("optional_int32: 5 optional_nested_message { bb: 6 }");
            Assert.IsTrue(message.HasOptionalInt32);
            Assert.AreEqual(5, message.OptionalInt32);
            Assert.AreEqual(6, message.OptionalNestedMessage.Bb);
            Assert.IsFalse(message.HasOptionalInt64);
        }

        [Test]
        [TestCase("Single_Int32: 1")]
        [TestCase("SINGLE_INT32: 1")]
        [TestCase("SingleNestedMessage { bb: 1 }")]
        [TestCase("NestedMessage { bb: 1 }")]
        public void FieldNamesAreCaseSensitiveExceptForGroups(string text)
        {
            AssertInvalid(text);
        }

        #endregion

        #region Repeated fields

        [Test]
        public void RepeatedScalarEntries()
        {
            var message = Parse("repeated_int32: 1 repeated_string: 'a' repeated_int32: 2 repeated_string: 'b' repeated_int32: -3");
            Assert.AreEqual(new[] { 1, 2, -3 }, message.RepeatedInt32);
            Assert.AreEqual(new[] { "a", "b" }, message.RepeatedString);
        }

        [Test]
        public void RepeatedScalarLists()
        {
            var message = Parse(@"
                repeated_int32: [1, 2]
                repeated_int32: [3]
                repeated_int32: []
                repeated_int32: 4
                repeated_string: ['a', ""b"" 'c', 'd']
                repeated_bool: [true, f, 0]
                repeated_float: [1.5, inf, -inf]
                repeated_nested_enum: [FOO, 2, NEG]
                repeated_bytes: ['\x01', '']");
            Assert.AreEqual(new[] { 1, 2, 3, 4 }, message.RepeatedInt32);
            Assert.AreEqual(new[] { "a", "bc", "d" }, message.RepeatedString);
            Assert.AreEqual(new[] { true, false, false }, message.RepeatedBool);
            Assert.AreEqual(new[] { 1.5f, float.PositiveInfinity, float.NegativeInfinity }, message.RepeatedFloat);
            Assert.AreEqual(
                new[] { TestAllTypes.Types.NestedEnum.Foo, TestAllTypes.Types.NestedEnum.Bar, TestAllTypes.Types.NestedEnum.Neg },
                message.RepeatedNestedEnum);
            Assert.AreEqual(new[] { ByteString.CopyFrom(1), ByteString.Empty }, message.RepeatedBytes);
        }

        [Test]
        public void RepeatedMessages()
        {
            var message = Parse(@"
                repeated_nested_message { bb: 1 }
                repeated_nested_message: < bb: 2 >
                repeated_nested_message: [{ bb: 3 }, < bb: 4 >, {}]
                repeated_nested_message: []
                repeated_foreign_message: [{ c: 5 }]");
            Assert.AreEqual(5, message.RepeatedNestedMessage.Count);
            Assert.AreEqual(1, message.RepeatedNestedMessage[0].Bb);
            Assert.AreEqual(2, message.RepeatedNestedMessage[1].Bb);
            Assert.AreEqual(3, message.RepeatedNestedMessage[2].Bb);
            Assert.AreEqual(4, message.RepeatedNestedMessage[3].Bb);
            Assert.AreEqual(0, message.RepeatedNestedMessage[4].Bb);
            Assert.AreEqual(5, message.RepeatedForeignMessage[0].C);
        }

        [Test]
        [TestCase("repeated_int32: [1 2]")]
        [TestCase("repeated_int32: [1,]")]
        [TestCase("repeated_int32: [1, 2,]")]
        [TestCase("repeated_int32: [,]")]
        [TestCase("repeated_int32: [,1]")]
        [TestCase("repeated_int32: [1;2]")]
        [TestCase("repeated_int32: [1,,2]")]
        [TestCase("repeated_int32: [1")]
        [TestCase("repeated_int32: 1]")]
        [TestCase("repeated_int32 [1]")]
        [TestCase("repeated_int32 1")]
        [TestCase("repeated_int32: [[1]]")]
        [TestCase("repeated_int32: [{ bb: 1 }]")]
        [TestCase("repeated_int32: { bb: 1 }")]
        [TestCase("repeated_nested_message: [1]")]
        [TestCase("repeated_nested_message: [{ bb: 1 } { bb: 2 }]")]
        [TestCase("repeated_nested_message [{ bb: 1 }]")]
        public void InvalidRepeatedFields(string text)
        {
            AssertInvalid(text);
        }

        #endregion

        #region Map fields

        [Test]
        public void MapEntries()
        {
            var message = TextParser.Default.Parse<TestMap>(@"
                map_int32_int32 { key: 1 value: 2 }
                map_int32_int32: < key: 3, value: 4; >
                map_int32_int32: [{ key: 5 value: 6 }, { value: 8 key: 7 }]
                map_int32_int32: []
                map_string_string { key: 'a' value: 'b' }
                map_bool_bool { key: true value: false }
                map_int32_enum { key: 1 value: MAP_ENUM_BAR }
                map_int32_enum { key: 2 value: 2 }
                map_int32_foreign_message { key: 1 value { c: 10 } }
                map_int32_foreign_message { key: 2 value: < c: 20 > }");
            var expected = new TestMap
            {
                MapInt32Int32 = { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } },
                MapStringString = { { "a", "b" } },
                MapBoolBool = { { true, false } },
                MapInt32Enum = { { 1, MapEnum.Bar }, { 2, MapEnum.Baz } },
                MapInt32ForeignMessage = { { 1, new ForeignMessage { C = 10 } }, { 2, new ForeignMessage { C = 20 } } },
            };
            Assert.AreEqual(expected, message);
        }

        [Test]
        public void MapEntryDefaultsAndOverwrites()
        {
            var message = TextParser.Default.Parse<TestMap>(@"
                map_int32_int32 {}
                map_int32_int32 { value: 1 }
                map_int32_int32 { key: 2 }
                map_string_string { key: 'k' value: 'first' }
                map_string_string { key: 'k' value: 'second' }
                map_int32_foreign_message { key: 5 }");
            Assert.AreEqual(2, message.MapInt32Int32.Count);
            Assert.AreEqual(1, message.MapInt32Int32[0]);
            Assert.AreEqual(0, message.MapInt32Int32[2]);
            Assert.AreEqual("second", message.MapStringString["k"]);
            Assert.AreEqual(new ForeignMessage(), message.MapInt32ForeignMessage[5]);
        }

        [Test]
        [TestCase("map_int32_int32 { key: 1 value: 2 other: 3 }")]
        [TestCase("map_int32_int32 { key: 1 key: 2 }")]
        [TestCase("map_int32_int32 { key: 1 value: 2 value: 3 }")]
        [TestCase("map_int32_int32 { key: 'a' value: 2 }")]
        [TestCase("map_int32_int32 { key: 1 value: 'b' }")]
        [TestCase("map_int32_int32 { key: 1 value { } }")]
        [TestCase("map_int32_int32: 1")]
        [TestCase("map_int32_int32: [1]")]
        [TestCase("map_int32_int32 [{ key: 1 value: 2 }]")]
        [TestCase("map_int32_int32 { key: 1 value: 2")]
        [TestCase("map_int32_int32 { key: 1 value: 2 >")]
        public void InvalidMapEntries(string text)
        {
            AssertInvalid<TestMap>(text);
        }

        #endregion

        #region Oneofs and duplicate fields

        [Test]
        public void OneofFields()
        {
            Assert.AreEqual(TestAllTypes.OneofFieldOneofCase.OneofUint32, Parse("oneof_uint32: 1").OneofFieldCase);
            Assert.AreEqual("x", Parse("oneof_string: 'x'").OneofString);
            Assert.AreEqual(5, Parse("oneof_nested_message { bb: 5 }").OneofNestedMessage.Bb);
        }

        [Test]
        [TestCase("oneof_uint32: 1 oneof_string: 'x'")]
        [TestCase("oneof_string: 'x' oneof_uint32: 1")]
        [TestCase("oneof_uint32: 1 oneof_uint32: 2")]
        [TestCase("oneof_nested_message { } oneof_string: 'x'")]
        [TestCase("oneof_nested_message { } oneof_nested_message { }")]
        [TestCase("oneof_uint32: 1 single_int32: 2 oneof_bytes: ''")]
        public void MultipleOneofMembersRejected(string text)
        {
            AssertInvalid(text);
        }

        [Test]
        [TestCase("single_int32: 1 single_int32: 2")]
        [TestCase("single_int32: 0 single_int32: 0")]
        [TestCase("single_string: 'a' single_int32: 1 single_string: 'b'")]
        [TestCase("single_nested_enum: FOO single_nested_enum: BAR")]
        [TestCase("single_nested_message { bb: 1 bb: 2 }")]
        public void DuplicateSingularScalarsRejected(string text)
        {
            AssertInvalid(text);
        }

        [Test]
        public void DuplicateScalarsInSeparateBlocksAllowed()
        {
            // Each message block is a separate scope, so the second block may set the same scalar field.
            var message = Parse("single_nested_message { bb: 1 } single_nested_message { bb: 2 }");
            Assert.AreEqual(2, message.SingleNestedMessage.Bb);
        }

        #endregion

        #region Unknown fields, extensions and settings

        [Test]
        [TestCase("unknown_field: 1")]
        [TestCase("unknown_field { a: 1 }")]
        [TestCase("single_nested_message { unknown: 1 }")]
        [TestCase("Single_Int32: 1")]
        public void UnknownFieldsRejectedByDefault(string text)
        {
            AssertInvalid(text);
        }

        [Test]
        public void UnknownFieldsIgnoredWhenConfigured()
        {
            var parser = new TextParser(TextParser.Settings.Default.WithIgnoreUnknownFields(true));
            string text = @"
                single_int32: 1
                unknown_scalar: 2
                unknown_string: 'a' 'b'
                unknown_float: -1.5e3,
                unknown_enum: SOME_VALUE;
                unknown_list: [1, 'two', 3.0, FOUR, { a: 5 }, < b: 6 >]
                unknown_empty_list: []
                unknown_message { nested { deeper: 1 } also: [1, 2] }
                unknown_message: < with_colon: true >
                single_nested_message { bb: 2 unknown_inner { x: 'y' } }
                [some.extension]: 3
                [type.googleapis.com/some.Type] { field: 4 }
                single_string: 'kept'";
            var expected = new TestAllTypes
            {
                SingleInt32 = 1,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 2 },
                SingleString = "kept",
            };
            Assert.AreEqual(expected, parser.Parse<TestAllTypes>(text));
        }

        [Test]
        [TestCase("unknown_field")]
        [TestCase("unknown_field: ")]
        [TestCase("unknown_field: }")]
        [TestCase("unknown_field [1]")]
        [TestCase("unknown_field: [1 2]")]
        [TestCase("unknown_field: [1,]")]
        [TestCase("unknown_field { a: 1 >")]
        [TestCase("unknown_field { a: 1")]
        [TestCase("unknown_field { a: 1 } }")]
        [TestCase("unknown_field: 1,,")]
        [TestCase("unknown_field: 'unterminated")]
        [TestCase("unknown_field: 08")]
        [TestCase("[unterminated.extension: 1")]
        [TestCase("[]: 1")]
        [TestCase("[.bad]: 1")]
        public void MalformedUnknownFieldsStillRejected(string text)
        {
            var parser = new TextParser(TextParser.Settings.Default.WithIgnoreUnknownFields(true));
            Assert.Throws<InvalidProtocolBufferException>(() => parser.Parse<TestAllTypes>(text));
        }

        [Test]
        public void ExtensionsNotSupported()
        {
            Assert.Throws<NotSupportedException>(
                () => TextParser.Default.Parse<Proto2.TestAllExtensions>("[protobuf_unittest.optional_int32_extension]: 1"));
            Assert.Throws<NotSupportedException>(
                () => TextParser.Default.Parse<TestWellKnownTypes>("any_field { [type.googleapis.com/protobuf_unittest3.TestAllTypes] { single_int32: 1 } }"));
        }

        [Test]
        public void RecursionLimit()
        {
            string data64 = MakeRecursiveText(64);
            string data65 = MakeRecursiveText(65);

            var parser64 = new TextParser(new TextParser.Settings(64));
            CodedInputStreamTest.AssertMessageDepth(parser64.Parse<TestRecursiveMessage>(data64), 64);
            Assert.Throws<InvalidProtocolBufferException>(() => parser64.Parse<TestRecursiveMessage>(data65));

            var parser63 = new TextParser(new TextParser.Settings(63));
            Assert.Throws<InvalidProtocolBufferException>(() => parser63.Parse<TestRecursiveMessage>(data64));

            // The default limit matches CodedInputStream.
            CodedInputStreamTest.AssertMessageDepth(
                TextParser.Default.Parse<TestRecursiveMessage>(MakeRecursiveText(CodedInputStream.DefaultRecursionLimit)),
                CodedInputStream.DefaultRecursionLimit);
            Assert.Throws<InvalidProtocolBufferException>(
                () => TextParser.Default.Parse<TestRecursiveMessage>(MakeRecursiveText(CodedInputStream.DefaultRecursionLimit + 1)));
        }

        [Test]
        public void RecursionLimitAppliesToListsAndMaps()
        {
            var parser = new TextParser(new TextParser.Settings(1));
            Assert.DoesNotThrow(() => parser.Parse<TestAllTypes>("repeated_nested_message: [{ bb: 1 }]"));
            Assert.DoesNotThrow(() => parser.Parse<TestMap>("map_int32_int32 { key: 1 value: 2 }"));
            Assert.Throws<InvalidProtocolBufferException>(
                () => parser.Parse<NestedTestAllTypes>("repeated_child: [{ child { } }]"));
            Assert.Throws<InvalidProtocolBufferException>(
                () => parser.Parse<TestMap>("map_int32_foreign_message { key: 1 value { } }"));
        }

        [Test]
        public void RecursionLimitAppliesToSkippedUnknownFields()
        {
            var parser = new TextParser(new TextParser.Settings(2).WithIgnoreUnknownFields(true));
            Assert.DoesNotThrow(() => parser.Parse<TestAllTypes>("unknown { a { } }"));
            Assert.DoesNotThrow(() => parser.Parse<TestAllTypes>("unknown: [{ a: [< >] }]"));
            Assert.DoesNotThrow(() => parser.Parse<NestedTestAllTypes>("child { unknown { } }"));
            Assert.DoesNotThrow(() => parser.Parse<TestMap>("map_int32_int32 { unknown { } }"));
            Assert.Throws<InvalidProtocolBufferException>(() => parser.Parse<TestAllTypes>("unknown { a { b { } } }"));
            Assert.Throws<InvalidProtocolBufferException>(() => parser.Parse<TestAllTypes>("unknown: [{ a: [< b { } >] }]"));
            Assert.Throws<InvalidProtocolBufferException>(() => parser.Parse<NestedTestAllTypes>("child { unknown { a { } } }"));
            Assert.Throws<InvalidProtocolBufferException>(() => parser.Parse<TestMap>("map_int32_int32 { unknown { a { } } }"));
        }

        [Test]
        public void SettingsBuilders()
        {
            var settings = TextParser.Settings.Default;
            Assert.AreEqual(CodedInputStream.DefaultRecursionLimit, settings.RecursionLimit);
            Assert.IsFalse(settings.IgnoreUnknownFields);

            var modified = settings.WithIgnoreUnknownFields(true).WithRecursionLimit(5);
            Assert.AreEqual(5, modified.RecursionLimit);
            Assert.IsTrue(modified.IgnoreUnknownFields);
            Assert.IsTrue(modified.WithRecursionLimit(7).IgnoreUnknownFields);
            Assert.AreEqual(5, modified.WithIgnoreUnknownFields(false).RecursionLimit);

            // The original settings are unchanged.
            Assert.AreEqual(CodedInputStream.DefaultRecursionLimit, settings.RecursionLimit);
            Assert.IsFalse(settings.IgnoreUnknownFields);
            Assert.AreEqual(3, new TextParser.Settings(3).RecursionLimit);
        }

        private static string MakeRecursiveText(int depth)
        {
            var builder = new StringBuilder();
            for (int i = 0; i < depth; i++)
            {
                builder.Append("a { ");
            }
            builder.Append("i: 5");
            for (int i = 0; i < depth; i++)
            {
                builder.Append(" }");
            }
            return builder.ToString();
        }

        #endregion

        #region Well-known types

        [Test]
        public void WellKnownTypesUseRegularFieldSyntax()
        {
            var message = TextParser.Default.Parse<TestWellKnownTypes>(@"
                timestamp_field { seconds: 1234567890 nanos: 500 }
                duration_field { seconds: -3 nanos: -500000000 }
                int32_field { value: 5 }
                int64_field { }
                string_field { value: 'text' }
                bytes_field { value: '\x01\x02' }
                bool_field { value: true }
                double_field: { value: 1.5 }
                field_mask_field { paths: 'a' paths: ['b.c', 'd'] }
                struct_field {
                  fields { key: 'n' value { number_value: 1 } }
                  fields { key: 'l' value { list_value { values { string_value: 'x' } values { null_value: NULL_VALUE } } } }
                }
                value_field { bool_value: false }
                any_field { type_url: 'type.googleapis.com/google.protobuf.Empty' }");
            var expected = new TestWellKnownTypes
            {
                TimestampField = new Timestamp { Seconds = 1234567890, Nanos = 500 },
                DurationField = new Duration { Seconds = -3, Nanos = -500000000 },
                Int32Field = 5,
                Int64Field = 0,
                StringField = "text",
                BytesField = ByteString.CopyFrom(1, 2),
                BoolField = true,
                DoubleField = 1.5,
                FieldMaskField = new FieldMask { Paths = { "a", "b.c", "d" } },
                StructField = new Struct
                {
                    Fields =
                    {
                        { "n", Value.ForNumber(1) },
                        { "l", Value.ForList(Value.ForString("x"), Value.ForNull()) },
                    }
                },
                ValueField = Value.ForBool(false),
                AnyField = new Any { TypeUrl = "type.googleapis.com/google.protobuf.Empty" },
            };
            Assert.AreEqual(expected, message);
        }

        [Test]
        public void WrapperTypesInCollectionsAndOneofs()
        {
            var repeated = TextParser.Default.Parse<RepeatedWellKnownTypes>(@"
                int32_field { value: 1 }
                int32_field: [{ value: 2 }, {}]
                string_field { value: 'x' }
                bytes_field: [< value: 'ab' >]
                timestamp_field { seconds: 5 }");
            Assert.AreEqual(new int?[] { 1, 2, 0 }, repeated.Int32Field);
            Assert.AreEqual(new[] { "x" }, repeated.StringField);
            Assert.AreEqual(new[] { ByteString.CopyFromUtf8("ab") }, repeated.BytesField);
            Assert.AreEqual(5, repeated.TimestampField[0].Seconds);

            var map = TextParser.Default.Parse<MapWellKnownTypes>(@"
                int32_field { key: 1 value { value: 10 } }
                int32_field { key: 2 }
                string_field: [{ key: 3 value: < value: 'y' > }]");
            Assert.AreEqual(10, map.Int32Field[1]);
            Assert.AreEqual(0, map.Int32Field[2]);
            Assert.AreEqual("y", map.StringField[3]);

            var oneof = TextParser.Default.Parse<OneofWellKnownTypes>("uint64_field { value: 7 }");
            Assert.AreEqual(OneofWellKnownTypes.OneofFieldOneofCase.Uint64Field, oneof.OneofFieldCase);
            Assert.AreEqual(7UL, oneof.Uint64Field);
            Assert.Throws<InvalidProtocolBufferException>(
                () => TextParser.Default.Parse<OneofWellKnownTypes>("uint64_field { value: 7 } int32_field { value: 1 }"));
        }

        [Test]
        public void WrapperValuesMergeAcrossBlocks()
        {
            // A second block for the same wrapper field without an inner value keeps the earlier value, as with
            // any other sub-message merge.
            var message = TextParser.Default.Parse<TestWellKnownTypes>("int32_field { value: 3 } int32_field { }");
            Assert.AreEqual(3, message.Int32Field);
            // ...and an inner value in the second block replaces it.
            message = TextParser.Default.Parse<TestWellKnownTypes>("int32_field { value: 3 } int32_field { value: 4 }");
            Assert.AreEqual(4, message.Int32Field);
            // An empty block is still presence.
            message = TextParser.Default.Parse<TestWellKnownTypes>("string_field { }");
            Assert.AreEqual("", message.StringField);
        }

        #endregion
    }
}
