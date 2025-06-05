#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Reflection;
using Google.Protobuf.TestProtos;
using Google.Protobuf.WellKnownTypes;
using NUnit.Framework;
using ProtobufTestMessages.Proto2;
using ProtobufTestMessages.Proto3;
using ProtobufUnittest;
using System;
using UnitTest.Issues.TestProtos;

namespace Google.Protobuf
{
    /// <summary>
    /// Unit tests for JSON parsing.
    /// </summary>
    public class JsonParserTest
    {
        // Sanity smoke test
        [Test]
        public void AllTypesRoundtrip()
        {
            AssertRoundtrip(SampleMessages.CreateFullTestAllTypes());
        }

        [Test]
        public void Maps()
        {
            AssertRoundtrip(new TestMap { MapStringString = { { "with spaces", "bar" }, { "a", "b" } } });
            AssertRoundtrip(new TestMap { MapInt32Int32 = { { 0, 1 }, { 2, 3 } } });
            AssertRoundtrip(new TestMap { MapBoolBool = { { false, true }, { true, false } } });
        }

        [Test]
        [TestCase(" 1 ")]
        [TestCase("+1")]
        [TestCase("1,000")]
        [TestCase("1.5")]
        public void IntegerMapKeysAreStrict(string keyText)
        {
            // Test that integer parsing is strict. We assume that if this is correct for int32,
            // it's correct for other numeric key types.
            var json = "{ \"mapInt32Int32\": { \"" + keyText + "\" : \"1\" } }";
            Assert.Throws<InvalidProtocolBufferException>(() => JsonParser.Default.Parse<TestMap>(json));
        }

