#region Copyright notice and license
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
#endregion

using System;
using Google.Protobuf.TestProtos;
using NUnit.Framework;
using UnitTest.Issues.TestProtos;
using Google.Protobuf.WellKnownTypes;
using Google.Protobuf.Reflection;

using static Google.Protobuf.JsonParserTest; // For WrapInQuotes
using System.IO;
using Google.Protobuf.Collections;
using ProtobufUnittest;

namespace Google.Protobuf
{
    /// <summary>
    /// Tests for the JSON formatter. Note that in these tests, double quotes are replaced with apostrophes
    /// for the sake of readability (embedding \" everywhere is painful). See the AssertJson method for details.
    /// </summary>
    public class JsonFormatterTest
    {
        [Test]
        public void DefaultValues_WhenOmitted()
        {
            var formatter = JsonFormatter.Default;

            AssertJson("{ }", formatter.Format(new ForeignMessage()));
            AssertJson("{ }", formatter.Format(new TestAllTypes()));
            AssertJson("{ }", formatter.Format(new TestMap()));
        }

        [Test]
        public void DefaultValues_WhenIncluded()
        {
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            AssertJson("{ 'c': 0 }", formatter.Format(new ForeignMessage()));
        }

        [Test]
        public void EnumAllowAlias()
        {
            var message = new TestEnumAllowAlias
            {
                Value = TestEnumWithDupValue.Foo2,
            };
            var actualText = JsonFormatter.Default.Format(message);
            var expectedText = "{ 'value': 'FOO1' }";
            AssertJson(expectedText, actualText);
        }

        [Test]
        public void EnumAsInt()
        {
            var message = new TestAllTypes
            {
                SingleForeignEnum = ForeignEnum.ForeignBar,
                RepeatedForeignEnum = { ForeignEnum.ForeignBaz, (ForeignEnum) 100, ForeignEnum.ForeignFoo }
            };
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatEnumsAsIntegers(true));
            var actualText = formatter.Format(message);
            var expectedText = "{ " +
                               "'singleForeignEnum': 5, " +
                               "'repeatedForeignEnum': [ 6, 100, 4 ]" +
                               " }";
            AssertJson(expectedText, actualText);
        }

        [Test]
        public void AllSingleFields()
        {
            var message = new TestAllTypes
            {
                SingleBool = true,
                SingleBytes = ByteString.CopyFrom(1, 2, 3, 4),
                SingleDouble = 23.5,
                SingleFixed32 = 23,
                SingleFixed64 = 1234567890123,
                SingleFloat = 12.25f,
                SingleForeignEnum = ForeignEnum.ForeignBar,
                SingleForeignMessage = new ForeignMessage { C = 10 },
                SingleImportEnum = ImportEnum.ImportBaz,
                SingleImportMessage = new ImportMessage { D = 20 },
                SingleInt32 = 100,
                SingleInt64 = 3210987654321,
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.Foo,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 35 },
                SinglePublicImportMessage = new PublicImportMessage { E = 54 },
                SingleSfixed32 = -123,
                SingleSfixed64 = -12345678901234,
                SingleSint32 = -456,
                SingleSint64 = -12345678901235,
                SingleString = "test\twith\ttabs",
                SingleUint32 = uint.MaxValue,
                SingleUint64 = ulong.MaxValue,
            };
            var actualText = JsonFormatter.Default.Format(message);

