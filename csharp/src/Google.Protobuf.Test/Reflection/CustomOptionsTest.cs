#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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

using Google.Protobuf.Reflection;
using Google.Protobuf.WellKnownTypes;
using NUnit.Framework;
using System.IO;
using System.Linq;
using UnitTest.Issues.TestProtos;
using static Google.Protobuf.WireFormat;
using static UnitTest.Issues.TestProtos.ComplexOptionType2.Types;
using static UnitTest.Issues.TestProtos.DummyMessageContainingEnum.Types;
using static Google.Protobuf.Test.Reflection.CustomOptionNumber;

namespace Google.Protobuf.Test.Reflection
{
    // Internal enum to allow us to use "using static" for convenience.
    // These are the options defined in unittest_custom_options_proto3.proto
    internal enum CustomOptionNumber
    {
        FileOpt1 = 7736974,
        MessageOpt1 = 7739036,
        FieldOpt1 = 7740936,
        OneofOpt1 = 7740111,
        EnumOpt1 = 7753576,
        EnumValueOpt1 = 1560678,
        ServiceOpt1 = 7887650,
        MethodOpt1 = 7890860,

        // All message options...
        BoolOpt = 7706090,
        Int32Opt = 7705709,
        Int64Opt = 7705542,
        UInt32Opt = 7704880,
        UInt64Opt = 7702367,
        SInt32Opt = 7701568,
        SInt64Opt = 7700863,
        Fixed32Opt = 7700307,
        Fixed64Opt = 7700194,
        SFixed32Opt = 7698645,
        SFixed64Opt = 7685475,
        FloatOpt = 7675390,
        DoubleOpt = 7673293,
        StringOpt = 7673285,
        BytesOpt = 7673238,
        EnumOpt = 7673233,
        MessageTypeOpt = 7665967,

        // Miscellaneous
        ComplexOpt4 = 7633546,
        ComplexOpt1 = 7646756,
        ComplexOpt2 = 7636949,
        ComplexOpt3 = 7636463,

        // Aggregates
        AggregateFileOpt = 15478479,
        AggregateMsgOpt = 15480088,
        AggregateFieldOpt = 15481374,
        AggregateEnumOpt = 15483218,
        AggregateEnumValueOpt = 15486921,
        AggregateServiceOpt = 15497145,
        AggregateMethodOpt = 15512713,
    }

    /// <summary>
    /// The majority of the testing here is done via parsed descriptors. That's simpler to
    /// achieve (and more important) than constructing a CodedInputStream manually.
    /// </summary>
    public class CustomOptionsTest
    {
        delegate bool OptionFetcher<T>(int field, out T value);

        [Test]
        public void EmptyOptionsIsShared()
        {
            var structOptions = Struct.Descriptor.CustomOptions;
            var timestampOptions = Struct.Descriptor.CustomOptions;
            Assert.AreSame(structOptions, timestampOptions);
        }

        [Test]
        public void SimpleIntegerTest()
        {
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(MakeTag(1, WireType.Varint));
            output.WriteInt32(1234567);
            output.Flush();
            stream.Position = 0;
            var input = new CodedInputStream(stream);
            input.ReadTag();

            var options = CustomOptions.Empty;
            options = options.ReadOrSkipUnknownField(input);

            int intValue;
            Assert.True(options.TryGetInt32(1, out intValue));
            Assert.AreEqual(1234567, intValue);

            string stringValue;
            // No ByteString stored values
            Assert.False(options.TryGetString(1, out stringValue));
            // Nothing stored for field 2
            Assert.False(options.TryGetInt32(2, out intValue));
        }

        [Test]
        public void SimpleStringTest()
        {
            var stream = new MemoryStream();
            var output = new CodedOutputStream(stream);
            output.WriteTag(MakeTag(1, WireType.LengthDelimited));
            output.WriteString("value");
            output.Flush();
            stream.Position = 0;
            var input = new CodedInputStream(stream);
            input.ReadTag();

            var options = CustomOptions.Empty;
            options = options.ReadOrSkipUnknownField(input);

            string stringValue;
            Assert.True(options.TryGetString(1, out stringValue));
            Assert.AreEqual("value", stringValue);

            int intValue;
            // No numeric stored values
            Assert.False(options.TryGetInt32(1, out intValue));
            // Nothing stored for field 2
            Assert.False(options.TryGetString(2, out stringValue));
        }

        [Test]
        public void ScalarOptions()
        {
            var options = CustomOptionOtherValues.Descriptor.CustomOptions;
            AssertOption(-100, options.TryGetInt32, Int32Opt);
            AssertOption(12.3456789f, options.TryGetFloat, FloatOpt);
            AssertOption(1.234567890123456789d, options.TryGetDouble, DoubleOpt);
            AssertOption("Hello, \"World\"", options.TryGetString, StringOpt);
            AssertOption(ByteString.CopyFromUtf8("Hello\0World"), options.TryGetBytes, BytesOpt);
            AssertOption((int) TestEnumType.TestOptionEnumType2, options.TryGetInt32, EnumOpt);
        }

