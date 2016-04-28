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

namespace Google.Protobuf
{
    /// <summary>
    /// Tests for the JSON formatter. Note that in these tests, double quotes are replaced with apostrophes
    /// for the sake of readability (embedding \" everywhere is painful). See the AssertJson method for details.
    /// </summary>
    public class JsonFormatterTest
    {
        #region Field descriptors

        // Definitions for the field descriptors to make the tests better readable.
        private static readonly FieldDescriptor TestAllTypesSingleInt32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleInt64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleUint32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleUint32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleUint64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleUint64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleSint32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleSint32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleSint64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleSint64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleFixed32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleFixed32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleFixed64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleFixed64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleSfixed32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleSfixed64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleFloatFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleDoubleFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleBoolFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleStringFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleBytesFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleBytesFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleNestedMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleNestedMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleForeignMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleForeignMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleImportMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleImportMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleNestedEnumFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleNestedEnumFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleForeignEnumFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleForeignEnumFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSingleImportEnumFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SingleImportEnumFieldNumber);
        private static readonly FieldDescriptor TestAllTypesSinglePublicImportMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.SinglePublicImportMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedInt32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedInt64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedUint32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedUint32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedUint64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedUint64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedSint32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedSint32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedSint64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedSint64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedFixed32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedFixed32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedFixed64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedFixed64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedSfixed32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedSfixed32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedSfixed64FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedSfixed64FieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedFloatFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedFloatFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedDoubleFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedDoubleFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedBoolFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedBoolFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedStringFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedStringFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedBytesFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedBytesFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedNestedMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedNestedMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedForeignMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedForeignMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedImportMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedImportMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedNestedEnumFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedNestedEnumFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedForeignEnumFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedForeignEnumFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedImportEnumFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedImportEnumFieldNumber);
        private static readonly FieldDescriptor TestAllTypesRepeatedPublicImportMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.RepeatedPublicImportMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesOneofUint32FieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.OneofUint32FieldNumber);
        private static readonly FieldDescriptor TestAllTypesOneofNestedMessageFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.OneofNestedMessageFieldNumber);
        private static readonly FieldDescriptor TestAllTypesOneofStringFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.OneofStringFieldNumber);
        private static readonly FieldDescriptor TestAllTypesOneofBytesFieldDescriptor = TestAllTypes.Descriptor.FindFieldByNumber(TestAllTypes.OneofBytesFieldNumber);
        private static readonly FieldDescriptor ForeignMessageCFieldDescriptor = ForeignMessage.Descriptor.FindFieldByNumber(ForeignMessage.CFieldNumber);
        private static readonly FieldDescriptor TestMapMapInt32Int32FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);
        private static readonly FieldDescriptor TestMapMapInt64Int64FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapInt64Int64FieldNumber);
        private static readonly FieldDescriptor TestMapMapUint32Uint32FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapUint32Uint32FieldNumber);
        private static readonly FieldDescriptor TestMapMapUint64Uint64FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapUint64Uint64FieldNumber);
        private static readonly FieldDescriptor TestMapMapSint32Sint32FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapSint32Sint32FieldNumber);
        private static readonly FieldDescriptor TestMapMapSint64Sint64FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapSint64Sint64FieldNumber);
        private static readonly FieldDescriptor TestMapMapFixed32Fixed32FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapFixed32Fixed32FieldNumber);
        private static readonly FieldDescriptor TestMapMapFixed64Fixed64FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapFixed64Fixed64FieldNumber);
        private static readonly FieldDescriptor TestMapMapSfixed32Sfixed32FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapSfixed32Sfixed32FieldNumber);
        private static readonly FieldDescriptor TestMapMapSfixed64Sfixed64FieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapSfixed64Sfixed64FieldNumber);
        private static readonly FieldDescriptor TestMapMapInt32FloatFieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapInt32FloatFieldNumber);
        private static readonly FieldDescriptor TestMapMapInt32DoubleFieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapInt32DoubleFieldNumber);
        private static readonly FieldDescriptor TestMapMapBoolBoolFieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapBoolBoolFieldNumber);
        private static readonly FieldDescriptor TestMapMapStringStringFieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapStringStringFieldNumber);
        private static readonly FieldDescriptor TestMapMapInt32BytesFieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapInt32BytesFieldNumber);
        private static readonly FieldDescriptor TestMapMapInt32EnumFieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapInt32EnumFieldNumber);
        private static readonly FieldDescriptor TestMapMapInt32ForeignMessageFieldDescriptor = TestMap.Descriptor.FindFieldByNumber(TestMap.MapInt32ForeignMessageFieldNumber);
        private static readonly FieldDescriptor TestOneofFooIntFieldDescriptor = TestOneof.Descriptor.FindFieldByNumber(TestOneof.FooIntFieldNumber);
        private static readonly FieldDescriptor TestOneofFooStringFieldDescriptor = TestOneof.Descriptor.FindFieldByNumber(TestOneof.FooStringFieldNumber);
        private static readonly FieldDescriptor TestOneofFooMessageFieldDescriptor = TestOneof.Descriptor.FindFieldByNumber(TestOneof.FooMessageFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesAnyFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.AnyFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesApiFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.ApiFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesDurationFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.DurationFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesEmptyFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.EmptyFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesFieldMaskFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.FieldMaskFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesSourceContextFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.SourceContextFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesStructFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.StructFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesTimestampFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.TimestampFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesTypeFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.TypeFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesDoubleFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.DoubleFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesFloatFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.FloatFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesInt64FieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.Int64FieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesUint64FieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.Uint64FieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesInt32FieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.Int32FieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesUint32FieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.Uint32FieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesBoolFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.BoolFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesStringFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.StringFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesBytesFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.BytesFieldFieldNumber);
        private static readonly FieldDescriptor TestWellKnownTypesValueFieldFieldDescriptor = TestWellKnownTypes.Descriptor.FindFieldByNumber(TestWellKnownTypes.ValueFieldFieldNumber);

        #endregion

        [Test]
        public void DefaultValues_WhenOmitted()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(formatDefaultValues: false));
            var foreignMessage = new ForeignMessage();
            var testAllTypesMessage = new TestAllTypes();
            var testMapMessage = new TestMap();

            AssertJson("{ }", formatter.Format(foreignMessage));
            AssertJson("{ }", formatter.Format(testAllTypesMessage));
            AssertJson("{ }", formatter.Format(testMapMessage));

            AssertJson("0", formatter.FormatFieldValue(foreignMessage, ForeignMessageCFieldDescriptor));
            AssertJson("false", formatter.FormatFieldValue(testAllTypesMessage, TestAllTypesSingleBoolFieldDescriptor));
            AssertJson("{ }", formatter.FormatFieldValue(testMapMessage, TestMapMapBoolBoolFieldDescriptor));
        }

        [Test]
        public void DefaultValues_WhenIncluded()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(formatDefaultValues: true));
            var foreignMessage = new ForeignMessage();

            AssertJson("{ 'c': 0 }", formatter.Format(foreignMessage));
            AssertJson("0", formatter.FormatFieldValue(foreignMessage, ForeignMessageCFieldDescriptor));
        }

        [Test]
        public void InvalidFieldDescriptor_FormatFieldValue()
        {
            Assert.Throws<ArgumentException>(() =>
                JsonFormatter.Default.FormatFieldValue(new ForeignMessage(), TestAllTypesSingleBoolFieldDescriptor)
            );
        }

        [Test]
        public void InvalidFieldDescriptor_FormatRepeatedFieldValue()
        {
            Assert.Throws<ArgumentException>(() =>
                JsonFormatter.Default.FormatRepeatedFieldValue(new ForeignMessage(), TestAllTypesSingleBoolFieldDescriptor, 0)
            );
        }

        [Test]
        public void InvalidFieldDescriptor_FormatMapFieldValue()
        {
            Assert.Throws<ArgumentException>(() =>
                JsonFormatter.Default.FormatMapFieldValue(new ForeignMessage(), TestAllTypesSingleBoolFieldDescriptor, "bar")
            );
        }

        [Test]
        public void FieldNotRepeated()
        {
            Assert.Throws<ArgumentException>(() =>
                JsonFormatter.Default.FormatRepeatedFieldValue(new TestMap(), TestMapMapBoolBoolFieldDescriptor, 0)
            );
        }

        [Test]
        public void FieldNotMap()
        {
            Assert.Throws<ArgumentException>(() =>
                JsonFormatter.Default.FormatMapFieldValue(new TestAllTypes(), TestAllTypesSingleBoolFieldDescriptor, "bar")
            );
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

            // Fields in numeric order
            var expectedSingleInt32Value = "100";
            var expectedSingleInt64Value = "'3210987654321'";
            var expectedSingleUint32Value = "4294967295";
            var expectedSingleUint64Value = "'18446744073709551615'";
            var expectedSingleSint32Value = "-456";
            var expectedSingleSint64Value = "'-12345678901235'";
            var expectedSingleFixed32Value = "23";
            var expectedSingleFixed64Value = "'1234567890123'";
            var expectedSingleSfixed32Value = "-123";
            var expectedSingleSfixed64Value = "'-12345678901234'";
            var expectedSingleFloatValue = "12.25";
            var expectedSingleDoubleValue = "23.5";
            var expectedSingleBoolValue = "true";
            var expectedSingleStringValue = "'test\\twith\\ttabs'";
            var expectedSingleBytesValue = "'AQIDBA=='";
            var expectedSingleNestedMessageValue = "{ 'bb': 35 }";
            var expectedSingleForeignMessageValue = "{ 'c': 10 }";
            var expectedSingleImportMessageValue = "{ 'd': 20 }";
            var expectedSingleNestedEnumValue = "'FOO'";
            var expectedSingleForeignEnumValue = "'FOREIGN_BAR'";
            var expectedSingleImportEnumValue = "'IMPORT_BAZ'";
            var expectedSinglePublicImportMessageValue = "{ 'e': 54 }";
            var expectedJson = "{ " +
                "'singleInt32': " + expectedSingleInt32Value + ", " +
                "'singleInt64': " + expectedSingleInt64Value + ", " +
                "'singleUint32': " + expectedSingleUint32Value + ", " +
                "'singleUint64': " + expectedSingleUint64Value + ", " +
                "'singleSint32': " + expectedSingleSint32Value + ", " +
                "'singleSint64': " + expectedSingleSint64Value + ", " +
                "'singleFixed32': " + expectedSingleFixed32Value + ", " +
                "'singleFixed64': " + expectedSingleFixed64Value + ", " +
                "'singleSfixed32': " + expectedSingleSfixed32Value + ", " +
                "'singleSfixed64': " + expectedSingleSfixed64Value + ", " +
                "'singleFloat': " + expectedSingleFloatValue + ", " +
                "'singleDouble': " + expectedSingleDoubleValue + ", " +
                "'singleBool': " + expectedSingleBoolValue + ", " +
                "'singleString': " + expectedSingleStringValue + ", " +
                "'singleBytes': " + expectedSingleBytesValue + ", " +
                "'singleNestedMessage': " + expectedSingleNestedMessageValue + ", " +
                "'singleForeignMessage': " + expectedSingleForeignMessageValue + ", " +
                "'singleImportMessage': " + expectedSingleImportMessageValue + ", " +
                "'singleNestedEnum': " + expectedSingleNestedEnumValue + ", " +
                "'singleForeignEnum': " + expectedSingleForeignEnumValue + ", " +
                "'singleImportEnum': " + expectedSingleImportEnumValue + ", " +
                "'singlePublicImportMessage': " + expectedSinglePublicImportMessageValue +
                " }";
            AssertJson(expectedJson, 
                JsonFormatter.Default.Format(message));

            AssertJson(expectedSingleInt32Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleInt32FieldDescriptor));
            AssertJson(expectedSingleInt64Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleInt64FieldDescriptor));
            AssertJson(expectedSingleUint32Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleUint32FieldDescriptor));
            AssertJson(expectedSingleUint64Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleUint64FieldDescriptor));
            AssertJson(expectedSingleSint32Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleSint32FieldDescriptor));
            AssertJson(expectedSingleSint64Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleSint64FieldDescriptor));
            AssertJson(expectedSingleFixed32Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleFixed32FieldDescriptor));
            AssertJson(expectedSingleFixed64Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleFixed64FieldDescriptor));
            AssertJson(expectedSingleSfixed32Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleSfixed32FieldDescriptor));
            AssertJson(expectedSingleSfixed64Value, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleSfixed64FieldDescriptor));
            AssertJson(expectedSingleFloatValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleFloatFieldDescriptor));
            AssertJson(expectedSingleDoubleValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleDoubleFieldDescriptor));
            AssertJson(expectedSingleBoolValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleBoolFieldDescriptor));
            AssertJson(expectedSingleStringValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleStringFieldDescriptor));
            AssertJson(expectedSingleBytesValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleBytesFieldDescriptor));
            AssertJson(expectedSingleNestedMessageValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleNestedMessageFieldDescriptor));
            AssertJson(expectedSingleForeignMessageValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleForeignMessageFieldDescriptor));
            AssertJson(expectedSingleImportMessageValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleImportMessageFieldDescriptor));
            AssertJson(expectedSingleNestedEnumValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleNestedEnumFieldDescriptor));
            AssertJson(expectedSingleForeignEnumValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleForeignEnumFieldDescriptor));
            AssertJson(expectedSingleImportEnumValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleImportEnumFieldDescriptor));
            AssertJson(expectedSinglePublicImportMessageValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSinglePublicImportMessageFieldDescriptor));
        }

        [Test]
        public void RepeatedField()
        {
            var message = new TestAllTypes { RepeatedInt32 = { 1, 2, 3, 4, 5 } };
            var expectedValueText = "[ 1, 2, 3, 4, 5 ]";
            var expectedJson = "{ 'repeatedInt32': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestAllTypesRepeatedInt32FieldDescriptor));
            AssertJson("1", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedInt32FieldDescriptor, 0));
            AssertJson("2", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedInt32FieldDescriptor, 1));
            AssertJson("3", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedInt32FieldDescriptor, 2));
            AssertJson("4", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedInt32FieldDescriptor, 3));
            AssertJson("5", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedInt32FieldDescriptor, 4));
        }

        [Test]
        public void MapField_StringString()
        {
            var message = new TestMap { MapStringString = { { "with spaces", "bar" }, { "a", "b" } } };
            var expectedValueText = "{ 'with spaces': 'bar', 'a': 'b' }";
            var expectedJson = "{ 'mapStringString': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestMapMapStringStringFieldDescriptor));
            AssertJson("'bar'", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapStringStringFieldDescriptor, "with spaces"));
            AssertJson("'b'", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapStringStringFieldDescriptor, "a"));
        }

        [Test]
        public void MapField_Int32Int32()
        {
            // The keys are quoted, but the values aren't.
            var expectedValueText = "{ '0': 1, '2': 3 }";
            var expectedJson = "{ 'mapInt32Int32': " + expectedValueText + " }";
            var message = new TestMap { MapInt32Int32 = { { 0, 1 }, { 2, 3 } } };
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestMapMapInt32Int32FieldDescriptor));
            AssertJson("1", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapInt32Int32FieldDescriptor, 0));
            AssertJson("3", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapInt32Int32FieldDescriptor, 2));
        }

        [Test]
        public void MapField_BoolBool()
        {
            // The keys are quoted, but the values aren't.
            var expectedValueText = "{ 'false': true, 'true': false }";
            var expectedJson = "{ 'mapBoolBool': " + expectedValueText + " }";
            var message = new TestMap { MapBoolBool = { { false, true }, { true, false } } };
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestMapMapBoolBoolFieldDescriptor));
            AssertJson("true", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapBoolBoolFieldDescriptor, false));
            AssertJson("false", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapBoolBoolFieldDescriptor, true));
        }

        [TestCase(1.0, "1")]
        [TestCase(double.NaN, "'NaN'")]
        [TestCase(double.PositiveInfinity, "'Infinity'")]
        [TestCase(double.NegativeInfinity, "'-Infinity'")]
        public void DoubleRepresentations(double value, string expectedValueText)
        {
            var message = new TestAllTypes { SingleDouble = value };
            var expectedJson = "{ 'singleDouble': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleDoubleFieldDescriptor));
        }

        [Test]
        public void UnknownEnumValueNumeric_SingleField()
        {
            var message = new TestAllTypes { SingleForeignEnum = (ForeignEnum)100 };
            var expectedValueText = "100";
            var expectedJson = "{ 'singleForeignEnum': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleForeignEnumFieldDescriptor));
        }

        [Test]
        public void UnknownEnumValueNumeric_RepeatedField()
        {
            var message = new TestAllTypes { RepeatedForeignEnum = { ForeignEnum.ForeignBaz, (ForeignEnum)100, ForeignEnum.ForeignFoo } };
            var expectedValueText = "[ 'FOREIGN_BAZ', 100, 'FOREIGN_FOO' ]";
            var expectedJson = "{ 'repeatedForeignEnum': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestAllTypesRepeatedForeignEnumFieldDescriptor));
            AssertJson("'FOREIGN_BAZ'", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedForeignEnumFieldDescriptor, 0));
            AssertJson("100", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedForeignEnumFieldDescriptor, 1));
            AssertJson("'FOREIGN_FOO'", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedForeignEnumFieldDescriptor, 2));
        }

        [Test]
        public void UnknownEnumValueNumeric_MapField()
        {
            var message = new TestMap { MapInt32Enum = { { 1, MapEnum.Foo }, { 2, (MapEnum)100 }, { 3, MapEnum.Bar } } };
            var expectedValueText = "{ '1': 'MAP_ENUM_FOO', '2': 100, '3': 'MAP_ENUM_BAR' }";
            var expectedJson = "{ 'mapInt32Enum': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestMapMapInt32EnumFieldDescriptor));
            AssertJson("'MAP_ENUM_FOO'", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapInt32EnumFieldDescriptor, 1));
            AssertJson("100", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapInt32EnumFieldDescriptor, 2));
            AssertJson("'MAP_ENUM_BAR'", JsonFormatter.Default.FormatMapFieldValue(message, TestMapMapInt32EnumFieldDescriptor, 3));
        }

        [Test]
        public void UnknownEnumValue_RepeatedField_AllEntriesUnknown()
        {
            var message = new TestAllTypes { RepeatedForeignEnum = { (ForeignEnum)200, (ForeignEnum)100 } };
            var expectedValueText = "[ 200, 100 ]";
            var expectedJson = "{ 'repeatedForeignEnum': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestAllTypesRepeatedForeignEnumFieldDescriptor));
            AssertJson("200", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedForeignEnumFieldDescriptor, 0));
            AssertJson("100", JsonFormatter.Default.FormatRepeatedFieldValue(message, TestAllTypesRepeatedForeignEnumFieldDescriptor, 1));
        }

        [Test]
        [TestCase("a\u17b4b", "a\\u17b4b")] // Explicit
        [TestCase("a\u0601b", "a\\u0601b")] // Ranged
        [TestCase("a\u0605b", "a\u0605b")] // Passthrough (note lack of double backslash...)
        public void SimpleNonAscii(string text, string encoded)
        {
            var message = new TestAllTypes { SingleString = text };
            var expectedValueText = "'" + encoded + "'";
            var expectedJson = "{ 'singleString': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleStringFieldDescriptor));
        }

        [Test]
        public void SurrogatePairEscaping()
        {
            var message = new TestAllTypes { SingleString = "a\uD801\uDC01b" };
            var expectedValueText = "'a\\ud801\\udc01b'";
            var expectedJson = "{ 'singleString': " + expectedValueText + " }";
            AssertJson(expectedJson, JsonFormatter.Default.Format(message));
            AssertJson(expectedValueText, JsonFormatter.Default.FormatFieldValue(message, TestAllTypesSingleStringFieldDescriptor));
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
        [TestCase("BANANABanana", "bananaBanana")]
        public void ToCamelCase(string original, string expected)
        {
            Assert.AreEqual(expected, JsonFormatter.ToCamelCase(original));
        }

        [Test]
        [TestCase(null, "{ }")]
        [TestCase("x", "{ 'fooString': 'x' }")]
        [TestCase("", "{ 'fooString': '' }")]
        public void OneofMessage(string fooStringValue, string expectedJson)
        {
            var message = new TestOneof();
            if (fooStringValue != null)
            {
                message.FooString = fooStringValue;
            }

            // We should get the same result both with and without "format default values".
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false));
            AssertJson(expectedJson, formatter.Format(message));

            formatter = new JsonFormatter(new JsonFormatter.Settings(true));
            AssertJson(expectedJson, formatter.Format(message));
        }

        [Test]
        [TestCase(null, "")]
        [TestCase("x", "'x'")]
        [TestCase("", "''")]
        public void OneofFieldValue(string fooStringValue, string expectedValueText)
        {
            var message = new TestOneof();
            if (fooStringValue != null)
            {
                message.FooString = fooStringValue;
            }

            // We should get the same result both with and without "format default values".
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false));
            AssertJson(expectedValueText, formatter.FormatFieldValue(message, TestOneofFooStringFieldDescriptor));

            formatter = new JsonFormatter(new JsonFormatter.Settings(true));
            AssertJson(expectedValueText, formatter.FormatFieldValue(message, TestOneofFooStringFieldDescriptor));
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
            var expectedInt64FieldValue = "'10'";
            var expectedInt32FieldValue = "0";
            var expectedStringFieldValue = "''";
            var expectedBytesFieldValue = "'ABCD'";
            var expectedJson = "{ " +
                "'int64Field': " + expectedInt64FieldValue + ", " +
                "'int32Field': " + expectedInt32FieldValue + ", " +
                "'stringField': " + expectedStringFieldValue + ", " +
                "'bytesField': " + expectedBytesFieldValue +
                " }";
            AssertJson(expectedJson,
                JsonFormatter.Default.Format(message));

            AssertJson(expectedInt64FieldValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestWellKnownTypesInt64FieldFieldDescriptor));
            AssertJson(expectedInt32FieldValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestWellKnownTypesInt32FieldFieldDescriptor));
            AssertJson(expectedStringFieldValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestWellKnownTypesStringFieldFieldDescriptor));
            AssertJson(expectedBytesFieldValue, 
                JsonFormatter.Default.FormatFieldValue(message, TestWellKnownTypesBytesFieldFieldDescriptor));
        }

        [Test]
        public void WrapperFormatting_Message()
        {
            Assert.AreEqual("\"\"", JsonFormatter.Default.Format(new StringValue()));
            Assert.AreEqual("0", JsonFormatter.Default.Format(new Int32Value()));
        }

        [Test]
        public void WrapperFormatting_IncludeNull()
        {
            // The actual JSON here is very large because there are lots of fields. Just test a couple of them.
            var message = new TestWellKnownTypes { Int32Field = 10 };
            var formatter = new JsonFormatter(new JsonFormatter.Settings(true));
            var actualJson = formatter.Format(message);
            Assert.IsTrue(actualJson.Contains("\"int64Field\": null"));
            Assert.IsFalse(actualJson.Contains("\"int32Field\": null"));
        }

        [Test]
        public void OutputIsInNumericFieldOrder_NoDefaults()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false));
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
            var formatter = new JsonFormatter(new JsonFormatter.Settings(true));
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
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false, TypeRegistry.FromMessages(Timestamp.Descriptor)));
            var timestamp = new DateTime(1673, 6, 19, 12, 34, 56, DateTimeKind.Utc).ToTimestamp();
            var any = Any.Pack(timestamp);
            AssertJson("{ '@type': 'type.googleapis.com/google.protobuf.Timestamp', 'value': '1673-06-19T12:34:56Z' }", formatter.Format(any));
        }

        [Test]
        public void AnyMessageType()
        {
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false, TypeRegistry.FromMessages(TestAllTypes.Descriptor)));
            var message = new TestAllTypes { SingleInt32 = 10, SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 20 } };
            var any = Any.Pack(message);
            AssertJson("{ '@type': 'type.googleapis.com/protobuf_unittest.TestAllTypes', 'singleInt32': 10, 'singleNestedMessage': { 'bb': 20 } }", formatter.Format(any));
        }

        [Test]
        public void AnyNested()
        {
            var registry = TypeRegistry.FromMessages(TestWellKnownTypes.Descriptor, TestAllTypes.Descriptor);
            var formatter = new JsonFormatter(new JsonFormatter.Settings(false, registry));

            // Nest an Any as the value of an Any.
            var doubleNestedMessage = new TestAllTypes { SingleInt32 = 20 };
            var nestedMessage = Any.Pack(doubleNestedMessage);
            var message = new TestWellKnownTypes { AnyField = Any.Pack(nestedMessage) };
            AssertJson("{ 'anyField': { '@type': 'type.googleapis.com/google.protobuf.Any', 'value': { '@type': 'type.googleapis.com/protobuf_unittest.TestAllTypes', 'singleInt32': 20 } } }",
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