            // Fields in numeric order
            var expectedText = "{ " +
                "'singleInt32': 100, " +
                "'singleInt64': '3210987654321', " +
                "'singleUint32': 4294967295, " +
                "'singleUint64': '18446744073709551615', " +
                "'singleSint32': -456, " +
                "'singleSint64': '-12345678901235', " +
                "'singleFixed32': 23, " +
                "'singleFixed64': '1234567890123', " +
                "'singleSfixed32': -123, " +
                "'singleSfixed64': '-12345678901234', " +
                "'singleFloat': 12.25, " +
                "'singleDouble': 23.5, " +
                "'singleBool': true, " +
                "'singleString': 'test\\twith\\ttabs', " +
                "'singleBytes': 'AQIDBA==', " +
                "'singleNestedMessage': { 'bb': 35 }, " +
                "'singleForeignMessage': { 'c': 10 }, " +
                "'singleImportMessage': { 'd': 20 }, " +
                "'singleNestedEnum': 'FOO', " +
                "'singleForeignEnum': 'FOREIGN_BAR', " +
                "'singleImportEnum': 'IMPORT_BAZ', " +
                "'singlePublicImportMessage': { 'e': 54 }" +
                " }";
            AssertJson(expectedText, actualText);
        }

        [Test]
        public void WithFormatDefaultValues_DoesNotAffectMessageFields()
        {
            var message = new TestAllTypes();
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            var json = formatter.Format(message);
            Assert.IsFalse(json.Contains("\"singleNestedMessage\""));
            Assert.IsFalse(json.Contains("\"singleForeignMessage\""));
            Assert.IsFalse(json.Contains("\"singleImportMessage\""));
        }

        [Test]
        public void WithFormatDefaultValues_DoesNotAffectProto3OptionalFields()
        {
            var message = new TestProto3Optional();
            message.OptionalInt32 = 0;
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            var json = formatter.Format(message);
            // The non-optional proto3 fields are formatted, as is the optional-but-specified field.
            AssertJson("{ 'optionalInt32': 0, 'singularInt32': 0, 'singularInt64': '0' }", json);
        }

        [Test]
        public void WithFormatDefaultValues_DoesNotAffectProto2Fields()
        {
            var message = new TestProtos.Proto2.ForeignMessage();
            message.C = 0;
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            var json = formatter.Format(message);
            // The specified field is formatted, but the non-specified field (d) is not.
            AssertJson("{ 'c': 0 }", json);
        }

        [Test]
        public void WithFormatDefaultValues_DoesNotAffectOneofFields()
        {
            var message = new TestOneof();
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            var json = formatter.Format(message);
            AssertJson("{ }", json);
        }

        [Test]
        public void RepeatedField()
        {
            AssertJson("{ 'repeatedInt32': [ 1, 2, 3, 4, 5 ] }",
                JsonFormatter.Default.Format(new TestAllTypes { RepeatedInt32 = { 1, 2, 3, 4, 5 } }));
        }

        [Test]
        public void MapField_StringString()
        {
            AssertJson("{ 'mapStringString': { 'with spaces': 'bar', 'a': 'b' } }",
                JsonFormatter.Default.Format(new TestMap { MapStringString = { { "with spaces", "bar" }, { "a", "b" } } }));
        }

        [Test]
        public void MapField_Int32Int32()
        {
            // The keys are quoted, but the values aren't.
            AssertJson("{ 'mapInt32Int32': { '0': 1, '2': 3 } }",
                JsonFormatter.Default.Format(new TestMap { MapInt32Int32 = { { 0, 1 }, { 2, 3 } } }));
        }

        [Test]
        public void MapField_BoolBool()
        {
            // The keys are quoted, but the values aren't.
            AssertJson("{ 'mapBoolBool': { 'false': true, 'true': false } }",
                JsonFormatter.Default.Format(new TestMap { MapBoolBool = { { false, true }, { true, false } } }));
        }

        [Test]
        public void NullValueOutsideStruct()
        {
            var message = new NullValueOutsideStruct { NullValue = NullValue.NullValue };
            AssertJson("{ 'nullValue': null }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void NullValueNotInOneof()
        {
            var message = new NullValueNotInOneof();
            AssertJson("{ }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void NullValueNotInOneof_FormatDefaults()
        {
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            var message = new NullValueNotInOneof();
            AssertJson("{ 'nullValue': null }", formatter.Format(message));
        }

        [TestCase(1.0, "1")]
        [TestCase(double.NaN, "'NaN'")]
        [TestCase(double.PositiveInfinity, "'Infinity'")]
        [TestCase(double.NegativeInfinity, "'-Infinity'")]
        public void DoubleRepresentations(double value, string expectedValueText)
        {
            var message = new TestAllTypes { SingleDouble = value };
            string actualText = JsonFormatter.Default.Format(message);
            string expectedText = "{ 'singleDouble': " + expectedValueText + " }";
            AssertJson(expectedText, actualText);
        }

        [Test]
        public void UnknownEnumValueNumeric_SingleField()
        {
            var message = new TestAllTypes { SingleForeignEnum = (ForeignEnum) 100 };
            AssertJson("{ 'singleForeignEnum': 100 }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void UnknownEnumValueNumeric_RepeatedField()
        {
            var message = new TestAllTypes { RepeatedForeignEnum = { ForeignEnum.ForeignBaz, (ForeignEnum) 100, ForeignEnum.ForeignFoo } };
            AssertJson("{ 'repeatedForeignEnum': [ 'FOREIGN_BAZ', 100, 'FOREIGN_FOO' ] }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void UnknownEnumValueNumeric_MapField()
        {
            var message = new TestMap { MapInt32Enum = { { 1, MapEnum.Foo }, { 2, (MapEnum) 100 }, { 3, MapEnum.Bar } } };
            AssertJson("{ 'mapInt32Enum': { '1': 'MAP_ENUM_FOO', '2': 100, '3': 'MAP_ENUM_BAR' } }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void UnknownEnumValue_RepeatedField_AllEntriesUnknown()
        {
            var message = new TestAllTypes { RepeatedForeignEnum = { (ForeignEnum) 200, (ForeignEnum) 100 } };
            AssertJson("{ 'repeatedForeignEnum': [ 200, 100 ] }", JsonFormatter.Default.Format(message));
        }

        [Test]
        [TestCase("a\u17b4b", "a\\u17b4b")] // Explicit
        [TestCase("a\u0601b", "a\\u0601b")] // Ranged
        [TestCase("a\u0605b", "a\u0605b")] // Passthrough (note lack of double backslash...)
        public void SimpleNonAscii(string text, string encoded)
        {
            var message = new TestAllTypes { SingleString = text };
            AssertJson("{ 'singleString': '" + encoded + "' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void SurrogatePairEscaping()
        {
            var message = new TestAllTypes { SingleString = "a\uD801\uDC01b" };
            AssertJson("{ 'singleString': 'a\\ud801\\udc01b' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void InvalidSurrogatePairsFail()
        {
            // Note: don't use TestCase for these, as the strings can't be reliably represented
            // See http://codeblog.jonskeet.uk/2014/11/07/when-is-a-string-not-a-string/

            // Lone low surrogate
            var message = new TestAllTypes { SingleString = "a\uDC01b" };
            Assert.Throws<ArgumentException>(() => JsonFormatter.Default.Format(message));

            // Lone high surrogate
            message = new TestAllTypes { SingleString = "a\uD801b" };
            Assert.Throws<ArgumentException>(() => JsonFormatter.Default.Format(message));
        }

        [Test]
        [TestCase("foo_bar", "fooBar")]
        [TestCase("bananaBanana", "bananaBanana")]
        [TestCase("BANANABanana", "BANANABanana")]
        [TestCase("simple", "simple")]
        [TestCase("ACTION_AND_ADVENTURE", "ACTIONANDADVENTURE")]
        [TestCase("action_and_adventure", "actionAndAdventure")]
        [TestCase("kFoo", "kFoo")]
        [TestCase("HTTPServer", "HTTPServer")]
        [TestCase("CLIENT", "CLIENT")]
        public void ToJsonName(string original, string expected)
        {
            Assert.AreEqual(expected, JsonFormatter.ToJsonName(original));
        }

        [Test]
        [TestCase(null, "{ }")]
        [TestCase("x", "{ 'fooString': 'x' }")]
        [TestCase("", "{ 'fooString': '' }")]
        public void Oneof(string fooStringValue, string expectedJson)
        {
            var message = new TestOneof();
            if (fooStringValue != null)
            {
                message.FooString = fooStringValue;
            }

            // We should get the same result both with and without "format default values".
            var formatter = JsonFormatter.Default;
            AssertJson(expectedJson, formatter.Format(message));
            formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            AssertJson(expectedJson, formatter.Format(message));
        }

        [Test]
        public void WrapperFormatting_Single()
        {
            // Just a few examples, handling both classes and value types, and
            // default vs non-default values
            var message = new TestWellKnownTypes
            {
                Int64Field = 10,
                Int32Field = 0,
                BytesField = ByteString.FromBase64("ABCD"),
                StringField = ""
            };
            var expectedJson = "{ 'int64Field': '10', 'int32Field': 0, 'stringField': '', 'bytesField': 'ABCD' }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
        }

        [Test]
        public void WrapperFormatting_Message()
        {
            Assert.AreEqual("\"\"", JsonFormatter.Default.Format(new StringValue()));
            Assert.AreEqual("0", JsonFormatter.Default.Format(new Int32Value()));
        }

        [Test]
        public void WrapperFormatting_FormatDefaultValuesDoesNotFormatNull()
        {
            // The actual JSON here is very large because there are lots of fields. Just test a couple of them.
            var message = new TestWellKnownTypes { Int32Field = 10 };
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            var actualJson = formatter.Format(message);
            // This *used* to include "int64Field": null, but that was a bug.
            // WithDefaultValues should not affect message fields, including wrapper types.
            Assert.IsFalse(actualJson.Contains("\"int64Field\": null"));
            Assert.IsTrue(actualJson.Contains("\"int32Field\": 10"));
        }

        [Test]
        public void OutputIsInNumericFieldOrder_NoDefaults()
        {
            var formatter = JsonFormatter.Default;
            var message = new TestJsonFieldOrdering { PlainString = "p1", PlainInt32 = 2 };
            AssertJson("{ 'plainString': 'p1', 'plainInt32': 2 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1Int32 = 5, O2String = "o2", PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o2String': 'o2', 'plainInt32': 10, 'o1Int32': 5 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1String = "", O2Int32 = 0, PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o1String': '', 'plainInt32': 10, 'o2Int32': 0 }", formatter.Format(message));
        }

        [Test]
        public void OutputIsInNumericFieldOrder_WithDefaults()
        {
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatDefaultValues(true));
            var message = new TestJsonFieldOrdering();
            AssertJson("{ 'plainString': '', 'plainInt32': 0 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1Int32 = 5, O2String = "o2", PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o2String': 'o2', 'plainInt32': 10, 'o1Int32': 5 }", formatter.Format(message));
            message = new TestJsonFieldOrdering { O1String = "", O2Int32 = 0, PlainInt32 = 10, PlainString = "plain" };
            AssertJson("{ 'plainString': 'plain', 'o1String': '', 'plainInt32': 10, 'o2Int32': 0 }", formatter.Format(message));
        }

        [Test]
        [TestCase("1970-01-01T00:00:00Z", 0)]
        [TestCase("1970-01-01T00:00:00.000000001Z", 1)]
        [TestCase("1970-01-01T00:00:00.000000010Z", 10)]
        [TestCase("1970-01-01T00:00:00.000000100Z", 100)]
        [TestCase("1970-01-01T00:00:00.000001Z", 1000)]
        [TestCase("1970-01-01T00:00:00.000010Z", 10000)]
        [TestCase("1970-01-01T00:00:00.000100Z", 100000)]
        [TestCase("1970-01-01T00:00:00.001Z", 1000000)]
        [TestCase("1970-01-01T00:00:00.010Z", 10000000)]
        [TestCase("1970-01-01T00:00:00.100Z", 100000000)]
        [TestCase("1970-01-01T00:00:00.120Z", 120000000)]
        [TestCase("1970-01-01T00:00:00.123Z", 123000000)]
        [TestCase("1970-01-01T00:00:00.123400Z", 123400000)]
        [TestCase("1970-01-01T00:00:00.123450Z", 123450000)]
        [TestCase("1970-01-01T00:00:00.123456Z", 123456000)]
        [TestCase("1970-01-01T00:00:00.123456700Z", 123456700)]
        [TestCase("1970-01-01T00:00:00.123456780Z", 123456780)]
        [TestCase("1970-01-01T00:00:00.123456789Z", 123456789)]
        public void TimestampStandalone(string expected, int nanos)
        {
            Assert.AreEqual(WrapInQuotes(expected), new Timestamp { Nanos = nanos }.ToString());
        }

        [Test]
        public void TimestampStandalone_FromDateTime()
        {
            // One before and one after the Unix epoch, more easily represented via DateTime.
            Assert.AreEqual("\"1673-06-19T12:34:56Z\"",
                new DateTime(1673, 6, 19, 12, 34, 56, DateTimeKind.Utc).ToTimestamp().ToString());
            Assert.AreEqual("\"2015-07-31T10:29:34Z\"",
                new DateTime(2015, 7, 31, 10, 29, 34, DateTimeKind.Utc).ToTimestamp().ToString());
        }

        [Test]
        [TestCase(-1, -1)] // Would be valid as duration
        [TestCase(1, Timestamp.MaxNanos + 1)]
        [TestCase(Timestamp.UnixSecondsAtBclMaxValue + 1, 0)]
        [TestCase(Timestamp.UnixSecondsAtBclMinValue - 1, 0)]
        public void TimestampStandalone_NonNormalized(long seconds, int nanoseconds)
        {
            var timestamp = new Timestamp { Seconds = seconds, Nanos = nanoseconds };
            Assert.Throws<InvalidOperationException>(() => JsonFormatter.Default.Format(timestamp));
        }

        [Test]
        public void TimestampField()
        {
            var message = new TestWellKnownTypes { TimestampField = new Timestamp() };
            AssertJson("{ 'timestampField': '1970-01-01T00:00:00Z' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        [TestCase(0, 0, "0s")]
        [TestCase(1, 0, "1s")]
        [TestCase(-1, 0, "-1s")]
        [TestCase(0, 1, "0.000000001s")]
        [TestCase(0, 10, "0.000000010s")]
        [TestCase(0, 100, "0.000000100s")]
        [TestCase(0, 1000, "0.000001s")]
        [TestCase(0, 10000, "0.000010s")]
        [TestCase(0, 100000, "0.000100s")]
        [TestCase(0, 1000000, "0.001s")]
        [TestCase(0, 10000000, "0.010s")]
        [TestCase(0, 100000000, "0.100s")]
        [TestCase(0, 120000000, "0.120s")]
        [TestCase(0, 123000000, "0.123s")]
        [TestCase(0, 123400000, "0.123400s")]
        [TestCase(0, 123450000, "0.123450s")]
        [TestCase(0, 123456000, "0.123456s")]
        [TestCase(0, 123456700, "0.123456700s")]
        [TestCase(0, 123456780, "0.123456780s")]
        [TestCase(0, 123456789, "0.123456789s")]
        [TestCase(0, -100000000, "-0.100s")]
        [TestCase(1, 100000000, "1.100s")]
        [TestCase(-1, -100000000, "-1.100s")]
        public void DurationStandalone(long seconds, int nanoseconds, string expected)
        {
            var json = JsonFormatter.Default.Format(new Duration { Seconds = seconds, Nanos = nanoseconds });
            Assert.AreEqual(WrapInQuotes(expected), json);
        }

        [Test]
        [TestCase(1, 2123456789)]
        [TestCase(1, -100000000)]
        public void DurationStandalone_NonNormalized(long seconds, int nanoseconds)
        {
            var duration = new Duration { Seconds = seconds, Nanos = nanoseconds };
            Assert.Throws<InvalidOperationException>(() => JsonFormatter.Default.Format(duration));
        }

        [Test]
        public void DurationField()
        {
            var message = new TestWellKnownTypes { DurationField = new Duration() };
            AssertJson("{ 'durationField': '0s' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void StructSample()
        {
            var message = new Struct
            {
                Fields =
                {
                    { "a", Value.ForNull() },
                    { "b", Value.ForBool(false) },
                    { "c", Value.ForNumber(10.5) },
                    { "d", Value.ForString("text") },
                    { "e", Value.ForList(Value.ForString("t1"), Value.ForNumber(5)) },
                    { "f", Value.ForStruct(new Struct { Fields = { { "nested", Value.ForString("value") } } }) }
                }
            };
            AssertJson("{ 'a': null, 'b': false, 'c': 10.5, 'd': 'text', 'e': [ 't1', 5 ], 'f': { 'nested': 'value' } }", message.ToString());
        }

        [Test]
        [TestCase("foo__bar")]
        [TestCase("foo_3_ar")]
        [TestCase("fooBar")]
        public void FieldMaskInvalid(string input)
        {
            var mask = new FieldMask { Paths = { input } };
            Assert.Throws<InvalidOperationException>(() => JsonFormatter.Default.Format(mask));
        }

        [Test]
        public void FieldMaskStandalone()
        {
            var fieldMask = new FieldMask { Paths = { "", "single", "with_underscore", "nested.field.name", "nested..double_dot" } };
            Assert.AreEqual("\",single,withUnderscore,nested.field.name,nested..doubleDot\"", fieldMask.ToString());

            // Invalid, but we shouldn't create broken JSON...
            fieldMask = new FieldMask { Paths = { "x\\y" } };
            Assert.AreEqual(@"""x\\y""", fieldMask.ToString());
        }

        [Test]
        public void FieldMaskField()
        {
            var message = new TestWellKnownTypes { FieldMaskField = new FieldMask { Paths = { "user.display_name", "photo" } } };
            AssertJson("{ 'fieldMaskField': 'user.displayName,photo' }", JsonFormatter.Default.Format(message));
        }

        // SourceContext is an example of a well-known type with no special JSON handling
        [Test]
        public void SourceContextStandalone()
        {
            var message = new SourceContext { FileName = "foo.proto" };
            AssertJson("{ 'fileName': 'foo.proto' }", JsonFormatter.Default.Format(message));
        }

        [Test]
        public void AnyWellKnownType()
        {
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithTypeRegistry(TypeRegistry.FromMessages(Timestamp.Descriptor)));
            var timestamp = new DateTime(1673, 6, 19, 12, 34, 56, DateTimeKind.Utc).ToTimestamp();
            var any = Any.Pack(timestamp);
            AssertJson("{ '@type': 'type.googleapis.com/google.protobuf.Timestamp', 'value': '1673-06-19T12:34:56Z' }", formatter.Format(any));
        }

        [Test]
        public void AnyMessageType()
        {
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithTypeRegistry(TypeRegistry.FromMessages(TestAllTypes.Descriptor)));
            var message = new TestAllTypes { SingleInt32 = 10, SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 20 } };
            var any = Any.Pack(message);
            AssertJson("{ '@type': 'type.googleapis.com/protobuf_unittest3.TestAllTypes', 'singleInt32': 10, 'singleNestedMessage': { 'bb': 20 } }", formatter.Format(any));
        }

        [Test]
        public void AnyMessageType_CustomPrefix()
        {
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithTypeRegistry(TypeRegistry.FromMessages(TestAllTypes.Descriptor)));
            var message = new TestAllTypes { SingleInt32 = 10 };
            var any = Any.Pack(message, "foo.bar/baz");
            AssertJson("{ '@type': 'foo.bar/baz/protobuf_unittest3.TestAllTypes', 'singleInt32': 10 }", formatter.Format(any));
        }

        [Test]
        public void AnyNested()
        {
            var registry = TypeRegistry.FromMessages(TestWellKnownTypes.Descriptor, TestAllTypes.Descriptor);
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithTypeRegistry(registry));

            // Nest an Any as the value of an Any.
            var doubleNestedMessage = new TestAllTypes { SingleInt32 = 20 };
            var nestedMessage = Any.Pack(doubleNestedMessage);
            var message = new TestWellKnownTypes { AnyField = Any.Pack(nestedMessage) };
            AssertJson("{ 'anyField': { '@type': 'type.googleapis.com/google.protobuf.Any', 'value': { '@type': 'type.googleapis.com/protobuf_unittest3.TestAllTypes', 'singleInt32': 20 } } }",
                formatter.Format(message));
        }

        [Test]
        public void AnyUnknownType()
        {
            // The default type registry doesn't have any types in it.
            var message = new TestAllTypes();
            var any = Any.Pack(message);
            Assert.Throws<InvalidOperationException>(() => JsonFormatter.Default.Format(any));
        }

        [Test]
        [TestCase(typeof(BoolValue), true, "true")]
        [TestCase(typeof(Int32Value), 32, "32")]
        [TestCase(typeof(Int64Value), 32L, "\"32\"")]
        [TestCase(typeof(UInt32Value), 32U, "32")]
        [TestCase(typeof(UInt64Value), 32UL, "\"32\"")]
        [TestCase(typeof(StringValue), "foo", "\"foo\"")]
        [TestCase(typeof(FloatValue), 1.5f, "1.5")]
        [TestCase(typeof(DoubleValue), 1.5d, "1.5")]
        public void Wrappers_Standalone(System.Type wrapperType, object value, string expectedJson)
        {
            IMessage populated = (IMessage)Activator.CreateInstance(wrapperType);
            populated.Descriptor.Fields[WrappersReflection.WrapperValueFieldNumber].Accessor.SetValue(populated, value);
            Assert.AreEqual(expectedJson, JsonFormatter.Default.Format(populated));
        }

        // Sanity tests for WriteValue. Not particularly comprehensive, as it's all covered above already,
        // as FormatMessage uses WriteValue.

        [TestCase(null, "null")]
        [TestCase(1, "1")]
        [TestCase(1L, "'1'")]
        [TestCase(0.5f, "0.5")]
        [TestCase(0.5d, "0.5")]
        [TestCase("text", "'text'")]
        [TestCase("x\ny", @"'x\ny'")]
        [TestCase(ForeignEnum.ForeignBar, "'FOREIGN_BAR'")]
        public void WriteValue_Constant(object value, string expectedJson)
        {
            AssertWriteValue(value, expectedJson);
        }

        [Test]
        public void WriteValue_Timestamp()
        {
            var value = new DateTime(1673, 6, 19, 12, 34, 56, DateTimeKind.Utc).ToTimestamp();
            AssertWriteValue(value, "'1673-06-19T12:34:56Z'");
        }

        [Test]
        public void WriteValue_Message()
        {
            var value = new TestAllTypes { SingleInt32 = 100, SingleInt64 = 3210987654321L };
            AssertWriteValue(value, "{ 'singleInt32': 100, 'singleInt64': '3210987654321' }");
        }

        [Test]
        public void WriteValue_List()
        {
            var value = new RepeatedField<int> { 1, 2, 3 };
            AssertWriteValue(value, "[ 1, 2, 3 ]");
        }

        [Test]
        public void Proto2_DefaultValuesWritten()
        {
            var value = new ProtobufTestMessages.Proto2.TestAllTypesProto2() { FieldName13 = 0 };
            AssertWriteValue(value, "{ 'FieldName13': 0 }");
        }

        private static void AssertWriteValue(object value, string expectedJson)
        {
            var writer = new StringWriter();
            JsonFormatter.Default.WriteValue(writer, value);
            string actual = writer.ToString();
            AssertJson(expectedJson, actual);
        }

        /// <summary>
        /// Checks that the actual JSON is the same as the expected JSON - but after replacing
        /// all apostrophes in the expected JSON with double quotes. This basically makes the tests easier
        /// to read.
        /// </summary>
        private static void AssertJson(string expectedJsonWithApostrophes, string actualJson)
        {
            var expectedJson = expectedJsonWithApostrophes.Replace("'", "\"");
            Assert.AreEqual(expectedJson, actualJson);
        }
    }
}