        [Test]
        public void OriginalFieldNameAccepted()
        {
            var json = "{ \"single_int32\": 10 }";
            var expected = new TestAllTypes { SingleInt32 = 10 };
            Assert.AreEqual(expected, TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        public void SourceContextRoundtrip()
        {
            AssertRoundtrip(new SourceContext { FileName = "foo.proto" });
        }

        [Test]
        public void SingularWrappers_DefaultNonNullValues()
        {
            var message = new TestWellKnownTypes
            {
                StringField = "",
                BytesField = ByteString.Empty,
                BoolField = false,
                FloatField = 0f,
                DoubleField = 0d,
                Int32Field = 0,
                Int64Field = 0,
                Uint32Field = 0,
                Uint64Field = 0
            };
            AssertRoundtrip(message);
        }

        [Test]
        public void SingularWrappers_NonDefaultValues()
        {
            var message = new TestWellKnownTypes
            {
                StringField = "x",
                BytesField = ByteString.CopyFrom(1, 2, 3),
                BoolField = true,
                FloatField = 12.5f,
                DoubleField = 12.25d,
                Int32Field = 1,
                Int64Field = 2,
                Uint32Field = 3,
                Uint64Field = 4
            };
            AssertRoundtrip(message);
        }

        [Test]
        public void SingularWrappers_ExplicitNulls()
        {
            // When we parse the "valueField": null part, we remember it... basically, it's one case
            // where explicit default values don't fully roundtrip.
            var message = new TestWellKnownTypes { ValueField = Value.ForNull() };
            var json = new JsonFormatter(new JsonFormatter.Settings(true)).Format(message);
            var parsed = JsonParser.Default.Parse<TestWellKnownTypes>(json);
            Assert.AreEqual(message, parsed);
        }

        [Test]
        [TestCase(typeof(BoolValue), "true", true)]
        [TestCase(typeof(Int32Value), "32", 32)]
        [TestCase(typeof(Int64Value), "32", 32L)]
        [TestCase(typeof(Int64Value), "\"32\"", 32L)]
        [TestCase(typeof(UInt32Value), "32", 32U)]
        [TestCase(typeof(UInt64Value), "\"32\"", 32UL)]
        [TestCase(typeof(UInt64Value), "32", 32UL)]
        [TestCase(typeof(StringValue), "\"foo\"", "foo")]
        [TestCase(typeof(FloatValue), "1.5", 1.5f)]
        [TestCase(typeof(DoubleValue), "1.5", 1.5d)]
        public void Wrappers_Standalone(System.Type wrapperType, string json, object expectedValue)
        {
            IMessage parsed = (IMessage)Activator.CreateInstance(wrapperType);
            IMessage expected = (IMessage)Activator.CreateInstance(wrapperType);
            JsonParser.Default.Merge(parsed, "null");
            Assert.AreEqual(expected, parsed);

            JsonParser.Default.Merge(parsed, json);
            expected.Descriptor.Fields[WrappersReflection.WrapperValueFieldNumber].Accessor.SetValue(expected, expectedValue);
            Assert.AreEqual(expected, parsed);
        }

        [Test]
        public void ExplicitNullValue()
        {
            string json = "{\"valueField\": null}";
            var message = JsonParser.Default.Parse<TestWellKnownTypes>(json);
            Assert.AreEqual(new TestWellKnownTypes { ValueField = Value.ForNull() }, message);
        }

        [Test]
        public void BytesWrapper_Standalone()
        {
            ByteString data = ByteString.CopyFrom(1, 2, 3);
            // Can't do this with attributes...
            var parsed = JsonParser.Default.Parse<BytesValue>(WrapInQuotes(data.ToBase64()));
            var expected = new BytesValue { Value = data };
            Assert.AreEqual(expected, parsed);
        }

        [Test]
        public void RepeatedWrappers()
        {
            var message = new RepeatedWellKnownTypes
            {
                BoolField = { true, false },
                BytesField = { ByteString.CopyFrom(1, 2, 3), ByteString.CopyFrom(4, 5, 6), ByteString.Empty },
                DoubleField = { 12.5, -1.5, 0d },
                FloatField = { 123.25f, -20f, 0f },
                Int32Field = { int.MaxValue, int.MinValue, 0 },
                Int64Field = { long.MaxValue, long.MinValue, 0L },
                StringField = { "First", "Second", "" },
                Uint32Field = { uint.MaxValue, uint.MinValue, 0U },
                Uint64Field = { ulong.MaxValue, ulong.MinValue, 0UL },
            };
            AssertRoundtrip(message);
        }

        [Test]
        public void RepeatedField_NullElementProhibited()
        {
            string json = "{ \"repeated_foreign_message\": [null] }";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        public void RepeatedField_NullOverallValueAllowed()
        {
            string json = "{ \"repeated_foreign_message\": null }";
            Assert.AreEqual(new TestAllTypes(), TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("{ \"mapInt32Int32\": { \"10\": null }")]
        [TestCase("{ \"mapStringString\": { \"abc\": null }")]
        [TestCase("{ \"mapInt32ForeignMessage\": { \"10\": null }")]
        public void MapField_NullValueProhibited(string json)
        {
            Assert.Throws<InvalidProtocolBufferException>(() => TestMap.Parser.ParseJson(json));
        }

        [Test]
        public void MapField_NullOverallValueAllowed()
        {
            string json = "{ \"mapInt32Int32\": null }";
            Assert.AreEqual(new TestMap(), TestMap.Parser.ParseJson(json));
        }

        [Test]
        public void IndividualWrapperTypes()
        {
            Assert.AreEqual(new StringValue { Value = "foo" }, StringValue.Parser.ParseJson("\"foo\""));
            Assert.AreEqual(new Int32Value { Value = 1 }, Int32Value.Parser.ParseJson("1"));
            // Can parse strings directly too
            Assert.AreEqual(new Int32Value { Value = 1 }, Int32Value.Parser.ParseJson("\"1\""));
        }

        private static void AssertRoundtrip<T>(T message) where T : IMessage<T>, new()
        {
            var clone = message.Clone();
            var json = JsonFormatter.Default.Format(message);
            var parsed = JsonParser.Default.Parse<T>(json);
            Assert.AreEqual(clone, parsed);
        }

        [Test]
        [TestCase("0", 0)]
        [TestCase("-0", 0)] // Not entirely clear whether we intend to allow this...
        [TestCase("1", 1)]
        [TestCase("-1", -1)]
        [TestCase("2147483647", 2147483647)]
        [TestCase("-2147483648", -2147483648)]
        public void StringToInt32_Valid(string jsonValue, int expectedParsedValue)
        {
            string json = "{ \"singleInt32\": \"" + jsonValue + "\"}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleInt32);
        }

        [Test]
        [TestCase("+0")]
        [TestCase(" 1")]
        [TestCase("1 ")]
        [TestCase("00")]
        [TestCase("-00")]
        [TestCase("--1")]
        [TestCase("+1")]
        [TestCase("1.5")]
        [TestCase("1e10")]
        [TestCase("2147483648")]
        [TestCase("-2147483649")]
        public void StringToInt32_Invalid(string jsonValue)
        {
            string json = "{ \"singleInt32\": \"" + jsonValue + "\"}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0U)]
        [TestCase("1", 1U)]
        [TestCase("4294967295", 4294967295U)]
        public void StringToUInt32_Valid(string jsonValue, uint expectedParsedValue)
        {
            string json = "{ \"singleUint32\": \"" + jsonValue + "\"}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleUint32);
        }

        // Assume that anything non-bounds-related is covered in the Int32 case
        [Test]
        [TestCase("-1")]
        [TestCase("4294967296")]
        public void StringToUInt32_Invalid(string jsonValue)
        {
            string json = "{ \"singleUint32\": \"" + jsonValue + "\"}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0L)]
        [TestCase("1", 1L)]
        [TestCase("-1", -1L)]
        [TestCase("9223372036854775807", 9223372036854775807)]
        [TestCase("-9223372036854775808", -9223372036854775808)]
        public void StringToInt64_Valid(string jsonValue, long expectedParsedValue)
        {
            string json = "{ \"singleInt64\": \"" + jsonValue + "\"}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleInt64);
        }

        // Assume that anything non-bounds-related is covered in the Int32 case
        [Test]
        [TestCase("-9223372036854775809")]
        [TestCase("9223372036854775808")]
        public void StringToInt64_Invalid(string jsonValue)
        {
            string json = "{ \"singleInt64\": \"" + jsonValue + "\"}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0UL)]
        [TestCase("1", 1UL)]
        [TestCase("18446744073709551615", 18446744073709551615)]
        public void StringToUInt64_Valid(string jsonValue, ulong expectedParsedValue)
        {
            string json = "{ \"singleUint64\": \"" + jsonValue + "\"}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleUint64);
        }

        // Assume that anything non-bounds-related is covered in the Int32 case
        [Test]
        [TestCase("-1")]
        [TestCase("18446744073709551616")]
        public void StringToUInt64_Invalid(string jsonValue)
        {
            string json = "{ \"singleUint64\": \"" + jsonValue + "\"}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0d)]
        [TestCase("1", 1d)]
        [TestCase("1.000000", 1d)]
        [TestCase("1.0000000000000000000000001", 1d)] // We don't notice that we haven't preserved the exact value
        [TestCase("-1", -1d)]
        [TestCase("1e1", 10d)]
        [TestCase("1e01", 10d)] // Leading decimals are allowed in exponents
        [TestCase("1E1", 10d)] // Either case is fine
        [TestCase("-1e1", -10d)]
        [TestCase("1.5e1", 15d)]
        [TestCase("-1.5e1", -15d)]
        [TestCase("15e-1", 1.5d)]
        [TestCase("-15e-1", -1.5d)]
        [TestCase("1.79769e308", 1.79769e308)]
        [TestCase("-1.79769e308", -1.79769e308)]
        [TestCase("Infinity", double.PositiveInfinity)]
        [TestCase("-Infinity", double.NegativeInfinity)]
        [TestCase("NaN", double.NaN)]
        public void StringToDouble_Valid(string jsonValue, double expectedParsedValue)
        {
            string json = "{ \"singleDouble\": \"" + jsonValue + "\"}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleDouble);
        }

        [Test]
        [TestCase("1.7977e308")]
        [TestCase("-1.7977e308")]
        [TestCase("1e309")]
        [TestCase("1,0")]
        [TestCase("1.0.0")]
        [TestCase("+1")]
        [TestCase("00")]
        [TestCase("01")]
        [TestCase("-00")]
        [TestCase("-01")]
        [TestCase("--1")]
        [TestCase(" Infinity")]
        [TestCase(" -Infinity")]
        [TestCase("NaN ")]
        [TestCase("Infinity ")]
        [TestCase("-Infinity ")]
        [TestCase(" NaN")]
        [TestCase("INFINITY")]
        [TestCase("nan")]
        [TestCase("\u00BD")] // 1/2 as a single Unicode character. Just sanity checking...
        public void StringToDouble_Invalid(string jsonValue)
        {
            string json = "{ \"singleDouble\": \"" + jsonValue + "\"}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0f)]
        [TestCase("1", 1f)]
        [TestCase("1.000000", 1f)]
        [TestCase("-1", -1f)]
        [TestCase("3.402823e38", 3.402823e38f)]
        [TestCase("-3.402823e38", -3.402823e38f)]
        [TestCase("1.5e1", 15f)]
        [TestCase("15e-1", 1.5f)]
        public void StringToFloat_Valid(string jsonValue, float expectedParsedValue)
        {
            string json = "{ \"singleFloat\": \"" + jsonValue + "\"}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleFloat);
        }

        [Test]
        [TestCase("3.402824e38")]
        [TestCase("-3.402824e38")]
        [TestCase("1,0")]
        [TestCase("1.0.0")]
        [TestCase("+1")]
        [TestCase("00")]
        [TestCase("--1")]
        public void StringToFloat_Invalid(string jsonValue)
        {
            string json = "{ \"singleFloat\": \"" + jsonValue + "\"}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0)]
        [TestCase("-0", 0)] // Not entirely clear whether we intend to allow this...
        [TestCase("1", 1)]
        [TestCase("-1", -1)]
        [TestCase("2147483647", 2147483647)]
        [TestCase("-2147483648", -2147483648)]
        [TestCase("1e1", 10)]
        [TestCase("-1e1", -10)]
        [TestCase("10.00", 10)]
        [TestCase("-10.00", -10)]
        public void NumberToInt32_Valid(string jsonValue, int expectedParsedValue)
        {
            string json = "{ \"singleInt32\": " + jsonValue + "}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleInt32);
        }

        [Test]
        [TestCase("+0", typeof(InvalidJsonException))]
        [TestCase("00", typeof(InvalidJsonException))]
        [TestCase("-00", typeof(InvalidJsonException))]
        [TestCase("--1", typeof(InvalidJsonException))]
        [TestCase("+1", typeof(InvalidJsonException))]
        [TestCase("1.5", typeof(InvalidProtocolBufferException))]
        // Value is out of range
        [TestCase("1e10", typeof(InvalidProtocolBufferException))]
        [TestCase("2147483648", typeof(InvalidProtocolBufferException))]
        [TestCase("-2147483649", typeof(InvalidProtocolBufferException))]
        public void NumberToInt32_Invalid(string jsonValue, System.Type expectedExceptionType)
        {
            string json = "{ \"singleInt32\": " + jsonValue + "}";
            Assert.Throws(expectedExceptionType, () => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0U)]
        [TestCase("1", 1U)]
        [TestCase("4294967295", 4294967295U)]
        public void NumberToUInt32_Valid(string jsonValue, uint expectedParsedValue)
        {
            string json = "{ \"singleUint32\": " + jsonValue + "}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleUint32);
        }

        // Assume that anything non-bounds-related is covered in the Int32 case
        [Test]
        [TestCase("-1")]
        [TestCase("4294967296")]
        public void NumberToUInt32_Invalid(string jsonValue)
        {
            string json = "{ \"singleUint32\": " + jsonValue + "}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0L)]
        [TestCase("1", 1L)]
        [TestCase("-1", -1L)]
        // long.MaxValue isn't actually representable as a double. This string value is the highest
        // representable value which isn't greater than long.MaxValue.
        [TestCase("9223372036854774784", 9223372036854774784)]
        [TestCase("-9223372036854775808", -9223372036854775808)]
        public void NumberToInt64_Valid(string jsonValue, long expectedParsedValue)
        {
            string json = "{ \"singleInt64\": " + jsonValue + "}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleInt64);
        }

        // Assume that anything non-bounds-related is covered in the Int32 case
        [Test]
        // Runtime implementation differences produce different results for values just outside
        // (long.MinValue, long.MaxValue) which cannot be exactly represented as a double. Use the
        // next values exactly representable as doubles to ensure consistency.
        [TestCase("9223372036854777856")]
        [TestCase("-9223372036854777856")]
        public void NumberToInt64_Invalid(string jsonValue)
        {
            string json = "{ \"singleInt64\": " + jsonValue + "}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0UL)]
        [TestCase("1", 1UL)]
        // ulong.MaxValue isn't representable as a double. This value is the largest double within
        // the range of ulong.
        [TestCase("18446744073709549568", 18446744073709549568UL)]
        public void NumberToUInt64_Valid(string jsonValue, ulong expectedParsedValue)
        {
            string json = "{ \"singleUint64\": " + jsonValue + "}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleUint64);
        }

        // Assume that anything non-bounds-related is covered in the Int32 case
        [Test]
        [TestCase("-1")]
        // Runtime implementation differences produce different results for values just beyond
        // ulong.MaxValue which cannot be exactly represented as a double. Use the  next value
        // exactly representable as a double to ensure consistency.
        [TestCase("18446744073709555712")]
        public void NumberToUInt64_Invalid(string jsonValue)
        {
            string json = "{ \"singleUint64\": " + jsonValue + "}";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0d)]
        [TestCase("1", 1d)]
        [TestCase("1.000000", 1d)]
        [TestCase("1.0000000000000000000000001", 1d)] // We don't notice that we haven't preserved the exact value
        [TestCase("-1", -1d)]
        [TestCase("1e1", 10d)]
        [TestCase("1e01", 10d)] // Leading decimals are allowed in exponents
        [TestCase("1E1", 10d)] // Either case is fine
        [TestCase("-1e1", -10d)]
        [TestCase("1.5e1", 15d)]
        [TestCase("-1.5e1", -15d)]
        [TestCase("15e-1", 1.5d)]
        [TestCase("-15e-1", -1.5d)]
        [TestCase("1.79769e308", 1.79769e308)]
        [TestCase("-1.79769e308", -1.79769e308)]
        public void NumberToDouble_Valid(string jsonValue, double expectedParsedValue)
        {
            string json = "{ \"singleDouble\": " + jsonValue + "}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleDouble);
        }

        [Test]
        [TestCase("1.7977e308")]
        [TestCase("-1.7977e308")]
        [TestCase("1e309")]
        [TestCase("1,0")]
        [TestCase("1.0.0")]
        [TestCase("+1")]
        [TestCase("00")]
        [TestCase("--1")]
        [TestCase("\u00BD")] // 1/2 as a single Unicode character. Just sanity checking...
        public void NumberToDouble_Invalid(string jsonValue)
        {
            string json = "{ \"singleDouble\": " + jsonValue + "}";
            Assert.Throws<InvalidJsonException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("0", 0f)]
        [TestCase("1", 1f)]
        [TestCase("1.000000", 1f)]
        [TestCase("-1", -1f)]
        [TestCase("3.402823e38", 3.402823e38f)]
        [TestCase("-3.402823e38", -3.402823e38f)]
        [TestCase("1.5e1", 15f)]
        [TestCase("15e-1", 1.5f)]
        [TestCase("3.4028235e38", float.MaxValue)]
        [TestCase("-3.4028235e38", float.MinValue)]
        [TestCase("3.4028235e+38", float.MaxValue)]
        [TestCase("-3.4028235e+38", float.MinValue)]
        public void NumberToFloat_Valid(string jsonValue, float expectedParsedValue)
        {
            string json = "{ \"singleFloat\": " + jsonValue + "}";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(expectedParsedValue, parsed.SingleFloat);
        }

        [Test]
        [TestCase("3.4028236e38", typeof(InvalidProtocolBufferException))]
        [TestCase("-3.4028236e38", typeof(InvalidProtocolBufferException))]
        [TestCase("3.4028236e+38", typeof(InvalidProtocolBufferException))]
        [TestCase("-3.4028236e+38", typeof(InvalidProtocolBufferException))]
        [TestCase("1,0", typeof(InvalidJsonException))]
        [TestCase("1.0.0", typeof(InvalidJsonException))]
        [TestCase("+1", typeof(InvalidJsonException))]
        [TestCase("00", typeof(InvalidJsonException))]
        [TestCase("--1", typeof(InvalidJsonException))]
        public void NumberToFloat_Invalid(string jsonValue, System.Type expectedExceptionType)
        {
            string json = "{ \"singleFloat\": " + jsonValue + "}";
            Assert.Throws(expectedExceptionType, () => TestAllTypes.Parser.ParseJson(json));
        }

        // The simplest way of testing that the value has parsed correctly is to reformat it,
        // as we trust the formatting. In many cases that will give the same result as the input,
        // so in those cases we accept an expectedFormatted value of null. Sometimes the results
        // will be different though, due to a different number of digits being provided.
        [Test]
        // Z offset
        [TestCase("2015-10-09T14:46:23.123456789Z", null)]
        [TestCase("2015-10-09T14:46:23.123456Z", null)]
        [TestCase("2015-10-09T14:46:23.123Z", null)]
        [TestCase("2015-10-09T14:46:23Z", null)]
        [TestCase("2015-10-09T14:46:23.123456000Z", "2015-10-09T14:46:23.123456Z")]
        [TestCase("2015-10-09T14:46:23.1234560Z", "2015-10-09T14:46:23.123456Z")]
        [TestCase("2015-10-09T14:46:23.123000000Z", "2015-10-09T14:46:23.123Z")]
        [TestCase("2015-10-09T14:46:23.1230Z", "2015-10-09T14:46:23.123Z")]
        [TestCase("2015-10-09T14:46:23.00Z", "2015-10-09T14:46:23Z")]

        // +00:00 offset
        [TestCase("2015-10-09T14:46:23.123456789+00:00", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-09T14:46:23.123456+00:00", "2015-10-09T14:46:23.123456Z")]
        [TestCase("2015-10-09T14:46:23.123+00:00", "2015-10-09T14:46:23.123Z")]
        [TestCase("2015-10-09T14:46:23+00:00", "2015-10-09T14:46:23Z")]
        [TestCase("2015-10-09T14:46:23.123456000+00:00", "2015-10-09T14:46:23.123456Z")]
        [TestCase("2015-10-09T14:46:23.1234560+00:00", "2015-10-09T14:46:23.123456Z")]
        [TestCase("2015-10-09T14:46:23.123000000+00:00", "2015-10-09T14:46:23.123Z")]
        [TestCase("2015-10-09T14:46:23.1230+00:00", "2015-10-09T14:46:23.123Z")]
        [TestCase("2015-10-09T14:46:23.00+00:00", "2015-10-09T14:46:23Z")]

        // Other offsets (assume by now that the subsecond handling is okay)
        [TestCase("2015-10-09T15:46:23.123456789+01:00", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-09T13:46:23.123456789-01:00", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-09T15:16:23.123456789+00:30", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-09T14:16:23.123456789-00:30", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-09T16:31:23.123456789+01:45", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-09T13:01:23.123456789-01:45", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-10T08:46:23.123456789+18:00", "2015-10-09T14:46:23.123456789Z")]
        [TestCase("2015-10-08T20:46:23.123456789-18:00", "2015-10-09T14:46:23.123456789Z")]

        // Leap years and min/max
        [TestCase("2016-02-29T14:46:23.123456789Z", null)]
        [TestCase("2000-02-29T14:46:23.123456789Z", null)]
        [TestCase("0001-01-01T00:00:00Z", null)]
        [TestCase("9999-12-31T23:59:59.999999999Z", null)]
        public void Timestamp_Valid(string jsonValue, string expectedFormatted)
        {
            expectedFormatted ??= jsonValue;
            string json = WrapInQuotes(jsonValue);
            var parsed = Timestamp.Parser.ParseJson(json);
            Assert.AreEqual(WrapInQuotes(expectedFormatted), parsed.ToString());
        }

        [Test]
        [TestCase("2015-10-09 14:46:23.123456789Z", Description = "No T between date and time")]
        [TestCase("2015/10/09T14:46:23.123456789Z", Description = "Wrong date separators")]
        [TestCase("2015-10-09T14.46.23.123456789Z", Description = "Wrong time separators")]
        [TestCase("2015-10-09T14:46:23,123456789Z", Description = "Wrong fractional second separators (valid ISO-8601 though)")]
        [TestCase(" 2015-10-09T14:46:23.123456789Z", Description = "Whitespace at start")]
        [TestCase("2015-10-09T14:46:23.123456789Z ", Description = "Whitespace at end")]
        [TestCase("2015-10-09T14:46:23.1234567890", Description = "Too many digits")]
        [TestCase("2015-10-09T14:46:23.123456789", Description = "No offset")]
        [TestCase("2015-13-09T14:46:23.123456789Z", Description = "Invalid month")]
        [TestCase("2015-10-32T14:46:23.123456789Z", Description = "Invalid day")]
        [TestCase("2015-10-09T24:00:00.000000000Z", Description = "Invalid hour (valid ISO-8601 though)")]
        [TestCase("2015-10-09T14:60:23.123456789Z", Description = "Invalid minutes")]
        [TestCase("2015-10-09T14:46:60.123456789Z", Description = "Invalid seconds")]
        [TestCase("2015-10-09T14:46:23.123456789+18:01", Description = "Offset too large (positive)")]
        [TestCase("2015-10-09T14:46:23.123456789-18:01", Description = "Offset too large (negative)")]
        [TestCase("2015-10-09T14:46:23.123456789-00:00", Description = "Local offset (-00:00) makes no sense here")]
        [TestCase("0001-01-01T00:00:00+00:01", Description = "Value before earliest when offset applied")]
        [TestCase("9999-12-31T23:59:59.999999999-00:01", Description = "Value after latest when offset applied")]
        [TestCase("2100-02-29T14:46:23.123456789Z", Description = "Feb 29th on a non-leap-year")]
        public void Timestamp_Invalid(string jsonValue)
        {
            string json = WrapInQuotes(jsonValue);
            Assert.Throws<InvalidProtocolBufferException>(() => Timestamp.Parser.ParseJson(json));
        }

        [Test]
        public void StructValue_Null()
        {
            Assert.AreEqual(new Value { NullValue = 0 }, Value.Parser.ParseJson("null"));
        }

        [Test]
        public void StructValue_String()
        {
            Assert.AreEqual(new Value { StringValue = "hi" }, Value.Parser.ParseJson("\"hi\""));
        }

        [Test]
        public void StructValue_Bool()
        {
            Assert.AreEqual(new Value { BoolValue = true }, Value.Parser.ParseJson("true"));
            Assert.AreEqual(new Value { BoolValue = false }, Value.Parser.ParseJson("false"));
        }

        [Test]
        public void StructValue_List()
        {
            Assert.AreEqual(Value.ForList(Value.ForNumber(1), Value.ForString("x")), Value.Parser.ParseJson("[1, \"x\"]"));
        }

        [Test]
        public void Value_List_WithNullElement()
        {
            var expected = Value.ForList(Value.ForString("x"), Value.ForNull(), Value.ForString("y"));
            var actual = Value.Parser.ParseJson("[\"x\", null, \"y\"]");
            Assert.AreEqual(expected, actual);
        }

        [Test]
        public void StructValue_NullElement()
        {
            var expected = Value.ForStruct(new Struct { Fields = { { "x", Value.ForNull() } } });
            var actual = Value.Parser.ParseJson("{ \"x\": null }");
            Assert.AreEqual(expected, actual);
        }

        [Test]
        public void ParseListValue()
        {
            Assert.AreEqual(new ListValue { Values = { Value.ForNumber(1), Value.ForString("x") } }, ListValue.Parser.ParseJson("[1, \"x\"]"));
        }

        [Test]
        public void StructValue_Struct()
        {
            Assert.AreEqual(
                Value.ForStruct(new Struct { Fields = { { "x", Value.ForNumber(1) }, { "y", Value.ForString("z") } } }),
                Value.Parser.ParseJson("{ \"x\": 1, \"y\": \"z\" }"));
        }

        [Test]
        public void ParseStruct()
        {
            Assert.AreEqual(new Struct { Fields = { { "x", Value.ForNumber(1) }, { "y", Value.ForString("z") } } },
                Struct.Parser.ParseJson("{ \"x\": 1, \"y\": \"z\" }"));
        }

        // TODO for duration parsing: upper and lower bounds.
        // +/- 315576000000 seconds

        [Test]
        [TestCase("1.123456789s", null)]
        [TestCase("1.123456s", null)]
        [TestCase("1.123s", null)]
        [TestCase("1.12300s", "1.123s")]
        [TestCase("1.12345s", "1.123450s")]
        [TestCase("1s", null)]
        [TestCase("-1.123456789s", null)]
        [TestCase("-1.123456s", null)]
        [TestCase("-1.123s", null)]
        [TestCase("-1s", null)]
        [TestCase("0.123s", null)]
        [TestCase("-0.123s", null)]
        [TestCase("123456.123s", null)]
        [TestCase("-123456.123s", null)]
        // Upper and lower bounds
        [TestCase("315576000000s", null)]
        [TestCase("-315576000000s", null)]
        public void Duration_Valid(string jsonValue, string expectedFormatted)
        {
            expectedFormatted ??= jsonValue;
            string json = WrapInQuotes(jsonValue);
            var parsed = Duration.Parser.ParseJson(json);
            Assert.AreEqual(WrapInQuotes(expectedFormatted), parsed.ToString());
        }

        // The simplest way of testing that the value has parsed correctly is to reformat it,
        // as we trust the formatting. In many cases that will give the same result as the input,
        // so in those cases we accept an expectedFormatted value of null. Sometimes the results
        // will be different though, due to a different number of digits being provided.
        [Test]
        [TestCase("1.1234567890s", Description = "Too many digits")]
        [TestCase("1.123456789", Description = "No suffix")]
        [TestCase("1.123456789ss", Description = "Too much suffix")]
        [TestCase("1.123456789S", Description = "Upper case suffix")]
        [TestCase("+1.123456789s", Description = "Leading +")]
        [TestCase(".123456789s", Description = "No integer before the fraction")]
        [TestCase("1,123456789s", Description = "Comma as decimal separator")]
        [TestCase("1x1.123456789s", Description = "Non-digit in integer part")]
        [TestCase("1.1x3456789s", Description = "Non-digit in fractional part")]
        [TestCase(" 1.123456789s", Description = "Whitespace before fraction")]
        [TestCase("1.123456789s ", Description = "Whitespace after value")]
        [TestCase("01.123456789s", Description = "Leading zero (positive)")]
        [TestCase("-01.123456789s", Description = "Leading zero (negative)")]
        [TestCase("--0.123456789s", Description = "Double minus sign")]
        // Violate upper/lower bounds in various ways
        [TestCase("315576000001s", Description = "Integer part too large")]
        [TestCase("3155760000000s", Description = "Integer part too long (positive)")]
        [TestCase("-3155760000000s", Description = "Integer part too long (negative)")]
        public void Duration_Invalid(string jsonValue)
        {
            string json = WrapInQuotes(jsonValue);
            Assert.Throws<InvalidProtocolBufferException>(() => Duration.Parser.ParseJson(json));
        }

        // Not as many tests for field masks as I'd like; more to be added when we have more
        // detailed specifications.

        [Test]
        [TestCase("")]
        [TestCase("foo", "foo")]
        [TestCase("foo,bar", "foo", "bar")]
        [TestCase("foo.bar", "foo.bar")]
        [TestCase("fooBar", "foo_bar")]
        [TestCase("fooBar.bazQux", "foo_bar.baz_qux")]
        public void FieldMask_Valid(string jsonValue, params string[] expectedPaths)
        {
            string json = WrapInQuotes(jsonValue);
            var parsed = FieldMask.Parser.ParseJson(json);
            CollectionAssert.AreEqual(expectedPaths, parsed.Paths);
        }

        [Test]
        [TestCase("foo_bar")]
        public void FieldMask_Invalid(string jsonValue)
        {
            string json = WrapInQuotes(jsonValue);
            Assert.Throws<InvalidProtocolBufferException>(() => FieldMask.Parser.ParseJson(json));
        }

        [Test]
        public void Any_RegularMessage()
        {
            var registry = TypeRegistry.FromMessages(TestAllTypes.Descriptor);
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false, TypeRegistry.FromMessages(TestAllTypes.Descriptor)));
            var message = new TestAllTypes { SingleInt32 = 10, SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 20 } };
            var original = Any.Pack(message);
            var json = formatter.Format(original); // This is tested in JsonFormatterTest
            var parser = new JsonParser(new JsonParser.Settings(10, registry));
            Assert.AreEqual(original, parser.Parse<Any>(json));
            string valueFirstJson = "{ \"singleInt32\": 10, \"singleNestedMessage\": { \"bb\": 20 }, \"@type\": \"type.googleapis.com/protobuf_unittest3.TestAllTypes\" }";
            Assert.AreEqual(original, parser.Parse<Any>(valueFirstJson));
        }

        [Test]
        public void Any_CustomPrefix()
        {
            var registry = TypeRegistry.FromMessages(TestAllTypes.Descriptor);
            var message = new TestAllTypes { SingleInt32 = 10 };
            var original = Any.Pack(message, "custom.prefix/middle-part");
            var parser = new JsonParser(new JsonParser.Settings(10, registry));
            string json = "{ \"@type\": \"custom.prefix/middle-part/protobuf_unittest3.TestAllTypes\", \"singleInt32\": 10 }";
            Assert.AreEqual(original, parser.Parse<Any>(json));
        }

        [Test]
        public void Any_UnknownType()
        {
            string json = "{ \"@type\": \"type.googleapis.com/bogus\" }";
            Assert.Throws<InvalidOperationException>(() => Any.Parser.ParseJson(json));
        }

        [Test]
        public void Any_NoTypeUrl()
        {
            // Currently if we're missing the type URL (@type) we return an empty Any message
            // regardless of other fields. In the future we could potentially fail if there's no
            // type URL but other fields are present.
            string json = "{ \"foo\": \"bar\" }";
            Assert.AreEqual(new Any(), Any.Parser.ParseJson(json));
        }

        [Test]
        public void Any_BadTypeUrl()
        {
            string json = "{ \"@type\": \"no-slash\" }";
            Assert.Throws<InvalidProtocolBufferException>(() => Any.Parser.ParseJson(json));
        }

        [Test]
        public void Any_WellKnownType()
        {
            var registry = TypeRegistry.FromMessages(Timestamp.Descriptor);
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false, registry));
            var timestamp = new DateTime(1673, 6, 19, 12, 34, 56, DateTimeKind.Utc).ToTimestamp();
            var original = Any.Pack(timestamp);
            var json = formatter.Format(original); // This is tested in JsonFormatterTest
            var parser = new JsonParser(new JsonParser.Settings(10, registry));
            Assert.AreEqual(original, parser.Parse<Any>(json));
            string valueFirstJson = "{ \"value\": \"1673-06-19T12:34:56Z\", \"@type\": \"type.googleapis.com/google.protobuf.Timestamp\" }";
            Assert.AreEqual(original, parser.Parse<Any>(valueFirstJson));
        }

        [Test]
        public void Any_Nested()
        {
            var registry = TypeRegistry.FromMessages(TestWellKnownTypes.Descriptor, TestAllTypes.Descriptor);
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false, registry));
            var parser = new JsonParser(new JsonParser.Settings(10, registry));
            var doubleNestedMessage = new TestAllTypes { SingleInt32 = 20 };
            var nestedMessage = Any.Pack(doubleNestedMessage);
            var message = new TestWellKnownTypes { AnyField = Any.Pack(nestedMessage) };
            var json = formatter.Format(message);
            // Use the descriptor-based parser just for a change.
            Assert.AreEqual(message, parser.Parse(json, TestWellKnownTypes.Descriptor));
        }

        [Test]
        public void DataAfterObject()
        {
            string json = "{} 10";
            Assert.Throws<InvalidJsonException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        /// <summary>
        /// JSON equivalent to <see cref="CodedInputStreamTest.MaliciousRecursion"/>
        /// </summary>
        [Test]
        public void MaliciousRecursion()
        {
            string data64 = CodedInputStreamTest.MakeRecursiveMessage(64).ToString();
            string data65 = CodedInputStreamTest.MakeRecursiveMessage(65).ToString();

            var parser64 = new JsonParser(new JsonParser.Settings(64));
            CodedInputStreamTest.AssertMessageDepth(parser64.Parse<TestRecursiveMessage>(data64), 64);
            Assert.Throws<InvalidProtocolBufferException>(() => parser64.Parse<TestRecursiveMessage>(data65));

            var parser63 = new JsonParser(new JsonParser.Settings(63));
            Assert.Throws<InvalidProtocolBufferException>(() => parser63.Parse<TestRecursiveMessage>(data64));
        }

        [Test]
        [TestCase("AQI")]
        [TestCase("_-==")]
        public void Bytes_InvalidBase64(string badBase64)
        {
            string json = "{ \"singleBytes\": \"" + badBase64 + "\" }";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        [TestCase("\"FOREIGN_BAR\"", TestProtos.ForeignEnum.ForeignBar)]
        [TestCase("5", TestProtos.ForeignEnum.ForeignBar)]
        [TestCase("100", (TestProtos.ForeignEnum)100)]
        public void EnumValid(string value, TestProtos.ForeignEnum expectedValue)
        {
            string json = "{ \"singleForeignEnum\": " + value + " }";
            var parsed = TestAllTypes.Parser.ParseJson(json);
            Assert.AreEqual(new TestAllTypes { SingleForeignEnum = expectedValue }, parsed);
        }

        [Test]
        [TestCase("\"NOT_A_VALID_VALUE\"")]
        [TestCase("5.5")]
        public void Enum_Invalid(string value)
        {
            string json = "{ \"singleForeignEnum\": " + value + " }";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        public void Enum_InvalidString_IgnoreUnknownFields()
        {
            // When ignoring unknown fields, invalid enum value strings are ignored too.
            // This test uses TestProto3Optional so we can check we're not just setting the field to the 0 value.
            var parser = new JsonParser(JsonParser.Settings.Default.WithIgnoreUnknownFields(true));
            string json = "{ \"optionalNestedEnum\": \"NOT_A_VALID_VALUE\" }";
            var parsed = parser.Parse<TestProto3Optional>(json);
            Assert.IsFalse(parsed.HasOptionalNestedEnum);
        }

        [Test]
        public void RepeatedEnum_InvalidString_IgnoreUnknownFields()
        {
            // When ignoring unknown fields, invalid enum value strings are ignored too.
            // For a repeated field, the value is removed entirely.
            var parser = new JsonParser(JsonParser.Settings.Default.WithIgnoreUnknownFields(true));
            string json = "{ \"repeatedForeignEnum\": [ \"FOREIGN_FOO\", \"NOT_A_VALID_VALUE\", \"FOREIGN_BAR\" ] }";
            var parsed = parser.Parse<TestAllTypes>(json);
            var expected = new[] { TestProtos.ForeignEnum.ForeignFoo, TestProtos.ForeignEnum.ForeignBar };
            Assert.AreEqual(expected, parsed.RepeatedForeignEnum);
        }

        [Test]
        public void EnumValuedMap_InvalidString_IgnoreUnknownFields()
        {
            // When ignoring unknown fields, invalid enum value strings are ignored too.
            // For a map field, the entry is removed entirely.
            var parser = new JsonParser(JsonParser.Settings.Default.WithIgnoreUnknownFields(true));
            string json = "{ \"mapInt32Enum\": { \"1\": \"MAP_ENUM_BAR\", \"2\": \"NOT_A_VALID_VALUE\" } }";
            var parsed = parser.Parse<TestMap>(json);
            Assert.AreEqual(1, parsed.MapInt32Enum.Count);
            Assert.AreEqual(MapEnum.Bar, parsed.MapInt32Enum[1]);
            Assert.False(parsed.MapInt32Enum.ContainsKey(2));
        }

        [Test]
        public void Enum_InvalidNumber_IgnoreUnknownFields()
        {
            // Even when ignoring unknown fields, fail for non-integer numeric values, because
            // they could *never* be valid.
            var parser = new JsonParser(JsonParser.Settings.Default.WithIgnoreUnknownFields(true));
            string json = "{ \"singleForeignEnum\": 5.5 }";
            Assert.Throws<InvalidProtocolBufferException>(() => parser.Parse<TestAllTypes>(json));
        }

        [Test]
        public void OneofDuplicate_Invalid()
        {
            string json = "{ \"oneofString\": \"x\", \"oneofUint32\": 10 }";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        public void UnknownField_NotIgnored()
        {
            string json = "{ \"unknownField\": 10, \"singleString\": \"x\" }";
            Assert.Throws<InvalidProtocolBufferException>(() => TestAllTypes.Parser.ParseJson(json));
        }

        [Test]
        public void Proto2_DefaultValuesPreserved()
        {
            string json = "{ \"FieldName13\": 0 }";
            var parsed = TestAllTypesProto2.Parser.ParseJson(json);
            Assert.False(parsed.HasFieldName10);
            Assert.True(parsed.HasFieldName13);
            Assert.AreEqual(0, parsed.FieldName13);
        }

        [Test]
        public void Proto2_Group()
        {
            string json = "{ \"data\": { \"groupInt32\": 2, \"groupUint32\": 3 } }";
            var parsed = TestAllTypesProto2.Parser.ParseJson(json);
            Assert.True(parsed.HasData);
            Assert.AreEqual(2, parsed.Data.GroupInt32);
            Assert.AreEqual(3, parsed.Data.GroupUint32);
        }

        [Test]
        [TestCase("5")]
        [TestCase("\"text\"")]
        [TestCase("[0, 1, 2]")]
        [TestCase("{ \"a\": { \"b\": 10 } }")]
        public void UnknownField_Ignored(string value)
        {
            var parser = new JsonParser(JsonParser.Settings.Default.WithIgnoreUnknownFields(true));
            string json = "{ \"unknownField\": " + value + ", \"singleString\": \"x\" }";
            var actual = parser.Parse<TestAllTypes>(json);
            var expected = new TestAllTypes { SingleString = "x" };
            Assert.AreEqual(expected, actual);
        }

        [Test]
        public void NullValueOutsideStruct_NullLiteral()
        {
            string json = "{ \"nullValue\": null }";
            var message = NullValueOutsideStruct.Parser.ParseJson(json);
            Assert.AreEqual(NullValueOutsideStruct.ValueOneofCase.NullValue, message.ValueCase);
        }

        [Test]
        public void NullValueNotInOneof_NullLiteral()
        {
            // We'd only normally see this with FormatDefaultValues set to true.
            string json = "{ \"nullValue\": null }";
            var message = NullValueNotInOneof.Parser.ParseJson(json);
            Assert.AreEqual(NullValue.NullValue, message.NullValue);
        }

        // NullValue used to only be converted to the null literal when part of a struct.
        // Otherwise, it would end up as a string "NULL_VALUE" (the name of the enum value).
        // We still parse that form, for compatibility.
        [Test]
        public void NullValueOutsideStruct_Compatibility()
        {
            string json = "{ \"nullValue\": \"NULL_VALUE\" }";
            var message = NullValueOutsideStruct.Parser.ParseJson(json);
            Assert.AreEqual(NullValueOutsideStruct.ValueOneofCase.NullValue, message.ValueCase);
        }

        [Test]
        public void NullValueNotInOneof_Compatibility()
        {
            // We'd only normally see this with FormatDefaultValues set to true.
            string json = "{ \"nullValue\": \"NULL_VALUE\" }";
            var message = NullValueNotInOneof.Parser.ParseJson(json);
            Assert.AreEqual(NullValue.NullValue, message.NullValue);
        }

        /// <summary>
        /// Various tests use strings which have quotes round them for parsing or as the result
        /// of formatting, but without those quotes being specified in the tests (for the sake of readability).
        /// This method simply returns the input, wrapped in double quotes.
        /// </summary>
        internal static string WrapInQuotes(string text)
        {
            return '"' + text + '"';
        }

        [Test]
        public void ParseAllNullValues()
        {
            string json = @"{
  ""optionalInt32"": null,
  ""optionalInt64"": null,
  ""optionalUint32"": null,
  ""optionalUint64"": null,
  ""optionalSint32"": null,
  ""optionalSint64"": null,
  ""optionalFixed32"": null,
  ""optionalFixed64"": null,
  ""optionalSfixed32"": null,
  ""optionalSfixed64"": null,
  ""optionalFloat"": null,
  ""optionalDouble"": null,
  ""optionalBool"": null,
  ""optionalString"": null,
  ""optionalBytes"": null,
  ""optionalNestedEnum"": null,
  ""optionalNestedMessage"": null,
  ""repeatedInt32"": null,
  ""repeatedInt64"": null,
  ""repeatedUint32"": null,
  ""repeatedUint64"": null,
  ""repeatedSint32"": null,
  ""repeatedSint64"": null,
  ""repeatedFixed32"": null,
  ""repeatedFixed64"": null,
  ""repeatedSfixed32"": null,
  ""repeatedSfixed64"": null,
  ""repeatedFloat"": null,
  ""repeatedDouble"": null,
  ""repeatedBool"": null,
  ""repeatedString"": null,
  ""repeatedBytes"": null,
  ""repeatedNestedEnum"": null,
  ""repeatedNestedMessage"": null,
  ""mapInt32Int32"": null,
  ""mapBoolBool"": null,
  ""mapStringNestedMessage"": null
}";

            var message = new TestAllTypesProto3
            {
                OptionalInt32 = 1,
                OptionalInt64 = 1,
                OptionalUint32 = 1,
                OptionalUint64 = 1,
                OptionalSint32 = 1,
                OptionalSint64 = 1,
                OptionalFixed32 = 1,
                OptionalFixed64 = 1,
                OptionalSfixed32 = 1,
                OptionalSfixed64 = 1,
                OptionalFloat = 1,
                OptionalDouble = 1,
                OptionalBool = true,
                OptionalString = "1",
                OptionalBytes = ByteString.CopyFrom(new byte[] { 1 }),
                OptionalNestedEnum = TestAllTypesProto3.Types.NestedEnum.Bar,
                OptionalNestedMessage = new TestAllTypesProto3.Types.NestedMessage()
            };
            message.RepeatedInt32.Add(1);
            message.RepeatedInt64.Add(1);
            message.RepeatedUint32.Add(1);
            message.RepeatedUint64.Add(1);
            message.RepeatedSint32.Add(1);
            message.RepeatedSint64.Add(1);
            message.RepeatedFixed32.Add(1);
            message.RepeatedFixed64.Add(1);
            message.RepeatedSfixed32.Add(1);
            message.RepeatedSfixed64.Add(1);
            message.RepeatedFloat.Add(1);
            message.RepeatedDouble.Add(1);
            message.RepeatedBool.Add(true);
            message.RepeatedString.Add("1");
            message.RepeatedBytes.Add(ByteString.CopyFrom(new byte[] { 1 }));
            message.RepeatedNestedEnum.Add(TestAllTypesProto3.Types.NestedEnum.Bar);
            message.RepeatedNestedMessage.Add(new TestAllTypesProto3.Types.NestedMessage());
            message.MapInt32Int32.Add(1, 1);
            message.MapBoolBool.Add(true, true);
            message.MapStringNestedMessage.Add(" ", new TestAllTypesProto3.Types.NestedMessage());

            JsonParser.Default.Merge(message, json);

            Assert.AreEqual(0, message.OptionalInt32);
            Assert.AreEqual(0, message.OptionalInt64);
            Assert.AreEqual(0, message.OptionalUint32);
            Assert.AreEqual(0, message.OptionalUint64);
            Assert.AreEqual(0, message.OptionalSint32);
            Assert.AreEqual(0, message.OptionalSint64);
            Assert.AreEqual(0, message.OptionalFixed32);
            Assert.AreEqual(0, message.OptionalFixed64);
            Assert.AreEqual(0, message.OptionalSfixed32);
            Assert.AreEqual(0, message.OptionalSfixed64);
            Assert.AreEqual(0, message.OptionalFloat);
            Assert.AreEqual(0, message.OptionalDouble);
            Assert.AreEqual(false, message.OptionalBool);
            Assert.AreEqual("", message.OptionalString);
            Assert.AreEqual(ByteString.Empty, message.OptionalBytes);
            Assert.AreEqual(TestAllTypesProto3.Types.NestedEnum.Foo, message.OptionalNestedEnum);
            Assert.AreEqual(null, message.OptionalNestedMessage);
            Assert.AreEqual(0, message.RepeatedInt32.Count);
            Assert.AreEqual(0, message.RepeatedInt64.Count);
            Assert.AreEqual(0, message.RepeatedUint32.Count);
            Assert.AreEqual(0, message.RepeatedUint64.Count);
            Assert.AreEqual(0, message.RepeatedSint32.Count);
            Assert.AreEqual(0, message.RepeatedSint64.Count);
            Assert.AreEqual(0, message.RepeatedFixed32.Count);
            Assert.AreEqual(0, message.RepeatedFixed64.Count);
            Assert.AreEqual(0, message.RepeatedSfixed32.Count);
            Assert.AreEqual(0, message.RepeatedFloat.Count);
            Assert.AreEqual(0, message.RepeatedDouble.Count);
            Assert.AreEqual(0, message.RepeatedBool.Count);
            Assert.AreEqual(0, message.RepeatedString.Count);
            Assert.AreEqual(0, message.RepeatedBytes.Count);
            Assert.AreEqual(0, message.RepeatedNestedEnum.Count);
            Assert.AreEqual(0, message.RepeatedNestedMessage.Count);
            Assert.AreEqual(0, message.MapInt32Int32.Count);
            Assert.AreEqual(0, message.MapBoolBool.Count);
            Assert.AreEqual(0, message.MapStringNestedMessage.Count);
        }
    }
}