#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
using System.Collections;
using Google.Protobuf.TestProtos.Proto2;
using NUnit.Framework;

using static Google.Protobuf.TestProtos.Proto2.UnittestExtensions;

namespace Google.Protobuf
{
    public class ExtensionSetTest
    {
        [Test]
        public void EmptyExtensionSet()
        {
            ExtensionSet<TestAllExtensions> extensions = new ExtensionSet<TestAllExtensions>();
            Assert.AreEqual(0, extensions.CalculateSize());
        }

        [Test]
        public void MergeExtensionSet()
        {
            ExtensionSet<TestAllExtensions> extensions = null;
            ExtensionSet.Set(ref extensions, OptionalBoolExtension, true);

            ExtensionSet<TestAllExtensions> other = null;

            Assert.IsFalse(ExtensionSet.Has(ref other, OptionalBoolExtension));
            ExtensionSet.MergeFrom(ref other, extensions);
            Assert.IsTrue(ExtensionSet.Has(ref other, OptionalBoolExtension));
        }

        [Test]
        public void TestMergeCodedInput()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalBoolExtension, true);
            var serialized = message.ToByteArray();

            MessageParsingHelpers.AssertWritingMessage(message);

            MessageParsingHelpers.AssertReadingMessage(
                TestAllExtensions.Parser.WithExtensionRegistry(new ExtensionRegistry() { OptionalBoolExtension }),
                serialized,
                other =>
                {
                    Assert.AreEqual(message, other);
                    Assert.AreEqual(message.CalculateSize(), other.CalculateSize());
                });
        }

        [Test]
        public void TestMergeMessage()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalBoolExtension, true);

            var other = new TestAllExtensions();

            Assert.AreNotEqual(message, other);
            Assert.AreNotEqual(message.CalculateSize(), other.CalculateSize());

            other.MergeFrom(message);

            Assert.AreEqual(message, other);
            Assert.AreEqual(message.CalculateSize(), other.CalculateSize());
        }

        [Test]
        public void TryMergeFieldFrom_CodedInputStream()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalStringExtension, "abcd");

            var input = new CodedInputStream(message.ToByteArray());
            input.ExtensionRegistry = new ExtensionRegistry { OptionalStringExtension };
            input.ReadTag(); // TryMergeFieldFrom expects that a tag was just read and will inspect the LastTag value

            ExtensionSet<TestAllExtensions> extensionSet = null;
            // test the legacy overload of TryMergeFieldFrom that takes a CodedInputStream
            Assert.IsTrue(ExtensionSet.TryMergeFieldFrom(ref extensionSet, input));
            Assert.AreEqual("abcd", ExtensionSet.Get(ref extensionSet, OptionalStringExtension));
        }

        [Test]
        public void GetSingle()
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

            var valueBytes = ((IMessage)value2).ToByteArray();
            var parsedValue = TestProtos.Proto2.TestAllTypes.Types.NestedMessage.Parser.ParseFrom(valueBytes);
            Assert.AreEqual(extensionValue, parsedValue);

            var ex = Assert.Throws<InvalidOperationException>(() => message.GetExtension(wrongTypedExtension));

            var fullAssemblyName = typeof(TestAllTypes).Assembly.FullName;
            var expectedMessage = $"The stored extension value has a type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes+Types+NestedMessage, {fullAssemblyName}'. " +
                $"This a different from the requested type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes, {fullAssemblyName}'.";
            Assert.AreEqual(expectedMessage, ex.Message);
        }

        [Test]
        public void GetRepeated()
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

            var valueBytes = ((IMessage)value2[0]).ToByteArray();
            var parsedValue = TestProtos.Proto2.TestAllTypes.Types.NestedMessage.Parser.ParseFrom(valueBytes);
            Assert.AreEqual(extensionValue, parsedValue);

            var ex = Assert.Throws<InvalidOperationException>(() => message.GetExtension(wrongTypedExtension));

            var fullAssemblyName = typeof(TestAllTypes).Assembly.FullName;
            var expectedMessage = $"The stored extension value has a type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes+Types+NestedMessage, {fullAssemblyName}'. " +
                $"This a different from the requested type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes, {fullAssemblyName}'.";
            Assert.AreEqual(expectedMessage, ex.Message);
        }

        [Test]
        public void TestEquals()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalBoolExtension, true);

            var other = new TestAllExtensions();

            Assert.AreNotEqual(message, other);
            Assert.AreNotEqual(message.CalculateSize(), other.CalculateSize());

            other.SetExtension(OptionalBoolExtension, true);

            Assert.AreEqual(message, other);
            Assert.AreEqual(message.CalculateSize(), other.CalculateSize());
        }

        [Test]
        public void TestHashCode()
        {
            var message = new TestAllExtensions();
            var hashCode = message.GetHashCode();

            message.SetExtension(OptionalBoolExtension, true);

            Assert.AreNotEqual(hashCode, message.GetHashCode());
        }

        [Test]
        public void TestClone()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalBoolExtension, true);

            var other = message.Clone();

            Assert.AreEqual(message, other);
            Assert.AreEqual(message.CalculateSize(), other.CalculateSize());
        }

        [Test]
        public void TestDefaultValueRoundTrip()
        {
            var message = new TestAllExtensions();
            message.SetExtension(OptionalBoolExtension, false);
            Assert.IsFalse(message.GetExtension(OptionalBoolExtension));
            Assert.IsTrue(message.HasExtension(OptionalBoolExtension));

            var bytes = message.ToByteArray();
            var registry = new ExtensionRegistry { OptionalBoolExtension };
            var parsed = TestAllExtensions.Parser.WithExtensionRegistry(registry).ParseFrom(bytes);
            Assert.IsFalse(parsed.GetExtension(OptionalBoolExtension));
            Assert.IsTrue(parsed.HasExtension(OptionalBoolExtension));
        }
    }
}
