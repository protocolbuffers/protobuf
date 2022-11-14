#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
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

using System.Collections;
using Google.Protobuf.TestProtos.Proto2;
using UnitTest.Issues.TestProtos;
using static Google.Protobuf.TestProtos.Proto2.UnittestExtensions;
using static UnitTest.Issues.TestProtos.UnittestCustomOptionsProto3Extensions;

namespace Google.Protobuf.Test.NativeAot.Reflection
{
    internal static class Extensions
    {
        public static void GetExtensionSingle()
        {
            var extensionValue = new TestAllTypes.Types.NestedMessage() { Bb = 42 };
            var untypedExtension = new Extension<TestAllExtensions, object>(OptionalNestedMessageExtension.FieldNumber, codec: null);
            var wrongTypedExtension = new Extension<TestAllExtensions, TestAllTypes>(OptionalNestedMessageExtension.FieldNumber, codec: null);

            var message = new TestAllExtensions();

            var value1 = message.GetExtension(untypedExtension);
            Assert.IsNull(value1);

            message.SetExtension(OptionalNestedMessageExtension, extensionValue);
            var value2 = message.GetExtension(untypedExtension);
            Assert.IsNotNull(value2);
            Assert.AreEqual(extensionValue, value2);

            var valueBytes = ((IMessage) value2).ToByteArray();
            var parsedValue = TestProtos.Proto2.TestAllTypes.Types.NestedMessage.Parser.ParseFrom(valueBytes);
            Assert.AreEqual(extensionValue, parsedValue);

            var ex = Assert.Throws<InvalidOperationException>(() => message.GetExtension(wrongTypedExtension));

            var expectedMessage = "The stored extension value has a type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes+Types+NestedMessage, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'. " +
                "This a different from the requested type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'.";
            Assert.AreEqual(expectedMessage, ex.Message);
        }

        public static void GetExtensionEnumSingle()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalForeignEnumExtension, ForeignEnum.ForeignBar);

            var value = message.GetExtension(OptionalForeignEnumExtension);
            Assert.AreEqual(ForeignEnum.ForeignBar, value);

