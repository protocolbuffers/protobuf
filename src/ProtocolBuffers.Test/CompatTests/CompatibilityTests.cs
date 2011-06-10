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

    }
}
