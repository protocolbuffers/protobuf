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
using System;
using System.IO;
using System.Linq;
using UnitTest.Issues.TestProtos;
using static Google.Protobuf.WireFormat;
using static UnitTest.Issues.TestProtos.ComplexOptionType2.Types;
using static UnitTest.Issues.TestProtos.UnittestCustomOptionsProto3Extensions;
using static UnitTest.Issues.TestProtos.DummyMessageContainingEnum.Types;
using Google.Protobuf.TestProtos;

#pragma warning disable CS0618

namespace Google.Protobuf.Test.Reflection
{
    /// <summary>
    /// The majority of the testing here is done via parsed descriptors. That's simpler to
    /// achieve (and more important) than constructing a CodedInputStream manually.
    /// </summary>
    public class CustomOptionsTest
    {
        delegate bool OptionFetcher<T>(int field, out T value);

        OptionFetcher<E> EnumFetcher<E>(CustomOptions options)
        {
            return (int i, out E v) => {
                if (options.TryGetInt32(i, out int value))
                {
                    v = (E)(object)value;
                    return true;
                }
                else
                {
                    v = default(E);
                    return false;
                }
            };
        }

        [Test]
        public void ScalarOptions()
        {
            var d = CustomOptionOtherValues.Descriptor;
            var options = d.CustomOptions;
            AssertOption(-100, options.TryGetInt32, Int32Opt, d.GetOption);
            AssertOption(12.3456789f, options.TryGetFloat, FloatOpt, d.GetOption);
            AssertOption(1.234567890123456789d, options.TryGetDouble, DoubleOpt, d.GetOption);
            AssertOption("Hello, \"World\"", options.TryGetString, StringOpt, d.GetOption);
            AssertOption(ByteString.CopyFromUtf8("Hello\0World"), options.TryGetBytes, BytesOpt, d.GetOption);
            AssertOption(TestEnumType.TestOptionEnumType2, EnumFetcher<TestEnumType>(options), EnumOpt, d.GetOption);
        }

        [Test]
        public void MessageOptions()
        {
            var d = VariousComplexOptions.Descriptor;
            var options = d.CustomOptions;
            AssertOption(new ComplexOptionType1 { Foo = 42, Foo4 = { 99, 88 } }, options.TryGetMessage, ComplexOpt1, d.GetOption);
            AssertOption(new ComplexOptionType2
            {
                Baz = 987,
                Bar = new ComplexOptionType1 { Foo = 743 },
                Fred = new ComplexOptionType4 { Waldo = 321 },
                Barney = { new ComplexOptionType4 { Waldo = 101 }, new ComplexOptionType4 { Waldo = 212 } }
            },
                options.TryGetMessage, ComplexOpt2, d.GetOption);
            AssertOption(new ComplexOptionType3 { Qux = 9 }, options.TryGetMessage, ComplexOpt3, d.GetOption);
        }

        [Test]
        public void OptionLocations()
        {
            var fileOptions = UnittestCustomOptionsProto3Reflection.Descriptor.CustomOptions;
            AssertOption(9876543210UL, fileOptions.TryGetUInt64, FileOpt1, UnittestCustomOptionsProto3Reflection.Descriptor.GetOption);

            var messageOptions = TestMessageWithCustomOptions.Descriptor.CustomOptions;
            AssertOption(-56, messageOptions.TryGetInt32, MessageOpt1, TestMessageWithCustomOptions.Descriptor.GetOption);

            var fieldOptions = TestMessageWithCustomOptions.Descriptor.Fields["field1"].CustomOptions;
            AssertOption(8765432109UL, fieldOptions.TryGetFixed64, FieldOpt1, TestMessageWithCustomOptions.Descriptor.Fields["field1"].GetOption);

            var oneofOptions = TestMessageWithCustomOptions.Descriptor.Oneofs[0].CustomOptions;
            AssertOption(-99, oneofOptions.TryGetInt32, OneofOpt1, TestMessageWithCustomOptions.Descriptor.Oneofs[0].GetOption);

            var enumOptions = TestMessageWithCustomOptions.Descriptor.EnumTypes[0].CustomOptions;
            AssertOption(-789, enumOptions.TryGetSFixed32, EnumOpt1, TestMessageWithCustomOptions.Descriptor.EnumTypes[0].GetOption);

            var enumValueOptions = TestMessageWithCustomOptions.Descriptor.EnumTypes[0].FindValueByNumber(2).CustomOptions;
            AssertOption(123, enumValueOptions.TryGetInt32, EnumValueOpt1, TestMessageWithCustomOptions.Descriptor.EnumTypes[0].FindValueByNumber(2).GetOption);

            var service = UnittestCustomOptionsProto3Reflection.Descriptor.Services
                .Single(s => s.Name == "TestServiceWithCustomOptions");
            var serviceOptions = service.CustomOptions;
            AssertOption(-9876543210, serviceOptions.TryGetSInt64, ServiceOpt1, service.GetOption);

            var methodOptions = service.Methods[0].CustomOptions;
            AssertOption(UnitTest.Issues.TestProtos.MethodOpt1.Val2, EnumFetcher<UnitTest.Issues.TestProtos.MethodOpt1>(methodOptions), UnittestCustomOptionsProto3Extensions.MethodOpt1, service.Methods[0].GetOption);
        }

