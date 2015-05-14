using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Serialization;
using NUnit.Framework;

namespace Google.ProtocolBuffers.Compatibility
{
    [TestFixture]
    public class DictionaryCompatibilityTests : CompatibilityTests
    {
        protected override object SerializeMessage<TMessage, TBuilder>(TMessage message)
        {
            DictionaryWriter writer = new DictionaryWriter();
            writer.WriteMessage(message);
            return writer.ToDictionary();
        }

        protected override TBuilder DeserializeMessage<TMessage, TBuilder>(object message, TBuilder builder, ExtensionRegistry registry)
        {
            new DictionaryReader((IDictionary<string, object>)message).Merge(builder);
            return builder;
        }

        protected override void AssertOutputEquals(object lhs, object rhs)
        {
            IDictionary<string, object> left = (IDictionary<string, object>)lhs;
            IDictionary<string, object> right = (IDictionary<string, object>)rhs;

            Assert.AreEqual(
                String.Join(",", new List<string>(left.Keys).ToArray()),
                String.Join(",", new List<string>(right.Keys).ToArray())
            );
        }
    }
}