        [Test]
        public void MessageOptions()
        {
            var options = VariousComplexOptions.Descriptor.CustomOptions;
            AssertOption(new ComplexOptionType1 { Foo = 42, Foo4 = { 99, 88 } }, options.TryGetMessage, ComplexOpt1);
            AssertOption(new ComplexOptionType2
                {
                    Baz = 987, Bar = new ComplexOptionType1 { Foo = 743 },
                    Fred = new ComplexOptionType4 { Waldo = 321 },
                    Barney = { new ComplexOptionType4 { Waldo = 101 }, new ComplexOptionType4 { Waldo = 212 } }
                },
                options.TryGetMessage, ComplexOpt2);
            AssertOption(new ComplexOptionType3 { Qux = 9 }, options.TryGetMessage, ComplexOpt3);
        }

        [Test]
        public void OptionLocations()
        {
            var fileOptions = UnittestCustomOptionsProto3Reflection.Descriptor.CustomOptions;
            AssertOption(9876543210UL, fileOptions.TryGetUInt64, FileOpt1);

            var messageOptions = TestMessageWithCustomOptions.Descriptor.CustomOptions;
            AssertOption(-56, messageOptions.TryGetInt32, MessageOpt1);

            var fieldOptions = TestMessageWithCustomOptions.Descriptor.Fields["field1"] .CustomOptions;
            AssertOption(8765432109UL, fieldOptions.TryGetFixed64, FieldOpt1);

            var oneofOptions = TestMessageWithCustomOptions.Descriptor.Oneofs[0].CustomOptions;
            AssertOption(-99, oneofOptions.TryGetInt32, OneofOpt1);

            var enumOptions = TestMessageWithCustomOptions.Descriptor.EnumTypes[0].CustomOptions;
            AssertOption(-789, enumOptions.TryGetSFixed32, EnumOpt1);

            var enumValueOptions = TestMessageWithCustomOptions.Descriptor.EnumTypes[0].FindValueByNumber(2).CustomOptions;
            AssertOption(123, enumValueOptions.TryGetInt32, EnumValueOpt1);

            var service = UnittestCustomOptionsProto3Reflection.Descriptor.Services
                .Single(s => s.Name == "TestServiceWithCustomOptions");
            var serviceOptions = service.CustomOptions;
            AssertOption(-9876543210, serviceOptions.TryGetSInt64, ServiceOpt1);

            var methodOptions = service.Methods[0].CustomOptions;
            AssertOption((int) UnitTest.Issues.TestProtos.MethodOpt1.Val2, methodOptions.TryGetInt32, CustomOptionNumber.MethodOpt1);
        }

        [Test]
        public void MinValues()
        {
            var options = CustomOptionMinIntegerValues.Descriptor.CustomOptions;
            AssertOption(false, options.TryGetBool, BoolOpt);
            AssertOption(int.MinValue, options.TryGetInt32, Int32Opt);
            AssertOption(long.MinValue, options.TryGetInt64, Int64Opt);
            AssertOption(uint.MinValue, options.TryGetUInt32, UInt32Opt);
            AssertOption(ulong.MinValue, options.TryGetUInt64, UInt64Opt);
            AssertOption(int.MinValue, options.TryGetSInt32, SInt32Opt);
            AssertOption(long.MinValue, options.TryGetSInt64, SInt64Opt);
            AssertOption(uint.MinValue, options.TryGetUInt32, Fixed32Opt);
            AssertOption(ulong.MinValue, options.TryGetUInt64, Fixed64Opt);
            AssertOption(int.MinValue, options.TryGetInt32, SFixed32Opt);
            AssertOption(long.MinValue, options.TryGetInt64, SFixed64Opt);
        }

        [Test]
        public void MaxValues()
        {
            var options = CustomOptionMaxIntegerValues.Descriptor.CustomOptions;
            AssertOption(true, options.TryGetBool, BoolOpt);
            AssertOption(int.MaxValue, options.TryGetInt32, Int32Opt);
            AssertOption(long.MaxValue, options.TryGetInt64, Int64Opt);
            AssertOption(uint.MaxValue, options.TryGetUInt32, UInt32Opt);
            AssertOption(ulong.MaxValue, options.TryGetUInt64, UInt64Opt);
            AssertOption(int.MaxValue, options.TryGetSInt32, SInt32Opt);
            AssertOption(long.MaxValue, options.TryGetSInt64, SInt64Opt);
            AssertOption(uint.MaxValue, options.TryGetFixed32, Fixed32Opt);
            AssertOption(ulong.MaxValue, options.TryGetFixed64, Fixed64Opt);
            AssertOption(int.MaxValue, options.TryGetSFixed32, SFixed32Opt);
            AssertOption(long.MaxValue, options.TryGetSFixed64, SFixed64Opt);
        }

        [Test]
        public void AggregateOptions()
        {
            // Just two examples
            var messageOptions = AggregateMessage.Descriptor.CustomOptions;
            AssertOption(new Aggregate { I = 101, S = "MessageAnnotation" }, messageOptions.TryGetMessage, AggregateMsgOpt);

            var fieldOptions = AggregateMessage.Descriptor.Fields["fieldname"].CustomOptions;
            AssertOption(new Aggregate { S = "FieldAnnotation" }, fieldOptions.TryGetMessage, AggregateFieldOpt);
        }

        private void AssertOption<T>(T expected, OptionFetcher<T> fetcher, CustomOptionNumber field)
        {
            T actual;
            Assert.IsTrue(fetcher((int) field, out actual));
            Assert.AreEqual(expected, actual);
        }
    }
}
