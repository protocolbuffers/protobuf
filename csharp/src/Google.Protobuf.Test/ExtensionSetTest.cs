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
            input.ExtensionRegistry = new ExtensionRegistry() { OptionalStringExtension };
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

            var expectedMessage = "The stored extension value has a type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes+Types+NestedMessage, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'. " +
                "This a different from the requested type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'.";
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

            var expectedMessage = "The stored extension value has a type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes+Types+NestedMessage, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'. " +
                "This a different from the requested type of 'Google.Protobuf.TestProtos.Proto2.TestAllTypes, Google.Protobuf.Test.TestProtos, Version=1.0.0.0, Culture=neutral, PublicKeyToken=a7d26565bac4d604'.";
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
