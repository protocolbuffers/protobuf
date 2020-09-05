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
            Assert.AreEqual(message.CalculateSize(), message.CalculateSize());
        }
    }
}