            Console.WriteLine($"Got value {value} for extension {OptionalForeignEnumExtension.FieldNumber}");
        }

        public static void GetExtensionRepeated()
        {
            var extensionValue = new TestAllTypes.Types.NestedMessage() { Bb = 42 };
            var untypedExtension = new Extension<TestAllExtensions, IList>(RepeatedNestedMessageExtension.FieldNumber, codec: null);
            var wrongTypedExtension = new RepeatedExtension<TestAllExtensions, TestAllTypes>(RepeatedNestedMessageExtension.FieldNumber, codec: null);

            var message = new TestAllExtensions();

            var value1 = message.GetExtension(untypedExtension);
            Assert.IsNull(value1);

            var repeatedField = message.GetOrInitializeExtension<TestAllTypes.Types.NestedMessage>(RepeatedNestedMessageExtension);
            repeatedField.Add(extensionValue);

            var value2 = message.GetExtension(untypedExtension);
            Assert.IsNotNull(value2);
            Assert.AreEqual(1, value2.Count);

            var valueBytes = ((IMessage) value2[0]!).ToByteArray();
            var parsedValue = TestProtos.Proto2.TestAllTypes.Types.NestedMessage.Parser.ParseFrom(valueBytes);
            Assert.AreEqual(extensionValue, parsedValue);

            var ex = Assert.Throws<InvalidOperationException>(() => message.GetExtension(wrongTypedExtension));

            var expectedMessage = "The stored extension value has a type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes+Types+NestedMessage, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'. " +
                "This a different from the requested type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'.";
            Assert.AreEqual(expectedMessage, ex.Message);
        }

        public static void GetExtensionValue()
        {
            var message = CreateFullTestAllExtensions();

            Assert.AreEqual(true, message.GetExtension(OptionalBoolExtension));

            var ex = Assert.Throws<NotSupportedException>(() => TestProtos.Proto2.TestAllExtensions.Descriptor.FindFieldByNumber(OptionalBoolExtension.FieldNumber).Accessor.GetValue(message));
            Assert.AreEqual("Extensions reflection is not supported with AOT.", ex.Message);
        }

        public static void ExtensionsIsInitialized()
        {
            var message = new TestAllExtensions();
            Assert.AreEqual(true, message.IsInitialized());
        }

        public static void OptionLocations()
        {
#pragma warning disable CS0618 // Type or member is obsolete
            var fileDescriptor = UnittestCustomOptionsProto3Reflection.Descriptor;
            AssertOption(9876543210UL, FileOpt1, fileDescriptor.GetOption, fileDescriptor.GetOptions().GetExtension);

            var messageDescriptor = TestMessageWithCustomOptions.Descriptor;
            AssertOption(-56, MessageOpt1, messageDescriptor.GetOption, messageDescriptor.GetOptions().GetExtension);

            var fieldDescriptor = TestMessageWithCustomOptions.Descriptor.Fields["field1"];
            AssertOption(8765432109UL, FieldOpt1, fieldDescriptor.GetOption, fieldDescriptor.GetOptions().GetExtension);

            var oneofDescriptor = TestMessageWithCustomOptions.Descriptor.Oneofs[0];
            AssertOption(-99, OneofOpt1, oneofDescriptor.GetOption, oneofDescriptor.GetOptions().GetExtension);

            var enumDescriptor = TestMessageWithCustomOptions.Descriptor.EnumTypes[0];
            AssertOption(-789, EnumOpt1, enumDescriptor.GetOption, enumDescriptor.GetOptions().GetExtension);

            var enumValueDescriptor = TestMessageWithCustomOptions.Descriptor.EnumTypes[0].FindValueByNumber(2);
            AssertOption(123, EnumValueOpt1, enumValueDescriptor.GetOption, enumValueDescriptor.GetOptions().GetExtension);

            var serviceDescriptor = UnittestCustomOptionsProto3Reflection.Descriptor.Services
                .Single(s => s.Name == "TestServiceWithCustomOptions");
            AssertOption(-9876543210, ServiceOpt1, serviceDescriptor.GetOption, serviceDescriptor.GetOptions().GetExtension);

            var methodDescriptor = serviceDescriptor.Methods[0];
            AssertOption(UnitTest.Issues.TestProtos.MethodOpt1.Val2, UnittestCustomOptionsProto3Extensions.MethodOpt1, methodDescriptor.GetOption, methodDescriptor.GetOptions().GetExtension);
#pragma warning restore CS0618 // Type or member is obsolete
        }

        private static void AssertOption<T, D>(T expected, Extension<D, T> extension, Func<Extension<D, T>, T> getOptionFetcher, Func<Extension<D, T>, T> extensionFetcher) where D : IExtendableMessage<D>
        {
            T getOptionValue = getOptionFetcher(extension);
            Assert.AreEqual(expected, getOptionValue);

            T extensionValue = extensionFetcher(extension);
            Assert.AreEqual(expected, extensionValue);
        }

        public static TestAllExtensions CreateFullTestAllExtensions()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalBoolExtension, true);
            message.SetExtension(OptionalBytesExtension, ByteString.CopyFrom(1, 2, 3, 4));
            message.SetExtension(OptionalDoubleExtension, 23.5);
            message.SetExtension(OptionalFixed32Extension, 23u);
            message.SetExtension(OptionalFixed64Extension, 1234567890123u);
            message.SetExtension(OptionalFloatExtension, 12.25f);
            message.SetExtension(OptionalForeignEnumExtension, ForeignEnum.ForeignBar);
            message.SetExtension(OptionalForeignMessageExtension, new ForeignMessage { C = 10 });
            message.SetExtension(OptionalImportEnumExtension, ImportEnum.ImportBaz);
            message.SetExtension(OptionalImportMessageExtension, new ImportMessage { D = 20 });
            message.SetExtension(OptionalInt32Extension, 100);
            message.SetExtension(OptionalInt64Extension, 3210987654321);
            message.SetExtension(OptionalNestedEnumExtension, TestProtos.Proto2.TestAllTypes.Types.NestedEnum.Foo);
            message.SetExtension(OptionalNestedMessageExtension, new TestProtos.Proto2.TestAllTypes.Types.NestedMessage { Bb = 35 });
            message.SetExtension(OptionalPublicImportMessageExtension, new PublicImportMessage { E = 54 });
            message.SetExtension(OptionalSfixed32Extension, -123);
            message.SetExtension(OptionalSfixed64Extension, -12345678901234);
            message.SetExtension(OptionalSint32Extension, -456);
            message.SetExtension(OptionalSint64Extension, -12345678901235);
            message.SetExtension(OptionalStringExtension, "test");
            message.SetExtension(OptionalUint32Extension, UInt32.MaxValue);
            message.SetExtension(OptionalUint64Extension, UInt64.MaxValue);
            message.SetExtension(OptionalGroupExtension, new OptionalGroup_extension { A = 10 });
            message.GetOrInitializeExtension(RepeatedBoolExtension).AddRange(new[] { true, false });
            message.GetOrInitializeExtension(RepeatedBytesExtension).AddRange(new[] { ByteString.CopyFrom(1, 2, 3, 4), ByteString.CopyFrom(5, 6), ByteString.CopyFrom(new byte[1000]) });
            message.GetOrInitializeExtension(RepeatedDoubleExtension).AddRange(new[] { -12.25, 23.5 });
            message.GetOrInitializeExtension(RepeatedFixed32Extension).AddRange(new[] { UInt32.MaxValue, 23u });
            message.GetOrInitializeExtension(RepeatedFixed64Extension).AddRange(new[] { UInt64.MaxValue, 1234567890123ul });
            message.GetOrInitializeExtension(RepeatedFloatExtension).AddRange(new[] { 100f, 12.25f });
            message.GetOrInitializeExtension(RepeatedForeignEnumExtension).AddRange(new[] { ForeignEnum.ForeignFoo, ForeignEnum.ForeignBar });
            message.GetOrInitializeExtension(RepeatedForeignMessageExtension).AddRange(new[] { new ForeignMessage(), new ForeignMessage { C = 10 } });
            message.GetOrInitializeExtension(RepeatedImportEnumExtension).AddRange(new[] { ImportEnum.ImportBaz, ImportEnum.ImportFoo });
            message.GetOrInitializeExtension(RepeatedImportMessageExtension).AddRange(new[] { new ImportMessage { D = 20 }, new ImportMessage { D = 25 } });
            message.GetOrInitializeExtension(RepeatedInt32Extension).AddRange(new[] { 100, 200 });
            message.GetOrInitializeExtension(RepeatedInt64Extension).AddRange(new[] { 3210987654321, Int64.MaxValue });
            message.GetOrInitializeExtension(RepeatedNestedEnumExtension).AddRange(new[] { TestProtos.Proto2.TestAllTypes.Types.NestedEnum.Foo, TestProtos.Proto2.TestAllTypes.Types.NestedEnum.Neg });
            message.GetOrInitializeExtension(RepeatedNestedMessageExtension).AddRange(new[] { new TestProtos.Proto2.TestAllTypes.Types.NestedMessage { Bb = 35 }, new TestProtos.Proto2.TestAllTypes.Types.NestedMessage { Bb = 10 } });
            message.GetOrInitializeExtension(RepeatedSfixed32Extension).AddRange(new[] { -123, 123 });
            message.GetOrInitializeExtension(RepeatedSfixed64Extension).AddRange(new[] { -12345678901234, 12345678901234 });
            message.GetOrInitializeExtension(RepeatedSint32Extension).AddRange(new[] { -456, 100 });
            message.GetOrInitializeExtension(RepeatedSint64Extension).AddRange(new[] { -12345678901235, 123 });
            message.GetOrInitializeExtension(RepeatedStringExtension).AddRange(new[] { "foo", "bar" });
            message.GetOrInitializeExtension(RepeatedUint32Extension).AddRange(new[] { UInt32.MaxValue, UInt32.MinValue });
            message.GetOrInitializeExtension(RepeatedUint64Extension).AddRange(new[] { UInt64.MaxValue, UInt32.MinValue });
            message.GetOrInitializeExtension(RepeatedGroupExtension).AddRange(new[] { new TestProtos.Proto2.RepeatedGroup_extension { A = 10 }, new TestProtos.Proto2.RepeatedGroup_extension { A = 20 } });
            message.SetExtension(OneofStringExtension, "Oneof string");
            return message;
        }
    }
}
