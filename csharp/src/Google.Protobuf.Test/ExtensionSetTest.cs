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

            var other = TestAllExtensions.Parser
                .WithExtensionRegistry(new ExtensionRegistry() { OptionalBoolExtension })
                .ParseFrom(serialized);

            Assert.AreEqual(message, other);
            Assert.AreEqual(message.CalculateSize(), other.CalculateSize());
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