        [Test]
        public void MinValues()
        {
            var d = CustomOptionMinIntegerValues.Descriptor;
            var options = d.CustomOptions;
            AssertOption(false, options.TryGetBool, BoolOpt, d.GetOption);
            AssertOption(int.MinValue, options.TryGetInt32, Int32Opt, d.GetOption);
            AssertOption(long.MinValue, options.TryGetInt64, Int64Opt, d.GetOption);
            AssertOption(uint.MinValue, options.TryGetUInt32, Uint32Opt, d.GetOption);
            AssertOption(ulong.MinValue, options.TryGetUInt64, Uint64Opt, d.GetOption);
            AssertOption(int.MinValue, options.TryGetSInt32, Sint32Opt, d.GetOption);
            AssertOption(long.MinValue, options.TryGetSInt64, Sint64Opt, d.GetOption);
            AssertOption(uint.MinValue, options.TryGetUInt32, Fixed32Opt, d.GetOption);
            AssertOption(ulong.MinValue, options.TryGetUInt64, Fixed64Opt, d.GetOption);
            AssertOption(int.MinValue, options.TryGetInt32, Sfixed32Opt, d.GetOption);
            AssertOption(long.MinValue, options.TryGetInt64, Sfixed64Opt, d.GetOption);
        }

        [Test]
        public void MaxValues()
        {
            var d = CustomOptionMaxIntegerValues.Descriptor;
            var options = d.CustomOptions;
            AssertOption(true, options.TryGetBool, BoolOpt, d.GetOption);
            AssertOption(int.MaxValue, options.TryGetInt32, Int32Opt, d.GetOption);
            AssertOption(long.MaxValue, options.TryGetInt64, Int64Opt, d.GetOption);
            AssertOption(uint.MaxValue, options.TryGetUInt32, Uint32Opt, d.GetOption);
            AssertOption(ulong.MaxValue, options.TryGetUInt64, Uint64Opt, d.GetOption);
            AssertOption(int.MaxValue, options.TryGetSInt32, Sint32Opt, d.GetOption);
            AssertOption(long.MaxValue, options.TryGetSInt64, Sint64Opt, d.GetOption);
            AssertOption(uint.MaxValue, options.TryGetFixed32, Fixed32Opt, d.GetOption);
            AssertOption(ulong.MaxValue, options.TryGetFixed64, Fixed64Opt, d.GetOption);
            AssertOption(int.MaxValue, options.TryGetSFixed32, Sfixed32Opt, d.GetOption);
            AssertOption(long.MaxValue, options.TryGetSFixed64, Sfixed64Opt, d.GetOption);
        }

        [Test]
        public void AggregateOptions()
        {
            // Just two examples
            var messageOptions = AggregateMessage.Descriptor.CustomOptions;
            AssertOption(new Aggregate { I = 101, S = "MessageAnnotation" }, messageOptions.TryGetMessage, Msgopt, AggregateMessage.Descriptor.GetOption);

            var fieldOptions = AggregateMessage.Descriptor.Fields["fieldname"].CustomOptions;
            AssertOption(new Aggregate { S = "FieldAnnotation" }, fieldOptions.TryGetMessage, Fieldopt, AggregateMessage.Descriptor.Fields["fieldname"].GetOption);
        }

        [Test]
        public void NoOptions()
        {
            var fileDescriptor = UnittestProto3Reflection.Descriptor;
            var messageDescriptor = TestAllTypes.Descriptor;
            Assert.NotNull(fileDescriptor.CustomOptions);
            Assert.NotNull(messageDescriptor.CustomOptions);
            Assert.NotNull(messageDescriptor.Fields[1].CustomOptions);
            Assert.NotNull(fileDescriptor.Services[0].CustomOptions);
            Assert.NotNull(fileDescriptor.Services[0].Methods[0].CustomOptions);
            Assert.NotNull(fileDescriptor.EnumTypes[0].CustomOptions);
            Assert.NotNull(fileDescriptor.EnumTypes[0].Values[0].CustomOptions);
            Assert.NotNull(TestAllTypes.Descriptor.Oneofs[0].CustomOptions);
        }

        [Test]
        public void MultipleImportOfSameFileWithExtension()
        {
            var descriptor = UnittestIssue6936CReflection.Descriptor;
            var foo = Foo.Descriptor;
            var bar = Bar.Descriptor;
            AssertOption("foo", foo.CustomOptions.TryGetString, UnittestIssue6936AExtensions.Opt, foo.GetOption);
            AssertOption("bar", bar.CustomOptions.TryGetString, UnittestIssue6936AExtensions.Opt, bar.GetOption);
        }

        private void AssertOption<T, D>(T expected, OptionFetcher<T> fetcher, Extension<D, T> extension, Func<Extension<D, T>, T> descriptorOptionFetcher) where D : IExtendableMessage<D>
        {
            T customOptionsValue;
            T extensionValue = descriptorOptionFetcher(extension);
            Assert.IsTrue(fetcher(extension.FieldNumber, out customOptionsValue));
            Assert.AreEqual(expected, customOptionsValue);
            Assert.AreEqual(expected, extensionValue);
        }
    }
}
