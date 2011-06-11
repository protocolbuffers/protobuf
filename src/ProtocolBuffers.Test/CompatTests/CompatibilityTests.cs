using System;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers.CompatTests
{
    public abstract class CompatibilityTests
    {
        protected abstract object SerializeMessage<TMessage, TBuilder>(TMessage message)
            where TMessage : IMessageLite<TMessage, TBuilder>
            where TBuilder : IBuilderLite<TMessage, TBuilder>;

        protected abstract TBuilder DeerializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
            where TMessage : IMessageLite<TMessage, TBuilder>
            where TBuilder : IBuilderLite<TMessage, TBuilder>;

        protected virtual void AssertOutputEquals(object lhs, object rhs)
        {
            Assert.AreEqual(lhs, rhs);
        }

        [Test]
        public virtual void RoundTripMessage1OptimizeSize()
        {
            SizeMessage1 msg = SizeMessage1.CreateBuilder().MergeFrom(TestResources.google_message1).Build();
            object content = SerializeMessage<SizeMessage1, SizeMessage1.Builder>(msg);

            SizeMessage1 copy = DeerializeMessage<SizeMessage1, SizeMessage1.Builder>(content, SizeMessage1.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            AssertOutputEquals(content, SerializeMessage<SizeMessage1, SizeMessage1.Builder>(copy));
            Assert.AreEqual(TestResources.google_message1, copy.ToByteArray());
        }

        [Test]
        public virtual void RoundTripMessage2OptimizeSize()
        {
            SizeMessage2 msg = SizeMessage2.CreateBuilder().MergeFrom(TestResources.google_message2).Build();
            object content = SerializeMessage<SizeMessage2, SizeMessage2.Builder>(msg);

            SizeMessage2 copy = DeerializeMessage<SizeMessage2, SizeMessage2.Builder>(content, SizeMessage2.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            AssertOutputEquals(content, SerializeMessage<SizeMessage2, SizeMessage2.Builder>(copy));
            Assert.AreEqual(TestResources.google_message2, copy.ToByteArray());
        }

        [Test]
        public virtual void RoundTripMessage1OptimizeSpeed()
        {
            SpeedMessage1 msg = SpeedMessage1.CreateBuilder().MergeFrom(TestResources.google_message1).Build();
            object content = SerializeMessage<SpeedMessage1, SpeedMessage1.Builder>(msg);

            SpeedMessage1 copy = DeerializeMessage<SpeedMessage1, SpeedMessage1.Builder>(content, SpeedMessage1.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            AssertOutputEquals(content, SerializeMessage<SpeedMessage1, SpeedMessage1.Builder>(copy));
            Assert.AreEqual(TestResources.google_message1, copy.ToByteArray());
        }

        [Test]
        public virtual void RoundTripMessage2OptimizeSpeed()
        {
            SpeedMessage2 msg = SpeedMessage2.CreateBuilder().MergeFrom(TestResources.google_message2).Build();
            object content = SerializeMessage<SpeedMessage2, SpeedMessage2.Builder>(msg);

            SpeedMessage2 copy = DeerializeMessage<SpeedMessage2, SpeedMessage2.Builder>(content, SpeedMessage2.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            AssertOutputEquals(content, SerializeMessage<SpeedMessage2, SpeedMessage2.Builder>(copy));
            Assert.AreEqual(TestResources.google_message2, copy.ToByteArray());
        }

        #region Test message builders

        private static TestAllTypes.Builder AddAllTypes(TestAllTypes.Builder builder)
        {
            return builder.SetOptionalInt32(1001)
                .SetOptionalInt64(1001)
                .SetOptionalUint32(1001)
                .SetOptionalUint64(1001)
                .SetOptionalSint32(-1001)
                .SetOptionalSint64(-1001)
                .SetOptionalFixed32(1001)
                .SetOptionalFixed64(1001)
                .SetOptionalSfixed32(-1001)
                .SetOptionalSfixed64(-1001)
                .SetOptionalFloat(1001.1001f)
                .SetOptionalDouble(1001.1001)
                .SetOptionalBool(true)
                .SetOptionalString("this is a string value")
                .SetOptionalBytes(ByteString.CopyFromUtf8("this is an array of bytes"))
                .SetOptionalGroup(new TestAllTypes.Types.OptionalGroup.Builder().SetA(1001))
                .SetOptionalNestedMessage(new TestAllTypes.Types.NestedMessage.Builder().SetBb(1001))
                .SetOptionalNestedEnum(TestAllTypes.Types.NestedEnum.FOO)
            ;
        }

        private static TestAllTypes.Builder AddRepeatedTypes(TestAllTypes.Builder builder, int size)
        {
            //repeated values
            for (int i = 0; i < size; i++)
                builder.AddRepeatedInt32(1001 + i)
                .AddRepeatedInt64(1001)
                .AddRepeatedUint32(1001)
                .AddRepeatedUint64(1001)
                .AddRepeatedSint32(-1001)
                .AddRepeatedSint64(-1001)
                .AddRepeatedFixed32(1001)
                .AddRepeatedFixed64(1001)
                .AddRepeatedSfixed32(-1001)
                .AddRepeatedSfixed64(-1001)
                .AddRepeatedFloat(1001.1001f)
                .AddRepeatedDouble(1001.1001)
                .AddRepeatedBool(true)
                .AddRepeatedString("this is a string value")
                .AddRepeatedBytes(ByteString.CopyFromUtf8("this is an array of bytes"))
                .AddRepeatedGroup(new TestAllTypes.Types.RepeatedGroup.Builder().SetA(1001))
                .AddRepeatedNestedMessage(new TestAllTypes.Types.NestedMessage.Builder().SetBb(1001))
                .AddRepeatedNestedEnum(TestAllTypes.Types.NestedEnum.FOO)
            ;
            return builder;
        }

        private static TestPackedTypes.Builder AddPackedTypes(TestPackedTypes.Builder builder, int size)
        {
            for(int i=0; i < size; i++ )
                builder.AddPackedInt32(1001)
                .AddPackedInt64(1001)
                .AddPackedUint32(1001)
                .AddPackedUint64(1001)
                .AddPackedSint32(-1001)
                .AddPackedSint64(-1001)
                .AddPackedFixed32(1001)
                .AddPackedFixed64(1001)
                .AddPackedSfixed32(-1001)
                .AddPackedSfixed64(-1001)
                .AddPackedFloat(1001.1001f)
                .AddPackedDouble(1001.1001)
                .AddPackedBool(true)
                .AddPackedEnum(ForeignEnum.FOREIGN_FOO)
            ;
            return builder;
        }

        #endregion

        [Test]
        public void TestRoundTripAllTypes()
        {
            TestAllTypes msg = AddAllTypes(new TestAllTypes.Builder()).Build();
            object content = SerializeMessage<TestAllTypes, TestAllTypes.Builder>(msg);

            TestAllTypes copy = DeerializeMessage<TestAllTypes, TestAllTypes.Builder>(content, TestAllTypes.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            AssertOutputEquals(content, SerializeMessage<TestAllTypes, TestAllTypes.Builder>(copy));
            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
        }

        [Test]
        public void TestRoundTripRepeatedTypes()
        {
            TestAllTypes msg = AddRepeatedTypes(new TestAllTypes.Builder(), 5).Build();
            object content = SerializeMessage<TestAllTypes, TestAllTypes.Builder>(msg);

            TestAllTypes copy = DeerializeMessage<TestAllTypes, TestAllTypes.Builder>(content, TestAllTypes.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            AssertOutputEquals(content, SerializeMessage<TestAllTypes, TestAllTypes.Builder>(copy));
            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
        }

        [Test]
        public void TestRoundTripPackedTypes()
        {
            TestPackedTypes msg = AddPackedTypes(new TestPackedTypes.Builder(), 5).Build();
            object content = SerializeMessage<TestPackedTypes, TestPackedTypes.Builder>(msg);

            TestPackedTypes copy = DeerializeMessage<TestPackedTypes, TestPackedTypes.Builder>(content, TestPackedTypes.CreateBuilder(), ExtensionRegistry.Empty).Build();

            Assert.AreEqual(msg, copy);
            AssertOutputEquals(content, SerializeMessage<TestPackedTypes, TestPackedTypes.Builder>(copy));
            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
        }
    }
}
